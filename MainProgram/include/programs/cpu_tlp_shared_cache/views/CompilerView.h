#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include <string>
#include <functional>

class CompilerView : public ICpuTLPView {
public:
    CompilerView();

    void handleEvent(sf::Event&) override {}
    void update(float dt) override;
    void render() override;

    // Utilidades pedidas
    void setText(const std::string& text);
    std::string getText() const;

    // Callback para compilación
    void setCompileCallback(std::function<void(const std::string&)> callback);

private:
    std::string m_source;
    std::function<void(const std::string&)> m_compileCallback;

    // Estado de compilación
    bool m_isCompiling = false;
    std::string m_compileMessage;
    float m_messageTimer = 0.0f;

    // Simula la compilación del código a binario
    bool compileToFile(const std::string& sourceCode);

    // Permite a ImGui redimensionar el std::string sin copias
    static int TextEditCallback(struct ImGuiInputTextCallbackData* data);
};