#include "MainMenuState.h"
#include "StateManager.h"

// Incluimos los demás estados para poder hacer el cambio
#include "SinglePlayerState.h"
#include "MultiplayerState.h"
#include "CreditsState.h"
#include "OptionsState.h"

#include "imgui.h"
#include <memory>
#include <string>
#include <cmath>

MainMenuState::MainMenuState(StateManager* stateManager, sf::RenderWindow* window)
    : State(stateManager, window)
    , m_showExitPrompt(false)  // inicialmente, pop-up desactivado
{
}

void MainMenuState::handleEvent(sf::Event& event) {
    // Detectamos si se presionó ESC
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        if (!m_escWasPressed) {
            // Acciones de la primera vez
            if (!m_showExitPrompt)
                m_showExitPrompt = true;
            else
                m_showExitPrompt = false;
            m_escWasPressed = true;
        }
    }
    else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape) {
        m_escWasPressed = false;
    }
}

void MainMenuState::update(float deltaTime) {
    // Lógica de actualización, si la hubiera
    // Por ejemplo, animaciones del menú, etc.
}

void MainMenuState::render() {
    // --- Dibujar fondo con shader ---
    static sf::Shader backgroundShader;
    static bool shaderLoaded = false;
    static sf::Clock shaderClock;
    if (!shaderLoaded) {
        std::string fragmentShader = R"(
            uniform vec2 iResolution;
            uniform float iTime;
            vec3 palette(float t) {
                vec3 a = vec3(0.5, 0.5, 0.5);
                vec3 b = vec3(0.5, 0.5, 0.5);
                vec3 c = vec3(1.0, 1.0, 1.0);
                vec3 d = vec3(0.263, 0.416, 0.557);
                return a + b * cos(6.28318 * (c * t + d));
            }
            void main() {
                vec2 fragCoord = gl_FragCoord.xy;
                vec2 uv = (fragCoord * 2.0 - iResolution) / iResolution.y;
                vec2 uv0 = uv;
                vec3 finalColor = vec3(0.0);
                for (float i = 0.0; i < 4.0; i++) {
                    uv = fract(uv * 1.5) - 0.5;
                    float d = length(uv) * exp(-length(uv0));
                    vec3 col = palette(length(uv0) + i * 0.4 + iTime * 0.4);
                    d = sin(d * 8.0 + iTime) / 8.0;
                    d = abs(d);
                    d = pow(0.01 / d, 1.2);
                    finalColor += col * d;
                }
                gl_FragColor = vec4(finalColor, 1.0);
            }
        )";
        backgroundShader.loadFromMemory(fragmentShader, sf::Shader::Fragment);
        shaderLoaded = true;
    }
    float currentTime = shaderClock.getElapsedTime().asSeconds();
    backgroundShader.setUniform("iTime", currentTime);
    backgroundShader.setUniform("iResolution", sf::Vector2f(m_window->getSize()));
    sf::RectangleShape backgroundRect(sf::Vector2f(m_window->getSize()));
    backgroundRect.setPosition(0, 0);
    m_window->draw(backgroundRect, &backgroundShader);

    // --- Ventana grande de MainMenu con fondo transparente ---
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // Fondo transparente
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Main Menu", nullptr, flags);

    if (m_showExitPrompt) {
        ImGui::BeginDisabled(true);
    }

    // --- Botones principales ---
    if (ImGui::Button("SinglePlayer", ImVec2(300, 60))) {
        m_stateManager->queueNextState(std::make_unique<SinglePlayerState>(m_stateManager, m_window));
    }
    if (ImGui::Button("MultiPlayer", ImVec2(300, 60))) {
        m_stateManager->queueNextState(std::make_unique<MultiplayerState>(m_stateManager, m_window));
    }
    if (ImGui::Button("Credits", ImVec2(300, 60))) {
        m_stateManager->queueNextState(std::make_unique<CreditsState>(m_stateManager, m_window));
    }
    if (ImGui::Button("Options", ImVec2(300, 60))) {
        m_stateManager->queueNextState(std::make_unique<OptionsState>(m_stateManager, m_window));
    }
    if (m_showExitPrompt) {
        ImGui::EndDisabled();
    }

    ImGui::End();
    ImGui::PopStyleColor();

    // -------------------------------
    //    2) Si m_showExitPrompt == true, mostramos pop-up
    // -------------------------------
    if (m_showExitPrompt) {
        // Calculamos el centro de la pantalla
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImVec2 windowSize(300, 150); // Tamaño del pop-up
        ImVec2 windowPos(
            (displaySize.x - windowSize.x) * 0.5f,
            (displaySize.y - windowSize.y) * 0.5f
        );

        // Ajustamos la posición y el tamaño de la ventana
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

        // Configuramos flags para que NO se pueda mover, ni redimensionar, etc.
        ImGuiWindowFlags popupFlags = 0;
        popupFlags |= ImGuiWindowFlags_NoMove;
        popupFlags |= ImGuiWindowFlags_NoResize;
        popupFlags |= ImGuiWindowFlags_NoCollapse;
        // (Si no quieres barra de título con X, no uses ImGuiWindowFlags_NoTitleBar)
        popupFlags |= ImGuiWindowFlags_NoTitleBar;

        // Comenzamos la ventana emergente
        ImGui::Begin("Salir del juego", nullptr, popupFlags);

        // Para hacer el texto multilinea si se pasa de ancho,
        // podemos usar TextWrapped o forzar wrap manual:
        ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
        ImGui::TextWrapped("Deseas salir del programa?");
        ImGui::PopTextWrapPos();

        // Espacio vertical opcional
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        // Botones SI - NO
        if (ImGui::Button("Si", ImVec2(100, 40))) {
            // Cerramos la ventana SFML
            m_window->close();
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(100, 40))) {
            // Cerramos el pop-up y seguimos en el menú
            m_showExitPrompt = false;
        }

        ImGui::End();
    }
}