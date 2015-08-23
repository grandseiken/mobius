#include "collision.h"
#include "mesh.h"
#include <glm/vec4.hpp>
#include <glm/gtc/packing.hpp>
#include <vector>

glm::vec3 Collision::bound_translation(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& translation) const
{
  std::vector<Triangle> object_physical;
  for (const auto& t : object.physical()) {
    object_physical.push_back(Triangle{
      glm::vec3{object_transform * glm::vec4{t.a, 1.}},
      glm::vec3{object_transform * glm::vec4{t.b, 1.}},
      glm::vec3{object_transform * glm::vec4{t.c, 1.}},
    });
  }

  // This is, of course, very inefficient. We can:
  // - consider vertices alone rather than repeating when shared by triangles
  // - use an acceleration structure (spatial index)
  float bound_scale = 2;
  for (const auto& to : object_physical) {
    for (const auto& te : environment.physical()) {
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(to.a, translation, te));
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(to.b, translation, te));
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(to.c, translation, te));
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(te.a, -translation, to));
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(te.b, -translation, to));
      bound_scale = std::min(
          bound_scale, ray_tri_intersection(te.c, -translation, to));
    }
  }
  return bound_scale * translation;
}

float Collision::ray_tri_intersection(
    const glm::vec3& origin, const glm::vec3& direction,
    const Triangle& t) const
{
  // Ray r(t) = origin + t * direction
  // Triangle r(u, v) = (1 - u - v) * tri.a + u * tri.b + v * tri.c
  // Solves for r(t) = t(u, v).
  static const float epsilon = 1. / (1024 * 1024);

  glm::vec3 ab = t.b - t.a;
  glm::vec3 ac = t.c - t.a;
  glm::vec3 pv = glm::cross(direction, ac);
  float determinant = glm::dot(pv, ab);

  // If |determinant| is small, ray lies in plane of triangle. If it is
  // negative, triangle is back-facing.
  if (determinant < epsilon) {
    return 2;
  }

  glm::vec3 tv = origin - t.a;
  float u = glm::dot(tv, pv);
  if (u <= 0 || u >= determinant) {
    return 2;
  }

  glm::vec3 qv = glm::cross(tv, ab);
  float v = glm::dot(direction, qv);
  if (v <= 0 || u + v >= determinant) {
    return 2;
  }

  float bound = glm::dot(ac, qv) / determinant;
  return bound < 0 ? 2.f : std::max(0.f, bound);
}
