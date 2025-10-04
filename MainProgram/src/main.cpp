#include <SFML/Graphics.hpp>
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiThemes.h"

#include "StateManager.h"
#include "MainMenuState.h"
#include "ConfigManager.h"


#define CONFIG_PATH RESOURCES_PATH "config.json"

int main() {
    // 1. Cargar la configuraci�n gr�fica desde el JSON
    ConfigManager config;
    if (!config.load(CONFIG_PATH)) {
        std::cout << "No se pudo cargar el archivo de configuraci�n, se usar�n valores por defecto." << std::endl;
        config.setDefault();
    }

    // 2. Crear la ventana con la configuraci�n cargada
    sf::VideoMode videoMode(config.getResolutionWidth(), config.getResolutionHeight());
    sf::RenderWindow window(videoMode, "Motor de dise�o y programacion gr�fica", config.getWindowStyle());
    window.setFramerateLimit(config.getFramerate());
    window.setVerticalSyncEnabled(config.isVSyncEnabled());

    // Inicializar ImGui con SFML
    ImGui::SFML::Init(window);
    ImGui::StyleColorsDark();
    imguiThemes::embraceTheDarkness();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 2.0f;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;

    // --- EJEMPLO de c�mo cargar un sprite (ahora comentado) ---
    /*
    sf::Texture t;
    // Recuerda: si quieres cargar algo, SIEMPRE se usa RESOURCES_PATH
    if (!t.loadFromFile(RESOURCES_PATH "snake.png")) {
        std::cout << "Error al cargar la textura snake.png" << std::endl;
    }
    sf::Sprite s(t);
    */

    // 3. Crear el StateManager y establecer el men� principal
    StateManager stateManager;
    stateManager.setCurrentState(std::make_unique<MainMenuState>(&stateManager, &window));

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
            if (stateManager.getCurrentState())
                stateManager.getCurrentState()->handleEvent(event);
        }

        sf::Time deltaTime = clock.restart();
        float deltaTimeSeconds = deltaTime.asSeconds();
        deltaTimeSeconds = std::min(deltaTimeSeconds, 1.f);
        deltaTimeSeconds = std::max(deltaTimeSeconds, 0.f);
        ImGui::SFML::Update(window, deltaTime);

        if (stateManager.getCurrentState())
            stateManager.getCurrentState()->update(deltaTimeSeconds);

        window.clear();
        if (stateManager.getCurrentState())
            stateManager.getCurrentState()->render();

        ImGui::SFML::Render(window);
        window.display();

        if (stateManager.hasNextState())
            stateManager.applyNextState();

        // Opcional: Si se detecta que se han aplicado cambios en la configuraci�n (desde OptionsState)
        // se puede recargar el archivo y actualizar la ventana sin interrumpir el programa.
        // Ejemplo (pseudoc�digo):
        // if (configChanged) {
        //     config.load(CONFIG_PATH);
        //     window.create(sf::VideoMode(config.getResolutionWidth(), config.getResolutionHeight()),
        //                   "Programas con diferentes motores gr�ficos!", config.getWindowStyle());
        //     window.setFramerateLimit(config.getFramerate());
        //     window.setVerticalSyncEnabled(config.isVSyncEnabled());
        // }
    }

    ImGui::SFML::Shutdown();
    return 0;
}