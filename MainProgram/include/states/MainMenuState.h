#pragma once
#include "core/State.h"
#include <vector>
#include <string>

class MainMenuState : public State {
public:
    MainMenuState(StateManager* stateManager, sf::RenderWindow* window);
    ~MainMenuState() override = default;

    void handleEvent(sf::Event& event) override;
    void update(float deltaTime) override;
    void render() override;
    void renderBackground() override;  // Nuevo método para renderizar solo el fondo

private:
    std::vector<std::string> m_items;
    bool buttonAutoFitText(const char* label, float targetWidth, float height);
};
