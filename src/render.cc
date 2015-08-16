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

#define SHADER(name, type) \
  create_shader( \
    #name, type, std::string(\
      src_shaders_##name##_glsl, \
      src_shaders_##name##_glsl + src_shaders_##name##_glsl_len))

#include "../gen/shaders/main.vertex.glsl.h"
#include "../gen/shaders/main.fragment.glsl.h"

Renderer::Renderer()
: _program{0}
, _vao{0}
, _vbo{0}
{
  glewInit();
  if (!GLEW_VERSION_3_2) {
    std::cerr << "Warning: OpenGL 3.2 not available\n";
  }

  _program = create_program("main", {
      SHADER(main_vertex, GL_VERTEX_SHADER),
      SHADER(main_fragment, GL_FRAGMENT_SHADER)});

  const float vertices[] = {
    .75f, .75f, 0.f, 1.f,
    .75f, -.75f, 0.f, 1.f,
    -.75f, .75f, 0.f, 1.f,
  };

  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);
}

void Renderer::render(uint32_t width, uint32_t height)
{
  glViewport(0, 0, width, height);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(_program);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisableVertexAttribArray(0);
  glUseProgram(0);
}
