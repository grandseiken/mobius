#include <SFML/Window.hpp>
#include "render.h"

int main()
{
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 2;
  settings.attributeFlags = sf::ContextSettings::Core;

  sf::Window window{sf::VideoMode::getDesktopMode(), "MOBIUS", sf::Style::None};
  window.setVerticalSyncEnabled(true);
  Renderer renderer;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    renderer.render(window.getSize().x, window.getSize().y);
    window.display();
  }
  return 0;
}
