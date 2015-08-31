#include "collision.h"
#include "mesh.h"
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtx/norm.hpp>

namespace {
  Triangle object_triangle(const Triangle& t, const glm::mat4& transform)
  {
    return {
      glm::vec3{transform * glm::vec4{t.a, 1.}},
      glm::vec3{transform * glm::vec4{t.b, 1.}},
      glm::vec3{transform * glm::vec4{t.c, 1.}}};
  }
}

float Collision::coefficient(
    const Object& object, const std::vector<Object>& environment,
    const glm::vec3& vector, glm::vec3* remaining) const
{
  // One problem with the collision system is if an object's vertices happen to
  // line up with the split in e.g. a quad it can fall through. Currently this
  // is solved by adding redundant vertices to the objects, but it might need
  // some thought.
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

  // We will want to consider some sort of acceleration structure (spatial
  // index) at some point.
  for (const auto& v : object.mesh->physical_vertices()) {
    auto vt = glm::vec3{object.transform * glm::vec4{v, 1.}};
    for (const auto& env : environment) {
      for (const auto& t : env.mesh->physical_faces()) {
        bound_by(vt, true, object_triangle(t, env.transform));
        if (bound_scale <= 0) {
          break;
        }
      }
      if (bound_scale <= 0) {
        break;
      }
    }
    if (bound_scale <= 0) {
      break;
    }
  }
  for (const auto& t : object.mesh->physical_faces()) {
    auto tt = object_triangle(t, object.transform);
    for (const auto& env : environment) {
      for (const auto& v : env.mesh->physical_vertices()) {
        bound_by(glm::vec3{env.transform * glm::vec4{v, 1.}}, false, tt);
        if (bound_scale <= 0) {
          break;
        }
      }
      if (bound_scale <= 0) {
        break;
      }
    }
    if (bound_scale <= 0) {
      break;
    }
  }
  return bound_scale;
}

glm::vec3 Collision::translation(
    const Object& object, const std::vector<Object>& environment,
    const glm::vec3& vector, bool recursive) const
{
  static const float epsilon = 1. / (1024 * 1024);
  if (!recursive) {
    return vector * coefficient(object, environment, vector, nullptr);
  }

  glm::vec3 remaining;
  float scale = coefficient(object, environment, vector, &remaining);
  if (scale >= 1) {
    return vector;
  }

  auto first_translation = scale * vector;
  if (glm::l2Norm(remaining) < epsilon) {
    return first_translation;
  }

  Object next_object{
      object.mesh,
      glm::translate(glm::mat4{1}, first_translation) * object.transform};
  return first_translation +
      translation(next_object, environment, remaining, true);
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
