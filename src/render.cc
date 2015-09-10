#include "render.h"
#include "mesh.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  GLuint create_shader(const std::string& name,
                       uint32_t shader_type, const std::string& source)
  {
    auto shader = glCreateShader(shader_type);
    auto data = source.data();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
      return shader;
    }

    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetShaderInfoLog(shader, log_length, nullptr, log.get());

    std::cerr <<
      "Compile error in shader '" << name << "':\n" << log.get() << "\n";
    return 0;
  }

  GLuint create_program(const std::string& name,
                        const std::vector<GLuint>& shaders)
  {
    for (const auto& shader : shaders) {
      if (shader == 0) {
        return 0;
      }
    }

    GLuint program = glCreateProgram();
    for (const auto& shader : shaders) {
      glAttachShader(program, shader);
    }
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) {
      for (const auto& shader : shaders) {
        glDetachShader(program, shader);
        glDeleteShader(shader);
      }
      return program;
    }

    GLint log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetProgramInfoLog(program, log_length, nullptr, log.get());

    std::cerr <<
      "Link error in program '" << name << "':\n" << log.get() << "\n";
    return 0;
  }

  void render_settings(
      bool depth_test, bool depth_mask, bool colour_mask, bool blend)
  {
    if (!depth_test && !depth_mask) {
      glDisable(GL_DEPTH_TEST);
    } else {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(depth_test ? GL_LEQUAL : GL_ALWAYS);
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

#define GLEW_CHECK(value) \
  if (!value) std::cerr << "Warning: " #value " not supported\n";

#define SHADER(name, type) \
  create_shader( \
    #name, type, std::string(\
      gen_shaders_##name##_glsl, \
      gen_shaders_##name##_glsl + gen_shaders_##name##_glsl_len))

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#include "../gen/shaders/draw.vertex.glsl.h"
#include "../gen/shaders/draw.fragment.glsl.h"
#include "../gen/shaders/quad.vertex.glsl.h"
#include "../gen/shaders/grain.fragment.glsl.h"
#include "../gen/shaders/world.vertex.glsl.h"
#include "../gen/tools/simplex_lut.h"

static const float quad_vertices[] = {
  -1, -1, 1, 1,
  -1,  1, 1, 1,
   1, -1, 1, 1,
   1,  1, 1, 1,
};

static const GLushort quad_indices[] = {
  0, 2, 1, 1, 2, 3,
};

Renderer::Renderer()
{
  auto glew_ok = glewInit();
  if (glew_ok != GLEW_OK) {
    std::cerr << "Couldn't initialize GLEW: " <<
        glewGetErrorString(glew_ok) << "\n";
  }

  GLEW_CHECK(GLEW_VERSION_3_3);
  GLEW_CHECK(GLEW_ARB_shading_language_100);
  GLEW_CHECK(GLEW_ARB_shader_objects);
  GLEW_CHECK(GLEW_ARB_vertex_shader);
  GLEW_CHECK(GLEW_ARB_fragment_shader);
  GLEW_CHECK(GLEW_ARB_framebuffer_object);
  GLEW_CHECK(GLEW_EXT_framebuffer_multisample);
  // Should we have multiple permutation resolutions for different texture
  // sizes? Or just use several 1D textures and pack them in?
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_max_texture_size);

  _draw_program = create_program("main", {
      SHADER(draw_vertex, GL_VERTEX_SHADER),
      SHADER(draw_fragment, GL_FRAGMENT_SHADER)});
  _grain_program = create_program("grain", {
      SHADER(quad_vertex, GL_VERTEX_SHADER),
      SHADER(grain_fragment, GL_FRAGMENT_SHADER)});
  _world_program = create_program("world", {
      SHADER(world_vertex, GL_VERTEX_SHADER)});

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

  glGenBuffers(1, &_quad_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &_quad_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &_grain_vao);
  glBindVertexArray(_grain_vao);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_ibo);
  glBindVertexArray(0);
}

Renderer::~Renderer()
{
  if (_fbo) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_fbt);
    glDeleteTextures(1, &_fbd);
  }
  glDeleteProgram(_draw_program);
  glDeleteProgram(_grain_program);
  glDeleteBuffers(1, &_quad_vbo);
  glDeleteBuffers(1, &_quad_ibo);
  glDeleteSamplers(1, &_sampler);
  glDeleteTextures(1, &_simplex_gradient_lut);
  glDeleteTextures(1, &_simplex_permutation_lut);
  glDeleteVertexArrays(1, &_grain_vao);
}

