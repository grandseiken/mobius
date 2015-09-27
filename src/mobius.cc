#include "render.h"
#include "world.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <sstream>

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

  // TODO: this isn't used, since it interferes with the OpenGL version somehow.
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 1;
  settings.majorVersion = 3;
  settings.minorVersion = 2;
  settings.attributeFlags = sf::ContextSettings::Core;

  sf::RenderWindow window{
      sf::VideoMode::getDesktopMode(), "MOBIUS", sf::Style::None};

  window.setVerticalSyncEnabled(true);
  Renderer renderer;
  renderer.resize(window_size(window));
  World world{world_path, renderer};

  sf::Clock clock;
  uint64_t frame_time_us = 1;
  sf::Font font;
  font.loadFromFile("assets/droidiga.otf");
  sf::Text debug_text{"", font, 24};
  debug_text.setColor(sf::Color::Black);
  debug_text.setPosition(sf::Vector2f{8, 8});

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
        auto size = window_size(window);
        renderer.resize(size);
        window.setView(sf::View{sf::Vector2f(size.x / 2, size.y / 2),
                                sf::Vector2f(size.x, size.y)});
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

    RenderMetrics metrics;
    world.update(control_data);
    world.render(metrics);

    std::stringstream ss;
    ss << "Chunks: " << metrics.chunks <<
        "\nDepth: " << metrics.depth <<
        "\nBreadth: " << metrics.breadth <<
        "\nFPS: " << uint32_t(1000000.f / frame_time_us);

    debug_text.setString(ss.str());
    window.resetGLStates();
    window.draw(debug_text);
    frame_time_us = clock.getElapsedTime().asMicroseconds();
    window.display();
    clock.restart();
  }
  return 0;
}
