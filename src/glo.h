#ifndef MOBIUS_GLO_H
#define MOBIUS_GLO_H

#include <GL/glew.h>
#include <glm/vec2.hpp>
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
  : program(program)
  {
    glUseProgram(program);
  }

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
    return {program};
  }

private:
  GLuint program = 0;
};

struct GlActiveFramebuffer {
public:
  ~GlActiveFramebuffer()
  {
    if (read) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    } else {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
  }

private:
  GlActiveFramebuffer(GLuint fbo, bool read)
  : read(read) {
    if (read) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    } else {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    }
  }

  bool read;
  friend struct GlFramebuffer;
};

struct GlFramebuffer {
public:
  GlFramebuffer(const glm::ivec2& dimensions,
                bool depth_stencil, bool attempt_multisampling)
  : multisampled{false}
  {
    GLint max_samples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
    multisampled = max_samples > 1 && attempt_multisampling;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &fbt);
    if (depth_stencil) {
      glGenTextures(1, &fbd);
    }

    if (multisampled) {
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fbt);
      glTexImage2DMultisample(
          GL_TEXTURE_2D_MULTISAMPLE, max_samples, GL_RGBA8,
          dimensions.x, dimensions.y, false);

      if (depth_stencil) {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fbd);
        glTexImage2DMultisample(
            GL_TEXTURE_2D_MULTISAMPLE, max_samples, GL_DEPTH24_STENCIL8,
            dimensions.x, dimensions.y, false);
      }
    } else {
      glBindTexture(GL_TEXTURE_2D, fbt);
      glTexImage2D(
          GL_TEXTURE_2D, 0, GL_RGBA8,
          dimensions.x, dimensions.y, 0,
          GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);

      if (depth_stencil) {
        glBindTexture(GL_TEXTURE_2D, fbd);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
            dimensions.x, dimensions.y, 0,
            GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
      }
    }

     auto target = multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbt, 0);
    if (depth_stencil) {
      glFramebufferTexture2D(
          GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, fbd, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "Intermediate framebuffer is not complete\n";
    }
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  ~GlFramebuffer()
  {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &fbt);
    if (fbd) {
      glDeleteTextures(1, &fbd);
    }
  }

  bool is_multisampled() const
  {
    return multisampled;
  }

  GlActiveFramebuffer draw() const
  {
    return {fbo, false};
  }

  GlActiveFramebuffer read() const
  {
    return {fbo, true};
  }

  GLuint texture() const
  {
    return fbt;
  }

private:
  bool multisampled;
  GLuint fbo = 0;
  GLuint fbt = 0;
  GLuint fbd = 0;
};

struct GlVertexData {
public:
  GlVertexData(const std::vector<GLfloat>& data,
               const std::vector<GLushort>& indices, GLuint hint)
  : size(indices.size())
  {
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(GLfloat) * data.size(), data.data(), hint);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(GLushort) * indices.size(), indices.data(), hint);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  ~GlVertexData()
  {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);
  }

  void enable_attribute(GLuint location, GLuint count,
                        GLuint stride, GLuint offset) const
  {
    glBindVertexArray(vao);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
        location, count, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
        reinterpret_cast<void*>(sizeof(float) * offset));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void draw() const
  {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
  }

private:
  GLuint size;
  GLuint vbo = 0;
  GLuint ibo = 0;
  GLuint vao = 0;
};

#undef GLEW_CHECK
#endif
