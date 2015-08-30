#ifndef MOBIUS_WORLD_H
#define MOBIUS_WORLD_H

#include "collision.h"
#include "player.h"
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Orientation {
  glm::vec3 origin;
  glm::vec3 normal;
  glm::vec3 up;
};

struct Portal {
  std::string chunk_name;
  std::unique_ptr<Mesh> portal_mesh;

  Orientation local;
  Orientation remote;
};

struct Chunk {
  std::unique_ptr<Mesh> mesh;
  std::vector<Portal> portals;
};

class Renderer;
class World {
public:
  World(const std::string& path, Renderer& renderer);

  void update(const ControlData& controls);
  void render() const;

private:
  Renderer& _renderer;

  std::unordered_map<std::string, Chunk> _chunks;
  std::string _active_chunk;
  Collision _collision;
  Player _player;
};

#endif
