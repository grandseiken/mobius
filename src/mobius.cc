#include <SFML/Window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "render.h"

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
  renderer.perspective(3.141592654 / 2, .5, 3);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      } else if (event.type == sf::Event::Resized) {
        renderer.resize(glm::ivec2{window.getSize().x, window.getSize().y});
      }
    }

    renderer.clear();
    renderer.world(
        glm::translate(glm::mat4{1}, glm::vec3{.5, .5, -2}) *
        glm::scale(glm::mat4{1}, glm::vec3{.25, .25, .75}));
    renderer.cube(glm::vec3{0.6, 0.2, 0.2});
    renderer.world(
        glm::translate(glm::mat4{1}, glm::vec3{.25, .25, -2.5}) *
        glm::scale(glm::mat4{1}, glm::vec3{1, .25, 1}));
    renderer.cube(glm::vec3{0.2, 0.6, 0.2});
    renderer.render();
    window.display();
  }
  return 0;
}
