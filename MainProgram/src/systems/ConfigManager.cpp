#include "systems/ConfigManager.h"
#include <SFML/Window.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

static int clamp01_100(int v) { return std::max(0, std::min(100, v)); }

unsigned ConfigManager::toUIntSafe(const std::string& s, unsigned def) {
    try {
        return static_cast<unsigned>(std::stoul(s));
    }
    catch (...) { return def; }
}
int ConfigManager::toIntSafe(const std::string& s, int def) {
    try {
        return static_cast<int>(std::stol(s));
    }
    catch (...) { return def; }
}

ConfigManager::ConfigManager() { setDefault(); }

void ConfigManager::setDefault() {
    m_resolutionWidth = 1920;
    m_resolutionHeight = 1080;
    m_framerate = 60;
    m_vsync = true;
    m_screenMode = "window";
    m_windowStyle = sf::Style::Titlebar | sf::Style::Close;
    m_bgmVolume = 40;
    m_sfxVolume = 60;
}

bool ConfigManager::load(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) {
        std::cout << "[Config] No se pudo abrir " << filepath << ", usando defaults.\n";
        return false;
    }
    json j;
    try { f >> j; }
    catch (const std::exception& e) {
        std::cout << "[Config] JSON invalido: " << e.what() << " (usando defaults)\n";
        return false;
    }

    // resolution
    if (j.contains("resolution") && j["resolution"].is_object()) {
        auto& r = j["resolution"];
        if (r.contains("width")) {
            if (r["width"].is_number_unsigned()) m_resolutionWidth = r["width"].get<unsigned>();
            else if (r["width"].is_string())     m_resolutionWidth = toUIntSafe(r["width"].get<std::string>(), m_resolutionWidth);
        }
        if (r.contains("height")) {
            if (r["height"].is_number_unsigned()) m_resolutionHeight = r["height"].get<unsigned>();
            else if (r["height"].is_string())     m_resolutionHeight = toUIntSafe(r["height"].get<std::string>(), m_resolutionHeight);
        }
    }

    // framerate
    if (j.contains("framerate")) {
        if (j["framerate"].is_number_unsigned()) m_framerate = j["framerate"].get<unsigned>();
        else if (j["framerate"].is_number_integer()) m_framerate = (unsigned)j["framerate"].get<int>();
        else if (j["framerate"].is_string()) m_framerate = toUIntSafe(j["framerate"].get<std::string>(), m_framerate);
    }

    // vsync
    if (j.contains("vsync")) {
        if (j["vsync"].is_boolean()) m_vsync = j["vsync"].get<bool>();
        else if (j["vsync"].is_string()) {
            auto s = j["vsync"].get<std::string>();
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            m_vsync = (s == "true" || s == "1" || s == "on");
        }
    }

    // screenMode
    if (j.contains("screenMode")) {
        if (j["screenMode"].is_string()) setWindowStyle(j["screenMode"].get<std::string>());
    }

    // audio
    if (j.contains("audio") && j["audio"].is_object()) {
        auto& a = j["audio"];
        if (a.contains("bgm")) {
            if (a["bgm"].is_number_integer()) m_bgmVolume = clamp01_100(a["bgm"].get<int>());
            else if (a["bgm"].is_string())    m_bgmVolume = clamp01_100(toIntSafe(a["bgm"].get<std::string>(), m_bgmVolume));
        }
        if (a.contains("sfx")) {
            if (a["sfx"].is_number_integer()) m_sfxVolume = clamp01_100(a["sfx"].get<int>());
            else if (a["sfx"].is_string())    m_sfxVolume = clamp01_100(toIntSafe(a["sfx"].get<std::string>(), m_sfxVolume));
        }
    }

    return true;
}

bool ConfigManager::save(const std::string& filepath) {
    json j;
    j["resolution"]["width"] = m_resolutionWidth;
    j["resolution"]["height"] = m_resolutionHeight;
    j["framerate"] = m_framerate;
    j["vsync"] = m_vsync;
    j["screenMode"] = m_screenMode;
    j["audio"]["bgm"] = m_bgmVolume;
    j["audio"]["sfx"] = m_sfxVolume;

    std::ofstream f(filepath);
    if (!f.is_open()) return false;
    f << j.dump(4);
    return true;
}

unsigned int ConfigManager::getResolutionWidth()  const { return m_resolutionWidth; }
unsigned int ConfigManager::getResolutionHeight() const { return m_resolutionHeight; }
unsigned int ConfigManager::getFramerate()        const { return m_framerate; }
bool         ConfigManager::isVSyncEnabled()      const { return m_vsync; }
int          ConfigManager::getWindowStyle()      const { return m_windowStyle; }
std::string  ConfigManager::getScreenModeString() const { return m_screenMode; }
int          ConfigManager::getBGMVolume()        const { return m_bgmVolume; }
int          ConfigManager::getSFXVolume()        const { return m_sfxVolume; }

void ConfigManager::setResolution(unsigned int w, unsigned int h) { m_resolutionWidth = w; m_resolutionHeight = h; }
void ConfigManager::setFramerate(unsigned int fps) { m_framerate = fps; }
void ConfigManager::setVSyncEnabled(bool e) { m_vsync = e; }
void ConfigManager::setAudio(int bgm, int sfx) { m_bgmVolume = clamp01_100(bgm); m_sfxVolume = clamp01_100(sfx); }

void ConfigManager::setWindowStyle(const std::string& mode) {
    m_screenMode = mode;
    if (mode == "window")      m_windowStyle = sf::Style::Titlebar | sf::Style::Close;
    else if (mode == "fullscreen") m_windowStyle = sf::Style::Fullscreen;
    else if (mode == "borderless") m_windowStyle = sf::Style::None;
    else { m_screenMode = "window"; m_windowStyle = sf::Style::Titlebar | sf::Style::Close; }
}

