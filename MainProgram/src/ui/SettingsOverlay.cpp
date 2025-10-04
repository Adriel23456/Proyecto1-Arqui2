#include "ui/SettingsOverlay.h"
#include "systems/ConfigManager.h"
#include "systems/AudioManager.h"
#include "imgui.h"
#include <algorithm>
#include <set>
#include <cfloat>
#include <string>
#include <cmath>

// Opciones de modo de pantalla
static const char* kScreenModes[] = { "window", "fullscreen", "borderless" };

SettingsOverlay::SettingsOverlay(sf::RenderWindow* window, ConfigManager* cfg, AudioManager* audio)
    : m_window(window), m_cfg(cfg), m_audio(audio) {
    buildResolutions();

    m_pendingBGM = m_cfg->getBGMVolume();
    m_pendingSFX = m_cfg->getSFXVolume();
    m_pendingVSync = m_cfg->isVSyncEnabled();

    std::string sm = m_cfg->getScreenModeString();
    if (sm == "fullscreen") m_screenModeIndex = 1;
    else if (sm == "borderless") m_screenModeIndex = 2;
    else                         m_screenModeIndex = 0;

    // ---- Texto por defecto para Credits (en inglés, corregido) ----
    m_creditsText =
        "===========\n"
        "CPU with Thread-Level Parallelism (TLP)\n"
        "Team: Andres Guzman, Adriel Chaves, and Daniel Duarte\n"
        "Course: Computer Architecture II ~ Project 1\n"
        "Institution: Instituto Tecnologico de Costa Rica (TEC)\n"
        "===========\n"
        "---------------------------\n"
        "Other Projects\n"
        "Author: Adriel Chaves\n"
        "---------------------------\n";
}

void SettingsOverlay::toggle() {
    bool opening = !m_open;
    m_open = opening;
    if (opening) {
        m_animationTimer = 0.0f;
        if (m_audio) m_audio->playSFX("enterSettings");  // SFX al abrir
    }
    else {
        if (m_audio) m_audio->playSFX("exitSettings");   // SFX al cerrar
    }
}

void SettingsOverlay::open() {
    if (!m_open) {
        m_open = true;
        m_animationTimer = 0.0f;
        if (m_audio) m_audio->playSFX("enterSettings");  // SFX al abrir
    }
}

void SettingsOverlay::close() {
    if (m_open) {
        m_open = false;
        if (m_audio) m_audio->playSFX("exitSettings");   // SFX al cerrar
    }
}

bool SettingsOverlay::consumeApplyRequested() {
    bool r = m_applyRequested;
    m_applyRequested = false;
    return r;
}

void SettingsOverlay::handleEvent(const sf::Event& e) {
    (void)e;
}

void SettingsOverlay::update(float dt) {
    if (m_open && m_animationTimer < 1.0f) {
        m_animationTimer += dt * 3.0f;
        if (m_animationTimer > 1.0f) m_animationTimer = 1.0f;
    }
}

void SettingsOverlay::rebuildAfterRecreate() {
    buildResolutions();
}

