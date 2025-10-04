#include "states/ProgramState.h"
#include "imgui.h"

void ProgramState::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("##ProgramStateWindow", nullptr, flags)) {
        ImGui::Text("Programa activo: %s", m_title.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("Esta es una vista stub del programa. Desde aqui NO se regresa al menu principal. "
            "Usa ESC para abrir Configuracion (overlay) y ajustar graficos/audio o salir.");
        ImGui::End();
    }
}

void ProgramState::renderBackground() {
    // Solo un fondo sólido para ProgramState cuando el overlay está abierto
    m_window->clear(sf::Color(30, 30, 30));
}
