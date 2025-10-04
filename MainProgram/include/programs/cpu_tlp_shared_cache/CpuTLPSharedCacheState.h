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
        PE0Mem,
        PE1Mem,
        PE2Mem,
        PE3Mem,
        RAM
    };

    static constexpr size_t kPanelCount = 7;
    static constexpr size_t panelIndex(Panel p) { return static_cast<size_t>(p); }

    Panel m_selected = Panel::Compiler;

    // Mantener TODAS las vistas vivas y ejecutándose
    std::array<std::unique_ptr<ICpuTLPView>, kPanelCount> m_views;

    // UI helpers
    bool sidebarButton(const char* label, bool selected, float width, float height);
    void buildAllViews();
    ICpuTLPView* getView(Panel p);
};
