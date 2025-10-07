#include "programs/cpu_tlp_shared_cache/views/PE1CPUView.h"
#include "programs/cpu_tlp_shared_cache/utils/TextureCache.h"
#include "imgui.h"
#include <iostream>

void PE1CPUView::ensureLoaded() {
    if (!m_tex) {
        const std::string fullPath = std::string(RESOURCES_PATH) + "Assets/CPU_TLP/CPU_Pipeline.png";
        m_tex = TextureCache::instance().get(fullPath);
        if (!m_tex) {
            std::cout << "[PE1CPUView] No se pudo cargar textura: " << fullPath << "\n";
        }
        // Misma configuración que PE0: sin zoom, 80% visible, min 50% visible
        m_viewer.setZoomEnabled(false);
        m_viewer.setLockedVisibleFraction(0.80f);
        m_viewer.setMinVisibleFraction(0.50f);
    }
}

void PE1CPUView::setLabel(size_t i, const std::string& t) {
    if (i < m_labels.size()) m_labels[i] = t;
}
void PE1CPUView::setLabels(const std::array<std::string, 5>& L) { m_labels = L; }

void PE1CPUView::render() {
    ensureLoaded();
    if (!m_tex) { ImGui::TextWrapped("No se pudo cargar 'Assets/CPU_TLP/CPU_Pipeline.png'."); return; }

    m_viewer.renderWithOverlay(*m_tex, "##PE1CPU_Viewer",
        [this](const ImVec2& origin, float scale, ImDrawList* dl) {
            // Coordenadas en "pixeles de imagen"
            constexpr float Y_IMG = 550.0f;     // vertical se mantiene
            constexpr float X0_IMG = 1800.0f;    // 1er cuadro
            constexpr float DX_IMG = 1400.0f;    // espaciamiento base
            constexpr float EXTRA_BETWEEN_2_3 = 250.0f; // <-- extra solo entre 2º y 3º
            constexpr int   COUNT = 5;

            // Tamaño MUY grande (en px de imagen), escala con la imagen
            constexpr float BOX_W_IMG = 1200.0f;
            constexpr float BOX_H_IMG = 180.0f;

            const ImU32 FILL = IM_COL32(0, 0, 0, 220);
            const ImU32 BORDER = IM_COL32(255, 255, 255, 220);
            const float R = 8.0f;
            const float BORDER_THICK = 2.0f;

            ImFont* font = ImGui::GetFont();
            float base = ImGui::GetFontSize();
            float fontPx = std::max(16.0f, base * scale * 1.6f);

            for (int i = 0; i < COUNT; ++i) {
                float xImg = X0_IMG + DX_IMG * i + ((i >= 2) ? EXTRA_BETWEEN_2_3 : 0.0f);

                ImVec2 tl(origin.x + scale * xImg, origin.y + scale * Y_IMG);
                ImVec2 br(tl.x + scale * BOX_W_IMG, tl.y + scale * BOX_H_IMG);

                dl->AddRectFilled(tl, br, FILL, R);
                dl->AddRect(tl, br, BORDER, R, 0, BORDER_THICK);

                const std::string& txt = m_labels[i];
                ImVec2 ts = font->CalcTextSizeA(fontPx, FLT_MAX, 0.0f, txt.c_str());
                ImVec2 tc(tl.x + ((br.x - tl.x) - ts.x) * 0.5f,
                    tl.y + ((br.y - tl.y) - ts.y) * 0.5f + 1.0f);
                dl->AddText(font, fontPx, tc, IM_COL32(255, 255, 255, 255), txt.c_str());
            }
        }
    );
}
