#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <string>
#include <vector>

class Mesh {
public:

  Mesh(const std::string& path);
  ~Mesh();

  struct tri {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
  };

  uint32_t vao() const;
  uint32_t vertex_count() const;
  const std::vector<tri>& physical() const;

private:

  uint32_t _vertex_count = 0;
  uint32_t _vao;
  uint32_t _vbo;
  uint32_t _ibo;

  std::vector<tri> _physical;

};

#endif
