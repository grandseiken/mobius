#include "collision.h"
#include "mesh.h"
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>

float Collision::coefficient(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& vector, glm::vec3* remaining) const
{
  std::vector<Triangle> object_physical;
  for (const auto& t : object.physical()) {
    object_physical.push_back(Triangle{
      glm::vec3{object_transform * glm::vec4{t.a, 1.}},
      glm::vec3{object_transform * glm::vec4{t.b, 1.}},
      glm::vec3{object_transform * glm::vec4{t.c, 1.}},
    });
  }

  float bound_scale = 1;
  auto bound_by = [&](const glm::vec3& v, bool positive, const Triangle& tri)
  {
    glm::vec3 p_vector = positive ? vector : -vector;
    float scale = ray_tri_intersection(v, p_vector, tri);

    if (scale < bound_scale) {
      // This could be computed once only for the actual bounding pair.
      bound_scale = scale;
      if (remaining) {
        auto p_remaining = point_tri_projection(v + p_vector, tri) -
            (v + bound_scale * p_vector);
        *remaining = positive ? p_remaining : -p_remaining;
      }
    }
  };

  // This is, of course, very inefficient. We can:
  // - consider vertices alone rather than repeating when shared by triangles
  // - use an acceleration structure (spatial index)
  for (const auto& to : object_physical) {
    for (const auto& te : environment.physical()) {
      bound_by(to.a, true, te);
      bound_by(to.b, true, te);
      bound_by(to.c, true, te);
      bound_by(te.a, true, to);
      bound_by(te.b, true, to);
      bound_by(te.c, true, to);
    }
  }
  return bound_scale;
}

glm::vec3 Collision::translation(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& vector, bool recursive) const
{
  static const float epsilon = 1. / (1024 * 1024);
  if (!recursive) {
    return vector * coefficient(
        object, environment, object_transform, vector, nullptr);
  }

  glm::vec3 remaining;
  float scale = coefficient(
      object, environment, object_transform, vector, &remaining);
  if (scale >= 1) {
    return vector;
  }

  auto first_translation = scale * vector;
  if (glm::l2Norm(remaining) < epsilon) {
    return first_translation;
  }

  auto next_transform =
      glm::translate(glm::mat4{1}, first_translation) * object_transform;
  return first_translation + translation(
      object, environment, next_transform, remaining, true);
}

float Collision::ray_tri_intersection(
    const glm::vec3& origin, const glm::vec3& direction,
    const Triangle& t) const
{
  // Ray r(t) = origin + t * direction
  // Triangle r(u, v) = (1 - u - v) * tri.a + u * tri.b + v * tri.c
  // Solves for r(t) = t(u, v).
  static const float epsilon = 1. / (1024 * 1024);

  auto ab = t.b - t.a;
  auto ac = t.c - t.a;
  auto pv = glm::cross(direction, ac);
  float determinant = glm::dot(pv, ab);

  // If |determinant| is small, ray lies in plane of triangle. If it is
  // negative, triangle is back-facing.
  if (determinant < epsilon) {
    return 2;
  }

  auto tv = origin - t.a;
  float u = glm::dot(tv, pv);
  if (u <= 0 || u >= determinant) {
    return 2;
  }

  auto qv = glm::cross(tv, ab);
  float v = glm::dot(direction, qv);
  if (v <= 0 || u + v >= determinant) {
    return 2;
  }

  float bound = glm::dot(ac, qv) / determinant;
  return bound < 0 ? 2.f : std::max(0.f, bound);
}

glm::vec3 Collision::point_tri_projection(
    const glm::vec3& point, const Triangle& t) const
{
  // Simplified version of the above.
  auto ab = t.b - t.a;
  auto ac = t.c - t.a;
  auto normal = glm::cross(ab, ac);

  auto pv = glm::cross(normal, ac);
  float determinant = glm::dot(pv, ab);

  auto tv = point - t.a;
  float u = glm::dot(tv, pv) / determinant;

  auto qv = glm::cross(tv, ab);
  float v = glm::dot(normal, qv) / determinant;
  return (1 - u - v) * t.a + u * t.b + v * t.c;
}
