#include "programs/cpu_tlp_shared_cache/views/CompilerView.h"
#include "imgui.h"
#include <iostream>

CompilerView::CompilerView() {
    m_source.reserve(16 * 1024);

    // Texto de ejemplo por defecto (solo la primera vez)
    if (m_source.empty()) {
        static const char* kDefaultSource =
            "#Simple example of summing from 1 to 10000:\n"
            "MOVI REG1, #1\n"
            "MOVI REG2, #0\n"
            ".Loop:\n"
            "\tADD REG2, REG2, REG1\n"
            "\tADDI REG1, REG1, #1\n"
            "\tCMPI REG1, #10001\n"
            "\tBLT .Loop\n"
            "SWI\n";

        m_source.assign(kDefaultSource);

        // Mantener buena capacidad para edición fluida
        if (m_source.capacity() - m_source.size() < 1024)
            m_source.reserve(m_source.size() + 4096);
    }
}

int CompilerView::TextEditCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* str = static_cast<std::string*>(data->UserData);
        str->resize(static_cast<size_t>(data->BufTextLen));
        data->Buf = const_cast<char*>(str->c_str());
    }
    return 0;
}

void CompilerView::render() {
    // Layout básico: editor ocupa todo menos la barra inferior
    const float BETWEEN = 10.0f; // separación vertical entre editor y barra
    const float BOTTOM_H = 46.0f; // alto de la barra inferior

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float editorH = avail.y - BOTTOM_H - BETWEEN;
    if (editorH < 0.0f) editorH = 0.0f;

    // Asegurar capacidad para edición fluida
    if (m_source.capacity() - m_source.size() < 1024)
        m_source.reserve(m_source.size() + 4096);

    // Flags:
    // - NO usamos NoHorizontalScroll -> se habilita scroll horizontal y se desactiva el wrap.
    // - CallbackResize para que ImGui pueda redimensionar el std::string.
    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_AllowTabInput |
        ImGuiInputTextFlags_CallbackResize;

    ImGui::InputTextMultiline(
        "##plain_text_editor",
        const_cast<char*>(m_source.c_str()),
        m_source.capacity() + 1,
        ImVec2(avail.x, editorH),   // ocupa todo el ancho y el alto calculado
        flags,
        &CompilerView::TextEditCallback,
        (void*)&m_source
    );

    // Separación antes de la barra inferior
    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    // ===== Barra inferior con 3 botones: Load | Save | Compile =====
    const float GAP = 10.0f; // separación horizontal entre botones
    const float w = (avail.x - 2.0f * GAP) / 3.0f;
    const float h = BOTTOM_H;

    // Botón Load
    if (ImGui::Button("Load", ImVec2(w, h))) {
        std::cout << "SE PRESIONO LOAD\n";
        // Hook de carga real si corresponde.
    }
    ImGui::SameLine(0.0f, GAP);

    // Botón Save
    if (ImGui::Button("Save", ImVec2(w, h))) {
        std::cout << "SE PRESIONO SAVE\n";
        // Hook de guardado real si corresponde.
    }
    ImGui::SameLine(0.0f, GAP);

    // Botón Compile (verde)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));
    if (ImGui::Button("Compile", ImVec2(w, h))) {
        std::cout << "SE PRESIONO COMPILE\n";
        // Hook de compilación real si corresponde.
    }
    ImGui::PopStyleColor(3);
}

void CompilerView::setText(const std::string& text) {
    m_source = text;
    if (m_source.capacity() - m_source.size() < 1024)
        m_source.reserve(m_source.size() + 4096);
}

std::string CompilerView::getText() const {
    return m_source;
}
