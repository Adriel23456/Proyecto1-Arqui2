#include "programs/cpu_tlp_shared_cache/CpuTLPSharedCacheState.h"
#include "programs/cpu_tlp_shared_cache/CpuTLPControlAPI.h"
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/views/GeneralView.h"
#include "programs/cpu_tlp_shared_cache/views/PE0CPUView.h"
#include "programs/cpu_tlp_shared_cache/views/PE0RegView.h"
#include "programs/cpu_tlp_shared_cache/views/PE0MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE1CPUView.h"
#include "programs/cpu_tlp_shared_cache/views/PE1RegView.h"
#include "programs/cpu_tlp_shared_cache/views/PE1MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE2CPUView.h"
#include "programs/cpu_tlp_shared_cache/views/PE2RegView.h"
#include "programs/cpu_tlp_shared_cache/views/PE2MemView.h"
#include "programs/cpu_tlp_shared_cache/views/PE3CPUView.h"
#include "programs/cpu_tlp_shared_cache/views/PE3RegView.h"
#include "programs/cpu_tlp_shared_cache/views/PE3MemView.h"
#include "programs/cpu_tlp_shared_cache/views/RAMView.h"
#include "programs/cpu_tlp_shared_cache/views/AnalysisDataView.h"
#include "programs/cpu_tlp_shared_cache/components/InstructionMemoryComponent.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/PE0Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE1Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE2Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE3Component.h"
#include "programs/cpu_tlp_shared_cache/views/CompilerView.h"
#include "programs/cpu_tlp_shared_cache/widgets/InstructionDisassembler.h"
#include <imgui.h>
#include <iostream>
#include <memory>

CpuTLPSharedCacheState::CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win)
    : State(sm, win) {

    // 1) Construir UNA SOLA estructura de datos compartidos
    m_cpuSystemData = std::make_shared<cpu_tlp::CPUSystemSharedData>();

    // 2) Lanzar InstructionMemory
    m_instructionMemory = std::make_unique<cpu_tlp::InstructionMemoryComponent>();
    if (!m_instructionMemory->initialize(m_cpuSystemData)) {
        std::cerr << "[CpuTLP] InstructionMemory init failed\n";
    }

    // 3) Crear PE0
    m_pe0 = std::make_unique<cpu_tlp::PE0Component>(0);
    if (!m_pe0->initialize(m_cpuSystemData)) {
        std::cerr << "[CpuTLP] PE0 init failed\n";
    }

    // 4) Crear PE1
    m_pe1 = std::make_unique<cpu_tlp::PE1Component>(1);
    if (!m_pe1->initialize(m_cpuSystemData)) {
        std::cerr << "[CpuTLP] PE1 init failed\n";
    }

    // 5) Crear PE2
    m_pe2 = std::make_unique<cpu_tlp::PE2Component>(2);
    if (!m_pe2->initialize(m_cpuSystemData)) {
        std::cerr << "[CpuTLP] PE2 init failed\n";
    }

    // 6) Crear PE3
    m_pe3 = std::make_unique<cpu_tlp::PE3Component>(3);
    if (!m_pe3->initialize(m_cpuSystemData)) {
        std::cerr << "[CpuTLP] PE3 init failed\n";
    }

    m_swiSeenCount.fill(0);
    m_activeSwiPopupPe = -1;

    // 7) Registrar callbacks PE0
    cpu_tlp_ui::onResetPE0 = [this] { this->resetPE0(); };
    cpu_tlp_ui::onStepPE0 = [this] { this->stepPE0(); };
    cpu_tlp_ui::onStepUntilPE0 = [this](int n) { this->stepUntilPE0(n); };
    cpu_tlp_ui::onStepIndefinitelyPE0 = [this] { this->stepIndefinitelyPE0(); };
    cpu_tlp_ui::onStopPE0 = [this] { this->stopPE0(); };

    // 8) Registrar callbacks PE1
    cpu_tlp_ui::onResetPE1 = [this] { this->resetPE1(); };
    cpu_tlp_ui::onStepPE1 = [this] { this->stepPE1(); };
    cpu_tlp_ui::onStepUntilPE1 = [this](int n) { this->stepUntilPE1(n); };
    cpu_tlp_ui::onStepIndefinitelyPE1 = [this] { this->stepIndefinitelyPE1(); };
    cpu_tlp_ui::onStopPE1 = [this] { this->stopPE1(); };

    // 9) Registrar callbacks PE2
    cpu_tlp_ui::onResetPE2 = [this] { this->resetPE2(); };
    cpu_tlp_ui::onStepPE2 = [this] { this->stepPE2(); };
    cpu_tlp_ui::onStepUntilPE2 = [this](int n) { this->stepUntilPE2(n); };
    cpu_tlp_ui::onStepIndefinitelyPE2 = [this] { this->stepIndefinitelyPE2(); };
    cpu_tlp_ui::onStopPE2 = [this] { this->stopPE2(); };

    // 10) Registrar callbacks PE3
    cpu_tlp_ui::onResetPE3 = [this] { this->resetPE3(); };
    cpu_tlp_ui::onStepPE3 = [this] { this->stepPE3(); };
    cpu_tlp_ui::onStepUntilPE3 = [this](int n) { this->stepUntilPE3(n); };
    cpu_tlp_ui::onStepIndefinitelyPE3 = [this] { this->stepIndefinitelyPE3(); };
    cpu_tlp_ui::onStopPE3 = [this] { this->stopPE3(); };

    buildAllViews();
}

