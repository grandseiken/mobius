#include "player.h"
#include "collision.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(const Collision& collision, const glm::vec3& position,
               float fov, float z_near, float z_far)
: _collision(collision)
, _mesh{"gen/data/player.mesh.pb"}
, _position{position}
, _look_dir{0, 0, 1}
, _fov{fov}
, _z_near{z_near}
, _z_far{z_far}
, _angle{0, 0}
{
}

void Player::update(const ControlData& controls,
                    const std::vector<Object>& environment)
{
  static const float epsilon = 1. / 1024;
  _angle += (1.f / 2048) * controls.mouse_move;
  _angle.y = glm::clamp(_angle.y, -glm::pi<float>() / 2 + epsilon,
                                   glm::pi<float>() / 2 - epsilon);
  _look_dir = glm::vec3{
      cos(_angle.y) * sin(-_angle.x),
      sin(_angle.y),
      cos(_angle.y) * cos(-_angle.x)};

  auto velocity =
      get_side_direction() * float(controls.right - controls.left) +
      get_forward_direction() * float(controls.forward - controls.reverse);

  if (velocity != glm::vec3{0, 0, 0}) {
    velocity = (1.f / 32) * glm::normalize(velocity);
    // TODO: sometimes the player gets stuck sliding along the wall. Why?
    _position += _collision.translation(
        {&_mesh, glm::translate(glm::mat4{}, _position)},
        environment, velocity, 8 /* iterations */);
  }

  if (controls.jump) {
    _fall_speed = -1. / 16;
  }
  _fall_speed = std::min(1. / 4, _fall_speed + 1. / 512);
  _fall_speed *= _collision.coefficient(
      {&_mesh, glm::translate(glm::mat4{}, _position)},
      environment, {0, -_fall_speed, 0});
  _position -= glm::vec3{0, _fall_speed, 0};
}

const glm::vec3& Player::get_position() const
{
  return _position;
}

const glm::vec3& Player::get_head_position() const
{
  return _position;
}

const glm::vec3& Player::get_look_direction() const
{
  return _look_dir;
}

glm::vec3 Player::get_side_direction() const
{
  return glm::normalize(glm::cross(_look_dir, {0, 1, 0}));
}

glm::vec3 Player::get_up_direction() const
{
  return glm::normalize(glm::cross(get_side_direction(), _look_dir));
}

glm::vec3 Player::get_forward_direction() const
{
  return glm::normalize(glm::cross({0, 1, 0}, get_side_direction()));
}

float Player::get_fov() const
{
  return _fov;
}

float Player::get_z_near() const
{
  return _z_near;
}

float Player::get_z_far() const
{
  return _z_far;
}

const Mesh& Player::get_mesh() const
{
  return _mesh;
}
