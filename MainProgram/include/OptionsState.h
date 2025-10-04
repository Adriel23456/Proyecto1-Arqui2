#pragma once

#include "State.h"

class OptionsState : public State {
public:
    OptionsState(StateManager* stateManager, sf::RenderWindow* window);
    ~OptionsState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;
};