void SettingsOverlay::render() {
    if (!m_open) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 disp = io.DisplaySize;

    // Calcular alpha para fade-in
    float alpha = m_animationTimer;
    float easeAlpha = alpha * alpha * (3.0f - 2.0f * alpha);

    ImGui::PushID("##SettingsOverlayContext");

    // 1) Ventana modal de fondo con fade-in
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(disp, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.4f * easeAlpha);

    ImGuiWindowFlags modalBgFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::Begin("##ModalBackground", nullptr, modalBgFlags);
    ImGui::End();

    // 2) Ventana principal con animación
    float scale = 0.85f + 0.15f * easeAlpha;
    ImVec2 winSize(disp.x * 0.95f * scale, disp.y * 0.95f * scale);
    ImVec2 winPos((disp.x - winSize.x) * 0.5f, (disp.y - winSize.y) * 0.5f);

    ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
    ImGui::SetNextWindowFocus();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, easeAlpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 30));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, easeAlpha);

    ImGuiWindowFlags mainFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin("##SettingsMainOverlay", nullptr, mainFlags)) {
        const float BTN_H = 56.0f;
        float fullWidth = ImGui::GetContentRegionAvail().x;

        if (m_showingCredits) {
            // ======= VISTA DE CREDITS =======
            ImGui::SetWindowFontScale(1.8f);
            ImGui::TextUnformatted("Credits");
            ImGui::SetWindowFontScale(1.2f);
            ImGui::Separator();
            ImGui::Spacing();

            // Región con scroll vertical (toma el alto disponible menos el espacio del botón Back)
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 scrollSize(avail.x, avail.y - BTN_H - 16.0f);
            if (scrollSize.y < 80.0f) scrollSize.y = 80.0f; // mínimo razonable

            ImGui::BeginChild("##CreditsScroll", scrollSize, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(m_creditsText.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndChild();

            ImGui::Spacing();

            // Botón Back (rojo) para regresar al menú de configuración
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Back", ImVec2(fullWidth, BTN_H))) {
                m_showingCredits = false;
            }
            ImGui::PopStyleColor(3);
        }
        else {
            // ======= VISTA NORMAL DE SETTINGS =======
            // Título
            ImGui::SetWindowFontScale(1.8f);
            ImGui::TextUnformatted("Settings");
            ImGui::SetWindowFontScale(1.2f);
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            float columnWidth = (fullWidth - 30) * 0.5f;

            // === SECCIÓN SUPERIOR - VIDEO SETTINGS ===
            ImGui::Text("Video Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // Primera fila - Resolution y Screen Mode
            ImGui::BeginGroup();

            // BOTONES CON FLECHAS PARA CAMBIAR VALORES
            ImGui::Text("Resolution");
            float btnArrowWidth = 40.0f;
            float resTextWidth = columnWidth - (btnArrowWidth * 2 + 10);

            ImGui::PushButtonRepeat(true);  // Repetición al mantener presionado
            if (ImGui::Button("<##ResLeft", ImVec2(btnArrowWidth, 30))) {
                m_resIndex--;
                if (m_resIndex < 0) m_resIndex = (int)m_resList.size() - 1;
            }
            ImGui::PopButtonRepeat();

            ImGui::SameLine();
            // Aspect ratio
            std::string aspectRatio = getAspectRatioString(m_resList[m_resIndex].w, m_resList[m_resIndex].h);
            std::string currentResText = std::to_string(m_resList[m_resIndex].w) + " x " +
                std::to_string(m_resList[m_resIndex].h) + " (" + aspectRatio + ")";
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            ImGui::Button(currentResText.c_str(), ImVec2(resTextWidth, 30));
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            if (ImGui::Button(">##ResRight", ImVec2(btnArrowWidth, 30))) {
                m_resIndex++;
                if (m_resIndex >= (int)m_resList.size()) m_resIndex = 0;
            }
            ImGui::PopButtonRepeat();

            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            ImGui::SameLine();

            ImGui::BeginGroup();

            // Screen Mode selector con botones < >
            ImGui::Text("Screen Mode");

            ImGui::PushButtonRepeat(true);
            if (ImGui::Button("<##ModeLeft", ImVec2(btnArrowWidth, 30))) {
                m_screenModeIndex--;
                if (m_screenModeIndex < 0) m_screenModeIndex = 2;
            }
            ImGui::PopButtonRepeat();

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            ImGui::Button(kScreenModes[m_screenModeIndex], ImVec2(resTextWidth, 30));
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            if (ImGui::Button(">##ModeRight", ImVec2(btnArrowWidth, 30))) {
                m_screenModeIndex++;
                if (m_screenModeIndex > 2) m_screenModeIndex = 0;
            }
            ImGui::PopButtonRepeat();

            ImGui::EndGroup();

            // Segunda fila - VSync y Apply
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::BeginGroup();
            ImGui::Checkbox("VSync", &m_pendingVSync);
            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            ImGui::SameLine();

            ImGui::BeginGroup();
            bool canApply = hasDisplayChangesPending();
            if (!canApply) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.35f, 0.15f, 1.0f));

            if (ImGui::Button("Apply Changes", ImVec2(columnWidth, BTN_H))) {
                if (canApply) {
                    applyAndSave();
                }
            }

            ImGui::PopStyleColor(3);

            if (!canApply) {
                ImGui::PopStyleVar();
            }
            ImGui::EndGroup();

            // === ESPACIADO ENTRE SECCIONES ===
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            // === SECCIÓN MEDIA - AUDIO SETTINGS ===
            ImGui::Text("Audio Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // BGM Slider - Ancho completo
            ImGui::SetNextItemWidth(fullWidth);
            int bgm = m_pendingBGM;
            if (ImGui::SliderInt("##BGMSlider", &bgm, 0, 100, "BGM Volume: %d%%")) {
                m_pendingBGM = bgm;
                m_audio->setBGMVolume(m_pendingBGM / 100.f);
                m_cfg->setAudio(m_pendingBGM, m_pendingSFX);
                m_cfg->save(std::string(RESOURCES_PATH) + "config.json");
            }

            ImGui::Spacing();

            // SFX Slider - Ancho completo
            ImGui::SetNextItemWidth(fullWidth);
            int sfx = m_pendingSFX;
            if (ImGui::SliderInt("##SFXSlider", &sfx, 0, 100, "SFX Volume: %d%%")) {
                m_pendingSFX = sfx;
                m_audio->setSFXVolume(m_pendingSFX / 100.f);
                m_cfg->setAudio(m_pendingBGM, m_pendingSFX);
                m_cfg->save(std::string(RESOURCES_PATH) + "config.json");
            }

            // === ESPACIADO ANTES DE BOTONES FINALES ===
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            // === SECCIÓN INFERIOR - BOTONES DE ACCIÓN ===
            // Credits button - Ancho completo con estilo destacado
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.25f, 0.45f, 1.0f));

            if (ImGui::Button("Credits", ImVec2(fullWidth, BTN_H))) {
                m_showingCredits = true;
            }

            ImGui::PopStyleColor(3);

            ImGui::Spacing();

            // Exit button - Ancho completo
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));

            if (ImGui::Button("Exit Game", ImVec2(fullWidth, BTN_H))) {
                m_window->close();
            }

            ImGui::PopStyleColor(3);
        }

        ImGui::End();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::PopID();
}

