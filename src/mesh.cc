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
  std::vector<float> visible_vertices;
  std::vector<GLushort> visible_indices;
  for (size_t i = 0; i < unsigned(mesh.submesh_size()); ++i) {
    generate_data(visible_vertices, visible_indices,
                  _physical_faces, _physical_vertices,
                  mesh, mesh.submesh(i));
  }
  _visible_vertex_count = visible_indices.size();

  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * visible_vertices.size(),
               visible_vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(GLushort) * visible_indices.size(),
               visible_indices.data(), GL_STATIC_DRAW);

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

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
  return _visible_vertex_count;
}

const std::vector<Triangle>& Mesh::physical_faces() const
{
  return _physical_faces;
}

const std::vector<glm::vec3>& Mesh::physical_vertices() const
{
  return _physical_vertices;
}

void Mesh::generate_data(std::vector<float>& visible_vertices,
                         std::vector<unsigned short>& visible_indices,
                         std::vector<Triangle>& physical_faces,
                         std::vector<glm::vec3>& physical_vertices,
                         const mobius::proto::mesh& mesh,
                         const mobius::proto::submesh& submesh) const
{
  const auto& geometry = mesh.geometry(submesh.geometry());
  std::unordered_set<size_t> physical_indices;

  glm::mat4 transform{1};
  if (submesh.has_translate()) {
    transform *= glm::translate(glm::mat4{1}, load_vec3(submesh.translate()));
  }
  if (submesh.has_scale()) {
    transform *= glm::scale(glm::mat4{1}, load_vec3(submesh.scale()));
  }

  auto add_visible_data = [&](const glm::vec3& v)
  {
    visible_vertices.push_back(v.x);
    visible_vertices.push_back(v.y);
    visible_vertices.push_back(v.z);
  };

  auto add_visible_vertex_data = [&](
      const glm::vec3& v, const glm::vec3& normal, const glm::vec3& colour)
  {
    add_visible_data(v);
    add_visible_data(normal);
    add_visible_data(colour);
  };

  auto add_triangle = [&](size_t a, size_t b, size_t c)
  {
    const auto& material = submesh.material();
    const auto& flags = submesh.flags();

    auto va = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(a)), 1.));
    auto vb = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(b)), 1.));
    auto vc = glm::vec3(transform * glm::vec4(load_vec3(mesh.vertex(c)), 1.));

    if (flags & mobius::proto::submesh::VISIBLE) {
      auto colour = load_rgb(material.colour());
      auto normal = glm::normalize(glm::cross(vb - va, vc - va));

      add_visible_vertex_data(va, normal, colour);
      add_visible_vertex_data(vb, normal, colour);
      add_visible_vertex_data(vc, normal, colour);

      visible_indices.push_back(visible_indices.size());
      visible_indices.push_back(visible_indices.size());
      visible_indices.push_back(visible_indices.size());
    }
    if (flags & mobius::proto::submesh::PHYSICAL) {
      physical_faces.push_back({va, vb, vc});
      physical_indices.insert(a);
      physical_indices.insert(b);
      physical_indices.insert(c);
    }
  };

  if (submesh.flags() & mobius::proto::submesh::PHYSICAL) {
    for (uint32_t point : geometry.point()) {
      physical_indices.insert(point);
    }
  }

  for (const auto& tri : geometry.tri()) {
    add_triangle(tri.a(), tri.b(), tri.c());
  }

  for (const auto& quad: geometry.quad()) {
    add_triangle(quad.a(), quad.b(), quad.c());
    add_triangle(quad.c(), quad.d(), quad.a());
  }

  for (const auto& index : physical_indices) {
    auto v = transform * glm::vec4(load_vec3(mesh.vertex(index)), 1.);
    physical_vertices.push_back(glm::vec3{v});
  }
}
