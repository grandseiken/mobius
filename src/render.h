#ifndef MOBIUS_RENDER_H
#define MOBIUS_RENDER_H

#include <cstdint>

class Renderer {
public:
  Renderer(uint32_t width, uint32_t height);
  ~Renderer();

  void resize(uint32_t width, uint32_t height);
  void render();

private:
  uint32_t _width;
  uint32_t _height;

  uint32_t _fbo = 0;
  uint32_t _fbt = 0;

  uint32_t _program = 0;
  uint32_t _vao = 0;
  uint32_t _vbo = 0;
};

#endif
