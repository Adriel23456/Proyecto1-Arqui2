#include <SFML/Graphics.hpp>
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiThemes.h"

#include "core/StateManager.h"
#include "states/MainMenuState.h"
#include "systems/ConfigManager.h"
#include "ui/SettingsOverlay.h"
#include "systems/AudioManager.h"

#define CONFIG_PATH RESOURCES_PATH "config.json"

static void recreateWindow(sf::RenderWindow& window, ConfigManager& cfg) {
    ImGui::SFML::Shutdown();

    sf::VideoMode vm(cfg.getResolutionWidth(), cfg.getResolutionHeight());
    window.create(vm, "Programs of Computer Engineering", cfg.getWindowStyle());
    window.setFramerateLimit(cfg.getFramerate());
    window.setVerticalSyncEnabled(cfg.isVSyncEnabled());

    ImGui::SFML::Init(window);
    ImGui::StyleColorsDark();
    imguiThemes::embraceTheDarkness();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 2.0f;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
}

int main() {
    ConfigManager config;
    if (!config.load(CONFIG_PATH)) {
        std::cout << "No se pudo cargar config.json, se usan valores por defecto.\n";
        config.setDefault();
        config.save(CONFIG_PATH);
    }

    sf::VideoMode videoMode(config.getResolutionWidth(), config.getResolutionHeight());
    sf::RenderWindow window(videoMode, "Programs of Computer Engineering", config.getWindowStyle());
    window.setFramerateLimit(config.getFramerate());
    window.setVerticalSyncEnabled(config.isVSyncEnabled());

    ImGui::SFML::Init(window);
    ImGui::StyleColorsDark();
    imguiThemes::embraceTheDarkness();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 2.0f;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;

    AudioManager audio;
    audio.setBGMVolume(config.getBGMVolume() / 100.f);
    audio.setSFXVolume(config.getSFXVolume() / 100.f);
    audio.init("Music/Mainmenu.mp3");
    audio.playBGM(true);

    // ==== NUEVO: cargar efectos de abrir/cerrar el SettingsOverlay ====
    audio.loadSFX("enterSettings", "SoundEffects/Settings.wav");
    audio.loadSFX("exitSettings", "SoundEffects/Settings.wav");
	// ==================================================================
    StateManager stateManager;
    stateManager.setCurrentState(std::make_unique<MainMenuState>(&stateManager, &window));

    SettingsOverlay overlay(&window, &config, &audio);

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                overlay.toggle();
            }

            if (!overlay.isOpen() && !io.WantCaptureMouse && !io.WantCaptureKeyboard) {
                if (stateManager.getCurrentState())
                    stateManager.getCurrentState()->handleEvent(event);
            }
        }

        sf::Time dt = clock.restart();
        float dtSec = std::min(1.f, std::max(0.f, dt.asSeconds()));
        ImGui::SFML::Update(window, dt);

        if (!overlay.isOpen() && stateManager.getCurrentState()) {
            stateManager.getCurrentState()->update(dtSec);
        }

        overlay.update(dtSec);

        window.clear();

        if (!overlay.isOpen() && stateManager.getCurrentState()) {
            stateManager.getCurrentState()->render();
        }
        else if (overlay.isOpen() && stateManager.getCurrentState()) {
            stateManager.getCurrentState()->renderBackground();
        }

        overlay.render();

        ImGui::SFML::Render(window);
        window.display();

        if (overlay.consumeApplyRequested()) {
            recreateWindow(window, config);
            overlay.rebuildAfterRecreate();
        }

        if (stateManager.hasNextState())
            stateManager.applyNextState();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
