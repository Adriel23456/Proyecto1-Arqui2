#pragma once
#include "core/State.h"
#include <memory>
#include <array>
#include <string>
#include <deque>

class ICpuTLPView;

#include "programs/cpu_tlp_shared_cache/components/InstructionMemoryComponent.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/PE0Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE1Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE2Component.h"
#include "programs/cpu_tlp_shared_cache/components/PE3Component.h"

class CpuTLPSharedCacheState : public State {
public:
    explicit CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win);
    ~CpuTLPSharedCacheState() override;

    void handleEvent(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
    void renderBackground() override;

    // PE0 control
    void resetPE0();
    void stepPE0();
    void stepUntilPE0(int steps);
    void stepIndefinitelyPE0();
    void stopPE0();

    // PE1 control
    void resetPE1();
    void stepPE1();
    void stepUntilPE1(int steps);
    void stepIndefinitelyPE1();
    void stopPE1();

    // PE2 control
    void resetPE2();
    void stepPE2();
    void stepUntilPE2(int steps);
    void stepIndefinitelyPE2();
    void stopPE2();

    // PE3 control
    void resetPE3();
    void stepPE3();
    void stepUntilPE3(int steps);
    void stepIndefinitelyPE3();
    void stopPE3();

private:
    enum class Panel {
        Compiler = 0,
        GeneralView,
        PE0CPU,
        PE0Reg,
        PE0Mem,
        PE1CPU,
        PE1Reg,
        PE1Mem,
        PE2CPU,
        PE2Reg,
        PE2Mem,
        PE3CPU,
        PE3Reg,
        PE3Mem,
        RAM,
        AnalysisData
    };

    static constexpr size_t kPanelCount = 16;
    static constexpr size_t panelIndex(Panel p) { return static_cast<size_t>(p); }

    Panel m_selected = Panel::Compiler;
    std::array<std::unique_ptr<ICpuTLPView>, kPanelCount> m_views;

    bool sidebarButton(const char* label, bool selected, float width, float height);
    void buildAllViews();
    ICpuTLPView* getView(Panel p);

    // Componentes asíncronos
    std::unique_ptr<cpu_tlp::InstructionMemoryComponent> m_instructionMemory;
    std::unique_ptr<cpu_tlp::PE0Component> m_pe0;
    std::unique_ptr<cpu_tlp::PE1Component> m_pe1;
    std::unique_ptr<cpu_tlp::PE2Component> m_pe2;
    std::unique_ptr<cpu_tlp::PE3Component> m_pe3;
    std::shared_ptr<cpu_tlp::CPUSystemSharedData> m_cpuSystemData;

    friend class PE0RegView;
    friend class PE1RegView;
    friend class PE2RegView;
    friend class PE3RegView;

    std::array<uint32_t, 4> m_swiSeenCount{};
    std::deque<int> m_swiQueue;
    int m_activeSwiPopupPe = -1;

    static std::string makeSwiPopupId(int pe) {
        return "SWI##PE" + std::to_string(pe);
    }
};