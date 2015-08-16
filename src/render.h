#ifndef MOBIUS_RENDER_H
#define MOBIUS_RENDER_H

#include <cstdint>

class Renderer {
public:
  Renderer();
  void render(uint32_t width, uint32_t height);

private:
  uint32_t _program;
  uint32_t _vao;
  uint32_t _vbo;
};

#endif
