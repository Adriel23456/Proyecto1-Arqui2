#pragma once
#include "core/State.h"
#include <memory>
#include <array>
#include <string>

class ICpuTLPView;

class CpuTLPSharedCacheState : public State {
public:
    explicit CpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win);
    ~CpuTLPSharedCacheState() override = default;

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

    // Mantener TODAS las vistas vivas y ejecut�ndose
    std::array<std::unique_ptr<ICpuTLPView>, kPanelCount> m_views;

    // UI helpers
    bool sidebarButton(const char* label, bool selected, float width, float height);
    void buildAllViews();
    ICpuTLPView* getView(Panel p);
};
