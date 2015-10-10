#ifndef MOBIUS_GLO_H
#define MOBIUS_GLO_H

#include <GL/glew.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#define GLEW_CHECK(value) \
  if (!value) std::cerr << "Warning: " #value " not supported\n";

struct GlInit {
public:
  GlInit()
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
  }
};

struct GlShader {
public:
  GlShader(const std::string& name, uint32_t shader_type,
           const std::string& source)
  {
    shader = glCreateShader(shader_type);

    auto data = source.data();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetShaderInfoLog(shader, log_length, nullptr, log.get());

    std::cerr <<
      "Compile error in shader '" << name << "':\n" << log.get() << "\n";
    shader = 0;
  }

  ~GlShader()
  {
    glDeleteShader(shader);
  }

private:
  GLuint shader = 0;
  friend struct GlProgram;
};

struct GlActiveProgram {
public:
  ~GlActiveProgram()
  {
    glUseProgram(0);
  }

  GLuint uniform(const std::string& name) const
  {
    return glGetUniformLocation(program, name.c_str());
  }

private:
  GlActiveProgram(GLuint program)
  : program(program) {}

  GLuint program;
  friend struct GlProgram;
};

struct GlProgram {
public:
  GlProgram(const std::string& name, const std::vector<GlShader>& shaders)
  {
    for (const auto& shader : shaders) {
      if (shader.shader == 0) {
        return;
      }
    }

    program = glCreateProgram();
    for (const auto& shader : shaders) {
      glAttachShader(program, shader.shader);
    }
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    for (const auto& shader : shaders) {
      glDetachShader(program, shader.shader);
    }
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetProgramInfoLog(program, log_length, nullptr, log.get());

    std::cerr <<
      "Link error in program '" << name << "':\n" << log.get() << "\n";
    program = 0;
    return;
  }

  ~GlProgram()
  {
    glDeleteProgram(program);
  }

  GlActiveProgram use() const
  {
    glUseProgram(program);
    return {program};
  }

private:
  GLuint program = 0;
};

#undef GLEW_CHECK
#endif
