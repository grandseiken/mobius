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
  Player(const Collision& collision, const glm::vec3& position,
         float fov, float z_near, float z_far);

  void update(const ControlData& controls,
              const std::vector<Object>& environment);

  const glm::vec3& get_position() const;
  const glm::vec3& get_head_position() const;
  const glm::vec3& get_look_direction() const;

  float get_fov() const;
  float get_z_near() const;
  float get_z_far() const;
  const Mesh& get_mesh() const;

private:
  const Collision& _collision;
  const Mesh _mesh;

  glm::vec3 _position;
  glm::vec3 _look_dir;
  float _fov;
  float _z_near;
  float _z_far;

  glm::vec2 _angle;
  float _fall_speed = 0;

};

#endif
