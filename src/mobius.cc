#include <SFML/Window.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "render.h"
#include "mesh.h"
#include "collision.h"

glm::ivec2 window_size(const sf::Window& window)
{
  return {window.getSize().x, window.getSize().y};
}

glm::ivec2 mouse_position(const sf::Window& window)
{
  return {sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y};
}

void set_mouse_position(const sf::Window& window, const glm::ivec2& position)
{
  sf::Mouse::setPosition({position.x, position.y}, window);
}

void reset_mouse_position(const sf::Window& window)
{
  set_mouse_position(window, window_size(window) / 2);
}

int main()
{
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 1;
  settings.majorVersion = 3;
  settings.minorVersion = 2;
  settings.attributeFlags = sf::ContextSettings::Core;

  sf::Window window{sf::VideoMode::getDesktopMode(), "MOBIUS", sf::Style::None};
  window.setVerticalSyncEnabled(true);
  Renderer renderer;
  renderer.resize(window_size(window));
  renderer.perspective(glm::pi<float>() / 2, 1. / 1024, 1024);

  Collision collision;
  Mesh player{"gen/player.mesh.pb"};
  Mesh level{"gen/level.mesh.pb"};
  glm::vec3 player_position{0, 8, 0};
  glm::vec2 player_angle{0, 0};

  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool focus = true;
  float fall_speed = 0.f;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        window.close();
      } else if (event.type == sf::Event::Resized) {
        renderer.resize(window_size(window));
        reset_mouse_position(window);
      } else if (event.type == sf::Event::GainedFocus) {
        focus = true;
        reset_mouse_position(window);
      } else if (event.type == sf::Event::LostFocus) {
        focus = false;
      } else if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::W) {
          forward = true;
        } else if (event.key.code == sf::Keyboard::S) {
          backward = true;
        } else if (event.key.code == sf::Keyboard::A) {
          left = true;
        } else if (event.key.code == sf::Keyboard::D) {
          right = true;
        } else if (event.key.code == sf::Keyboard::Space) {
          fall_speed = -1. / 16;
        }
      } else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::W) {
          forward = false;
        } else if (event.key.code == sf::Keyboard::S) {
          backward = false;
        } else if (event.key.code == sf::Keyboard::A) {
          left = false;
        } else if (event.key.code == sf::Keyboard::D) {
          right = false;
        }
      }
    }

    window.setMouseCursorVisible(!focus);
    if (focus) {
      auto offset = (1.f / 2048) * (glm::vec2(mouse_position(window)) -
                                    glm::vec2(window_size(window)) / 2.f);
      reset_mouse_position(window);

      static const float epsilon = 1. / 1024;
      player_angle += offset;
      player_angle.y = glm::clamp(
          player_angle.y,
          -glm::pi<float>() / 2 + epsilon, glm::pi<float>() / 2 - epsilon);
    }

    glm::vec3 player_look{
        cos(player_angle.y) * sin(-player_angle.x),
        sin(player_angle.y),
        cos(player_angle.y) * cos(-player_angle.x)};

    auto player_side = glm::cross(player_look, {0, 1, 0});
    auto player_forward = glm::cross({0, 1, 0}, player_side);

    auto velocity =
        player_forward * ((forward ? 1.f : 0.f) + (backward ? -1.f : 0.f)) +
        player_side * ((right ? 1.f : 0.f) + (left ? -1.f: 0.f));
    if (velocity != glm::vec3{0, 0, 0}) {
      velocity = (1.f / 32) * glm::normalize(velocity);
      player_position += collision.translation(
          player, level,
          glm::translate(glm::mat4{}, player_position),
          velocity, true /* recursive */);
    }

    fall_speed = std::min(1. / 4, fall_speed + 1. / 512);
    fall_speed *= collision.coefficient(
        player, level,
        glm::translate(glm::mat4{}, player_position), {0, -fall_speed, 0});
    player_position -= glm::vec3{0, fall_speed, 0};

    renderer.camera(
      player_position + glm::vec3{0, .5, 0},
      player_position + glm::vec3{0, .5, 0} + player_look, {0, 1, 0});
    renderer.light(player_position + glm::vec3{0, .5, 0}, 1.f);

    renderer.clear();
    renderer.world(glm::mat4{});
    renderer.mesh(level);
    renderer.grain(1. / 16);
    renderer.render();
    window.display();
  }
  return 0;
}
