#include "collision.h"
#include "mesh.h"
#include <glm/vec4.hpp>
#include <glm/gtc/packing.hpp>
#include <vector>

float Collision::coefficient(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& translation, Triangle* blocker) const
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
  auto bound_by = [&](const glm::vec3& v, bool positive,
                      const Triangle& tri, const Triangle& env_tri)
  {
    float scale =
        ray_tri_intersection(v, positive ? translation : -translation, tri);
    if (scale < bound_scale) {
      bound_scale = scale;
      if (blocker) {
        *blocker = env_tri;
      }
    }
  };

  // This is, of course, very inefficient. We can:
  // - consider vertices alone rather than repeating when shared by triangles
  // - use an acceleration structure (spatial index)
  for (const auto& to : object_physical) {
    for (const auto& te : environment.physical()) {
      bound_by(to.a, true, te, te);
      bound_by(to.b, true, te, te);
      bound_by(to.c, true, te, te);
      bound_by(te.a, true, to, te);
      bound_by(te.b, true, to, te);
      bound_by(te.c, true, to, te);
    }
  }
  return bound_scale;
}

glm::vec3 Collision::translation(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& translation, bool recursive) const
{
  if (!recursive) {
    return translation * coefficient(
        object, environment, object_transform, translation, nullptr);
  }

  Triangle blocker;
  float scale =
      coefficient(object, environment, object_transform, translation, &blocker);
  if (scale >= 1) {
    return translation;
  }
  return scale * translation;
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
