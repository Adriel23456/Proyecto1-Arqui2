#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>

class AudioManager {
public:
    AudioManager() = default;

    // bgmPath relativo a RESOURCES_PATH (p.ej. "Music/Mainmenu.mp3")
    bool init(const std::string& bgmPath);
    void playBGM(bool loop = true);
    void stopBGM();

    void setBGMVolume(float vol01); // 0.0 - 1.0
    void setSFXVolume(float vol01); // 0.0 - 1.0

    float getBGMVolume() const { return m_bgmVolume; }
    float getSFXVolume() const { return m_sfxVolume; }

    // ---- API existente (compatibilidad con c�digo previo) ----
    bool loadSFX(const std::string& sfxPath); // carga un �nico SFX "r�pido"
    void playSFX();                            // reproduce el SFX "r�pido"

    // ---- NUEVO: m�ltiples SFX direccionados por clave ----
    bool loadSFX(const std::string& key, const std::string& sfxPath);
    void playSFX(const std::string& key);

private:
    // BGM
    sf::Music m_bgm;

    // Soporte antiguo (un SFX r�pido)
    sf::SoundBuffer m_sfxBuffer;
    sf::Sound       m_sfx;

    // Soporte nuevo (varios SFX por clave)
    std::unordered_map<std::string, sf::SoundBuffer> m_sfxBuffers;
    std::unordered_map<std::string, sf::Sound>       m_sfxMap;

    float m_bgmVolume = 0.4f;
    float m_sfxVolume = 0.6f;
};
