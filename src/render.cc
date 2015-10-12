#include "render.h"
#include "geometry.h"
#include "player.h"
#include "mesh.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  void render_settings(bool depth_test, bool depth_mask, bool depth_eq,
                       bool colour_mask, bool blend)
  {
    if (!depth_test && !depth_mask) {
      glDisable(GL_DEPTH_TEST);
    } else {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(!depth_test ? GL_ALWAYS : depth_eq ? GL_EQUAL : GL_LEQUAL);
      glDepthMask(depth_mask);
      glDepthRange(0, 1);
    }
    glColorMask(colour_mask, colour_mask, colour_mask, colour_mask);
    if (blend) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
      glDisable(GL_BLEND);
    }
  }

  void stencil_settings(uint32_t ref, uint32_t test_mask, uint32_t write_mask) {
    if (test_mask || write_mask) {
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(test_mask ? GL_EQUAL : GL_ALWAYS, ref, test_mask);
      glStencilOp(GL_KEEP, GL_KEEP, write_mask ? GL_REPLACE : GL_KEEP);
      glStencilMask(write_mask);
    } else {
      glDisable(GL_STENCIL_TEST);
    }
  }
}

#define SHADER(name, type) \
  GlShader( \
    #name, type, std::string(\
      gen_shaders_##name##_glsl, \
      gen_shaders_##name##_glsl + gen_shaders_##name##_glsl_len))

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#include "../gen/shaders/draw.vertex.glsl.h"
#include "../gen/shaders/draw.fragment.glsl.h"
#include "../gen/shaders/quad.vertex.glsl.h"
#include "../gen/shaders/post.fragment.glsl.h"
#include "../gen/shaders/world.vertex.glsl.h"
#include "../gen/shaders/outline.vertex.glsl.h"
#include "../gen/shaders/outline.fragment.glsl.h"
#include "../gen/tools/simplex_lut.h"

std::vector<GLfloat> quad_vertices{
  -1, -1, 1, 1,
  -1,  1, 1, 1,
   1, -1, 1, 1,
   1,  1, 1, 1,
};

std::vector<GLushort> quad_indices{
  0, 2, 1, 1, 2, 3,
};

Renderer::Renderer()
: _draw_program("main", {SHADER(draw_vertex, GL_VERTEX_SHADER),
                         SHADER(draw_fragment, GL_FRAGMENT_SHADER)})
, _quad_program{"quad", {SHADER(quad_vertex, GL_VERTEX_SHADER)}}
, _post_program{"post", {SHADER(quad_vertex, GL_VERTEX_SHADER),
                         SHADER(post_fragment, GL_FRAGMENT_SHADER)}}
, _world_program{"world", {SHADER(world_vertex, GL_VERTEX_SHADER)}}
, _outline_program{"outline", {SHADER(outline_vertex, GL_VERTEX_SHADER),
                               SHADER(outline_fragment, GL_FRAGMENT_SHADER)}}
, _quad_data{quad_vertices, quad_indices, GL_STATIC_DRAW}
{
  // Should we have multiple permutation resolutions for different texture
  // sizes? Or just use several 1D textures and pack them in?
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_max_texture_size);

  glGenTextures(1, &_simplex_gradient_lut);
  glBindTexture(GL_TEXTURE_1D, _simplex_gradient_lut);
  glTexImage1D(
      GL_TEXTURE_1D, 0, GL_RGB8,
      ARRAY_LENGTH(gen_simplex_gradient_lut) / 3,
      0, GL_RGB, GL_FLOAT, gen_simplex_gradient_lut);

  glGenTextures(1, &_simplex_permutation_lut);
  glBindTexture(GL_TEXTURE_1D, _simplex_permutation_lut);
  glTexImage1D(
      GL_TEXTURE_1D, 0, GL_R32F,
      ARRAY_LENGTH(gen_simplex_permutation_lut),
      0, GL_RED, GL_FLOAT, gen_simplex_permutation_lut);

  glGenSamplers(1, &_sampler);
  glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  _quad_data.enable_attribute(0, 4, 0, 0);
}

Renderer::~Renderer()
{
  glDeleteSamplers(1, &_sampler);
  glDeleteTextures(1, &_simplex_gradient_lut);
  glDeleteTextures(1, &_simplex_permutation_lut);
}

void Renderer::resize(const glm::ivec2& dimensions)
{
  _dimensions = dimensions;
  _vp_transform_dirty = true;

  _framebuffer.reset(new GlFramebuffer{dimensions, true, true});
  if (_framebuffer->is_multisampled()) {
    _framebuffer_intermediate.reset(
        new GlFramebuffer{dimensions, false, false});
  } else {
    _framebuffer_intermediate.reset();
  }
}

