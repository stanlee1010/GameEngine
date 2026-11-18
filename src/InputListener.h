#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Vector2.hpp>

// Observer interface: register with Engine::addInputListener() to be
// notified about input events. Override only what you need.
class InputListener {
public:
    virtual ~InputListener() = default;

    // Empty bodies {}: a listener overrides only the events it needs.
    // Called by Engine::processEvents() for every event, once per press.
    virtual void onKeyPressed(sf::Keyboard::Key /*key*/) {}
    virtual void onKeyReleased(sf::Keyboard::Key /*key*/) {}
    virtual void onMouseButtonPressed(sf::Mouse::Button /*button*/, sf::Vector2i /*position*/) {}
    virtual void onMouseButtonReleased(sf::Mouse::Button /*button*/, sf::Vector2i /*position*/) {}
    virtual void onMouseMoved(sf::Vector2i /*position*/) {}
    virtual void onMouseWheelScrolled(float /*delta*/, sf::Vector2i /*position*/) {}
};
