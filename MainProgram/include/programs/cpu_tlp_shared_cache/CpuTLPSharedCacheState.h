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

class CpuTLPSharedCacheState : public State {
public:
    explicit CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win);
    ~CpuTLPSharedCacheState() override;

    void handleEvent(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
    void renderBackground() override;

private:
    enum class Panel {
        Compiler = 0,
        GeneralView,
        PE0CPU,
        PE0Mem,
        PE1CPU,
        PE1Mem,
        PE2CPU,
        PE2Mem,
        PE3CPU,
        PE3Mem,
        RAM,
        AnalysisData
    };

    static constexpr size_t kPanelCount = 12;
    static constexpr size_t panelIndex(Panel p) { return static_cast<size_t>(p); }

    Panel m_selected = Panel::Compiler;

    // Mantener TODAS las vistas vivas y ejecutándose
    std::array<std::unique_ptr<ICpuTLPView>, kPanelCount> m_views;

    // Componentes asíncronos
    std::shared_ptr<cpu_tlp::InstructionMemorySharedData> m_instructionMemoryData;
    std::unique_ptr<cpu_tlp::InstructionMemoryComponent> m_instructionMemory;

    // UI helpers
    bool sidebarButton(const char* label, bool selected, float width, float height);
    void buildAllViews();
    ICpuTLPView* getView(Panel p);
};
