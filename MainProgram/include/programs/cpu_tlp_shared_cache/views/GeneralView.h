#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include <SFML/Graphics/Texture.hpp>

class GeneralView : public ICpuTLPView {
public:
    GeneralView();
    void render() override;

private:
    sf::Texture m_texture;
    bool m_loaded = false;
    int  m_untilSteps = 1; // valor mínimo 1

    void ensureLoaded();
};
