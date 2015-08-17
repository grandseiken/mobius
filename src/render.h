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
  void translate(float x, float y, float z);
  void render() const;

private:
  void calculate_perspective_matrix() const;
  void calculate_transform_matrix() const;

  uint32_t _width = 0;
  uint32_t _height = 0;

  uint32_t _fbo = 0;
  uint32_t _fbt = 0;

  uint32_t _program = 0;
  uint32_t _vao = 0;
  uint32_t _vbo = 0;

  struct {
    float frustum_scale = 0;
    float z_near = 0;
    float z_far = 0;
  } _camera;

  struct {
    float x = 0;
    float y = 0;
    float z = 0;
  } _translate;

  struct matrix {
    std::array<float, 16> matrix = {{0}};
    bool dirty = false;
  };

  mutable matrix _perspective;
  mutable matrix _transform;
};

#endif
