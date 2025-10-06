#pragma once
#include "core/State.h"
#include <memory>
#include <array>
#include <string>

// Forward declarations para las vistas
class ICpuTLPView;

// Includes para los componentes asíncronos
#include "programs/cpu_tlp_shared_cache/components/InstructionMemoryComponent.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/PE0Component.h"

class CpuTLPSharedCacheState : public State {
public:
    explicit CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win);
    ~CpuTLPSharedCacheState() override;

    void handleEvent(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
    void renderBackground() override;

    void resetPE0();
    void stepPE0();
    void stepUntilPE0(int steps);
    void stepIndefinitelyPE0();
    void stopPE0();

private:
    enum class Panel {
        Compiler = 0,
        GeneralView,

        PE0CPU,
        PE0Reg,   // <<< NUEVO
        PE0Mem,

        PE1CPU,
        PE1Reg,   // <<< NUEVO
        PE1Mem,

        PE2CPU,
        PE2Reg,   // <<< NUEVO
        PE2Mem,

        PE3CPU,
        PE3Reg,   // <<< NUEVO
        PE3Mem,

        RAM,
        AnalysisData
    };

    static constexpr size_t kPanelCount = 16;
    static constexpr size_t panelIndex(Panel p) { return static_cast<size_t>(p); }

    Panel m_selected = Panel::Compiler;

    // Mantener TODAS las vistas vivas y ejecutándose
    std::array<std::unique_ptr<ICpuTLPView>, kPanelCount> m_views;

    // UI helpers
    bool sidebarButton(const char* label, bool selected, float width, float height);
    void buildAllViews();
    ICpuTLPView* getView(Panel p);

    // Componentes asíncronos
    std::unique_ptr<cpu_tlp::InstructionMemoryComponent> m_instructionMemory;

    std::unique_ptr<cpu_tlp::PE0Component> m_pe0;
    std::shared_ptr<cpu_tlp::CPUSystemSharedData> m_cpuSystemData; // ÚNICA estructura compartida

    friend class PE0RegView;
};
