#pragma once
#include <SFML/Graphics/Texture.hpp>
#include <memory>
#include <string>
#include <unordered_map>

class TextureCache {
public:
    static TextureCache& instance();

    // Devuelve una textura compartida cargada desde 'fullPath'.
    // Se mantiene en cache (weak_ptr) para reutilizarla entre vistas.
    std::shared_ptr<sf::Texture> get(const std::string& fullPath);

    void clear();

private:
    TextureCache() = default;
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<sf::Texture>> m_cache;
};
