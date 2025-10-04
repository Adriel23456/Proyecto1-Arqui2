#pragma once

#include "State.h"

class SinglePlayerState : public State {
public:
    SinglePlayerState(StateManager* stateManager, sf::RenderWindow* window);
    ~SinglePlayerState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;
};
