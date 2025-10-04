#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include <string>

class CompilerView : public ICpuTLPView {
public:
    CompilerView();

    void handleEvent(sf::Event&) override {}
    void update(float) override {}
    void render() override;

    // Utilidades pedidas
    void setText(const std::string& text);
    std::string getText() const;

private:
    std::string m_source;

    // Permite a ImGui redimensionar el std::string sin copias
    static int TextEditCallback(struct ImGuiInputTextCallbackData* data);
};
