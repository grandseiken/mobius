#include "mesh.h"
#include "proto_util.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <GL/glew.h>
#include <unordered_set>

namespace {
  struct TriIndex {
    size_t a;
    size_t b;
    size_t c;
  };

  glm::mat4 submesh_transform(const mobius::proto::submesh& submesh)
  {
    glm::mat4 transform{1};
    if (submesh.has_translate()) {
      transform *= glm::translate(glm::mat4{1}, load_vec3(submesh.translate()));
    }
    if (submesh.has_scale()) {
      transform *= glm::scale(glm::mat4{1}, load_vec3(submesh.scale()));
    }
    return transform;
  }

  size_t geometry_size(const mobius::proto::geometry& geometry)
  {
    return geometry.tri_size() + 2 * geometry.quad_size();
  }

  TriIndex geometry_tri(
      const mobius::proto::geometry& geometry, size_t index)
  {
    if (index < size_t(geometry.tri_size())) {
      const auto& t = geometry.tri(index);
      return TriIndex{t.a(), t.b(), t.c()};
    }

    const auto& q = geometry.quad((index - geometry.tri_size()) / 2);
    return index % 2 ? TriIndex{q.a(), q.b(), q.c()} :
        TriIndex{q.c(), q.d(), q.a()};
  }
}

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
    generate_data(visible_vertices, visible_indices, mesh, mesh.submesh(i));
    generate_outlines(mesh, mesh.submesh(i));
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

const std::vector<Mesh::outline_data>& Mesh::outlines() const
{
  return _outline_data;
}

void Mesh::generate_data(std::vector<float>& visible_vertices,
                         std::vector<unsigned short>& visible_indices,
                         const mobius::proto::mesh& mesh,
                         const mobius::proto::submesh& submesh)
{
  const auto& geometry = mesh.geometry(submesh.geometry());
  std::unordered_set<size_t> physical_indices;
  auto transform = submesh_transform(submesh);

  auto add_visible_data = [&](const glm::vec3& v)
  {
    visible_vertices.push_back(v.x);
    visible_vertices.push_back(v.y);
    visible_vertices.push_back(v.z);
  };

  auto add_visible_vertex_data = [&](const glm::vec3& v, const glm::vec3& n,
                                     const glm::vec3& c)
  {
    add_visible_data(v);
    add_visible_data(n);
    add_visible_data(c);
  };

  auto add_triangle = [&](const TriIndex& t)
  {
    const auto& flags = submesh.flags();
    auto colour = load_rgb(submesh.material().colour());
    glm::vec3 va{transform * glm::vec4{load_vec3(mesh.vertex(t.a)), 1}};
    glm::vec3 vb{transform * glm::vec4{load_vec3(mesh.vertex(t.b)), 1}};
    glm::vec3 vc{transform * glm::vec4{load_vec3(mesh.vertex(t.c)), 1}};

    auto normal = glm::cross(vb - va, vc - va);
    if (normal == glm::vec3{}) {
      return;
    }
    normal = glm::normalize(normal);

    if (flags & mobius::proto::submesh::VISIBLE) {
      add_visible_vertex_data(va, normal, colour);
      add_visible_vertex_data(vb, normal, colour);
      add_visible_vertex_data(vc, normal, colour);

      visible_indices.push_back(visible_indices.size());
      visible_indices.push_back(visible_indices.size());
      visible_indices.push_back(visible_indices.size());
    }
    if (flags & mobius::proto::submesh::PHYSICAL) {
      _physical_faces.push_back({va, vb, vc});
      physical_indices.insert(t.a);
      physical_indices.insert(t.b);
      physical_indices.insert(t.c);
    }
  };

  if (submesh.flags() & mobius::proto::submesh::PHYSICAL) {
    for (uint32_t point : geometry.point()) {
      physical_indices.insert(point);
    }
  }

  for (size_t i = 0; i < geometry_size(geometry); ++i) {
    add_triangle(geometry_tri(geometry, i));
  }

  for (const auto& index : physical_indices) {
    auto v = transform * glm::vec4(load_vec3(mesh.vertex(index)), 1.);
    _physical_vertices.push_back(glm::vec3{v});
  }
}

void Mesh::generate_outlines(const mobius::proto::mesh& mesh,
                             const mobius::proto::submesh& submesh)
{
  auto transform = submesh_transform(submesh);
  auto colour = load_rgb(submesh.material().colour());
  const auto& geometry = mesh.geometry(submesh.geometry());
  if (!(submesh.flags() & mobius::proto::submesh::VISIBLE)) {
    return;
  }

  auto check = [&](const glm::vec3& a0, const glm::vec3& a1,
                   const glm::vec3& b0, const glm::vec3& b1,
                   const glm::vec3& at,
                   const glm::vec3& a_normal, const glm::vec3& b_normal)
  {
    // Skip concave edges.
    if (a0 == b1 && a1 == b0 && glm::dot(at - a0, b_normal) < 0) {
      _outline_data.push_back({a0, a1, a_normal, b_normal, colour});
    }
  };

  for (size_t i = 0; i < geometry_size(geometry); ++i) {
    for (size_t j = 1 + i; j < geometry_size(geometry); ++j) {
      auto t = geometry_tri(geometry, i);
      auto u = geometry_tri(geometry, j);

      glm::vec3 ta{transform * glm::vec4{load_vec3(mesh.vertex(t.a)), 1}};
      glm::vec3 tb{transform * glm::vec4{load_vec3(mesh.vertex(t.b)), 1}};
      glm::vec3 tc{transform * glm::vec4{load_vec3(mesh.vertex(t.c)), 1}};

      glm::vec3 ua{transform * glm::vec4{load_vec3(mesh.vertex(u.a)), 1}};
      glm::vec3 ub{transform * glm::vec4{load_vec3(mesh.vertex(u.b)), 1}};
      glm::vec3 uc{transform * glm::vec4{load_vec3(mesh.vertex(u.c)), 1}};

      auto t_normal = glm::cross(tb - ta, tc - ta);
      auto u_normal = glm::cross(ub - ua, uc - ua);
      if (t_normal == glm::vec3{} || u_normal == glm::vec3{}) {
        continue;
      }
      t_normal = glm::normalize(t_normal);
      u_normal = glm::normalize(u_normal);
      if (t_normal == u_normal) {
        continue;
      }

      check(ta, tb, ua, ub, tc, t_normal, u_normal);
      check(ta, tb, ub, uc, tc, t_normal, u_normal);
      check(ta, tb, uc, ua, tc, t_normal, u_normal);
      check(tb, tc, ua, ub, ta, t_normal, u_normal);
      check(tb, tc, ub, uc, ta, t_normal, u_normal);
      check(tb, tc, uc, ua, ta, t_normal, u_normal);
      check(tc, ta, ua, ub, tb, t_normal, u_normal);
      check(tc, ta, ub, uc, tb, t_normal, u_normal);
      check(tc, ta, uc, ua, tb, t_normal, u_normal);
    }
  }
}
