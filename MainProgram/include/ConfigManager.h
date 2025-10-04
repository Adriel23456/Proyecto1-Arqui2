#pragma once
#include <string>

class ConfigManager {
public:
    ConfigManager();
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    void setDefault();

    unsigned int getResolutionWidth() const;
    unsigned int getResolutionHeight() const;
    unsigned int getFramerate() const;
    bool isVSyncEnabled() const;
    int getWindowStyle() const; // SFML: e.g. sf::Style::Titlebar | sf::Style::Close

    // Setters para actualizar la configuración
    void setResolution(unsigned int width, unsigned int height);
    void setFramerate(unsigned int fps);
    void setVSyncEnabled(bool enabled);
    void setWindowStyle(const std::string& mode); // "window", "fullscreen" o "borderless"

private:
    unsigned int m_resolutionWidth;
    unsigned int m_resolutionHeight;
    unsigned int m_framerate;
    bool m_vsync;
    int m_windowStyle;
};