#include "programs/cpu_tlp_shared_cache/views/GeneralView.h"
#include "programs/cpu_tlp_shared_cache/CpuTLPControlAPI.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <string>

GeneralView::GeneralView() {}

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

    const float BETWEEN = 10.0f;
    const float BOTTOM_H = 46.0f;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float imageH = avail.y - BOTTOM_H - BETWEEN;
    if (imageH < 0.0f) imageH = 0.0f;

    if (m_loaded) {
        ImGui::Image(m_texture, sf::Vector2f(avail.x, imageH));
    }
    else {
        ImGui::Dummy(ImVec2(avail.x, imageH));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - imageH);
        ImGui::TextWrapped("No se pudo cargar la imagen de GeneralView. Verifique 'resources/Assets/CPU_TLP/GeneralView.png'");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + imageH);
    }

    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    const float GAP = 10.0f;
    float segmentW = (avail.x - 3.0f * GAP) / 4.0f;
    if (segmentW < 0.0f) segmentW = 0.0f;

    // RESET
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
    if (ImGui::Button("RESET", ImVec2(segmentW, BOTTOM_H))) {
        if (cpu_tlp_ui::onResetPE0) cpu_tlp_ui::onResetPE0();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0.0f, GAP);

    // Step
    if (ImGui::Button("Step", ImVec2(segmentW, BOTTOM_H))) {
        if (cpu_tlp_ui::onStepPE0) cpu_tlp_ui::onStepPE0();
    }

    ImGui::SameLine(0.0f, GAP);

    // Grupo: InputInt + StepUntilNum
    ImGui::BeginGroup();
    if (m_untilSteps < 1) m_untilSteps = 1;

    const float inputW = 90.0f;
    ImGui::PushItemWidth(inputW);
    int tmp = m_untilSteps;
    ImGui::InputInt("##until_steps", &tmp);
    if (tmp < 1) tmp = 1;
    m_untilSteps = tmp;
    ImGui::PopItemWidth();

    ImGui::SameLine();
    float btnUntilW = segmentW - inputW - ImGui::GetStyle().ItemSpacing.x;
    if (btnUntilW < 80.0f) btnUntilW = 80.0f;

    if (ImGui::Button("StepUntilNum", ImVec2(btnUntilW, BOTTOM_H))) {
        if (cpu_tlp_ui::onStepUntilPE0) cpu_tlp_ui::onStepUntilPE0(m_untilSteps);
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, GAP);

    // InfiniteStep + STOP al lado
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));
    if (ImGui::Button("InfiniteStep", ImVec2(segmentW * 0.6f, BOTTOM_H))) {
        if (cpu_tlp_ui::onStepIndefinitelyPE0) cpu_tlp_ui::onStepIndefinitelyPE0();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::Button("STOP", ImVec2(segmentW * 0.4f - GAP, BOTTOM_H))) {
        if (cpu_tlp_ui::onStopPE0) cpu_tlp_ui::onStopPE0();
    }
}
