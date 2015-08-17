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

const float vertices[] = {
   0.25f,  0.25f, 0.75f, 1.0f,
  -0.25f,  0.25f, 0.75f, 1.0f,
   0.25f, -0.25f, 0.75f, 1.0f,

   0.25f, -0.25f, 0.75f, 1.0f,
  -0.25f,  0.25f, 0.75f, 1.0f,
  -0.25f, -0.25f, 0.75f, 1.0f,

   0.25f,  0.25f, -0.75f, 1.0f,
   0.25f, -0.25f, -0.75f, 1.0f,
  -0.25f,  0.25f, -0.75f, 1.0f,

   0.25f, -0.25f, -0.75f, 1.0f,
  -0.25f, -0.25f, -0.75f, 1.0f,
  -0.25f,  0.25f, -0.75f, 1.0f,

  -0.25f,  0.25f,  0.75f, 1.0f,
  -0.25f, -0.25f, -0.75f, 1.0f,
  -0.25f, -0.25f,  0.75f, 1.0f,

  -0.25f,  0.25f,  0.75f, 1.0f,
  -0.25f,  0.25f, -0.75f, 1.0f,
  -0.25f, -0.25f, -0.75f, 1.0f,

   0.25f,  0.25f,  0.75f, 1.0f,
   0.25f, -0.25f,  0.75f, 1.0f,
   0.25f, -0.25f, -0.75f, 1.0f,

   0.25f,  0.25f,  0.75f, 1.0f,
   0.25f, -0.25f, -0.75f, 1.0f,
   0.25f,  0.25f, -0.75f, 1.0f,

   0.25f,  0.25f, -0.75f, 1.0f,
  -0.25f,  0.25f,  0.75f, 1.0f,
   0.25f,  0.25f,  0.75f, 1.0f,

   0.25f,  0.25f, -0.75f, 1.0f,
  -0.25f,  0.25f, -0.75f, 1.0f,
  -0.25f,  0.25f,  0.75f, 1.0f,

   0.25f, -0.25f, -0.75f, 1.0f,
   0.25f, -0.25f,  0.75f, 1.0f,
  -0.25f, -0.25f,  0.75f, 1.0f,

   0.25f, -0.25f, -0.75f, 1.0f,
  -0.25f, -0.25f,  0.75f, 1.0f,
  -0.25f, -0.25f, -0.75f, 1.0f,
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);
}

Renderer::~Renderer()
{
  if (_fbo) {
    glDeleteFramebuffers(1, &_fbo);
  }
  if (_fbt) {
    glDeleteTextures(1, &_fbt);
  }
  glDeleteProgram(_program);
  glDeleteVertexArrays(1, &_vao);
  glDeleteBuffers(1, &_vbo);
}

void Renderer::resize(uint32_t width, uint32_t height)
{
  _width = width;
  _height = height;

  if (_fbo) {
    glDeleteFramebuffers(1, &_fbo);
  }
  if (_fbt) {
    glDeleteTextures(1, &_fbt);
  }
  glGenTextures(1, &_fbt);
  glGenFramebuffers(1, &_fbo);

  auto samples = 0;
  glGetIntegerv(GL_MAX_SAMPLES, &samples);

  auto target = samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
  glBindTexture(target, _fbt);
  if (samples > 1) {
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, _width, _height, false);
  } else {
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _fbt, 0);
}

void Renderer::camera(float frustum_scale, float z_near, float z_far)
{
  // Assumes:
  // + camera is at the origin
  // + viewing plane is axis-aligned with centre (0, 0, -1) and dimensions
  //   [-1, 1] in the X and Y axes.
  float s = frustum_scale;
  float zs = (z_near + z_far) / (z_near - z_far);
  float zt = (2 * z_near * z_far) / (z_near - z_far);

  _perspective_matrix = {
    s,  0,  0,  0,
    0,  s,  0,  0,
    0,  0, zs, zt,
    0,  0, -1,  0,
  };
}

void Renderer::render()
{
  glViewport(0, 0, _width, _height);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(_program);
  glUniformMatrix4fv(
      glGetUniformLocation(_program, "perspective_matrix"),
      1, GL_TRUE /* row-major */, _perspective_matrix.data());

  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 3 * 12);

  glDisableVertexAttribArray(0);
  glUseProgram(0);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK);
  glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
}