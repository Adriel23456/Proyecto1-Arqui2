#include "programs/cpu_tlp_shared_cache/views/CompilerView.h"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

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

bool CompilerView::compileToFile(const std::string& sourceCode) {
    // Simulación de compilación: convertir el código fuente a instrucciones binarias
    std::vector<uint64_t> instructions;

    // Parser muy simplificado (esto es solo una simulación)
    std::istringstream stream(sourceCode);
    std::string line;

    while (std::getline(stream, line)) {
        // Quitar espacios en blanco al inicio y final
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Ignorar comentarios y líneas vacías
        if (line.empty() || line[0] == '#') continue;

        // Ignorar etiquetas
        if (line[0] == '.') continue;

        // Simular conversión de instrucciones a binario
        uint64_t instruction = 0x0000000000000000ULL;

        // Ejemplos de instrucciones simuladas
        if (line.find("MOVI") != std::string::npos) {
            instruction = 0x1000000000000001ULL;  // Código simulado para MOVI
        }
        else if (line.find("ADD") != std::string::npos) {
            instruction = 0x2000000000000002ULL;  // Código simulado para ADD
        }
        else if (line.find("CMPI") != std::string::npos) {
            instruction = 0x3000000000000003ULL;  // Código simulado para CMPI
        }
        else if (line.find("BLT") != std::string::npos) {
            instruction = 0x4000000000000004ULL;  // Código simulado para BLT
        }
        else if (line.find("SWI") != std::string::npos) {
            instruction = 0x5000000000000005ULL;  // Código simulado para SWI
        }
        else {
            // Instrucción desconocida, usar un patrón aleatorio
            instruction = 0x0F00000000000000ULL | (instructions.size() & 0xFFFF);
        }

        instructions.push_back(instruction);
    }

    // Si no hay instrucciones, agregar algunas por defecto
    if (instructions.empty()) {
        instructions.push_back(0x0100000000000000ULL);  // NOP simulado
    }

    // Escribir el archivo binario
    std::string filePath = std::string(RESOURCES_PATH) + "Assets/CPU_TLP/InstMem.bin";

    // Intentar crear el directorio si no existe
    std::string dirPath = std::string(RESOURCES_PATH) + "Assets/CPU_TLP";
    MKDIR(dirPath.c_str());

    // Intentar escribir el archivo
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        std::cerr << "[CompilerView] Could not open file for writing: " << filePath << std::endl;
        return false;
    }

    // Escribir cada instrucción en formato Little Endian
    for (uint64_t inst : instructions) {
        uint8_t bytes[8];
        for (int i = 0; i < 8; ++i) {
            bytes[i] = static_cast<uint8_t>((inst >> (i * 8)) & 0xFF);
        }
        file.write(reinterpret_cast<const char*>(bytes), 8);
    }

    file.close();

    std::cout << "[CompilerView] Compiled " << instructions.size()
        << " instructions to " << filePath << std::endl;

    return true;
}

void CompilerView::setCompileCallback(std::function<void(const std::string&)> callback) {
    m_compileCallback = callback;
}

void CompilerView::update(float dt) {
    // Actualizar el timer del mensaje
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
        if (m_messageTimer <= 0.0f) {
            m_compileMessage.clear();
        }
    }
}

void CompilerView::render() {
    // Layout básico: editor ocupa todo menos la barra inferior
    const float BETWEEN = 10.0f; // separación vertical entre editor y barra
    const float BOTTOM_H = 46.0f; // alto de la barra inferior

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float editorH = avail.y - BOTTOM_H - BETWEEN;

    // Si hay un mensaje, reservar espacio para mostrarlo
    if (!m_compileMessage.empty()) {
        editorH -= 30.0f;
    }

    if (editorH < 0.0f) editorH = 0.0f;

    // Asegurar capacidad para edición fluida
    if (m_source.capacity() - m_source.size() < 1024)
        m_source.reserve(m_source.size() + 4096);

    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_AllowTabInput |
        ImGuiInputTextFlags_CallbackResize;

    ImGui::InputTextMultiline(
        "##plain_text_editor",
        const_cast<char*>(m_source.c_str()),
        m_source.capacity() + 1,
        ImVec2(avail.x, editorH),
        flags,
        &CompilerView::TextEditCallback,
        (void*)&m_source
    );

    // Mostrar mensaje de compilación si existe
    if (!m_compileMessage.empty()) {
        ImGui::Spacing();

        // Color según el tipo de mensaje
        if (m_compileMessage.find("Success") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        }

        ImGui::TextWrapped("%s", m_compileMessage.c_str());
        ImGui::PopStyleColor();
    }

    // Separación antes de la barra inferior
    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    // ===== Barra inferior con 3 botones: Load | Save | Compile =====
    const float GAP = 10.0f;
    const float w = (avail.x - 2.0f * GAP) / 3.0f;
    const float h = BOTTOM_H;

    // Botón Load
    if (ImGui::Button("Load", ImVec2(w, h))) {
        std::cout << "[CompilerView] Load button pressed\n";
        // TODO: Implementar carga de archivo
    }
    ImGui::SameLine(0.0f, GAP);

    // Botón Save
    if (ImGui::Button("Save", ImVec2(w, h))) {
        std::cout << "[CompilerView] Save button pressed\n";
        // TODO: Implementar guardado de archivo
    }
    ImGui::SameLine(0.0f, GAP);

    // Botón Compile (verde)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));

    // Deshabilitar si está compilando
    if (m_isCompiling) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::Button("Compiling...", ImVec2(w, h));
        ImGui::PopStyleVar();
    }
    else {
        if (ImGui::Button("Compile", ImVec2(w, h))) {
            m_isCompiling = true;

            // Compilar el código
            bool success = compileToFile(m_source);

            if (success) {
                m_compileMessage = "Compilation Success! Instructions loaded.";

                // Notificar al callback si existe
                if (m_compileCallback) {
                    m_compileCallback(m_source);
                }
            }
            else {
                m_compileMessage = "Compilation Error: Could not write to InstMem.bin";
            }

            m_messageTimer = 3.0f; // Mostrar mensaje por 3 segundos
            m_isCompiling = false;
        }
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
