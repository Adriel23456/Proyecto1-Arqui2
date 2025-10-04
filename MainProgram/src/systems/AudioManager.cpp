#include "systems/AudioManager.h"
#include <iostream>
#include <algorithm>

bool AudioManager::init(const std::string& bgmPath) {
    if (!m_bgm.openFromFile(std::string(RESOURCES_PATH) + bgmPath)) {
        std::cout << "[Audio] No se pudo cargar BGM: " << bgmPath << "\n";
        return false;
    }
    m_bgm.setVolume(m_bgmVolume * 100.f);
    m_bgm.setLoop(true);
    return true;
}

void AudioManager::playBGM(bool loop) {
    m_bgm.setLoop(loop);
    m_bgm.play();
}

void AudioManager::stopBGM() {
    m_bgm.stop();
}

void AudioManager::setBGMVolume(float vol01) {
    m_bgmVolume = std::max(0.f, std::min(1.f, vol01));
    m_bgm.setVolume(m_bgmVolume * 100.f);
}

void AudioManager::setSFXVolume(float vol01) {
    m_sfxVolume = std::max(0.f, std::min(1.f, vol01));
    // SFX "antiguo"
    m_sfx.setVolume(m_sfxVolume * 100.f);
    // Todos los SFX "nuevos"
    for (auto& kv : m_sfxMap) {
        kv.second.setVolume(m_sfxVolume * 100.f);
    }
}

// ---- API existente (compatibilidad) ----
bool AudioManager::loadSFX(const std::string& sfxPath) {
    if (!m_sfxBuffer.loadFromFile(std::string(RESOURCES_PATH) + sfxPath)) {
        std::cout << "[Audio] No se pudo cargar SFX: " << sfxPath << "\n";
        return false;
    }
    m_sfx.setBuffer(m_sfxBuffer);
    m_sfx.setVolume(m_sfxVolume * 100.f);
    return true;
}

void AudioManager::playSFX() {
    if (m_sfx.getBuffer()) m_sfx.play();
}

// ---- NUEVO: múltiples SFX por clave ----
bool AudioManager::loadSFX(const std::string& key, const std::string& sfxPath) {
    const std::string full = std::string(RESOURCES_PATH) + sfxPath;

    sf::SoundBuffer buf;
    if (!buf.loadFromFile(full)) {
        std::cout << "[Audio] No se pudo cargar SFX (" << key << "): " << sfxPath << "\n";
        return false;
    }

    // Guarda/actualiza el buffer
    m_sfxBuffers[key] = std::move(buf);

    // Crea o actualiza el sf::Sound asociado
    sf::Sound s;
    s.setBuffer(m_sfxBuffers[key]);
    s.setVolume(m_sfxVolume * 100.f);
    m_sfxMap[key] = std::move(s);

    return true;
}

void AudioManager::playSFX(const std::string& key) {
    auto it = m_sfxMap.find(key);
    if (it != m_sfxMap.end()) {
        if (it->second.getStatus() == sf::Sound::Playing) {
            it->second.stop(); // reinicia para un disparo snappy
        }
        it->second.play();
    }
}
