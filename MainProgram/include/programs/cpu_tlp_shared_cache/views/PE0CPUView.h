#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/ZoomPanImage.h"
#include <array>
#include <memory>
#include <string>

namespace sf { class Texture; }

class PE0CPUView : public ICpuTLPView {
public:
    void render() override;

    // --- API específica de PE0 para modificar textos de los 5 cuadros ---
    void setLabel(size_t index, const std::string& text);                 // 0..4
    void setLabels(const std::array<std::string, 5>& labels);

private:
    std::shared_ptr<sf::Texture> m_tex;
    ZoomPanImage m_viewer;

    // Textos (por defecto vacíos)
    std::array<std::string, 5> m_labels{ "NOP", "NOP", "NOP", "NOP", "NOP" };

    void ensureLoaded();
};