void Renderer::resize(const glm::ivec2& dimensions)
{
  _dimensions = dimensions;
  _vp_transform_dirty = true;

  if (_fbo) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_fbt);
    glDeleteTextures(1, &_fbd);
  }
  glGenTextures(1, &_fbt);
  glGenTextures(1, &_fbd);
  glGenFramebuffers(1, &_fbo);

  auto samples = 0;
  glGetIntegerv(GL_MAX_SAMPLES, &samples);

  auto target = samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
  if (samples > 1) {
    glBindTexture(target, _fbt);
    glTexImage2DMultisample(
        target, samples, GL_RGBA8,
        _dimensions.x, _dimensions.y, false);
    glBindTexture(target, _fbd);
    glTexImage2DMultisample(
        target, samples, GL_DEPTH24_STENCIL8,
        _dimensions.x, _dimensions.y, false);
  } else {
    glBindTexture(target, _fbt);
    glTexImage2D(
        target, 0, GL_RGBA8,
        _dimensions.x, _dimensions.y, 0,
        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
    glBindTexture(target, _fbd);
    glTexImage2D(
        target, 0, GL_DEPTH24_STENCIL8,
        _dimensions.x, _dimensions.y, 0,
        GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _fbt, 0);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, _fbd, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer is not complete\n";
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
  world(world_transform, {}, {});
}

void Renderer::world(const glm::mat4& world_transform,
                     const glm::vec3& clip_point, const glm::vec3& clip_normal)
{
  _world_transform = world_transform;
  _normal_transform_dirty = true;
  _clip_point = clip_point;
  _clip_normal = clip_normal;
}

void Renderer::light(const glm::vec3& source, float intensity)
{
  _light.source = source;
  _light.intensity = intensity;
}

void Renderer::clear() const
{
  ++_frame;

  glViewport(0, 0, _dimensions.x, _dimensions.y);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
  glEnable(GL_MULTISAMPLE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilMask(0xff);
  glClearColor(0, 0, 0, 0);
  glClearDepth(1);
  glClearStencil(0x00);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::clear_stencil(uint32_t stencil_mask) const
{
  glStencilMask(stencil_mask);
  glClearStencil(0x00);
  glClear(GL_STENCIL_BUFFER_BIT);
}

void Renderer::stencil(const Mesh& mesh, uint32_t stencil_ref,
                       uint32_t test_mask, uint32_t write_mask) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, test_mask, write_mask);

  glUseProgram(_world_program);
  set_mvp_uniforms(_world_program);

  glBindVertexArray(mesh.vao());
  glDrawElements(GL_TRIANGLES, mesh.vertex_count(), GL_UNSIGNED_SHORT, 0);
  glUseProgram(0);
}

void Renderer::depth(const Mesh& mesh, uint32_t stencil_ref,
                                       uint32_t stencil_mask) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);

  glUseProgram(_world_program);
  set_mvp_uniforms(_world_program);

  glBindVertexArray(mesh.vao());
  glDrawElements(GL_TRIANGLES, mesh.vertex_count(), GL_UNSIGNED_SHORT, 0);
  glUseProgram(0);
}

