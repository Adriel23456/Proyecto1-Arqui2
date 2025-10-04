#pragma once
#include <SFML/Graphics.hpp>

class StateManager;

class State {
public:
    State(StateManager* stateManager, sf::RenderWindow* window);
    virtual ~State() = default;

    virtual void handleEvent(sf::Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

    // Nuevo método opcional para renderizar solo el fondo
    virtual void renderBackground() {}

protected:
    StateManager* m_stateManager;
    sf::RenderWindow* m_window;
};