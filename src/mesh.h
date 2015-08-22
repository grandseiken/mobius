#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <cstdint>
#include <string>

class Mesh {
public:

  Mesh(const std::string& path);
  ~Mesh();

private:

  friend class Renderer;
  uint32_t _vertex_count = 0;
  uint32_t _vbo;
  uint32_t _ibo;

};

#endif
