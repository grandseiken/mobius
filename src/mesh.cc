#include "mesh.h"
#include "proto_util.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <GL/glew.h>
#include <unordered_map>
#include <unordered_set>

Mesh::Mesh()
{
}

Mesh::Mesh(const std::string& path)
: Mesh{load_proto<mobius::proto::mesh>(path)}
{
}

Mesh::Mesh(const mobius::proto::mesh& mesh)
{
  std::vector<float> vertices;
  std::vector<GLushort> indices;
  std::unordered_map<size_t, glm::mat4> submesh_transforms;
  std::unordered_map<size_t, std::unordered_set<size_t>>
      physical_indices;

  auto submesh_transform = [&](size_t submesh)
  {
    if (submesh_transforms.find(submesh) == submesh_transforms.end()) {
      const auto& proto = mesh.submesh(submesh);
      glm::mat4 transform{1};
      if (proto.has_translate()) {
        transform *= glm::translate(glm::mat4{1}, load_vec3(proto.translate()));
      }
      if (proto.has_scale()) {
        transform *= glm::scale(glm::mat4{1}, load_vec3(proto.scale()));
      }
      submesh_transforms[submesh] = transform;
    }
    return submesh_transforms[submesh];
  };

  auto add_vec3 = [&](const glm::vec3& v)
  {
    vertices.push_back(v.x);
    vertices.push_back(v.y);
    vertices.push_back(v.z);
  };

  auto add_triangle = [&](size_t submesh, size_t a, size_t b, size_t c)
  {
    auto transform = submesh_transform(submesh);
    const auto& material = mesh.submesh(submesh).material();
    const auto& flags = mesh.submesh(submesh).flags();

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
      _physical_faces.push_back({va, vb, vc});
      physical_indices[submesh].insert(a);
      physical_indices[submesh].insert(b);
      physical_indices[submesh].insert(c);
    }
  };

  for (size_t i = 0; i < unsigned(mesh.submesh_size()); ++i) {
    const auto& geometry = mesh.geometry(mesh.submesh(i).geometry());
    for (uint32_t point : geometry.point()) {
      physical_indices[i].insert(point);
    }

    for (const auto& tri : geometry.tri()) {
      add_triangle(i, tri.a(), tri.b(), tri.c());
    }

    for (const auto& quad: geometry.quad()) {
      add_triangle(i, quad.a(), quad.b(), quad.c());
      add_triangle(i, quad.c(), quad.d(), quad.a());
    }
  }

  for (const auto& pair : physical_indices) {
    auto transform = submesh_transform(pair.first);
    for (const auto& index : pair.second) {
      auto v = transform * glm::vec4(load_vec3(mesh.vertex(index)), 1.);
      _physical_vertices.push_back(glm::vec3{v});
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

const std::vector<Triangle>& Mesh::physical_faces() const
{
  return _physical_faces;
}

const std::vector<glm::vec3>& Mesh::physical_vertices() const
{
  return _physical_vertices;
}
