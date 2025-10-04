#pragma once
#include <SFML/Graphics/Texture.hpp>
#include <imgui.h>
#include <functional>

class ZoomPanImage {
public:
    // Render simple (como antes)
    void render(const sf::Texture& tex, const char* id);

    // Render con overlay (recibes origen y escala pantalla/imagen)
    void renderWithOverlay(
        const sf::Texture& tex,
        const char* id,
        const std::function<void(const ImVec2& origin, float scale, ImDrawList* dl)>& overlay
    );

    void reset() { m_zoom = 1.0f; m_pan = ImVec2(0, 0); }

    // --- NUEVO: configuraci�n ---
    void setZoomEnabled(bool enabled) { m_zoomEnabled = enabled; }
    // Fracci�n de imagen visible cuando el zoom est� deshabilitado (0<frac<=1). Ej: 0.8 -> ver 80%
    void setLockedVisibleFraction(float frac) { m_lockedVisibleFrac = frac; }
    // Fracci�n m�nima que debe permanecer visible (0<frac<=1). Ej: 0.5 -> 50%
    void setMinVisibleFraction(float frac) { m_minVisibleFrac = frac; }

private:
    float  m_zoom = 1.0f;         // multiplicador sobre el "fit" (solo si m_zoomEnabled)
    ImVec2 m_pan = ImVec2(0, 0);  // desplazamiento en p�xeles pantalla (post-fit)

    bool  m_zoomEnabled = true;
    float m_lockedVisibleFrac = 1.0f; // usado si m_zoomEnabled=false
    float m_minVisibleFrac = 0.15f;

    static float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
};