void SettingsOverlay::buildResolutions() {
    std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
    std::set<std::pair<unsigned, unsigned>> uniq;
    for (auto& m : modes) {
        uniq.insert({ m.width, m.height });
    }

    auto curW = m_window->getSize().x;
    auto curH = m_window->getSize().y;
    uniq.insert({ (unsigned)curW, (unsigned)curH });

    m_resList.clear();
    for (auto& p : uniq) {
        m_resList.push_back({ p.first, p.second });
    }

    if (m_resList.empty()) {
        m_resList.push_back({ 1920, 1080 });
        m_resList.push_back({ 1600, 900 });
        m_resList.push_back({ 1280, 720 });
        m_resList.push_back({ (unsigned)curW, (unsigned)curH });
    }

    m_resIndex = 0;
    for (int i = 0; i < (int)m_resList.size(); ++i) {
        if (sameRes(m_resList[i].w, m_resList[i].h,
            m_cfg->getResolutionWidth(), m_cfg->getResolutionHeight())) {
            m_resIndex = i;
            break;
        }
    }
}

std::string SettingsOverlay::getAspectRatioString(unsigned w, unsigned h) {
    // Calcular el máximo común divisor
    auto gcd = [](unsigned a, unsigned b) -> unsigned {
        while (b) {
            unsigned temp = b;
            b = a % b;
            a = temp;
        }
        return a;
        };

    unsigned divisor = gcd(w, h);
    unsigned ratioW = w / divisor;
    unsigned ratioH = h / divisor;

    // Detectar aspect ratios comunes
    float ratio = (float)w / (float)h;

    // Tolerancia para comparaciones flotantes
    const float epsilon = 0.01f;

    if (std::abs(ratio - 16.0f / 9.0f) < epsilon) return "16:9";
    if (std::abs(ratio - 16.0f / 10.0f) < epsilon) return "16:10";
    if (std::abs(ratio - 4.0f / 3.0f) < epsilon) return "4:3";
    if (std::abs(ratio - 21.0f / 9.0f) < epsilon) return "21:9";
    if (std::abs(ratio - 32.0f / 9.0f) < epsilon) return "32:9";
    if (std::abs(ratio - 5.0f / 4.0f) < epsilon) return "5:4";
    if (std::abs(ratio - 3.0f / 2.0f) < epsilon) return "3:2";

    // Si no es un ratio común, simplificar al máximo
    return std::to_string(ratioW) + ":" + std::to_string(ratioH);
}

bool SettingsOverlay::sameRes(unsigned w1, unsigned h1, unsigned w2, unsigned h2) {
    return w1 == w2 && h1 == h2;
}

bool SettingsOverlay::hasDisplayChangesPending() const {
    bool resChanged = !sameRes(m_resList[m_resIndex].w, m_resList[m_resIndex].h,
        m_cfg->getResolutionWidth(), m_cfg->getResolutionHeight());
    bool vsChanged = (m_pendingVSync != m_cfg->isVSyncEnabled());
    std::string smPending = kScreenModes[m_screenModeIndex];
    bool smChanged = (smPending != m_cfg->getScreenModeString());
    return resChanged || vsChanged || smChanged;
}

void SettingsOverlay::applyAndSave() {
    m_cfg->setResolution(m_resList[m_resIndex].w, m_resList[m_resIndex].h);
    m_cfg->setVSyncEnabled(m_pendingVSync);
    m_cfg->setWindowStyle(kScreenModes[m_screenModeIndex]);

    m_cfg->save(std::string(RESOURCES_PATH) + "config.json");

    m_applyRequested = true;
    close();  // centraliza cierre + SFX de salida
}
