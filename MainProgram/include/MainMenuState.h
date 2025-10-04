#pragma once

#include "State.h"

class MainMenuState : public State {
public:
    MainMenuState(StateManager* stateManager, sf::RenderWindow* window);
    ~MainMenuState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    bool m_showExitPrompt = false;
    bool m_escWasPressed = false;
};