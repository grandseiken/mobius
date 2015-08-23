#ifndef MOBIOS_COLLISION_H
#define MOBIUS_COLLISION_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Triangle;
class Mesh;
class Collision {
public:
  float coefficient(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& vector, glm::vec3* remaining = nullptr) const;

  glm::vec3 translation(
    const Mesh& object, const Mesh& environment,
    const glm::mat4x4& object_transform,
    const glm::vec3& vector, bool recursive = false) const;

private:
  float ray_tri_intersection(
    const glm::vec3& origin, const glm::vec3& direction,
    const Triangle& t) const;

  glm::vec3 point_tri_projection(
    const glm::vec3& point, const Triangle& t) const;
};

#endif
