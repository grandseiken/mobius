#include "visibility.h"
#include "player.h"
#include "world.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {
  std::vector<Plane>
  calculate_bounding_planes(const glm::vec3& eye,
                            const glm::vec3& bl, const glm::vec3& tl,
                            const glm::vec3& br, const glm::vec3& tr)
  {
    auto bn = glm::normalize(glm::cross(br - eye, bl - eye));
    auto tn = glm::normalize(glm::cross(tl - eye, tr - eye));
    auto ln = glm::normalize(glm::cross(bl - eye, tl - eye));
    auto rn = glm::normalize(glm::cross(tr - eye, br - eye));

    std::vector<Plane> result;
    result.emplace_back(bl, bn);
    result.emplace_back(tr, tn);
    result.emplace_back(tl, ln);
    result.emplace_back(br, rn);
    return result;
  }
}

std::vector<Plane>
calculate_view_frustum(const Player& player, float aspect_ratio)
{
  const auto& eye = player.get_head_position();
  const auto& dir = player.get_look_direction();
  auto f = std::tan(player.get_fov() / 2);

  auto h = f * aspect_ratio * player.get_side_direction();
  auto v = f * player.get_up_direction();

  auto bl = eye + dir - v - h;
  auto tl = eye + dir + v - h;
  auto br = eye + dir - v + h;
  auto tr = eye + dir + v + h;

  auto result = calculate_bounding_planes(eye, bl, tl, br, tr);
  result.emplace_back(eye + player.get_z_near() * dir, dir);
  result.emplace_back(eye + player.get_z_far() * dir, -dir);
  return result;
}

std::vector<Plane>
calculate_bounding_frustum(const Player& player, float aspect_ratio,
                           const glm::mat4& transform, const Portal& portal)
{
  const auto& eye = player.get_head_position();
  const auto& dir = player.get_look_direction();
  auto max_y = std::tan(player.get_fov() / 2);
  auto max_x = aspect_ratio * max_y;

  auto side = player.get_side_direction();
  auto up = player.get_up_direction();

  bool first = true;
  glm::vec2 min;
  glm::vec2 max;

  // Perspective-project each point onto plane. This is basically
  // reimplementing the look-at matrix, right?
  for (const auto& v : portal.portal_mesh->physical_vertices()) {
    glm::vec3 vt{transform * glm::vec4{v, 1}};
    auto depth = glm::dot(vt - eye, dir);
    auto distance = glm::dot(vt - eye - dir, dir);
    auto projection = vt - distance * dir;
    glm::vec2 coords{glm::dot(projection - eye - dir, side),
                     glm::dot(projection - eye - dir, up)};
    if (depth > 0) {
      coords /= depth;
    } else {
      coords.x = coords.x > 0 ? max_x : -max_x;
      coords.y = coords.y > 0 ? max_y : -max_y;
    }

    // Just take the 2D bounding box in plane coordinates. This is not
    // precise; a better way would be to reduce to a convex hull and from
    // there to limited number of bounding planes, or to somehow consider many
    // possible coordinate systems on the 2D plane and pick the best fit.
    if (first) {
      min = coords;
      max = coords;
    } else {
      min = glm::min(min, coords);
      max = glm::max(max, coords);
    }
    first = false;
  }

  min = glm::max(min, {-max_x, -max_y});
  max = glm::min(max, {max_x, max_y});

  auto bl = eye + dir + min.x * side + min.y * up;
  auto br = eye + dir + max.x * side + min.y * up;
  auto tl = eye + dir + min.x * side + max.y * up;
  auto tr = eye + dir + max.x * side + max.y * up;

  auto result = calculate_bounding_planes(eye, bl, tl, br, tr);
  // We also have to clip behind the portal so that we don't see overlapping
  // geometry hanging about.
  auto normal_transform = glm::transpose(glm::inverse(glm::mat3{transform}));
  glm::vec3 clip_point{transform * glm::vec4{portal.local.origin, 1}};
  auto clip_normal = -normal_transform * portal.local.normal;
  result.emplace_back(clip_point, clip_normal);
  return result;
}

bool mesh_visible(const std::vector<Plane>& planes, const glm::vec3& eye,
                  const glm::mat4& transform, const Mesh& mesh)
{
  auto check = [&](const glm::vec3& point, const glm::vec3& normal,
                   const glm::vec3& v)
  {
    return glm::dot(v - point, normal) >= 0;
  };

  auto inside_all = [&](const glm::vec3& v)
  {
    for (const auto& plane : planes) {
      if (!check(plane.first, plane.second, v)) {
        return false;
      }
    }
    return true;
  };

  auto outside_any = [&](
      const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
  {
    for (const auto& plane : planes) {
      if (!check(plane.first, plane.second, a) &&
          !check(plane.first, plane.second, b) &&
          !check(plane.first, plane.second, c)) {
        return true;
      }
    }
    return false;
  };

  // Simple visibility determination. We will probably need something more
  // robust.
  for (const auto& t : mesh.physical_faces()) {
    glm::vec3 a{transform * glm::vec4{t.a, 1}};
    glm::vec3 b{transform * glm::vec4{t.b, 1}};
    glm::vec3 c{transform * glm::vec4{t.c, 1}};

    // Back face cull.
    auto normal = glm::cross(b - a, c - a);
    if (glm::dot(normal, eye - a) <= 0) {
      continue;
    }

    // (Conservative) view frustum intersection.
    if (inside_all(a) || inside_all(b) || inside_all(c) ||
        !outside_any(a, b, c)) {
      return true;
    }
  }

  return false;
}
