#include "mesh.h"
#include "../gen/mobius.pb.h"

#include <glm/gtc/packing.hpp>
#include <GL/glew.h>
#include <fstream>

Mesh::Mesh(const std::string& path)
{
  mobius::proto::mesh mesh;
  std::ifstream ifstream(path);
  mesh.ParseFromIstream(&ifstream);

  std::vector<float> vertices;
  std::vector<GLushort> indices;

  auto add_vec3 = [&](const glm::vec3& v)
  {
    vertices.push_back(v.x);
    vertices.push_back(v.y);
    vertices.push_back(v.z);
  };

  auto add_triangle = [&](const mobius::proto::material& material,
                          uint32_t flags, uint32_t a, uint32_t b, uint32_t c)
  {
    auto va = glm::vec3{
        mesh.vertex(a).x(), mesh.vertex(a).y(), mesh.vertex(a).z()};
    auto vb = glm::vec3{
        mesh.vertex(b).x(), mesh.vertex(b).y(), mesh.vertex(b).z()};
    auto vc = glm::vec3{
        mesh.vertex(c).x(), mesh.vertex(c).y(), mesh.vertex(c).z()};

    if (flags & mobius::proto::sub_mesh_flags::VISIBLE) {
      auto colour = glm::vec3{
          material.colour().r(), material.colour().g(), material.colour().b()};
      auto normal = glm::normalize(glm::cross(vb - va, vc - va));

      add_vec3(va);
      add_vec3(normal);
      add_vec3(colour);
      add_vec3(vb);
      add_vec3(normal);
      add_vec3(colour);
      add_vec3(vc);
      add_vec3(normal);
      add_vec3(colour);

      indices.push_back(_vertex_count++);
      indices.push_back(_vertex_count++);
      indices.push_back(_vertex_count++);
    }
    if (flags & mobius::proto::sub_mesh_flags::PHYSICAL) {
      _physical.push_back(tri{va, vb, vc});
    }
  };

  for (const auto& sub : mesh.sub()) {
    for (const auto& tri : sub.tri()) {
      add_triangle(sub.material(), sub.flags(), tri.a(), tri.b(), tri.c());
    }
    for (const auto& quad: sub.quad()) {
      add_triangle(sub.material(), sub.flags(), quad.a(), quad.b(), quad.c());
      add_triangle(sub.material(), sub.flags(), quad.c(), quad.d(), quad.a());
    }
  }

  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(),
               indices.data(), GL_STATIC_DRAW);

  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        reinterpret_cast<void*>(sizeof(float) * 0));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        reinterpret_cast<void*>(sizeof(float) * 3));
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        reinterpret_cast<void*>(sizeof(float) * 6));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
  glBindVertexArray(0);
}

Mesh::~Mesh()
{
  glDeleteBuffers(1, &_vbo);
  glDeleteBuffers(1, &_ibo);
  glDeleteVertexArrays(1, &_vao);
}

uint32_t Mesh::vao() const
{
  return _vao;
}

uint32_t Mesh::vertex_count() const
{
  return _vertex_count;
}

const std::vector<Mesh::tri>& Mesh::physical() const
{
  return _physical;
}
