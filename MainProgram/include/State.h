#pragma once
#include <SFML/Graphics.hpp>

// Declaración previa (forward declaration) para evitar includes cíclicos
class StateManager;

class State {
public:
    State(StateManager* stateManager, sf::RenderWindow* window);
    virtual ~State() = default;

    // Métodos que cada estado debe implementar
    virtual void handleEvent(sf::Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

protected:
    StateManager* m_stateManager;
    sf::RenderWindow* m_window;
};