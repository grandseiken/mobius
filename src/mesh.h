#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <cstdint>
#include <string>

class Mesh {
public:

  Mesh(const std::string& path);
  ~Mesh();

  uint32_t vao() const;
  uint32_t vertex_count() const;

private:

  uint32_t _vertex_count = 0;
  uint32_t _vao;
  uint32_t _vbo;
  uint32_t _ibo;

};

#endif
