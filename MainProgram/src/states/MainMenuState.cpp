#include "states/MainMenuState.h"
#include "states/ProgramState.h"
#include "core/StateManager.h"
#include "imgui.h"
#include <memory>
#include <string>

MainMenuState::MainMenuState(StateManager* stateManager, sf::RenderWindow* window)
    : State(stateManager, window)
{
    m_items = {
        "CPU TLP con Cache Compartida",
        "CPU Tomasulo",
        "Quicksort",
    };
}

void MainMenuState::handleEvent(sf::Event& event) {
    (void)event;
}

void MainMenuState::update(float) {}

bool MainMenuState::buttonAutoFitText(const char* label, float targetWidth, float height) {
    ImVec2 textSize = ImGui::CalcTextSize(label);
    float paddingX = ImGui::GetStyle().FramePadding.x * 2.f;
    float available = targetWidth - paddingX;
    float scale = available > 0.f ? (available / std::max(1.f, textSize.x)) : 1.f;
    scale = std::clamp(scale, 0.7f, 1.4f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    ImGui::SetWindowFontScale(scale);
    bool clicked = ImGui::Button(label, ImVec2(targetWidth, height));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar();
    return clicked;
}

void MainMenuState::renderBackground() {
    // Solo renderiza el fondo animado, sin UI
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
}

void MainMenuState::render() {
    // Primero renderizar el fondo
    renderBackground();

    // Luego la UI
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImVec2 panelSize(ds.x * 0.8f, ds.y * 0.8f);
    ImVec2 panelPos((ds.x - panelSize.x) * 0.5f, (ds.y - panelSize.y) * 0.5f);

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

    // CAMBIO IMPORTANTE: NO forzar el foco aquí
    // ImGui::SetNextWindowFocus(); // REMOVIDO

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.85f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

    // CAMBIO: Usar un ID único que no interfiera
    if (ImGui::Begin("##MainMenuPanel", nullptr, flags)) {
        ImGui::BeginChild("##MenuScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        float innerWidth = ImGui::GetContentRegionAvail().x;
        float btnWidth = innerWidth * 0.95f;
        float btnHeight = 64.f;

        for (const auto& it : m_items) {
            if (buttonAutoFitText(it.c_str(), btnWidth, btnHeight)) {
                m_stateManager->queueNextState(std::make_unique<ProgramState>(m_stateManager, m_window, it));
            }
            ImGui::Dummy(ImVec2(1, 8));
        }

        ImGui::EndChild();
        ImGui::End();
    }
    ImGui::PopStyleColor();
}