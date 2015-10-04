#ifndef MOBIUS_GEOMETRY_H
#define MOBIUS_GEOMETRY_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

inline glm::vec3 side_direction(const glm::vec3& dir)
{
  return glm::normalize(glm::cross(dir, {0, 1, 0}));
}

inline glm::vec3 up_direction(const glm::vec3& dir)
{
  return glm::normalize(glm::cross(side_direction(dir), dir));
}

inline glm::vec3 forward_direction(const glm::vec3& dir)
{
  return glm::normalize(glm::cross({0, 1, 0}, side_direction(dir)));
}

inline glm::vec2 view_plane_coords(
    const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& v)
{
  // Perspective-project each point onto plane. This is basically
  // reimplementing the look-at matrix, right?
  auto depth = glm::dot(v - eye, dir);
  auto distance = glm::dot(v - eye - dir, dir);
  auto projection = v - distance * dir;

  return {glm::dot(projection - eye - dir, side_direction(dir)) / depth,
          glm::dot(projection - eye - dir, up_direction(dir)) / depth};
}

#endif
