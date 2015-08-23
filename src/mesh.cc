#include "mesh.h"
#include "../gen/mobius.pb.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <GL/glew.h>
#include <fstream>

namespace {
  glm::vec3 load_vec3(const mobius::proto::vec3& v)
  {
    return {v.x(), v.y(), v.z()};
  }

  glm::vec3 load_rgb(const mobius::proto::rgb& v)
  {
    return {v.r(), v.g(), v.b()};
  }
}

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
                          const glm::mat4& transform,
                          uint32_t flags, uint32_t a, uint32_t b, uint32_t c)
  {
    auto va = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(a)), 1.));
    auto vb = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(b)), 1.));
    auto vc = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(c)), 1.));

    if (flags & mobius::proto::submesh::VISIBLE) {
      auto colour = load_rgb(material.colour());
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
    if (flags & mobius::proto::submesh::PHYSICAL) {
      _physical.push_back({va, vb, vc});
    }
  };

  for (const auto& submesh : mesh.submesh()) {
    const auto& faces = mesh.faces(submesh.faces());
    glm::mat4 transform{1};
    if (submesh.has_translate()) {
      transform *= glm::translate(glm::mat4{1}, load_vec3(submesh.translate()));
    }
    if (submesh.has_scale()) {
      transform *= glm::scale(glm::mat4{1}, load_vec3(submesh.scale()));
    }

    for (const auto& tri : faces.tri()) {
      add_triangle(submesh.material(), transform, submesh.flags(),
                   tri.a(), tri.b(), tri.c());
    }

    for (const auto& quad: faces.quad()) {
      add_triangle(submesh.material(), transform, submesh.flags(),
                   quad.a(), quad.b(), quad.c());
      add_triangle(submesh.material(), transform, submesh.flags(),
                   quad.c(), quad.d(), quad.a());
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

const std::vector<Triangle>& Mesh::physical() const
{
  return _physical;
}
