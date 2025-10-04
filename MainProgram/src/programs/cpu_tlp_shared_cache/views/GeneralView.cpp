#include "programs/cpu_tlp_shared_cache/views/GeneralView.h"
#include "imgui.h"
#include "imgui-SFML.h"   // para ImGui::Image(sf::Texture,...)
#include <iostream>
#include <string>

GeneralView::GeneralView() {
    // Carga diferida (on-demand)
}

void GeneralView::ensureLoaded() {
    if (!m_loaded) {
        const std::string path = std::string(RESOURCES_PATH) + "Assets/CPU_TLP/GeneralView.png";
        m_loaded = m_texture.loadFromFile(path);
        if (!m_loaded) {
            std::cout << "[GeneralView] No se pudo cargar la imagen: " << path << "\n";
        }
    }
}

void GeneralView::render() {
    ensureLoaded();

    // Layout tipo CompilerView: imagen grande + barra inferior
    const float BETWEEN = 10.0f; // separación vertical
    const float BOTTOM_H = 46.0f; // alto barra inferior

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float imageH = avail.y - BOTTOM_H - BETWEEN;
    if (imageH < 0.0f) imageH = 0.0f;

    // Imagen extendida a todo el ancho disponible y alto calculado
    if (m_loaded) {
        ImGui::Image(m_texture, sf::Vector2f(avail.x, imageH));
    }
    else {
        ImGui::Dummy(ImVec2(avail.x, imageH));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - imageH);
        ImGui::TextWrapped("No se pudo cargar la imagen de GeneralView. Verifique el archivo en resources/Assets/CPU_TLP/GeneralView.png");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + imageH);
    }

    // Separación antes de la barra inferior
    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    // ===== Barra inferior =====
    const float GAP = 10.0f;
    // Cuatro segmentos: [RESET] | [Step] | [InputInt + StepUntilNum] | [InfiniteStep]
    float segmentW = (avail.x - 3.0f * GAP) / 4.0f;
    if (segmentW < 0.0f) segmentW = 0.0f;

    // --- Botón RESET (rojo) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
    if (ImGui::Button("RESET", ImVec2(segmentW, BOTTOM_H))) {
        std::cout << "[GeneralView] RESET PRESSED\n";
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0.0f, GAP);

    // --- Botón Step (normal) ---
    if (ImGui::Button("Step", ImVec2(segmentW, BOTTOM_H))) {
        std::cout << "[GeneralView] Step PRESSED\n";
    }

    ImGui::SameLine(0.0f, GAP);

    // --- Grupo central: InputInt + StepUntilNum ---
    ImGui::BeginGroup();
    // Asegurar valor mínimo válido
    if (m_untilSteps < 1) m_untilSteps = 1;

    const float inputW = 90.0f;
    ImGui::PushItemWidth(inputW);
    if (ImGui::InputInt("##StepUntilInput", &m_untilSteps, 0, 0)) {
        if (m_untilSteps < 1) m_untilSteps = 1; // clamp
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    float btnUntilW = segmentW - inputW - ImGui::GetStyle().ItemSpacing.x;
    if (btnUntilW < 80.0f) btnUntilW = 80.0f; // ancho mínimo
    if (ImGui::Button("StepUntilNum", ImVec2(btnUntilW, BOTTOM_H))) {
        // m_untilSteps ya está clamped a >= 1
        std::cout << "[GeneralView] StepUntilNum PRESSED -> steps = " << m_untilSteps << "\n";
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, GAP);

    // --- Botón InfiniteStep (verde) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));
    if (ImGui::Button("InfiniteStep", ImVec2(segmentW, BOTTOM_H))) {
        std::cout << "[GeneralView] InfiniteStep PRESSED\n";
    }
    ImGui::PopStyleColor(3);
}
