#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace mobius {
  namespace proto {
    class mesh;
    class submesh;
    class geometry;
  }
}

struct Triangle {
  glm::vec3 a;
  glm::vec3 b;
  glm::vec3 c;
};

class Mesh {
public:
  Mesh();
  Mesh(const std::string& path);
  Mesh(const mobius::proto::mesh& mesh);
  ~Mesh();

  uint32_t vao() const;
  uint32_t vertex_count() const;
  const std::vector<Triangle>& physical_faces() const;
  const std::vector<glm::vec3>& physical_vertices() const;

private:
  void generate_data(std::vector<float>& visible_vertices,
                     std::vector<unsigned short>& visible_indices,
                     std::vector<Triangle>& physical_faces,
                     std::vector<glm::vec3>& physical_vertices,
                     const mobius::proto::mesh& mesh,
                     const mobius::proto::submesh& submesh) const;

  uint32_t _visible_vertex_count = 0;
  uint32_t _vao;
  uint32_t _vbo;
  uint32_t _ibo;

  std::vector<Triangle> _physical_faces;
  std::vector<glm::vec3> _physical_vertices;
};

#endif
