#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "systems/ConfigManager.h"
#include "systems/AudioManager.h"

class SettingsOverlay {
public:
    struct ResItem { unsigned w; unsigned h; };

    SettingsOverlay(sf::RenderWindow* window, ConfigManager* cfg, AudioManager* audio);

    void toggle();
    void open();
    void close();
    bool isOpen() const { return m_open; }

    void handleEvent(const sf::Event& e);
    void update(float dt);
    void render();

    void rebuildAfterRecreate();
    bool consumeApplyRequested();

private:
    sf::RenderWindow* m_window;
    ConfigManager* m_cfg;
    AudioManager* m_audio;

    bool m_open = false;
    bool m_applyRequested = false;

    // Variable para animación de fade-in
    float m_animationTimer = 0.0f;

    // cache UI
    std::vector<ResItem> m_resList;
    int   m_resIndex = 0;

    int   m_pendingBGM = 40;
    int   m_pendingSFX = 60;
    bool  m_pendingVSync = true;
    int   m_screenModeIndex = 0;

    // ---- NUEVO: modo "Credits" dentro del overlay ----
    bool        m_showingCredits = false;
    std::string m_creditsText;

    void buildResolutions();
    bool hasDisplayChangesPending() const;
    void applyAndSave();
    static bool sameRes(unsigned w1, unsigned h1, unsigned w2, unsigned h2);
    std::string getAspectRatioString(unsigned w, unsigned h);
};

