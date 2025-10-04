#pragma once

#include "State.h"

class MultiplayerState : public State {
public:
    MultiplayerState(StateManager* stateManager, sf::RenderWindow* window);
    ~MultiplayerState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;
};
