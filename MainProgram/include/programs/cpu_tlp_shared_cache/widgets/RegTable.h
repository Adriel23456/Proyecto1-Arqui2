#pragma once
#include <array>
#include <string>
#include <cstdint>
#include <imgui.h>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstring>

class RegTable {
public:
    // Orden solicitado:
    // 0..8:  REG0..REG8
    // 9:     PEID
    // 10:    UPPER_REG
    // 11:    LOWER_REG
    static constexpr int kRegCount = 12;

    enum RegIndex : int {
        REG0 = 0, REG1, REG2, REG3, REG4, REG5, REG6, REG7, REG8,
        PEID,
        UPPER_REG,
        LOWER_REG
    };

    explicit RegTable(int peIndex = 0) {
        // Nombres con códigos
        m_names[REG0] = "REG0 (0000)";
        m_names[REG1] = "REG1 (0001)";
        m_names[REG2] = "REG2 (0010)";
        m_names[REG3] = "REG3 (0011)";
        m_names[REG4] = "REG4 (0100)";
        m_names[REG5] = "REG5 (0101)";
        m_names[REG6] = "REG6 (0110)";
        m_names[REG7] = "REG7 (0111)";
        m_names[REG8] = "REG8 (1000)";
        m_names[PEID] = "PEID (1001)";
        m_names[UPPER_REG] = "UPPER_REG (1010)";
        m_names[LOWER_REG] = "LOWER_REG (1011)";

        // Valores por defecto: 64 bits
        for (auto& v : m_values) v = 0x0000000000000000ull;
        // LOWER_REG: ahora todo 1s (64 bits)
        m_values[LOWER_REG] = 0xFFFFFFFFFFFFFFFFull;
        // PEID según PE (0..3)
        m_values[PEID] = static_cast<uint64_t>(peIndex);

        // Textos normalizados "0x%016llX"
        for (int i = 0; i < kRegCount; ++i) {
            m_valueText[i] = toHex(m_values[i]);
        }
    }

    void setPEID(int peIndex) {
        m_values[PEID] = static_cast<uint64_t>(peIndex);
        m_valueText[PEID] = toHex(m_values[PEID]);
    }

    // === API de escritura programática (64 bits) ===
    void setValueByIndex(int idx, uint64_t val) {
        if (idx < 0 || idx >= kRegCount) return;
        m_values[idx] = val;
        m_valueText[idx] = toHex(val);
    }
    void setValueByName(const std::string& key, uint64_t val) {
        int idx = indexFromName(key);
        if (idx >= 0) setValueByIndex(idx, val);
    }

    uint64_t getValueByIndex(int idx) const {
        if (idx < 0 || idx >= kRegCount) return 0ull;
        return m_values[idx];
    }

    // Renderiza: REG Number | Value | Decimal (int64, 2's comp) | Float (double de los 64 bits)
    void render(const char* id) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        if (ImGui::BeginTable(id, 4, flags, ImVec2(avail.x, avail.y))) {
            ImGui::TableSetupScrollFreeze(0, 1);

            // Columna REG pequeña (ancho fijo). Las demás se reparten (stretch).
            const float charW = ImGui::CalcTextSize("W").x;
            const float regMin = std::max(140.0f, charW * 18.0f); // cabe "UPPER_REG (1010)"
            ImGui::TableSetupColumn("REG Number", ImGuiTableColumnFlags_WidthFixed, regMin);

            // Más espacio para Value y resto
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("Decimal (int64, 2's comp)", ImGuiTableColumnFlags_WidthStretch, 0.25f);
            ImGui::TableSetupColumn("Float (IEEE 754, 64-bit)", ImGuiTableColumnFlags_WidthStretch, 0.30f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < kRegCount; ++i) {
                ImGui::TableNextRow();

                // Col 0: nombre
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(m_names[i].c_str());

                // Col 1: Value (editable) - 64 bits hex normalizado
                ImGui::TableSetColumnIndex(1);
                {
                    char buf[24];
                    std::snprintf(buf, sizeof(buf), "%s", m_valueText[i].c_str());

                    ImGuiInputTextFlags inputFlags =
                        ImGuiInputTextFlags_CharsUppercase |
                        ImGuiInputTextFlags_AutoSelectAll |
                        ImGuiInputTextFlags_EnterReturnsTrue;

                    std::string inputId = "##reg_val64_" + std::to_string(i);
                    bool edited = ImGui::InputText(inputId.c_str(), buf, IM_ARRAYSIZE(buf), inputFlags);

                    if (edited || ImGui::IsItemDeactivatedAfterEdit()) {
                        std::string s(buf);
                        if (normalizeHex64(s)) {
                            m_valueText[i] = s;
                            m_values[i] = parseHex64(s);
                        }
                        else {
                            m_valueText[i] = toHex(m_values[i]);
                        }
                    }
                }

                // Col 2: Decimal int64 (complemento a 2 sobre los 64 bits)
                ImGui::TableSetColumnIndex(2);
                {
                    int64_t asSigned = static_cast<int64_t>(m_values[i]);
                    ImGui::Text("%lld", static_cast<long long>(asSigned));
                }

                // Col 3: Float double (reinterpretación de los mismos 64 bits)
                ImGui::TableSetColumnIndex(3);
                {
                    double dbl = 0.0;
                    uint64_t bits = m_values[i];
                    std::memcpy(&dbl, &bits, sizeof(double));
                    char dblStr[64];
                    std::snprintf(dblStr, sizeof(dblStr), "%.17g", dbl);
                    ImGui::TextUnformatted(dblStr);
                }
            }

            ImGui::EndTable();
        }
    }

private:
    std::array<std::string, kRegCount>   m_names{};
    std::array<uint64_t, kRegCount> m_values{};
    std::array<std::string, kRegCount> m_valueText{}; // "0x%016llX"

    static std::string toHex(uint64_t v) {
        char b[20];
        std::snprintf(b, sizeof(b), "0x%016llX", static_cast<unsigned long long>(v));
        return std::string(b);
    }

    static bool isHex(unsigned char c) {
        return std::isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    // Normaliza a "0x" + 16 HEX
    static bool normalizeHex64(std::string& s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) s = s.substr(2);

        std::string onlyHex; onlyHex.reserve(s.size());
        for (unsigned char c : s) if (isHex(c)) onlyHex.push_back(static_cast<char>(std::toupper(c)));

        if (onlyHex.empty()) onlyHex = "0";
        if (onlyHex.size() > 16) onlyHex = onlyHex.substr(onlyHex.size() - 16);
        if (onlyHex.size() < 16) onlyHex = std::string(16 - onlyHex.size(), '0') + onlyHex;

        s = "0x" + onlyHex;
        return true;
    }

    static uint64_t parseHex64(const std::string& sNorm) {
        unsigned long long v = 0ull;
        std::sscanf(sNorm.c_str(), "0x%16llX", &v);
        return static_cast<uint64_t>(v);
    }

    static int indexFromName(const std::string& keyIn) {
        // Acepta "REG0".."REG8", "PEID", "UPPER_REG", "LOWER_REG" (case-insensitive)
        std::string k; k.reserve(keyIn.size());
        for (char c : keyIn) k.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

        if (k == "PEID") return PEID;
        if (k == "UPPER_REG") return UPPER_REG;
        if (k == "LOWER_REG") return LOWER_REG;

        if (k.rfind("REG", 0) == 0 && k.size() >= 4) {
            int n = k[3] - '0';
            if (n >= 0 && n <= 8) return n;
        }
        return -1;
    }
};
