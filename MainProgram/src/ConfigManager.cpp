#include "ConfigManager.h"
#include <SFML/Window.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ConfigManager::ConfigManager() {
    setDefault();
}

void ConfigManager::setDefault() {
    m_resolutionWidth = 1920;
    m_resolutionHeight = 1080;
    m_framerate = 60;
    m_vsync = true;
    m_windowStyle = sf::Style::Titlebar | sf::Style::Close;
}

bool ConfigManager::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;
    json j;
    file >> j;
    m_resolutionWidth = j["resolution"]["width"];
    m_resolutionHeight = j["resolution"]["height"];
    m_framerate = j["framerate"];
    m_vsync = j["vsync"];
    std::string mode = j["screenMode"];
    if (mode == "window")
        m_windowStyle = sf::Style::Titlebar | sf::Style::Close;
    else if (mode == "fullscreen")
        m_windowStyle = sf::Style::Fullscreen;
    else if (mode == "borderless")
        m_windowStyle = sf::Style::None;
    return true;
}

bool ConfigManager::save(const std::string& filepath) {
    json j;
    j["resolution"]["width"] = m_resolutionWidth;
    j["resolution"]["height"] = m_resolutionHeight;
    j["framerate"] = m_framerate;
    j["vsync"] = m_vsync;
    if (m_windowStyle == (sf::Style::Titlebar | sf::Style::Close))
        j["screenMode"] = "window";
    else if (m_windowStyle == sf::Style::Fullscreen)
        j["screenMode"] = "fullscreen";
    else if (m_windowStyle == sf::Style::None)
        j["screenMode"] = "borderless";
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;
    file << j.dump(4);
    return true;
}

unsigned int ConfigManager::getResolutionWidth() const { return m_resolutionWidth; }
unsigned int ConfigManager::getResolutionHeight() const { return m_resolutionHeight; }
unsigned int ConfigManager::getFramerate() const { return m_framerate; }
bool ConfigManager::isVSyncEnabled() const { return m_vsync; }
int ConfigManager::getWindowStyle() const { return m_windowStyle; }

void ConfigManager::setResolution(unsigned int width, unsigned int height) {
    m_resolutionWidth = width;
    m_resolutionHeight = height;
}
void ConfigManager::setFramerate(unsigned int fps) {
    m_framerate = fps;
}
void ConfigManager::setVSyncEnabled(bool enabled) {
    m_vsync = enabled;
}
void ConfigManager::setWindowStyle(const std::string& mode) {
    if (mode == "window")
        m_windowStyle = sf::Style::Titlebar | sf::Style::Close;
    else if (mode == "fullscreen")
        m_windowStyle = sf::Style::Fullscreen;
    else if (mode == "borderless")
        m_windowStyle = sf::Style::None;
}