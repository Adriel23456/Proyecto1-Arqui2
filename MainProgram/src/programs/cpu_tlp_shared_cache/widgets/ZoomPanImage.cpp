#include "programs/cpu_tlp_shared_cache/widgets/ZoomPanImage.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include <algorithm>

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void ZoomPanImage::render(const sf::Texture& tex, const char* id) {
    renderWithOverlay(tex, id, nullptr);
}

void ZoomPanImage::renderWithOverlay(
    const sf::Texture& tex,
    const char* id,
    const std::function<void(const ImVec2& origin, float scale, ImDrawList* dl)>& overlay
) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x <= 0.f || avail.y <= 0.f) {
        ImGui::TextUnformatted("No space to render image.");
        return;
    }

    ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::BeginChild(id, avail, true, childFlags);

    auto ts = tex.getSize();
    ImVec2 img(ts.x > 0 ? static_cast<float>(ts.x) : 1.f,
        ts.y > 0 ? static_cast<float>(ts.y) : 1.f);

    float fitScale = std::min(avail.x / img.x, avail.y / img.y);
    fitScale = std::max(fitScale, 0.0001f);

    // --- SCALE: fijo si zoom deshabilitado, o m_zoom si está habilitado ---
    float lockedFrac = clampf(m_lockedVisibleFrac, 0.05f, 1.0f); // seguridad
    float scale = m_zoomEnabled ? (fitScale * m_zoom)
        : (fitScale / lockedFrac); // ej: frac=0.8 -> 1.25x

    ImVec2 dispSize(img.x * scale, img.y * scale);
    ImVec2 centerTL((avail.x - dispSize.x) * 0.5f, (avail.y - dispSize.y) * 0.5f);
    ImVec2 origin(centerTL.x + m_pan.x, centerTL.y + m_pan.y);

    ImVec2 childScreenPos = ImGui::GetCursorScreenPos();
    ImVec2 mouse = ImGui::GetIO().MousePos;
    ImVec2 localMouse(mouse.x - childScreenPos.x, mouse.y - childScreenPos.y);

    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    // ======= LÍMITE: fracción mínima visible =======
    float minFrac = clampf(m_minVisibleFrac, 0.05f, 1.0f);
    auto clamp_to_min_visible = [&](ImVec2& originRef) {
        const float minVisX = std::min(minFrac * dispSize.x, avail.x);
        const float minVisY = std::min(minFrac * dispSize.y, avail.y);

        const float minOriginX = (minVisX - dispSize.x);
        const float maxOriginX = (avail.x - minVisX);
        const float minOriginY = (minVisY - dispSize.y);
        const float maxOriginY = (avail.y - minVisY);

        originRef.x = clampf(originRef.x, minOriginX, maxOriginX);
        originRef.y = clampf(originRef.y, minOriginY, maxOriginY);

        m_pan.x = originRef.x - centerTL.x;
        m_pan.y = originRef.y - centerTL.y;
        };

    // Clamp inicial
    clamp_to_min_visible(origin);

    // Doble click: recenter; si zoom habilitado, también reset zoom
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (m_zoomEnabled) m_zoom = 1.0f;
        m_pan = ImVec2(0, 0);

        // Recalcular con estado reseteado
        scale = m_zoomEnabled ? (fitScale * m_zoom) : (fitScale / lockedFrac);
        dispSize = ImVec2(img.x * scale, img.y * scale);
        centerTL = ImVec2((avail.x - dispSize.x) * 0.5f, (avail.y - dispSize.y) * 0.5f);
        origin = ImVec2(centerTL.x + m_pan.x, centerTL.y + m_pan.y);
        clamp_to_min_visible(origin);
    }

    // Zoom con la rueda SOLO si está habilitado
    if (hovered && m_zoomEnabled) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float zoomStep = 1.1f;
            float newZoom = (wheel > 0.0f) ? (m_zoom * zoomStep) : (m_zoom / zoomStep);
            newZoom = clamp(newZoom, 0.2f, 8.0f);

            if (newZoom != m_zoom) {
                float oldScale = scale;
                float newScale = fitScale * newZoom;

                ImVec2 rel(localMouse.x - origin.x, localMouse.y - origin.y);
                ImVec2 texCoord(rel.x / oldScale, rel.y / oldScale);
                origin.x = localMouse.x - texCoord.x * newScale;
                origin.y = localMouse.y - texCoord.y * newScale;

                m_zoom = newZoom;
                scale = newScale;
                dispSize = ImVec2(img.x * scale, img.y * scale);
                centerTL = ImVec2((avail.x - dispSize.x) * 0.5f, (avail.y - dispSize.y) * 0.5f);

                clamp_to_min_visible(origin);
            }
        }
    }

    // Pan con drag izquierdo (siempre)
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        origin.x += delta.x;
        origin.y += delta.y;
        clamp_to_min_visible(origin);
    }

    // Dibujo imagen
    ImGui::SetCursorPos(origin);
    ImGui::Image(tex, sf::Vector2f(dispSize.x, dispSize.y));

    // Overlay
    if (overlay) {
        overlay(origin, scale, ImGui::GetWindowDrawList());
    }

    ImGui::EndChild();
}

