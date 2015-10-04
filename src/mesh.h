#ifndef MOBIUS_MESH_H
#define MOBIUS_MESH_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace mobius {
  namespace proto {
    class mesh;
    class submesh;
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

  struct outline_data {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 t_normal;
    glm::vec3 u_normal;
  };

  uint32_t vao() const;
  uint32_t vertex_count() const;
  const std::vector<Triangle>& physical_faces() const;
  const std::vector<glm::vec3>& physical_vertices() const;
  const std::vector<outline_data>& outlines() const;

private:
  void generate_data(std::vector<float>& visible_vertices,
                     std::vector<unsigned short>& visible_indices,
                     const mobius::proto::mesh& mesh,
                     const mobius::proto::submesh& submesh);

  void generate_outlines(const mobius::proto::mesh& mesh,
                         const mobius::proto::submesh& submesh);

  uint32_t _visible_vertex_count = 0;
  uint32_t _vao;
  uint32_t _vbo;
  uint32_t _ibo;

  std::vector<Triangle> _physical_faces;
  std::vector<glm::vec3> _physical_vertices;
  std::vector<outline_data> _outline_data;
};

#endif