void Renderer::depth_clear(const Mesh& mesh, uint32_t stencil_ref,
                                             uint32_t stencil_mask) const
{
  compute_transform();
  render_settings(/* dtest */ false, /* dmask */ true,
                  /* cmask */ false, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);
  glDepthRange(1, 1);

  glUseProgram(_world_program);
  set_mvp_uniforms(_world_program);

  glBindVertexArray(mesh.vao());
  glDrawElements(GL_TRIANGLES, mesh.vertex_count(), GL_UNSIGNED_SHORT, 0);
  glUseProgram(0);
}

void Renderer::draw(const Mesh& mesh, uint32_t stencil_ref,
                                      uint32_t stencil_mask) const
{
  compute_transform();
  render_settings(/* dtest */ true, /* dmask */ true,
                  /* cmask */ true, /* blend */ false);
  stencil_settings(stencil_ref, stencil_mask, 0x00);

  glUseProgram(_draw_program);
  set_simplex_uniforms(_draw_program);
  set_mvp_uniforms(_draw_program);
  glUniformMatrix3fv(
      glGetUniformLocation(_draw_program, "normal_transform"),
      1, GL_FALSE, glm::value_ptr(_normal_transform));

  glUniform3fv(
      glGetUniformLocation(_draw_program, "light_source"), 1,
      glm::value_ptr(_light.source));
  glUniform1f(
      glGetUniformLocation(_draw_program, "light_intensity"), _light.intensity);

  glBindVertexArray(mesh.vao());
  glDrawElements(GL_TRIANGLES, mesh.vertex_count(), GL_UNSIGNED_SHORT, 0);
  glUseProgram(0);
}

void Renderer::grain(float amount) const
{
  render_settings(/* dtest */ false, /* dmask */ false,
                  /* cmask */ true, /* blend */ true);
  stencil_settings(0x00, 0x00, 0x00);

  glUseProgram(_grain_program);
  glUniform1f(glGetUniformLocation(_grain_program, "amount"), amount);
  glUniform1f(glGetUniformLocation(_grain_program, "frame"), _frame);
  set_simplex_uniforms(_grain_program);

  glBindVertexArray(_grain_vao);
  glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(quad_indices[0]),
                 GL_UNSIGNED_SHORT, 0);
  glUseProgram(0);
}

void Renderer::render() const
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK);
  glBlitFramebuffer(0, 0, _dimensions.x, _dimensions.y,
                    0, 0, _dimensions.x, _dimensions.y,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

float Renderer::get_aspect_ratio() const
{
  return _dimensions.x / _dimensions.y;
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

void Renderer::set_mvp_uniforms(uint32_t program) const
{
  glUniformMatrix4fv(
      glGetUniformLocation(program, "world_transform"),
      1, GL_FALSE, glm::value_ptr(_world_transform));
  glUniformMatrix4fv(
      glGetUniformLocation(program, "vp_transform"),
      1, GL_FALSE, glm::value_ptr(_vp_transform));

  if (_clip_normal != glm::vec3{}) {
    glEnable(GL_CLIP_DISTANCE0);
    glUniform3fv(
        glGetUniformLocation(program, "clip_point"),
        1, glm::value_ptr(_clip_point));
    glUniform3fv(
        glGetUniformLocation(program, "clip_normal"),
        1, glm::value_ptr(_clip_normal));
  } else {
    glDisable(GL_CLIP_DISTANCE0);
  }
}

void Renderer::set_simplex_uniforms(uint32_t program) const
{
  glUniform1i(
      glGetUniformLocation(program, "simplex_gradient_lut"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_1D, _simplex_gradient_lut);
  glBindSampler(0, _sampler);
  glUniform1i(
      glGetUniformLocation(program, "simplex_use_permutation_lut"),
      uint32_t(_max_texture_size) >= ARRAY_LENGTH(gen_simplex_permutation_lut));
  glUniform1i(
      glGetUniformLocation(program, "simplex_permutation_lut"), 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _simplex_permutation_lut);
  glBindSampler(1, _sampler);

}