CpuTLPSharedCacheState::~CpuTLPSharedCacheState() {
    // Orden inverso
    if (m_pe3) m_pe3->shutdown();
    m_pe3.reset();

    if (m_pe2) m_pe2->shutdown();
    m_pe2.reset();

    if (m_pe1) m_pe1->shutdown();
    m_pe1.reset();

    if (m_pe0) m_pe0->shutdown();
    m_pe0.reset();

    if (m_instructionMemory) m_instructionMemory->shutdown();
    m_instructionMemory.reset();
}

void CpuTLPSharedCacheState::buildAllViews() {
    // Orden EXACTO:
    // Compiler, General View,
    // PE0 CPU, PE0 Reg, PE0 Mem,
    // PE1 CPU, PE1 Reg, PE1 Mem,
    // PE2 CPU, PE2 Reg, PE2 Mem,
    // PE3 CPU, PE3 Reg, PE3 Mem,
    // RAM, Analysis Data

    // Crear CompilerView con callback configurado (sin sobrescribirlo después)
    auto compilerView = std::make_unique<CompilerView>();
    compilerView->setCompileCallback([this](const std::string& /*sourceCode*/) {
        std::cout << "[CpuTLPSharedCacheState] Compilation callback triggered" << std::endl;
        if (m_instructionMemory) {
            bool reloadSuccess = m_instructionMemory->reloadInstructionMemory();
            if (reloadSuccess) {
                std::cout << "[CpuTLPSharedCacheState] Instruction memory reloaded successfully" << std::endl;
            }
            else {
                std::cerr << "[CpuTLPSharedCacheState] Failed to reload instruction memory" << std::endl;
            }
        }
        });
    m_views[panelIndex(Panel::Compiler)] = std::move(compilerView);

    m_views[panelIndex(Panel::GeneralView)] = std::make_unique<GeneralView>();

    m_views[panelIndex(Panel::PE0CPU)] = std::make_unique<PE0CPUView>();
    m_views[panelIndex(Panel::PE0Reg)] = std::make_unique<PE0RegView>();
    m_views[panelIndex(Panel::PE0Mem)] = std::make_unique<PE0MemView>();

    m_views[panelIndex(Panel::PE1CPU)] = std::make_unique<PE1CPUView>();
    m_views[panelIndex(Panel::PE1Reg)] = std::make_unique<PE1RegView>();
    m_views[panelIndex(Panel::PE1Mem)] = std::make_unique<PE1MemView>();

    m_views[panelIndex(Panel::PE2CPU)] = std::make_unique<PE2CPUView>();
    m_views[panelIndex(Panel::PE2Reg)] = std::make_unique<PE2RegView>();
    m_views[panelIndex(Panel::PE2Mem)] = std::make_unique<PE2MemView>();

    m_views[panelIndex(Panel::PE3CPU)] = std::make_unique<PE3CPUView>();
    m_views[panelIndex(Panel::PE3Reg)] = std::make_unique<PE3RegView>();
    m_views[panelIndex(Panel::PE3Mem)] = std::make_unique<PE3MemView>();

    m_views[panelIndex(Panel::RAM)] = std::make_unique<RAMView>();
    m_views[panelIndex(Panel::AnalysisData)] = std::make_unique<AnalysisDataView>();
}

