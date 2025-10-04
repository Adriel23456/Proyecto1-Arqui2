#include "MultiplayerState.h"
#include "StateManager.h"
#include "MainMenuState.h"
#include "imgui.h"
#include <memory>

MultiplayerState::MultiplayerState(StateManager* stateManager, sf::RenderWindow* window)
    : State(stateManager, window)
{
}

void MultiplayerState::handleEvent(sf::Event& event) {
    // Detectar ESC para volver a MainMenu
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    {
        m_stateManager->queueNextState(std::make_unique<MainMenuState>(m_stateManager, m_window));
    }
}

void MultiplayerState::update(float deltaTime) {
}

void MultiplayerState::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("MultiplayerPlayer", nullptr, flags);

    ImGui::Text("Modo MultiplayerPlayer activo");

    ImGui::End();
}