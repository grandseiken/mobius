#include "render.h"
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
}

#define GLEW_CHECK(value) \
  if (!value) std::cerr << "Warning: " #value " not supported\n";

#define SHADER(name, type) \
  create_shader( \
    #name, type, std::string(\
      gen_shaders_##name##_glsl, \
      gen_shaders_##name##_glsl + gen_shaders_##name##_glsl_len))

#include "../gen/shaders/main.vertex.glsl.h"
#include "../gen/shaders/main.fragment.glsl.h"
#include "../gen/shaders/quad.vertex.glsl.h"
#include "../gen/shaders/grain.fragment.glsl.h"

static const float quad_vertices[] = {
  -1, -1, 1, 1,
  -1,  1, 1, 1,
   1, -1, 1, 1,
   1,  1, 1, 1,
};

static const GLushort quad_indices[] = {
  0, 2, 1, 1, 2, 3,
};

static const float cube_vertices[] = {
  // Vertices.
  -1, -1, -1, 1,
   1, -1, -1, 1,
  -1,  1, -1, 1,
   1,  1, -1, 1,

  -1, -1,  1, 1,
   1, -1,  1, 1,
  -1,  1,  1, 1,
   1,  1,  1, 1,

  -1, -1, -1, 1,
   1, -1, -1, 1,
  -1, -1,  1, 1,
   1, -1,  1, 1,

  -1,  1, -1, 1,
   1,  1, -1, 1,
  -1,  1,  1, 1,
   1,  1,  1, 1,

  -1, -1, -1, 1,
  -1,  1, -1, 1,
  -1, -1,  1, 1,
  -1,  1,  1, 1,

   1, -1, -1, 1,
   1,  1, -1, 1,
   1, -1,  1, 1,
   1,  1,  1, 1,

  // Normals.
   0,  0, -1,
   0,  0, -1,
   0,  0, -1,
   0,  0, -1,

   0,  0,  1,
   0,  0,  1,
   0,  0,  1,
   0,  0,  1,

   0, -1,  0,
   0, -1,  0,
   0, -1,  0,
   0, -1,  0,

   0,  1,  0,
   0,  1,  0,
   0,  1,  0,
   0,  1,  0,

  -1,  0,  0,
  -1,  0,  0,
  -1,  0,  0,
  -1,  0,  0,

   1,  0,  0,
   1,  0,  0,
   1,  0,  0,
   1,  0,  0,
};

static const GLushort cube_indices[] = {
   0,  2,  1,  1,  2,  3,
   4,  5,  6,  6,  5,  7,
   8,  9, 10, 10,  9, 11,
  12, 14, 13, 13, 14, 15,
  16, 18, 17, 17, 18, 19,
  20, 21, 22, 22, 21, 23,
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

  _main_program = create_program("main", {
      SHADER(main_vertex, GL_VERTEX_SHADER),
      SHADER(main_fragment, GL_FRAGMENT_SHADER)});
  _grain_program = create_program("grain", {
      SHADER(quad_vertex, GL_VERTEX_SHADER),
      SHADER(grain_fragment, GL_FRAGMENT_SHADER)});

  glGenBuffers(1, &_quad_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &_quad_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

  glGenBuffers(1, &_cube_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _cube_vbo);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &_cube_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cube_ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);
}

Renderer::~Renderer()
{
  if (_fbo) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_fbt);
    glDeleteTextures(1, &_fbd);
  }
  glDeleteProgram(_main_program);
  glDeleteProgram(_grain_program);
  glDeleteVertexArrays(1, &_vao);
  glDeleteBuffers(1, &_quad_vbo);
  glDeleteBuffers(1, &_quad_ibo);
  glDeleteBuffers(1, &_cube_vbo);
  glDeleteBuffers(1, &_cube_ibo);
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
        GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8,
        _dimensions.x, _dimensions.y, false);
    glBindTexture(target, _fbd);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_COMPONENT24,
        _dimensions.x, _dimensions.y, false);
  } else {
    glBindTexture(target, _fbt);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        _dimensions.x, _dimensions.y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(target, _fbd);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
        _dimensions.x, _dimensions.y, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _fbt, 0);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, _fbd, 0);
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
  _world_transform = world_transform;
  _normal_transform_dirty = true;
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
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::cube(const glm::vec3& colour) const
{
  compute_transform();

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glDepthRange(0, 1);
  glDisable(GL_BLEND);

  glUseProgram(_main_program);
  glUniformMatrix3fv(
      glGetUniformLocation(_main_program, "normal_transform"),
      1, GL_FALSE, glm::value_ptr(_normal_transform));
  glUniformMatrix4fv(
      glGetUniformLocation(_main_program, "world_transform"),
      1, GL_FALSE, glm::value_ptr(_world_transform));
  glUniformMatrix4fv(
      glGetUniformLocation(_main_program, "vp_transform"),
      1, GL_FALSE, glm::value_ptr(_vp_transform));
  glUniform3fv(
      glGetUniformLocation(_main_program, "colour"), 1, glm::value_ptr(colour));
  glUniform3fv(
      glGetUniformLocation(_main_program, "light_source"), 1,
      glm::value_ptr(_light.source));
  glUniform1f(
      glGetUniformLocation(_main_program, "light_intensity"), _light.intensity);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _cube_vbo);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
                        reinterpret_cast<void*>(sizeof(float) * 4 * 24));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cube_ibo);
  glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]),
                 GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
}

void Renderer::grain(float amount) const
{
  glUseProgram(_grain_program);
  glUniform1f(glGetUniformLocation(_grain_program, "amount"), amount);
  glUniform1f(glGetUniformLocation(_grain_program, "frame"), _frame);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_ibo);
  glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(quad_indices[0]),
                 GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(0);
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
