#include "render.h"
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
      src_shaders_##name##_glsl, \
      src_shaders_##name##_glsl + src_shaders_##name##_glsl_len))

#include "../gen/shaders/main.vertex.glsl.h"
#include "../gen/shaders/main.fragment.glsl.h"

const float cube_vertices[] = {
   1,  1,  1, 1,
   1,  1, -1, 1,
   1, -1,  1, 1,
   1, -1, -1, 1,
  -1,  1,  1, 1,
  -1,  1, -1, 1,
  -1, -1,  1, 1,
  -1, -1, -1, 1,
};

const GLushort cube_indices[] = {
  0, 4, 2, 2, 4, 6,
  1, 3, 5, 3, 7, 5,
  4, 7, 6, 4, 5, 7,
  0, 2, 3, 0, 3, 1,
  1, 4, 0, 1, 5, 4,
  3, 2, 6, 3, 6, 7,
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

  _program = create_program("main", {
      SHADER(main_vertex, GL_VERTEX_SHADER),
      SHADER(main_fragment, GL_FRAGMENT_SHADER)});

  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
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
  glDeleteProgram(_program);
  glDeleteVertexArrays(1, &_vao);
  glDeleteBuffers(1, &_vbo);
  glDeleteBuffers(1, &_ibo);
}

void Renderer::resize(uint32_t width, uint32_t height)
{
  if (_width == width && _height == height) {
    return;
  }
  _width = width;
  _height = height;
  _perspective.dirty = true;

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
        GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, _width, _height, false);
    glBindTexture(target, _fbd);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE, samples,
        GL_DEPTH_COMPONENT24, _width, _height, false);
  } else {
    glBindTexture(target, _fbt);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(target, _fbd);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _width, _height, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _fbt, 0);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, _fbd, 0);
}

void Renderer::camera(float frustum_scale, float z_near, float z_far)
{
  _camera.frustum_scale = frustum_scale;
  _camera.z_near = z_near;
  _camera.z_far = z_far;
  _perspective.dirty = true;
}

void Renderer::translate(float x, float y, float z)
{
  _translate.x = x;
  _translate.y = y;
  _translate.z = z;
  _transform.dirty = true;
}

void Renderer::scale(float x, float y, float z)
{
  _scale.x = x;
  _scale.y = y;
  _scale.z = z;
  _transform.dirty = true;
}

void Renderer::clear() const
{
  glViewport(0, 0, _width, _height);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glDepthRange(0, 1);
}

void Renderer::cube(float r, float g, float b) const
{
  calculate_perspective_matrix();
  calculate_transform_matrix();

  glUseProgram(_program);
  glUniformMatrix4fv(
      glGetUniformLocation(_program, "perspective_matrix"),
      1, GL_TRUE /* row-major */, _perspective.matrix.data());
  glUniformMatrix4fv(
      glGetUniformLocation(_program, "transform_matrix"),
      1, GL_TRUE /* row-major */, _transform.matrix.data());
  glUniform4f(
      glGetUniformLocation(_program, "colour"), r, g, b, 1);


  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
  glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]),
                 GL_UNSIGNED_SHORT, 0);
  glDrawArrays(GL_TRIANGLES, 0, 3 * 12);

  glDisableVertexAttribArray(0);
  glUseProgram(0);
}

void Renderer::render() const
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK);
  glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::calculate_perspective_matrix() const
{
  if (!_perspective.dirty) {
    return;
  }
  _perspective.dirty = false;

  // Assumes:
  // + camera is at the origin
  // + viewing plane is axis-aligned with centre (0, 0, -1)
  // + viewing plane is [-1, 1] in Y axis and [-1, 1] multiplied by aspect
  //   ratio in the X axis.
  float sx = _camera.frustum_scale * (float(_height) / _width);
  float sy = _camera.frustum_scale;
  float sz = (_camera.z_near + _camera.z_far) /
      (_camera.z_near - _camera.z_far);
  float tz = (2 * _camera.z_near * _camera.z_far) /
      (_camera.z_near - _camera.z_far);

  _perspective.matrix = {
    sx,  0,  0,  0,
     0, sy,  0,  0,
     0,  0, sz, tz,
     0,  0, -1,  0,
  };
}

void Renderer::calculate_transform_matrix() const
{
  if (!_transform.dirty) {
    return;
  }
  _transform.dirty = false;

  float x = _translate.x;
  float y = _translate.y;
  float z = _translate.z;
  float sx = _scale.x;
  float sy = _scale.y;
  float sz = _scale.z;
  _transform.matrix = {
    sx,  0,  0,  x,
     0, sy,  0,  y,
     0,  0, sz,  z,
     0,  0,  0,  1,
  };
}
