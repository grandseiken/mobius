#ifndef MOBIUS_WORLD_H
#define MOBIUS_WORLD_H

#include "mesh.h"
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct Portal {
  std::string chunk_name;
  Mesh portal_mesh;

  glm::vec3 local_origin;
  glm::vec3 local_normal;
  glm::vec3 remote_origin;
  glm::vec3 remote_normal;
};

struct Chunk {
  Mesh mesh;
  std::vector<Portal> portals;
};

class World {
public:
  World(const std::string& path);

private:
  std::unordered_map<std::string, Chunk> _chunks;

};

#endif
