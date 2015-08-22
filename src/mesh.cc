#include "mesh.h"
#include "../gen/mobius.pb.h"

#include <glm/vec3.hpp>
#include <glm/gtc/packing.hpp>
#include <GL/glew.h>
#include <fstream>
#include <vector>

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
                          uint32_t a, uint32_t b, uint32_t c)
  {
    auto va = glm::vec3{
        mesh.vertex(a).x(), mesh.vertex(a).y(), mesh.vertex(a).z()};
    auto vb = glm::vec3{
        mesh.vertex(b).x(), mesh.vertex(b).y(), mesh.vertex(b).z()};
    auto vc = glm::vec3{
        mesh.vertex(c).x(), mesh.vertex(c).y(), mesh.vertex(c).z()};

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
  };

  for (const auto& sub_mesh : mesh.sub()) {
    for (const auto& tri : sub_mesh.tri()) {
      add_triangle(sub_mesh.material(), tri.a(), tri.b(), tri.c());
    }
    for (const auto& quad: sub_mesh.quad()) {
      add_triangle(sub_mesh.material(), quad.a(), quad.b(), quad.c());
      add_triangle(sub_mesh.material(), quad.c(), quad.d(), quad.a());
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
}

Mesh::~Mesh()
{
  glDeleteBuffers(1, &_vbo);
  glDeleteBuffers(1, &_ibo);
}
