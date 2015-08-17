#include <SFML/Window.hpp>
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
  renderer.resize(window.getSize().x, window.getSize().y);
  renderer.camera(1, .5f, 3);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      } else if (event.type == sf::Event::Resized) {
        renderer.resize(window.getSize().x, window.getSize().y);
      }
    }

    renderer.clear();
    renderer.translate(.5f, .5f, -2);
    renderer.scale(.25f, .25f, .75f);
    renderer.cube(0.6, 0.2, 0.2);
    renderer.translate(.25f, .25f, -2.5);
    renderer.scale(1, .25f, 1);
    renderer.cube(0.2, 0.6, 0.2);
    renderer.render();
    window.display();
  }
  return 0;
}
