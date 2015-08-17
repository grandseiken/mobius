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

void Renderer::resize(const glm::ivec2& dimensions)
{
  _dimensions = dimensions;
  _transform_dirty = true;

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
  _transform_dirty = true;
}

void Renderer::world(const glm::mat4& world_transform)
{
  _world_transform = world_transform;
  _transform_dirty = true;
}

void Renderer::clear() const
{
  glViewport(0, 0, _dimensions.x, _dimensions.y);
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

void Renderer::cube(const glm::vec3& colour) const
{
  compute_transform();

  glUseProgram(_program);
  glUniformMatrix4fv(
      glGetUniformLocation(_program, "transform"),
      1, GL_FALSE, glm::value_ptr(_transform));
  glUniform3fv(
      glGetUniformLocation(_program, "colour"), 1, glm::value_ptr(colour));

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
  glBlitFramebuffer(0, 0, _dimensions.x, _dimensions.y,
                    0, 0, _dimensions.x, _dimensions.y,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::compute_transform() const
{
  if (!_transform_dirty) {
    return;
  }
  _transform_dirty = false;

  auto perspective_transform = glm::perspectiveRH(
      _perspective.fov, (float)_dimensions.x / _dimensions.y,
      _perspective.z_near, _perspective.z_far);

  _transform = perspective_transform * _world_transform;
}
