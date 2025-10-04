#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/ZoomPanImage.h"
#include <array>
#include <memory>
#include <string>

namespace sf { class Texture; }

class PE2CPUView : public ICpuTLPView {
public:
    void render() override;

    // Textos propios de PE2
    void setLabel(size_t index, const std::string& text);
    void setLabels(const std::array<std::string, 5>& labels);
    const std::array<std::string, 5>& getLabels() const { return m_labels; }

private:
    std::shared_ptr<sf::Texture> m_tex;
    ZoomPanImage m_viewer;
    std::array<std::string, 5> m_labels{ "NOP","NOP","NOP","NOP","NOP" };
    void ensureLoaded();
};
