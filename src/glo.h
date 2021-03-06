#ifndef MOBIUS_GLO_H
#define MOBIUS_GLO_H

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
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

struct GlActiveTexture {
public:
  ~GlActiveTexture()
  {
    glBindTexture(target, 0);
  }

private:
  GlActiveTexture(GLuint texture, GLuint target)
  : target(target)
  {
    glBindTexture(target, texture);
  }

  GLuint target;
  friend struct GlTexture;
};

struct GlTexture {
public:
  GlTexture()
  {
    glGenTextures(1, &texture);
    glGenSamplers(1, &sampler);

    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  GlActiveTexture bind() const
  {
    return {texture, target};
  }

  void create_1d(GLsizei width, GLuint components, const float* data)
  {
    target = GL_TEXTURE_1D;
    auto active = bind();
    auto internalFormat = components == 3 ? GL_RGB8 :
                          components == 1 ? GL_R32F : 0;
    auto format = components == 3 ? GL_RGB :
                  components == 1 ? GL_RED : 0;
    glTexImage1D(target, 0, internalFormat, width, 0, format, GL_FLOAT, data);
  }

  void create_rgba(const glm::ivec2& dimensions, GLint samples = 1)
  {
    if (samples > 1) {
      target = GL_TEXTURE_2D_MULTISAMPLE;
      auto active = bind();
      glTexImage2DMultisample(
          target, samples, GL_RGBA8,
          dimensions.x, dimensions.y, false);
    } else {
      target = GL_TEXTURE_2D;
      auto active = bind();
      glTexImage2D(
          GL_TEXTURE_2D, 0, GL_RGBA8,
          dimensions.x, dimensions.y, 0,
          GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
    }
  }

  void create_depth_stencil(const glm::ivec2& dimensions, GLint samples = 1)
  {
    if (samples > 1) {
      target = GL_TEXTURE_2D_MULTISAMPLE;
      auto active = bind();
      glTexImage2DMultisample(
          target, samples, GL_DEPTH24_STENCIL8,
          dimensions.x, dimensions.y, false);
    } else {
      target = GL_TEXTURE_2D;
      auto active = bind();
      glTexImage2D(
          target, 0, GL_DEPTH24_STENCIL8,
          dimensions.x, dimensions.y, 0,
          GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    }
  }

  void attach_to_framebuffer(GLuint attachment) const
  {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, texture, 0);
  }

  ~GlTexture()
  {
    glDeleteSamplers(1, &sampler);
    glDeleteTextures(1, &texture);
  }

private:
  GLuint texture = 0;
  GLuint sampler = 0;
  GLuint target = 0;
  friend struct GlActiveProgram;
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

  void uniform_texture(const std::string& name, const GlTexture& texture) const
  {
    glUniform1i(uniform(name), texture_index);
    glActiveTexture(GL_TEXTURE0 + texture_index);
    glBindTexture(texture.target, texture.texture);
    glBindSampler(texture_index, texture.sampler);
    ++texture_index;
  }

private:
  GlActiveProgram(GLuint program)
  : program(program)
  {
    glUseProgram(program);
  }

  GLuint program;
  mutable GLuint texture_index = 0;
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
    GLint samples = 1;
    if (attempt_multisampling) {
      glGetIntegerv(GL_MAX_SAMPLES, &samples);
    }
    multisampled = samples > 1;

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    rgba_texture.create_rgba(dimensions, samples);
    rgba_texture.attach_to_framebuffer(GL_COLOR_ATTACHMENT0);
    if (depth_stencil) {
      depth_stencil_texture.reset(new GlTexture);
      depth_stencil_texture->create_depth_stencil(dimensions, samples);
      depth_stencil_texture->attach_to_framebuffer(GL_DEPTH_STENCIL_ATTACHMENT);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "Intermediate framebuffer is not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  ~GlFramebuffer()
  {
    glDeleteFramebuffers(1, &framebuffer);
  }

  bool is_multisampled() const
  {
    return multisampled;
  }

  GlActiveFramebuffer draw() const
  {
    return {framebuffer, false};
  }

  GlActiveFramebuffer read() const
  {
    return {framebuffer, true};
  }

  const GlTexture& texture() const
  {
    return rgba_texture;
  }

private:
  bool multisampled;
  GLuint framebuffer = 0;
  GlTexture rgba_texture;
  std::unique_ptr<GlTexture> depth_stencil_texture;
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
