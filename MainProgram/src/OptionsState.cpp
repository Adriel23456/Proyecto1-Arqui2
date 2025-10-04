#include "OptionsState.h"
#include "StateManager.h"
#include "MainMenuState.h"
#include "imgui.h"
#include <memory>

OptionsState::OptionsState(StateManager* stateManager, sf::RenderWindow* window)
    : State(stateManager, window)
{
}

void OptionsState::handleEvent(sf::Event& event) {
    // Detectar ESC para volver a MainMenu
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    {
        m_stateManager->queueNextState(std::make_unique<MainMenuState>(m_stateManager, m_window));
    }
}

void OptionsState::update(float deltaTime) {
}

void OptionsState::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Options", nullptr, flags);

    ImGui::Text("Ejemplo");

    ImGui::End();
}