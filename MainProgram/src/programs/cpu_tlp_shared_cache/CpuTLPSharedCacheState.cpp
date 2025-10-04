#include "programs/cpu_tlp_shared_cache/CpuTLPSharedCacheState.h"
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/views/CompilerView.h"
#include "programs/cpu_tlp_shared_cache/views/GeneralView.h"
#include "programs/cpu_tlp_shared_cache/views/PE0MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE1MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE2MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE3MemView.h"
#include "programs/cpu_tlp_shared_cache/views/RAMView.h"

#include "imgui.h"
#include <algorithm> // std::clamp

CpuTLPSharedCacheState::CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win)
    : State(sm, win) {
    buildAllViews(); // <- Instanciamos todas las sub-vistas una sola vez
}

void CpuTLPSharedCacheState::buildAllViews() {
    m_views[panelIndex(Panel::Compiler)] = std::make_unique<CompilerView>();
    m_views[panelIndex(Panel::GeneralView)] = std::make_unique<GeneralView>();
    m_views[panelIndex(Panel::PE0Mem)] = std::make_unique<PE0MemView>();
    m_views[panelIndex(Panel::PE1Mem)] = std::make_unique<PE1MemView>();
    m_views[panelIndex(Panel::PE2Mem)] = std::make_unique<PE2MemView>();
    m_views[panelIndex(Panel::PE3Mem)] = std::make_unique<PE3MemView>();
    m_views[panelIndex(Panel::RAM)] = std::make_unique<RAMView>();
}

ICpuTLPView* CpuTLPSharedCacheState::getView(Panel p) {
    return m_views[panelIndex(p)].get();
}

void CpuTLPSharedCacheState::handleEvent(sf::Event& e) {
    // Por defecto, solo la vista visible recibe eventos de entrada.
    // Si quieres que TODAS reciban eventos, cambia este bloque por un loop
    // sobre m_views y llama v->handleEvent(e) para cada una.
    if (auto* v = getView(m_selected)) v->handleEvent(e);
}

void CpuTLPSharedCacheState::update(float dt) {
    // MUY IMPORTANTE: todas las sub-vistas se "ejecutan" (update) simultáneamente.
    for (auto& v : m_views) {
        if (v) v->update(dt);
    }
}

bool CpuTLPSharedCacheState::sidebarButton(const char* label, bool selected, float width, float height) {
    ImVec2 textSize = ImGui::CalcTextSize(label);
    float paddingX = ImGui::GetStyle().FramePadding.x * 2.f;
    float available = width - paddingX;
    float scale = available > 0.f ? (available / std::max(1.f, textSize.x)) : 1.f;
    scale = std::clamp(scale, 0.85f, 1.35f);

    ImVec4 normal = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    ImVec4 normalHov = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    ImVec4 normalAct = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

    ImVec4 active = ImVec4(normal.x * 0.7f + 0.3f, normal.y * 0.7f + 0.3f, normal.z * 0.7f + 0.3f, 1.0f);
    ImVec4 activeHov = ImVec4(normalHov.x * 0.7f + 0.3f, normalHov.y * 0.7f + 0.3f, normalHov.z * 0.7f + 0.3f, 1.0f);
    ImVec4 activeAct = ImVec4(normalAct.x * 0.7f + 0.3f, normalAct.y * 0.7f + 0.3f, normalAct.z * 0.7f + 0.3f, 1.0f);

    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, active);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeHov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeAct);
    }

    ImGui::SetWindowFontScale(scale);
    bool clicked = ImGui::Button(label, ImVec2(width, height));
    ImGui::SetWindowFontScale(1.0f);

    if (selected) ImGui::PopStyleColor(3);
    return clicked;
}

void CpuTLPSharedCacheState::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);

    // IMPORTANTE: sin scroll en el window raíz
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##CpuTLPSharedCache", nullptr, flags)) {

        // Usar el content region disponible (evita overflow por paddings)
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const float leftW = avail.x * 0.20f;   // 20%
        const float SEP = 4.0f;              // separación visual entre paneles
        const float rightW = avail.x - leftW - SEP; // 80% ajustado al espacio real
        const float contentH = avail.y;        // alto disponible sin forzar scroll

        // === Columna Izquierda (con scroll vertical) ===
        ImGui::BeginChild("##LeftSidebar", ImVec2(leftW, contentH), true,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

        const float TOP_PAD = 14.0f;           // padding superior extra
        const float BTN_H = 56.0f;
        // ancho de botón según el ancho real del child (evita restas mágicas)
        float btnW = ImGui::GetContentRegionAvail().x;

        ImGui::Dummy(ImVec2(1, TOP_PAD));
        if (sidebarButton("Compiler", (m_selected == Panel::Compiler), btnW, BTN_H)) m_selected = Panel::Compiler;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("General View", (m_selected == Panel::GeneralView), btnW, BTN_H)) m_selected = Panel::GeneralView;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE0 Mem", (m_selected == Panel::PE0Mem), btnW, BTN_H)) m_selected = Panel::PE0Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE1 Mem", (m_selected == Panel::PE1Mem), btnW, BTN_H)) m_selected = Panel::PE1Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE2 Mem", (m_selected == Panel::PE2Mem), btnW, BTN_H)) m_selected = Panel::PE2Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE3 Mem", (m_selected == Panel::PE3Mem), btnW, BTN_H)) m_selected = Panel::PE3Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("RAM", (m_selected == Panel::RAM), btnW, BTN_H)) m_selected = Panel::RAM;

        ImGui::EndChild();

        // Separación mínima entre columnas
        ImGui::SameLine(0.0f, SEP);

        // === Panel Derecho (SIN SCROLL) ===
        ImGuiWindowFlags rightFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("##RightPane", ImVec2(rightW, contentH), true, rightFlags);
        if (auto* v = getView(m_selected)) v->render();
        ImGui::EndChild();

        ImGui::End();
    }
}

void CpuTLPSharedCacheState::renderBackground() {
    m_window->clear(sf::Color(20, 20, 25));
}