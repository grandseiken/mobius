#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <string>
#include <vector>

struct Triangle {
  glm::vec3 a;
  glm::vec3 b;
  glm::vec3 c;
};

class Mesh {
public:
  Mesh(const std::string& path);
  ~Mesh();

  uint32_t vao() const;
  uint32_t vertex_count() const;
  const std::vector<Triangle>& physical() const;

private:
  uint32_t _vertex_count = 0;
  uint32_t _vao;
  uint32_t _vbo;
  uint32_t _ibo;

  std::vector<Triangle> _physical;
};

#endif
