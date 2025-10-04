#pragma once
#include <string>

class ConfigManager {
public:
    ConfigManager();
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    void setDefault();

    // Video
    unsigned int getResolutionWidth() const;
    unsigned int getResolutionHeight() const;
    unsigned int getFramerate() const;
    bool isVSyncEnabled() const;
    int  getWindowStyle() const;              // SFML style flags
    std::string getScreenModeString() const;  // "window"/"fullscreen"/"borderless"

    // Audio (0-100)
    int  getBGMVolume() const;
    int  getSFXVolume() const;

    // Setters
    void setResolution(unsigned int width, unsigned int height);
    void setFramerate(unsigned int fps);
    void setVSyncEnabled(bool enabled);
    void setWindowStyle(const std::string& mode); // "window","fullscreen","borderless"
    void setAudio(int bgm, int sfx);

private:
    // helpers robustos
    static unsigned toUIntSafe(const std::string& s, unsigned def);
    static int      toIntSafe(const std::string& s, int def);

    // datos
    unsigned int m_resolutionWidth = 1920;
    unsigned int m_resolutionHeight = 1080;
    unsigned int m_framerate = 60;
    bool         m_vsync = true;
    int          m_windowStyle = 0;      // sf::Style
    std::string  m_screenMode = "window";

    int          m_bgmVolume = 40;     // 0-100
    int          m_sfxVolume = 60;     // 0-100
};
