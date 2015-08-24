#include "player.h"
#include "collision.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(const Collision& collision, const glm::vec3& position)
: _collision(collision)
, _mesh{"gen/data/player.mesh.pb"}
, _position{position}
, _look_dir{0, 0, 1}
, _angle{0, 0}
{
}

void Player::update(const ControlData& controls, const Mesh& environment)
{
  static const float epsilon = 1. / 1024;
  _angle += (1.f / 2048) * controls.mouse_move;
  _angle.y = glm::clamp(_angle.y, -glm::pi<float>() / 2 + epsilon,
                                   glm::pi<float>() / 2 - epsilon);

  _look_dir = glm::vec3{
      cos(_angle.y) * sin(-_angle.x),
      sin(_angle.y),
      cos(_angle.y) * cos(-_angle.x)};

  auto side_dir = glm::cross(_look_dir, {0, 1, 0});
  auto forward_dir = glm::cross({0, 1, 0}, side_dir);

  auto velocity =
      forward_dir * float(controls.forward - controls.reverse) +
      side_dir * float(controls.right - controls.left);

  if (velocity != glm::vec3{0, 0, 0}) {
    velocity = (1.f / 32) * glm::normalize(velocity);
    _position += _collision.translation(
        _mesh, environment,
        glm::translate(glm::mat4{}, _position),
        velocity, true /* recursive */);
  }

  if (controls.jump) {
    _fall_speed = -1. / 16;
  }
  _fall_speed = std::min(1. / 4, _fall_speed + 1. / 512);
  _fall_speed *= _collision.coefficient(
      _mesh, environment,
      glm::translate(glm::mat4{}, _position), {0, -_fall_speed, 0});
  _position -= glm::vec3{0, _fall_speed, 0};
}

glm::vec3 Player::get_head_position() const
{
  return _position + glm::vec3{0, .5, 0};
}

glm::vec3 Player::get_look_position() const
{
  return get_head_position() + _look_dir;
}
