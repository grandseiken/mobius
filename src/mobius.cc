#include <SFML/Window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "render.h"
#include "mesh.h"
#include "collision.h"

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
  renderer.resize(glm::ivec2{window.getSize().x, window.getSize().y});
  renderer.perspective(3.141592654 / 2, 1. / 1024, 1024);

  Collision collision;
  Mesh player{"gen/player.mesh.pb"};
  Mesh level{"gen/level.mesh.pb"};
  glm::vec3 player_position{0.2, 3, 0.2};

  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  float angle = 0.f;
  float fall_speed = 0.f;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        window.close();
      } else if (event.type == sf::Event::Resized) {
        renderer.resize(glm::ivec2{window.getSize().x, window.getSize().y});
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

    angle += (1. / 16) * ((right ? -1 : 0) + (left ? 1 : 0));
    float speed = (1. / 32) * ((forward ? 1 : 0) + (backward ? -1 : 0));
    glm::vec3 player_direction{sin(angle), 0, cos(angle)};
    player_position += collision.translation(
        player, level,
        glm::translate(glm::mat4{1}, player_position), speed * player_direction);

    fall_speed = std::min(1. / 4, fall_speed + 1. / 512);
    fall_speed *= collision.coefficient(
        player, level,
        glm::translate(glm::mat4{1}, player_position),
        -glm::vec3{0, fall_speed, 0});
    player_position -= glm::vec3{0, fall_speed, 0};

    renderer.camera(
      player_position + glm::vec3{0, .5, 0},
      player_position + player_direction, glm::vec3{0, 1, 0});
    renderer.light(player_position + glm::vec3{0, .5, 0}, 1.f);

    renderer.clear();
    renderer.world(glm::mat4{1});
    renderer.mesh(level);
    renderer.grain(1. / 16);
    renderer.render();
    window.display();
  }
  return 0;
}
