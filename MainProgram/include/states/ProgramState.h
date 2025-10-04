#pragma once
#include "core/State.h"
#include <string>

class ProgramState : public State {
public:
    ProgramState(StateManager* sm, sf::RenderWindow* win, std::string title)
        : State(sm, win), m_title(std::move(title)) {
    }

    void handleEvent(sf::Event& event) override { (void)event; }
    void update(float) override {}
    void render() override;
    void renderBackground() override;  // Nuevo método para renderizar solo el fondo

private:
    std::string m_title;
};
