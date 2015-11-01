#ifndef MOBIUS_VISIBILITY_H
#define MOBIUS_VISIBILITY_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <utility>
#include <vector>

class Player;
class Mesh;
struct Portal;
typedef std::pair<glm::vec3, glm::vec3> Plane;

std::vector<Plane>
calculate_view_frustum(const Player& player, float aspect_ratio);

std::vector<Plane>
calculate_bounding_frustum(const Player& player, float aspect_ratio,
                           const glm::mat4& transform, const Portal& portal);

bool mesh_visible(const std::vector<Plane>& planes,
                  const glm::vec3& eye,
                  const glm::mat4& transform, const Mesh& mesh);

#endif