void Renderer::perspective(float fov, float z_near, float z_far)
{
  _perspective.fov = fov;
  _perspective.z_near = z_near;
  _perspective.z_far = z_far;
  _vp_transform_dirty = true;
}

void Renderer::camera(const glm::vec3& eye, const glm::vec3& target,
                      const glm::vec3& up)
{
  _view_transform = glm::lookAt(eye, target, up);
  _vp_transform_dirty = true;
}

void Renderer::world(const glm::mat4& world_transform)
{
  world(world_transform, {});
}

void Renderer::world(const glm::mat4& world_transform,
                     const std::vector<plane>& clip_planes)
{
  _world_transform = world_transform;
  _normal_transform_dirty = true;
  _clip_planes = clip_planes;
}

void Renderer::clear() const
{
  ++_frame;

  glViewport(0, 0, _dimensions.x, _dimensions.y);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  auto draw = _framebuffer->draw();
  glEnable(GL_MULTISAMPLE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilMask(0xff);
  glClearColor(0, 0, 0, 0);
  glClearDepth(1);
  glClearStencil(0x00);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::clear_depth(uint32_t stencil_ref, uint32_t stencil_mask) const
{
  render_settings(/* dtest */ false, /* dmask */ true, /* depth_eq */ false,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);

  auto program = _post_program.use();
  auto draw = _framebuffer->draw();
  _quad_data.draw();
}

void Renderer::clear_stencil(uint32_t stencil_mask) const
{
  auto draw = _framebuffer->draw();
  glStencilMask(stencil_mask);
  glClearStencil(0x00);
  glClear(GL_STENCIL_BUFFER_BIT);
}

void Renderer::stencil(
    const GlVertexData& data, uint32_t stencil_ref,
    uint32_t test_mask, uint32_t write_mask, bool depth_eq) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true, depth_eq,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, test_mask, write_mask);

  auto program = _world_program.use();
  auto draw = _framebuffer->draw();

  set_mvp_uniforms(program);
  data.draw();
}

void Renderer::depth(const GlVertexData& data, uint32_t stencil_ref,
                                               uint32_t stencil_mask) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true, /* depth_eq */ false,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);

  auto program = _world_program.use();
  auto draw = _framebuffer->draw();

  set_mvp_uniforms(program);
  data.draw();
}

void Renderer::draw(const Mesh& mesh, const Player& player,
                    uint32_t stencil_ref, uint32_t stencil_mask) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true, /* depth_eq */ false,
                  /* cmask */ true, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);

  {
    auto program = _draw_program.use();
    auto draw = _framebuffer->draw();

    set_simplex_uniforms(program);
    set_mvp_uniforms(program);
    glUniformMatrix3fv(program.uniform("normal_transform"),
                       1, GL_FALSE, glm::value_ptr(_normal_transform));
    glUniform3fv(program.uniform("light_source"),
                 1, glm::value_ptr(player.get_head_position()));

    mesh.visible_data().draw();
  }

  // TODO: this outline code shouldn't really be here. It could also do
  // visibility calculations.
  const auto& eye = player.get_head_position();
  const auto& dir = player.get_look_direction();
  auto side = side_direction(dir);
  auto up = up_direction(dir);
  auto z_near = player.get_z_near();
  const float outline_width = 2. / _dimensions.y;

  std::vector<float> outline_vertices;
  std::vector<GLushort> outline_indices;
  size_t vertex_count = 0;
  auto add_outline = [&](const glm::vec3& v, const Mesh::outline_data& outline)
  {
    outline_vertices.push_back(v.x);
    outline_vertices.push_back(v.y);
    outline_vertices.push_back(v.z);
    outline_vertices.push_back(outline.hue);
    outline_vertices.push_back(outline.hue_shift);
  };

  for (const auto& outline : mesh.outlines()) {
    glm::vec3 a{_world_transform * glm::vec4{outline.a, 1.}};
    glm::vec3 b{_world_transform * glm::vec4{outline.b, 1.}};

    auto tn = _normal_transform * outline.t_normal;
    auto un = _normal_transform * outline.u_normal;
    bool t_front = glm::dot(tn, eye - a) >= 0;
    bool u_front = glm::dot(un, eye - a) >= 0;

    if (t_front != u_front) {
      // Clip against near-plane.
      auto da = glm::dot(a - eye - dir * z_near, dir);
      auto db = glm::dot(b - eye - dir * z_near, dir);
      if (da < 0 && db < 0) {
        continue;
      } else if (da < 0) {
        a = b + db / (db - da) * (a - b);
      } else if (db < 0) {
        b = a + da / (da - db) * (b - a);
      }

      // Project onto view-plane, work out directions.
      // TODO: there are still small inconsistencies in line thickness.
      auto coords_a = view_plane_coords(eye, dir, a);
      auto coords_b = view_plane_coords(eye, dir, b);
      auto offset = glm::normalize(coords_b - coords_a);
      glm::vec2 perp{offset.y, -offset.x};

      auto perp3 = perp.x * side + perp.y * up;
      auto offset3 = offset.x * side + offset.y * up;

      auto a0 = a + da * outline_width * (-offset3 - perp3);
      auto a1 = a + da * outline_width * (-offset3 + perp3);
      auto b0 = b + db * outline_width * (offset3 - perp3);
      auto b1 = b + db * outline_width * (offset3 + perp3);

      add_outline(a0, outline);
      add_outline(a1, outline);
      add_outline(b0, outline);
      add_outline(b1, outline);

      outline_indices.push_back(0 + vertex_count);
      outline_indices.push_back(1 + vertex_count);
      outline_indices.push_back(2 + vertex_count);
      outline_indices.push_back(1 + vertex_count);
      outline_indices.push_back(3 + vertex_count);
      outline_indices.push_back(2 + vertex_count);
      vertex_count += 4;
    }
  }
  if (outline_indices.empty()) {
    return;
  }

  GlVertexData outline_data{outline_vertices, outline_indices, GL_STREAM_DRAW};
  outline_data.enable_attribute(0, 3, 5, 0);
  outline_data.enable_attribute(1, 1, 5, 3);
  outline_data.enable_attribute(2, 1, 5, 4);

  auto program = _outline_program.use();
  auto draw = _framebuffer->draw();
  set_mvp_uniforms(program);
  glUniform3fv(program.uniform("light_source"),
               1, glm::value_ptr(player.get_head_position()));
  outline_data.draw();
}

