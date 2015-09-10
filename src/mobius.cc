#include <SFML/Window.hpp>
#include <glm/gtc/constants.hpp>
#include "render.h"
#include "world.h"

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

int main(int argc, char** argv)
{
  std::string world_path = "gen/data/test.world.pb";
  if (argc > 1) {
    world_path = argv[1];
  }

  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 1;
  settings.majorVersion = 3;
  settings.minorVersion = 2;
  settings.attributeFlags = sf::ContextSettings::Core;

  sf::Window window{sf::VideoMode::getDesktopMode(), "MOBIUS", sf::Style::None};
  window.setVerticalSyncEnabled(true);
  Renderer renderer;
  renderer.resize(window_size(window));
  World world{world_path, renderer};

  bool focus = true;
  ControlData control_data;
  while (window.isOpen()) {
    sf::Event event;
    control_data.jump = false;
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
          control_data.forward = true;
        } else if (event.key.code == sf::Keyboard::S) {
          control_data.reverse = true;
        } else if (event.key.code == sf::Keyboard::A) {
          control_data.left = true;
        } else if (event.key.code == sf::Keyboard::D) {
          control_data.right = true;
        } else if (event.key.code == sf::Keyboard::Space) {
          control_data.jump = true;
        }
      } else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::W) {
          control_data.forward = false;
        } else if (event.key.code == sf::Keyboard::S) {
          control_data.reverse = false;
        } else if (event.key.code == sf::Keyboard::A) {
          control_data.left = false;
        } else if (event.key.code == sf::Keyboard::D) {
          control_data.right = false;
        }
      }
    }

    window.setMouseCursorVisible(!focus);
    if (focus) {
      control_data.mouse_move = (glm::vec2(mouse_position(window)) -
                                 glm::vec2(window_size(window)) / 2.f);
      reset_mouse_position(window);
    } else {
      control_data.mouse_move = glm::vec2{};
    }

    world.update(control_data);
    world.render();
    window.display();
  }
  return 0;
}
