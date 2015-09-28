#ifndef MOBIUS_WORLD_H
#define MOBIUS_WORLD_H

#include "collision.h"
#include "player.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
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
  uint32_t portal_id;

  std::unique_ptr<Mesh> portal_mesh;
  Orientation local;
  Orientation remote;
};

struct Chunk {
  std::unique_ptr<Mesh> mesh;
  std::vector<Portal> portals;
};

struct RenderMetrics {
  uint32_t chunks;
  uint32_t depth;
  uint32_t breadth;
};

class Renderer;
class World {
public:
  World(const std::string& path, Renderer& renderer);

  void update(const ControlData& controls);
  void render(RenderMetrics& metrics) const;

private:
  typedef std::pair<glm::vec3, glm::vec3> plane;
  struct world_data {
    glm::mat4 orientation;
    std::vector<plane> clip_planes;
  };

  struct chunk_entry {
    const Chunk* chunk;
    const Portal* source;
    const Chunk* source_chunk;
    uint32_t stencil;

    world_data data;
    world_data source_data;
  };

  void render_iteration(
      uint32_t iteration, RenderMetrics& metrics,
      const std::vector<chunk_entry>& read_buffer,
      std::vector<chunk_entry>& write_buffer) const;

  void render_objects_in_chunk(
      uint32_t iteration, const Chunk* chunk,
      const world_data& data, uint32_t stencil_ref) const;

  static const uint32_t MAX_ITERATIONS = 3;

  Renderer& _renderer;
  std::unordered_map<std::string, Chunk> _chunks;
  std::string _active_chunk;
  glm::mat4 _orientation;
  Collision _collision;
  Player _player;
};

#endif