void Renderer::render() const
{
  stencil_settings(0x00, 0x00, 0x00);
  render_settings(/* dtest */ false, /* dmask */ false, /* depth_eq */ false,
                  /* cmask */ true, /* blend */ false);

  glm::vec2 dimensions = _dimensions;
  if (_framebuffer_intermediate) {
    auto read = _framebuffer->read();
    auto draw = _framebuffer_intermediate->draw();
    glDrawBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, dimensions.x, dimensions.y,
                      0, 0, dimensions.x, dimensions.y,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }


  auto program = _post_program.use();
  glUniform1f(program.uniform("frame"), _frame);
  glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
  set_simplex_uniforms(program);

  auto texture = _framebuffer_intermediate ?
      _framebuffer_intermediate->texture() : _framebuffer->texture();
  glUniform1i(program.uniform("read_framebuffer"), 2);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindSampler(2, _sampler);

  _quad_data.draw();
}

float Renderer::get_aspect_ratio() const
{
  return float(_dimensions.x) / _dimensions.y;
}

void Renderer::compute_transform() const
{
  if (_vp_transform_dirty) {
    auto perspective_transform = glm::perspectiveRH(
        _perspective.fov, (float)_dimensions.x / _dimensions.y,
        _perspective.z_near, _perspective.z_far);

    _vp_transform_dirty = false;
    _vp_transform = perspective_transform * _view_transform;
  }

  if (_normal_transform_dirty) {
    _normal_transform_dirty = false;
    _normal_transform =
        glm::transpose(glm::inverse(glm::mat3{_world_transform}));
  }
}

void Renderer::set_mvp_uniforms(const GlActiveProgram& program) const
{
  glUniformMatrix4fv(program.uniform("world_transform"),
                     1, GL_FALSE, glm::value_ptr(_world_transform));
  glUniformMatrix4fv(program.uniform("vp_transform"),
                     1, GL_FALSE, glm::value_ptr(_vp_transform));

  static const uint32_t max_clip_distances = 8;
  for (uint32_t i = 0; i < max_clip_distances; ++i) {
    if (i < _clip_planes.size()) {
      glEnable(i + GL_CLIP_DISTANCE0);
    } else {
      glDisable(i + GL_CLIP_DISTANCE0);
    }
  }

  std::vector<glm::vec3> points;
  std::vector<glm::vec3> normals;
  for (const auto& pair : _clip_planes) {
    points.push_back(pair.first);
    normals.push_back(pair.second);
  }
  glUniform3fv(program.uniform("clip_points"),
               points.size(), glm::value_ptr(*points.data()));
  glUniform3fv(program.uniform("clip_normals"),
               normals.size(), glm::value_ptr(*normals.data()));
}

void Renderer::set_simplex_uniforms(const GlActiveProgram& program) const
{
  glUniform1i(
      program.uniform("simplex_use_permutation_lut"),
      uint32_t(_max_texture_size) >= ARRAY_LENGTH(gen_simplex_permutation_lut));

  glUniform1i(program.uniform("simplex_gradient_lut"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_1D, _simplex_gradient_lut);
  glBindSampler(0, _sampler);

  glUniform1i(program.uniform("simplex_permutation_lut"), 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _simplex_permutation_lut);
  glBindSampler(1, _sampler);

}
