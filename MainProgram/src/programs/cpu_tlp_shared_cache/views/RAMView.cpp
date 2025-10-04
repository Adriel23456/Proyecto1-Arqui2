#include "programs/cpu_tlp_shared_cache/views/RAMView.h"
#include <imgui.h>
#include <iostream>

void RAMView::render() {
    // Tamaños disponibles y layout
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float BETWEEN = 8.0f;   // separación vertical entre tabla y barra
    const float BOTTOM_H = 46.0f;  // alto de la barra de botones

    float tableH = avail.y - (BETWEEN + BOTTOM_H);
    if (tableH < 0.0f) tableH = 0.0f;

    // --- Área de tabla: SIN scroll del child; el scroll lo maneja la tabla internamente ---
    ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::BeginChild("##RAMTableArea", ImVec2(avail.x, tableH), false, childFlags);
    {
        // Tu widget RamTable tal cual (NO modificado)
        m_table.render("##RAM_Table");
    }
    ImGui::EndChild();

    // Separación antes de la barra inferior
    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    // ---- Barra inferior: Reset (rojo) | Load (verde) ----
    const float GAP = 10.0f;
    const float w = (avail.x - GAP) * 0.5f;
    const ImVec2 btnSize(w, BOTTOM_H);

    // Reset (rojo)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.16f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.58f, 0.12f, 0.12f, 1.0f));
    if (ImGui::Button("Reset", btnSize)) {
        std::cout << "[RAM] RESET pressed\n";
        // aquí podrás limpiar la RAM cuando toque
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0.0f, GAP);

    // Load (verde)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));
    if (ImGui::Button("Load", btnSize)) {
        std::cout << "[RAM] LOAD pressed\n";
        // aquí podrás cargar datos desde archivo/fuente cuando toque
    }
    ImGui::PopStyleColor(3);
}
