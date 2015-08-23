#include <SFML/Window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "render.h"
#include "mesh.h"

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
  glm::vec3 camera{0, 0, 4};
  Mesh level{"gen/level.mesh.pb"};

  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
  float r = 0.f;

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
        } else if (event.key.code == sf::Keyboard::Up) {
          up = true;
        } else if (event.key.code == sf::Keyboard::Down) {
          down = true;
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
        } else if (event.key.code == sf::Keyboard::Up) {
          up = false;
        } else if (event.key.code == sf::Keyboard::Down) {
          down = false;
        }
      }
    }

    auto camera_offset =
        glm::vec3{ 0,  0, -1} * (forward ? 1.f : 0.f) +
        glm::vec3{ 0,  0,  1} * (backward ? 1.f : 0.f) +
        glm::vec3{-1,  0,  0} * (left ? 1.f : 0.f) +
        glm::vec3{ 1,  0,  0} * (right ? 1.f : 0.f) +
        glm::vec3{ 0, -1,  0} * (down ? 1.f : 0.f) +
        glm::vec3{ 0,  1,  0} * (up ? 1.f : 0.f);
    camera += camera_offset * .05f;
    r += .01f;
    renderer.camera(camera, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
    renderer.light(camera, 1.f);

    renderer.clear();
    renderer.world(
        glm::translate(glm::mat4{1}, glm::vec3{0, 1, 0}) *
        glm::scale(glm::mat4{1}, glm::vec3{1, 1, 1}));
    renderer.mesh(level);
    renderer.world(
        glm::translate(glm::mat4{1}, glm::vec3{-4, 1, 0}) *
        glm::rotate(glm::mat4{1}, r, glm::vec3{0, 1, 0}) *
        glm::scale(glm::mat4{1}, glm::vec3{1, 1, 1}));
    renderer.mesh(level);
    renderer.world(
        glm::translate(glm::mat4{1}, glm::vec3{8, 2, 0}) *
        glm::rotate(glm::mat4{1}, r, glm::vec3{0, 1, 0}) *
        glm::scale(glm::mat4{1}, glm::vec3{2, 2, 2}));
    renderer.mesh(level);
    renderer.grain(0.0625);
    renderer.render();
    window.display();
  }
  return 0;
}
