#include "visibility.h"
#include "geometry.h"
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

  auto h = f * aspect_ratio * side_direction(dir);
  auto v = f * up_direction(dir);

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

  auto side = side_direction(dir);
  auto up = up_direction(dir);
  auto z_near = player.get_z_near();

  bool first = true;
  glm::vec2 min;
  glm::vec2 max;

  auto handle_vertex = [&](const glm::vec3& v)
  {
    // Just take the 2D bounding box in plane coordinates. This is not
    // precise; a better way would be to reduce to a convex hull and from
    // there to limited number of bounding planes, or to somehow consider many
    // possible coordinate systems on the 2D plane and pick the best fit.
    auto coords = view_plane_coords(eye, dir, v);
    if (first) {
      min = coords;
      max = coords;
    } else {
      min = glm::min(min, coords);
      max = glm::max(max, coords);
    }
    first = false;
  };

  for (const auto& t : portal.portal_mesh->physical_faces()) {
    // Clip the triangle against the player plane to avoid complications.
    glm::vec3 ta{transform * glm::vec4{t.a, 1}};
    glm::vec3 tb{transform * glm::vec4{t.b, 1}};
    glm::vec3 tc{transform * glm::vec4{t.c, 1}};

    auto da = glm::dot(ta - eye - dir * z_near, dir);
    auto db = glm::dot(tb - eye - dir * z_near, dir);
    auto dc = glm::dot(tc - eye - dir * z_near, dir);

    if (da < 0 && db < 0 && dc < 0) {
      continue;
    } else if (da < 0 && db < 0) {
      handle_vertex(tc + dc / (dc - da) * (ta - tc));
      handle_vertex(tc + dc / (dc - db) * (tb - tc));
      handle_vertex(tc);
    } else if (db < 0 && dc < 0) {
      handle_vertex(ta + da / (da - db) * (tb - ta));
      handle_vertex(ta + da / (da - dc) * (tc - ta));
      handle_vertex(ta);
    } else if (dc < 0 && da < 0) {
      handle_vertex(tb + db / (db - dc) * (tc - tb));
      handle_vertex(tb + db / (db - da) * (ta - tb));
      handle_vertex(tb);
    } else if (da < 0) {
      handle_vertex(tb + db / (db - da) * (ta - tb));
      handle_vertex(tc + dc / (dc - da) * (ta - tc));
      handle_vertex(tb);
      handle_vertex(tc);
    } else if (db < 0) {
      handle_vertex(tc + dc / (dc - db) * (tb - tc));
      handle_vertex(ta + da / (da - db) * (tb - ta));
      handle_vertex(tc);
      handle_vertex(ta);
    } else if (dc < 0) {
      handle_vertex(ta + da / (da - dc) * (tc - ta));
      handle_vertex(tb + db / (db - dc) * (tc - tb));
      handle_vertex(ta);
      handle_vertex(tb);
    } else {
      handle_vertex(ta);
      handle_vertex(tb);
      handle_vertex(tc);
    }
  }

  min = glm::max(min, glm::vec2{-max_x, -max_y});
  max = glm::min(max, glm::vec2{max_x, max_y});

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
