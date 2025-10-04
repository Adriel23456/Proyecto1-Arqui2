#include "programs/cpu_tlp_shared_cache/utils/TextureCache.h"
#include <iostream>

TextureCache& TextureCache::instance() {
    static TextureCache inst;
    return inst;
}

std::shared_ptr<sf::Texture> TextureCache::get(const std::string& fullPath) {
    auto it = m_cache.find(fullPath);
    if (it != m_cache.end()) {
        if (auto sp = it->second.lock()) return sp;
    }

    auto tex = std::make_shared<sf::Texture>();
    if (!tex->loadFromFile(fullPath)) {
        std::cout << "[TextureCache] No se pudo cargar textura: " << fullPath << "\n";
        return nullptr;
    }
    tex->setSmooth(true);
    m_cache[fullPath] = tex;
    return tex;
}

void TextureCache::clear() {
    m_cache.clear();
}
