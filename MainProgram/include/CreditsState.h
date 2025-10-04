#pragma once

#include "State.h"

class CreditsState : public State {
public:
    CreditsState(StateManager* stateManager, sf::RenderWindow* window);
    ~CreditsState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;
};
