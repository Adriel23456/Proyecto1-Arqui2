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
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    const float textPadding = 30.0f;

    float wReset = ImGui::CalcTextSize("RESET").x + textPadding;
    float wStep = ImGui::CalcTextSize("Step").x + textPadding;
    float wStepUntil = ImGui::CalcTextSize("StepUntilNum").x + textPadding;
    float wInf = ImGui::CalcTextSize("InfiniteStep").x + textPadding;
    float wStop = ImGui::CalcTextSize("STOP").x + textPadding;

    // ========== RESET - TODOS LOS PEs ==========
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
    if (ImGui::Button("RESET", ImVec2(wReset, BOTTOM_H))) {
        // Llamar RESET para TODOS los PEs
        if (cpu_tlp_ui::onResetPE0) cpu_tlp_ui::onResetPE0();
        if (cpu_tlp_ui::onResetPE1) cpu_tlp_ui::onResetPE1();
        if (cpu_tlp_ui::onResetPE2) cpu_tlp_ui::onResetPE2();
        if (cpu_tlp_ui::onResetPE3) cpu_tlp_ui::onResetPE3();
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0.0f, GAP);

    // ========== STEP - TODOS LOS PEs ==========
    if (ImGui::Button("Step", ImVec2(wStep, BOTTOM_H))) {
        // Llamar STEP para TODOS los PEs
        if (cpu_tlp_ui::onStepPE0) cpu_tlp_ui::onStepPE0();
        if (cpu_tlp_ui::onStepPE1) cpu_tlp_ui::onStepPE1();
        if (cpu_tlp_ui::onStepPE2) cpu_tlp_ui::onStepPE2();
        if (cpu_tlp_ui::onStepPE3) cpu_tlp_ui::onStepPE3();
    }
    ImGui::SameLine(0.0f, GAP);

    // ========== STEP UNTIL - TODOS LOS PEs ==========
    ImGui::BeginGroup();
    if (m_untilSteps < 1) m_untilSteps = 1;
    float inputW = avail.x * 0.15f;
    ImGui::PushItemWidth(inputW);
    int tmp = m_untilSteps;
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsDecimal;
    ImGui::InputScalar("##until_steps", ImGuiDataType_S32, &tmp, nullptr, nullptr, nullptr, flags);
    if (tmp < 1) tmp = 1;
    m_untilSteps = tmp;
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("StepUntilNum", ImVec2(wStepUntil, BOTTOM_H))) {
        // Llamar STEP UNTIL para TODOS los PEs
        if (cpu_tlp_ui::onStepUntilPE0) cpu_tlp_ui::onStepUntilPE0(m_untilSteps);
        if (cpu_tlp_ui::onStepUntilPE1) cpu_tlp_ui::onStepUntilPE1(m_untilSteps);
        if (cpu_tlp_ui::onStepUntilPE2) cpu_tlp_ui::onStepUntilPE2(m_untilSteps);
        if (cpu_tlp_ui::onStepUntilPE3) cpu_tlp_ui::onStepUntilPE3(m_untilSteps);
    }
    ImGui::EndGroup();
    ImGui::SameLine(0.0f, GAP);

    // ========== INFINITE STEP - TODOS LOS PEs ==========
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));
    if (ImGui::Button("InfiniteStep", ImVec2(wInf, BOTTOM_H))) {
        // Llamar INFINITE STEP para TODOS los PEs
        if (cpu_tlp_ui::onStepIndefinitelyPE0) cpu_tlp_ui::onStepIndefinitelyPE0();
        if (cpu_tlp_ui::onStepIndefinitelyPE1) cpu_tlp_ui::onStepIndefinitelyPE1();
        if (cpu_tlp_ui::onStepIndefinitelyPE2) cpu_tlp_ui::onStepIndefinitelyPE2();
        if (cpu_tlp_ui::onStepIndefinitelyPE3) cpu_tlp_ui::onStepIndefinitelyPE3();
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    // ========== STOP - TODOS LOS PEs ==========
    if (ImGui::Button("STOP", ImVec2(wStop, BOTTOM_H))) {
        // Llamar STOP para TODOS los PEs
        if (cpu_tlp_ui::onStopPE0) cpu_tlp_ui::onStopPE0();
        if (cpu_tlp_ui::onStopPE1) cpu_tlp_ui::onStopPE1();
        if (cpu_tlp_ui::onStopPE2) cpu_tlp_ui::onStopPE2();
        if (cpu_tlp_ui::onStopPE3) cpu_tlp_ui::onStopPE3();
    }
}