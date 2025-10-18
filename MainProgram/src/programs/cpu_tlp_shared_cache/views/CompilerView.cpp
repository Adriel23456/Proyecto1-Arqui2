#include "programs/cpu_tlp_shared_cache/views/CompilerView.h"
#include "programs/cpu_tlp_shared_cache/components/Assembler.h"
#include "../include/ui/tinyfiledialogs.h"
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
    try {
        std::string cleanedSource = sourceCode;

        // 1. Eliminación de BOM UTF-8 (Ya existente, solo se revisa el target)
        if (!cleanedSource.empty() &&
            (static_cast<unsigned char>(cleanedSource[0]) == 0xEF ||
                static_cast<unsigned char>(cleanedSource[0]) == 0xFF)) {
            // ... (Lógica de eliminación de BOM UTF-8 que ya tienes)
            // Asegúrate de que m_source se actualice correctamente si usas el texto limpio:
            if (cleanedSource.size() > 2 &&
                static_cast<unsigned char>(cleanedSource[1]) == 0xBB &&
                static_cast<unsigned char>(cleanedSource[2]) == 0xBF) {
                cleanedSource = cleanedSource.substr(3);
            }
            else {
                cleanedSource = cleanedSource.substr(1);
            }
        }

        // **2. LIMPIEZA DE CARACTERES NO ASCII Y ESPACIOS NO ESTÁNDAR (MEJORADO)**
        std::string finalSource;
        finalSource.reserve(cleanedSource.size());

        for (size_t i = 0; i < cleanedSource.length(); ++i) {
            unsigned char c = static_cast<unsigned char>(cleanedSource[i]);

            // --- Normalización de saltos de línea ---
            if (c == '\r') {
                finalSource += '\n'; // Convertir \r a \n
                // Si es \r\n, saltar el siguiente \n para no duplicar
                if (i + 1 < cleanedSource.length() && cleanedSource[i + 1] == '\n') {
                    i++;
                }
            }
            else if (c == '\n') {
                finalSource += '\n'; // Mantener \n
            }
            // --- Mantener caracteres ASCII imprimibles estándar ---
            else if (c >= 32 && c <= 126) {
                // Rango ASCII imprimible estándar (incluye espacio, #, etc.)
                finalSource += static_cast<char>(c);
            }
            // --- Ignorar el resto (caracteres de control, no ASCII, etc.) ---
        }

        // 3. Guardar temporalmente el texto limpio en un archivo
        std::string tmpPath = std::string(RESOURCES_PATH) + "Assets/CPU_TLP/TempSource.txt";
        MKDIR((std::string(RESOURCES_PATH) + "Assets/CPU_TLP").c_str());
        std::ofstream tmpFile(tmpPath);
        tmpFile << finalSource; // <-- Usar la cadena limpia
        tmpFile.close();

        // 4. Usar el ensamblador (el resto del código sigue igual)
        Assembler assembler;

        std::vector<uint64_t> instructions = assembler.assembleFile(tmpPath);

        // 3. Escribir el resultado binario
        std::string outPath = std::string(RESOURCES_PATH) + "Assets/CPU_TLP/InstMem.bin";
        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "[CompilerView] Could not open file for writing: " << outPath << std::endl;
            return false;
        }

        for (uint64_t inst : instructions) {
            uint8_t bytes[8];
            for (int i = 0; i < 8; ++i)
                bytes[i] = static_cast<uint8_t>((inst >> (i * 8)) & 0xFF);
            out.write(reinterpret_cast<const char*>(bytes), 8);
        }
        out.close();

        std::cout << "[CompilerView] Compiled " << instructions.size()
            << " instructions to " << outPath << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[CompilerView] Compilation failed: " << e.what() << std::endl;
        m_compileMessage = std::string("Compilation Error: ") + e.what();
        return false;
    }
}

void CompilerView::setCompileCallback(std::function<void(const std::string&)> callback) {
    m_compileCallback = callback;
}

void CompilerView::update(float dt) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
        if (m_messageTimer <= 0.0f) {
            m_compileMessage.clear();
        }
    }
}

void CompilerView::render() {
    const float BETWEEN = 10.0f;
    const float BOTTOM_H = 46.0f;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float editorH = avail.y - BOTTOM_H - BETWEEN;

    if (!m_compileMessage.empty()) editorH -= 30.0f;
    if (editorH < 0.0f) editorH = 0.0f;

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

    if (!m_compileMessage.empty()) {
        ImGui::Spacing();
        if (m_compileMessage.find("Success") != std::string::npos)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

        ImGui::TextWrapped("%s", m_compileMessage.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(1.0f, BETWEEN));

    const float GAP = 10.0f;
    const float w = (avail.x - 2.0f * GAP) / 3.0f;
    const float h = BOTTOM_H;

    // ========== Botón Load ==========
    if (ImGui::Button("Load", ImVec2(w, h))) {
        const char* filters[] = { "*.txt" };
        const char* filePath = tinyfd_openFileDialog(
            "Select Assembly Source File",
            "",
            1, filters, "Text files (*.txt)",
            0
        );

        if (filePath) {
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::ostringstream buffer;
                buffer << file.rdbuf();
                m_source = buffer.str();
                std::cout << "[CompilerView] Loaded file: " << filePath << std::endl;
                m_compileMessage = std::string("Loaded: ") + filePath;
            }
            else {
                m_compileMessage = "Error: Could not open file.";
            }
        }
        else {
            std::cout << "[CompilerView] Load cancelled.\n";
        }

        m_messageTimer = 3.0f;
    }

    ImGui::SameLine(0.0f, GAP);

    // ========== Botón Save ==========
    if (ImGui::Button("Save", ImVec2(w, h))) {
        const char* filePath = tinyfd_saveFileDialog(
            "Save Assembly File",
            "program.txt",
            0, nullptr, nullptr
        );

        if (filePath) {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file << m_source;
                file.close();
                m_compileMessage = std::string("Saved: ") + filePath;
            }
            else {
                m_compileMessage = "Error: Could not save file.";
            }
        }
        m_messageTimer = 3.0f;
    }

    ImGui::SameLine(0.0f, GAP);

    // ========== Botón Compile ==========
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.45f, 0.16f, 1.0f));

    if (m_isCompiling) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::Button("Compiling...", ImVec2(w, h));
        ImGui::PopStyleVar();
    }
    else {
        if (ImGui::Button("Compile", ImVec2(w, h))) {
            m_isCompiling = true;
            bool success = compileToFile(m_source);

            if (success) {
                m_compileMessage = "Compilation Success! Instructions loaded.";
                if (m_compileCallback)
                    m_compileCallback(m_source);
            }
            else {
                if (m_compileMessage.empty())
                    m_compileMessage = "Compilation Error.";
            }

            m_messageTimer = 3.0f;
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
