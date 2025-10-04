#include "CreditsState.h"
#include "StateManager.h"
#include "imgui.h"
#include "MainMenuState.h" // Para volver al MainMenu
#include <memory>

CreditsState::CreditsState(StateManager* stateManager, sf::RenderWindow* window)
    : State(stateManager, window)
{
}

void CreditsState::handleEvent(sf::Event& event) {
    // 2.1 Detectar si se presiona 'ESC' para volver a MainMenu
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    {
        // Cambiamos el estado al MainMenu
        m_stateManager->queueNextState(std::make_unique<MainMenuState>(m_stateManager, m_window));
    }
}

void CreditsState::update(float deltaTime) {
    // Lógica de actualización (si hubiera)
}

void CreditsState::render() {
    // Posición y tamaño al 100% de la ventana
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Credits", nullptr, flags);

    ImGui::Text("Aqui van los creditos xddd");

    ImGui::End();
}