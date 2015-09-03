#ifndef MOBIUS_RENDER_H
#define MOBIUS_RENDER_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <array>
#include <cstdint>

class Mesh;

class Renderer {
public:
  Renderer();
  ~Renderer();

  void resize(const glm::ivec2& dimensions);
  void perspective(float fov, float z_near, float z_far);
  void camera(const glm::vec3& eye, const glm::vec3& target,
              const glm::vec3& up);
  void world(const glm::mat4& world_transform);
  void world(const glm::mat4& world_transform,
             const glm::vec3& clip_point, const glm::vec3& clip_normal);
  void light(const glm::vec3& source, float intensity);

  void clear() const;
  void stencil(const Mesh& mesh, uint32_t stencil_write) const;
  void depth(const Mesh& mesh, uint32_t stencil_target) const;
  void depth_clear(const Mesh& mesh, uint32_t stencil_target) const;
  void draw(const Mesh& mesh, uint32_t stencil_target) const;
  void grain(float amount) const;
  void render() const;

private:
  void compute_transform() const;
  // We should really avoid setting uniforms that haven't changed. Maybe using
  // uniform buffers?
  void set_mvp_uniforms(uint32_t program) const;
  void set_simplex_uniforms(uint32_t program) const;

  uint32_t _fbo = 0;
  uint32_t _fbt = 0;
  uint32_t _fbd = 0;

  uint32_t _draw_program = 0;
  uint32_t _world_program = 0;
  uint32_t _grain_program = 0;
  uint32_t _simplex_gradient_lut = 0;
  uint32_t _simplex_permutation_lut = 0;
  uint32_t _sampler = 0;

  int32_t _max_texture_size = 0;
  mutable uint32_t _frame = 0;
  uint32_t _grain_vao = 0;
  uint32_t _quad_vbo = 0;
  uint32_t _quad_ibo = 0;

  struct {
    glm::vec3 source;
    float intensity;
  } _light;

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

  // For custom clipping.
  glm::vec3 _clip_point;
  glm::vec3 _clip_normal;

  mutable glm::mat4 _vp_transform;
  mutable glm::mat3 _normal_transform;
  mutable bool _vp_transform_dirty = false;
  mutable bool _normal_transform_dirty = false;
};

#endif
