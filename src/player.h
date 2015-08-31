#ifndef MOBIUS_PLAYER_H
#define MOBIUS_PLAYER_H

#include "mesh.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

struct ControlData {
  bool left = false;
  bool right = false;
  bool forward = false;
  bool reverse = false;
  bool jump = false;
  glm::vec2 mouse_move;
};

class Collision;
class Object;
class Player {
public:
  Player(const Collision& collision, const glm::vec3& position);

  void update(const ControlData& controls,
              const std::vector<Object>& environment);
  glm::vec3 get_position() const;
  glm::vec3 get_head_position() const;
  glm::vec3 get_look_position() const;

private:
  const Collision& _collision;
  const Mesh _mesh;

  glm::vec3 _position;
  glm::vec3 _look_dir;
  glm::vec2 _angle;
  float _fall_speed = 0;

};

#endif
