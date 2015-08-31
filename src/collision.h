#ifndef MOBIOS_COLLISION_H
#define MOBIUS_COLLISION_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

struct Triangle;
class Mesh;
struct Object {
  const Mesh* mesh;
  glm::mat4x4 transform;
};

class Collision {
public:
  float coefficient(
    const Object& object, const std::vector<Object>& environment,
    const glm::vec3& vector, glm::vec3* remaining = nullptr) const;

  glm::vec3 translation(
    const Object& object, const std::vector<Object>& environment,
    const glm::vec3& vector, uint32_t iterations = 1) const;

  bool intersection(
    const glm::vec3& origin, const glm::vec3& direction,
    const Object& object) const;

private:
  float ray_tri_intersection(
    const glm::vec3& origin, const glm::vec3& direction,
    const Triangle& t) const;

  glm::vec3 point_tri_projection(
    const glm::vec3& point, const Triangle& t) const;
};

#endif
