#ifndef MOBIUS_RENDER_H
#define MOBIUS_RENDER_H

#include <array>
#include <cstdint>

class Renderer {
public:
  Renderer();
  ~Renderer();

  void resize(uint32_t width, uint32_t height);
  void camera(float frustum_scale, float z_near, float z_far);
  void render();

private:
  uint32_t _width = 0;
  uint32_t _height = 0;

  uint32_t _fbo = 0;
  uint32_t _fbt = 0;

  uint32_t _program = 0;
  uint32_t _vao = 0;
  uint32_t _vbo = 0;

  std::array<float, 16> _perspective_matrix = {{0}};
};

#endif
