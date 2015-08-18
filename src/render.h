#ifndef MOBIUS_RENDER_H
#define MOBIUS_RENDER_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <array>
#include <cstdint>

class Renderer {
public:
  Renderer();
  ~Renderer();

  void resize(const glm::ivec2& dimensions);
  void perspective(float fov, float z_near, float z_far);
  void camera(const glm::vec3& eye, const glm::vec3& target,
              const glm::vec3& up);
  void world(const glm::mat4& world_transform);

  void clear() const;
  void cube(const glm::vec3& colour) const;
  void grain(float amount) const;
  void render() const;

private:
  void compute_transform() const;

  uint32_t _fbo = 0;
  uint32_t _fbt = 0;
  uint32_t _fbd = 0;

  uint32_t _main_program = 0;
  uint32_t _quad_program = 0;
  uint32_t _vao = 0;
  mutable uint32_t _frame = 0;

  uint32_t _quad_vbo = 0;
  uint32_t _quad_ibo = 0;
  uint32_t _cube_vbo = 0;
  uint32_t _cube_ibo = 0;

  // For perspective (camera space to clip space) transform.
  glm::ivec2 _dimensions;
  struct {
    float fov = 0;
    float z_near = 0;
    float z_far = 0;
  } _perspective;

  // For view (world space to camera space) transform.
  glm::mat4 _view_transform;

  // For world (model space to world space) transform.
  glm::mat4 _world_transform;

  mutable glm::mat4 _transform;
  mutable bool _transform_dirty;
};

#endif