ICpuTLPView* CpuTLPSharedCacheState::getView(Panel p) {
    return m_views[panelIndex(p)].get();
}

void CpuTLPSharedCacheState::handleEvent(sf::Event& e) {
    if (auto* v = getView(m_selected)) v->handleEvent(e);
}

void CpuTLPSharedCacheState::update(float dt) {
    // PE0
    if (auto* pe0RegView = dynamic_cast<PE0RegView*>(getView(Panel::PE0Reg))) {
        auto& snapshot = m_cpuSystemData->pe_registers[0];
        for (int i = 0; i < 12; ++i) {
            uint64_t value = snapshot.registers[i].load(std::memory_order_acquire);
            pe0RegView->setRegValueByIndex_(i, value);
        }
    }

    // PE1
    if (auto* pe1RegView = dynamic_cast<PE1RegView*>(getView(Panel::PE1Reg))) {
        auto& snapshot = m_cpuSystemData->pe_registers[1];
        for (int i = 0; i < 12; ++i) {
            uint64_t value = snapshot.registers[i].load(std::memory_order_acquire);
            pe1RegView->setRegValueByIndex_(i, value);
        }
    }

    // PE2
    if (auto* pe2RegView = dynamic_cast<PE2RegView*>(getView(Panel::PE2Reg))) {
        auto& snapshot = m_cpuSystemData->pe_registers[2];
        for (int i = 0; i < 12; ++i) {
            uint64_t value = snapshot.registers[i].load(std::memory_order_acquire);
            pe2RegView->setRegValueByIndex_(i, value);
        }
    }

    // PE3
    if (auto* pe3RegView = dynamic_cast<PE3RegView*>(getView(Panel::PE3Reg))) {
        auto& snapshot = m_cpuSystemData->pe_registers[3];
        for (int i = 0; i < 12; ++i) {
            uint64_t value = snapshot.registers[i].load(std::memory_order_acquire);
            pe3RegView->setRegValueByIndex_(i, value);
        }
    }

    // PE0 CPU View
    if (auto* pe0CPUView = dynamic_cast<PE0CPUView*>(getView(Panel::PE0CPU))) {
        auto& tracking = m_cpuSystemData->pe_instruction_tracking[0];
        std::array<std::string, 5> labels;
        for (int attempt = 0; attempt < 2; ++attempt) {
            uint64_t v1 = tracking.version.load(std::memory_order_acquire);
            if (v1 & 1ULL) continue;
            uint64_t raw[5];
            for (int i = 0; i < 5; ++i) {
                raw[i] = tracking.stage_instructions[i].load(std::memory_order_acquire);
            }
            uint64_t v2 = tracking.version.load(std::memory_order_acquire);
            if (v1 == v2 && !(v2 & 1ULL)) {
                for (int i = 0; i < 5; ++i) {
                    labels[i] = cpu_tlp::InstructionDisassembler::disassemble(raw[i]);
                }
                pe0CPUView->setLabels(labels);
                break;
            }
        }
    }

    // PE1 CPU View
    if (auto* pe1CPUView = dynamic_cast<PE1CPUView*>(getView(Panel::PE1CPU))) {
        auto& tracking = m_cpuSystemData->pe_instruction_tracking[1];
        std::array<std::string, 5> labels;
        for (int attempt = 0; attempt < 2; ++attempt) {
            uint64_t v1 = tracking.version.load(std::memory_order_acquire);
            if (v1 & 1ULL) continue;
            uint64_t raw[5];
            for (int i = 0; i < 5; ++i) {
                raw[i] = tracking.stage_instructions[i].load(std::memory_order_acquire);
            }
            uint64_t v2 = tracking.version.load(std::memory_order_acquire);
            if (v1 == v2 && !(v2 & 1ULL)) {
                for (int i = 0; i < 5; ++i) {
                    labels[i] = cpu_tlp::InstructionDisassembler::disassemble(raw[i]);
                }
                pe1CPUView->setLabels(labels);
                break;
            }
        }
    }

    // PE2 CPU View
    if (auto* pe2CPUView = dynamic_cast<PE2CPUView*>(getView(Panel::PE2CPU))) {
        auto& tracking = m_cpuSystemData->pe_instruction_tracking[2];
        std::array<std::string, 5> labels;
        for (int attempt = 0; attempt < 2; ++attempt) {
            uint64_t v1 = tracking.version.load(std::memory_order_acquire);
            if (v1 & 1ULL) continue;
            uint64_t raw[5];
            for (int i = 0; i < 5; ++i) {
                raw[i] = tracking.stage_instructions[i].load(std::memory_order_acquire);
            }
            uint64_t v2 = tracking.version.load(std::memory_order_acquire);
            if (v1 == v2 && !(v2 & 1ULL)) {
                for (int i = 0; i < 5; ++i) {
                    labels[i] = cpu_tlp::InstructionDisassembler::disassemble(raw[i]);
                }
                pe2CPUView->setLabels(labels);
                break;
            }
        }
    }

    // PE3 CPU View
    if (auto* pe3CPUView = dynamic_cast<PE3CPUView*>(getView(Panel::PE3CPU))) {
        auto& tracking = m_cpuSystemData->pe_instruction_tracking[3];
        std::array<std::string, 5> labels;
        for (int attempt = 0; attempt < 2; ++attempt) {
            uint64_t v1 = tracking.version.load(std::memory_order_acquire);
            if (v1 & 1ULL) continue;
            uint64_t raw[5];
            for (int i = 0; i < 5; ++i) {
                raw[i] = tracking.stage_instructions[i].load(std::memory_order_acquire);
            }
            uint64_t v2 = tracking.version.load(std::memory_order_acquire);
            if (v1 == v2 && !(v2 & 1ULL)) {
                for (int i = 0; i < 5; ++i) {
                    labels[i] = cpu_tlp::InstructionDisassembler::disassemble(raw[i]);
                }
                pe3CPUView->setLabels(labels);
                break;
            }
        }
    }

    // Detectar SWI
    for (int pe = 0; pe < 4; ++pe) {
        uint32_t seen = m_swiSeenCount[pe];
        uint32_t cur = m_cpuSystemData->ui_signals[pe].swi_count.load(std::memory_order_acquire);
        if (cur > seen) {
            uint32_t delta = cur - seen;
            for (uint32_t k = 0; k < delta; ++k) {
                m_swiQueue.push_back(pe);
            }
            m_swiSeenCount[pe] = cur;
        }
    }

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

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##CpuTLPSharedCache", nullptr, flags)) {

        // --- Abrir el siguiente popup pendiente (uno a la vez) ---
        if (m_activeSwiPopupPe == -1 && !m_swiQueue.empty()) {
            m_activeSwiPopupPe = m_swiQueue.front();
            m_swiQueue.pop_front();
            ImGui::OpenPopup(makeSwiPopupId(m_activeSwiPopupPe).c_str());
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        const float leftW = avail.x * 0.20f;         // 20%
        const float SEP = 4.0f;
        const float rightW = avail.x - leftW - SEP;  // 80%
        const float contentH = avail.y;

        // === Sidebar con scroll ===
        ImGui::BeginChild("##LeftSidebar", ImVec2(leftW, contentH), true,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

        const float TOP_PAD = 14.0f;
        const float BTN_H = 56.0f;
        float btnW = ImGui::GetContentRegionAvail().x;

        ImGui::Dummy(ImVec2(1, TOP_PAD));
        if (sidebarButton("Compiler", (m_selected == Panel::Compiler), btnW, BTN_H)) m_selected = Panel::Compiler;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("General View", (m_selected == Panel::GeneralView), btnW, BTN_H)) m_selected = Panel::GeneralView;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE0 CPU", (m_selected == Panel::PE0CPU), btnW, BTN_H)) m_selected = Panel::PE0CPU;
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE0 Reg", (m_selected == Panel::PE0Reg), btnW, BTN_H)) m_selected = Panel::PE0Reg;     // NUEVO
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE0 Mem", (m_selected == Panel::PE0Mem), btnW, BTN_H)) m_selected = Panel::PE0Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE1 CPU", (m_selected == Panel::PE1CPU), btnW, BTN_H)) m_selected = Panel::PE1CPU;
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE1 Reg", (m_selected == Panel::PE1Reg), btnW, BTN_H)) m_selected = Panel::PE1Reg;     // NUEVO
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE1 Mem", (m_selected == Panel::PE1Mem), btnW, BTN_H)) m_selected = Panel::PE1Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE2 CPU", (m_selected == Panel::PE2CPU), btnW, BTN_H)) m_selected = Panel::PE2CPU;
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE2 Reg", (m_selected == Panel::PE2Reg), btnW, BTN_H)) m_selected = Panel::PE2Reg;     // NUEVO
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE2 Mem", (m_selected == Panel::PE2Mem), btnW, BTN_H)) m_selected = Panel::PE2Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("PE3 CPU", (m_selected == Panel::PE3CPU), btnW, BTN_H)) m_selected = Panel::PE3CPU;
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE3 Reg", (m_selected == Panel::PE3Reg), btnW, BTN_H)) m_selected = Panel::PE3Reg;     // NUEVO
        ImGui::Dummy(ImVec2(1, 10));
        if (sidebarButton("PE3 Mem", (m_selected == Panel::PE3Mem), btnW, BTN_H)) m_selected = Panel::PE3Mem;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("RAM", (m_selected == Panel::RAM), btnW, BTN_H)) m_selected = Panel::RAM;
        ImGui::Dummy(ImVec2(1, 10));

        if (sidebarButton("Analysis Data", (m_selected == Panel::AnalysisData), btnW, BTN_H)) m_selected = Panel::AnalysisData;

        ImGui::EndChild();

        ImGui::SameLine(0.0f, SEP);

        // === Panel derecho (sin scroll en el child, cada vista gestiona lo suyo) ===
        ImGuiWindowFlags rightFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("##RightPane", ImVec2(rightW, contentH), true, rightFlags);
        if (auto* v = getView(m_selected)) v->render();
        ImGui::EndChild();

        // --- Popup SWI activo ---
        if (m_activeSwiPopupPe != -1) {
            const int pe = m_activeSwiPopupPe;
            const std::string popupId = makeSwiPopupId(pe);
            const std::string msg = "!PE" + std::to_string(pe) + " stopped via SWI!";

            // Calcular tamaño mínimo para que el texto quepa en una línea + botones
            ImVec2 textSize = ImGui::CalcTextSize(msg.c_str());
            const float sidePad = 48.0f;  // margen horizontal
            const float vertPad = 80.0f;  // margen vertical + botones
            const float minWidth = textSize.x + sidePad;
            const float minHeight = textSize.y + vertPad;

            // Asegurar tamaño cómodo al aparecer (no guardado entre runs)
            ImGui::SetNextWindowSize(ImVec2(std::max(360.0f, minWidth),
                std::max(160.0f, minHeight)),
                ImGuiCond_Appearing);

            // Modal: impide que se mezclen varios a la vez
            if (ImGui::BeginPopupModal(popupId.c_str(), nullptr,
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted(msg.c_str());
                ImGui::Separator();

                // Botón OK
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                // Tu botón exacto: "Change to PE"
                if (ImGui::Button("PE", ImVec2(120, 0))) {
                    switch (pe) {
                    case 0: m_selected = Panel::PE0CPU; break;
                    case 1: m_selected = Panel::PE1CPU; break;
                    case 2: m_selected = Panel::PE2CPU; break;
                    case 3: m_selected = Panel::PE3CPU; break;
                    }
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // Si el modal ya no está abierto, liberar el activo para abrir el siguiente de la cola
            if (!ImGui::IsPopupOpen(popupId.c_str())) {
                m_activeSwiPopupPe = -1;
                // El próximo frame, si hay pendientes en m_swiQueue, se abrirá el siguiente
            }
        }

        ImGui::End();
    }
}

void CpuTLPSharedCacheState::renderBackground() {
    m_window->clear(sf::Color(20, 20, 25));
}

// ---- Wrappers de control PE0 ----
void CpuTLPSharedCacheState::resetPE0() {
    std::cout << "[CpuTLPSharedCacheState] resetPE0() called\n";
    if (m_pe0) {
        m_pe0->reset();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE0 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepPE0() {
    std::cout << "[CpuTLPSharedCacheState] stepPE0() called\n";
    if (m_pe0) {
        m_pe0->step();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE0 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepUntilPE0(int n) {
    std::cout << "[CpuTLPSharedCacheState] stepUntilPE0(" << n << ") called\n";
    if (m_pe0) {
        m_pe0->stepUntil(n);
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE0 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepIndefinitelyPE0() {
    std::cout << "[CpuTLPSharedCacheState] stepIndefinitelyPE0() called\n";
    if (m_pe0) {
        m_pe0->stepIndefinitely();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE0 is null!\n";
    }
}

void CpuTLPSharedCacheState::stopPE0() {
    std::cout << "[CpuTLPSharedCacheState] stopPE0() called\n";
    if (m_pe0) {
        m_pe0->stopExecution();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE0 is null!\n";
    }
}

// ---- Wrappers de control PE1 ----
void CpuTLPSharedCacheState::resetPE1() {
    std::cout << "[CpuTLPSharedCacheState] resetPE1() called\n";
    if (m_pe1) {
        m_pe1->reset();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE1 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepPE1() {
    std::cout << "[CpuTLPSharedCacheState] stepPE1() called\n";
    if (m_pe1) {
        m_pe1->step();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE1 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepUntilPE1(int n) {
    std::cout << "[CpuTLPSharedCacheState] stepUntilPE1(" << n << ") called\n";
    if (m_pe1) {
        m_pe1->stepUntil(n);
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE1 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepIndefinitelyPE1() {
    std::cout << "[CpuTLPSharedCacheState] stepIndefinitelyPE1() called\n";
    if (m_pe1) {
        m_pe1->stepIndefinitely();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE1 is null!\n";
    }
}

void CpuTLPSharedCacheState::stopPE1() {
    std::cout << "[CpuTLPSharedCacheState] stopPE1() called\n";
    if (m_pe1) {
        m_pe1->stopExecution();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE1 is null!\n";
    }
}

// ---- Wrappers de control PE2 ----
void CpuTLPSharedCacheState::resetPE2() {
    std::cout << "[CpuTLPSharedCacheState] resetPE2() called\n";
    if (m_pe2) {
        m_pe2->reset();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE2 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepPE2() {
    std::cout << "[CpuTLPSharedCacheState] stepPE2() called\n";
    if (m_pe2) {
        m_pe2->step();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE2 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepUntilPE2(int n) {
    std::cout << "[CpuTLPSharedCacheState] stepUntilPE2(" << n << ") called\n";
    if (m_pe2) {
        m_pe2->stepUntil(n);
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE2 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepIndefinitelyPE2() {
    std::cout << "[CpuTLPSharedCacheState] stepIndefinitelyPE2() called\n";
    if (m_pe2) {
        m_pe2->stepIndefinitely();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE2 is null!\n";
    }
}

void CpuTLPSharedCacheState::stopPE2() {
    std::cout << "[CpuTLPSharedCacheState] stopPE2() called\n";
    if (m_pe2) {
        m_pe2->stopExecution();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE2 is null!\n";
    }
}

// ---- Wrappers de control PE3 ----
void CpuTLPSharedCacheState::resetPE3() {
    std::cout << "[CpuTLPSharedCacheState] resetPE3() called\n";
    if (m_pe3) {
        m_pe3->reset();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE3 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepPE3() {
    std::cout << "[CpuTLPSharedCacheState] stepPE3() called\n";
    if (m_pe3) {
        m_pe3->step();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE3 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepUntilPE3(int n) {
    std::cout << "[CpuTLPSharedCacheState] stepUntilPE3(" << n << ") called\n";
    if (m_pe3) {
        m_pe3->stepUntil(n);
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE3 is null!\n";
    }
}

void CpuTLPSharedCacheState::stepIndefinitelyPE3() {
    std::cout << "[CpuTLPSharedCacheState] stepIndefinitelyPE3() called\n";
    if (m_pe3) {
        m_pe3->stepIndefinitely();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE3 is null!\n";
    }
}

void CpuTLPSharedCacheState::stopPE3() {
    std::cout << "[CpuTLPSharedCacheState] stopPE3() called\n";
    if (m_pe3) {
        m_pe3->stopExecution();
    }
    else {
        std::cerr << "[CpuTLPSharedCacheState] PE3 is null!\n";
    }
}