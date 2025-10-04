#include "programs/cpu_tlp_shared_cache/widgets/RamTable.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

std::string RamTable::addrString(uint64_t addr64) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(addr64));
    return std::string(buf);
}

// Limpia "0x", espacios, valida 64 hex chars, devuelve bytes[32] (little-endian interpret-ready)
// y una versión normalizada "0x" + 64 HEX (para mostrar).
bool RamTable::sanitizeHex32B(const std::string& in, std::array<uint8_t, kBytesPerRow>& outBytes, std::string* normalizedHexOut) {
    std::string s; s.reserve(in.size());
    // Quitar espacios y prefijos "0x"
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (std::isspace(c)) continue;
        // omitir "0x"/"0X" en cualquier posición
        if (c == '0' && i + 1 < in.size() && (in[i + 1] == 'x' || in[i + 1] == 'X')) { ++i; continue; }
        s.push_back(static_cast<char>(c));
    }
    // Deben ser exactamente 64 hex
    if (s.size() != 64) return false;
    auto isHex = [](unsigned char c) {
        return std::isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        };
    for (char c : s) if (!isHex((unsigned char)c)) return false;

    // Pasar a bytes (cada 2 chars = 1 byte) en orden natural (byte[0] = s[0..1])
    for (size_t i = 0; i < kBytesPerRow; ++i) {
        unsigned v = 0;
        std::sscanf(s.substr(i * 2, 2).c_str(), "%2X", &v);
        outBytes[i] = static_cast<uint8_t>(v);
    }

    if (normalizedHexOut) {
        std::string up; up.reserve(66);
        up += "0x";
        for (char c : s) up += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        *normalizedHexOut = std::move(up);
    }
    return true;
}

void RamTable::setDataByIndex(int rowIndex, const std::string& hex) {
    if (rowIndex < 0 || rowIndex >= kRows) return;
    std::array<uint8_t, kBytesPerRow> tmp{};
    std::string normalized;
    if (!sanitizeHex32B(hex, tmp, &normalized)) {
        std::cout << "[RAM] setDataByIndex: hex invalido (se esperan 32 bytes / 64 hex)." << std::endl;
        return;
    }
    m_dataHex[rowIndex] = std::move(normalized);
}

void RamTable::setDataByAddress(uint64_t address, const std::string& hex) {
    if (address < kBaseAddr || address >= (kBaseAddr + kRows * kStep) || ((address - kBaseAddr) % kStep) != 0) {
        std::cout << "[RAM] setDataByAddress: direccion fuera de rango o no alineada: " << addrString(address) << std::endl;
        return;
    }
    int rowIndex = static_cast<int>((address - kBaseAddr) / kStep);
    setDataByIndex(rowIndex, hex);
}

void RamTable::render(const char* id) {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // -------- Column sizing por porcentajes para que Address+Data+Float0..3 llenen el ancho visible ----------
    // Usamos SizingFixedFit + ScrollX:
    //  - fijas por frame en función de avail.x
    //  - la suma de las primeras 6 columnas ~= avail.x
    //  - las 4 columnas Int quedan a la derecha, accesibles con scroll horizontal
    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_SizingFixedFit;

    // Porcentajes base
    // Visible por defecto: Address (15%), Data (45%), Float0..3 (4x 10% = 40%) => 110%? No, normalizamos a 100% con límites.
    // Mejor: Address 14%, Data 46%, Floats 4x 10% = 40%  -> suma 100% exacto.
    float wAddrPct = 0.14f;
    float wDataPct = 0.46f;
    float wFloatPct = 0.10f; // cada FloatN

    // Mínimos prácticos por contenido (si la ventana es muy angosta)
    const float charW = ImGui::CalcTextSize("F").x;
    const float mAddr = charW * 20.0f;   // "0x"+16 hex + margen
    const float mData = charW * 70.0f;   // "0x"+64 hex + margen
    const float mFloat = charW * 12.0f;   // ~"-1.23e+38" tamaño decente
    const float mInt = charW * 12.0f;

    // Anchos derivados de avail (clamp con mínimos)
    float wAddr = std::max(mAddr, avail.x * wAddrPct);
    float wData = std::max(mData, avail.x * wDataPct);
    float wF0 = std::max(mFloat, avail.x * wFloatPct);
    float wF1 = std::max(mFloat, avail.x * wFloatPct);
    float wF2 = std::max(mFloat, avail.x * wFloatPct);
    float wF3 = std::max(mFloat, avail.x * wFloatPct);

    // Ints: cualquier ancho fijo razonable; quedarán fuera inicialmente
    float wI = std::max(mInt, avail.x * 0.10f);

    if (ImGui::BeginTable("##ram_table", 10, flags, ImVec2(avail.x, avail.y))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, wAddr);
        ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_WidthFixed, wData);
        ImGui::TableSetupColumn("Float0", ImGuiTableColumnFlags_WidthFixed, wF0);
        ImGui::TableSetupColumn("Float1", ImGuiTableColumnFlags_WidthFixed, wF1);
        ImGui::TableSetupColumn("Float2", ImGuiTableColumnFlags_WidthFixed, wF2);
        ImGui::TableSetupColumn("Float3", ImGuiTableColumnFlags_WidthFixed, wF3);
        ImGui::TableSetupColumn("Int0", ImGuiTableColumnFlags_WidthFixed, wI);
        ImGui::TableSetupColumn("Int1", ImGuiTableColumnFlags_WidthFixed, wI);
        ImGui::TableSetupColumn("Int2", ImGuiTableColumnFlags_WidthFixed, wI);
        ImGui::TableSetupColumn("Int3", ImGuiTableColumnFlags_WidthFixed, wI);
        ImGui::TableHeadersRow();

        // Render filas
        for (int i = 0; i < kRows; ++i) {
            uint64_t addr = kBaseAddr + static_cast<uint64_t>(i) * kStep;

            // Parsear data si válida
            const std::string& hex = m_dataHex[i];
            bool has32B = false;
            std::array<uint8_t, kBytesPerRow> bytes{};
            if (!hex.empty()) {
                // hex normalizado empieza con "0x" + 64 hex
                // Podemos volver a sanitizar por robustez si el caller cambió el formato manualmente
                std::string dummy;
                has32B = sanitizeHex32B(hex, bytes, &dummy);
                if (!has32B) {
                    // el valor se mostrará textual, pero no calculamos floats/ints
                }
            }

            // Calcular floats/ints (little-endian)
            float f[4]{};
            int32_t iv[4]{};
            if (has32B) {
                // primero 16 bytes => 4 floats32
                for (int k = 0; k < 4; ++k) {
                    uint32_t word =
                        (static_cast<uint32_t>(bytes[k * 4 + 0])) |
                        (static_cast<uint32_t>(bytes[k * 4 + 1]) << 8) |
                        (static_cast<uint32_t>(bytes[k * 4 + 2]) << 16) |
                        (static_cast<uint32_t>(bytes[k * 4 + 3]) << 24);
                    std::memcpy(&f[k], &word, sizeof(uint32_t));
                }
                // siguientes 16 bytes => 4 int32
                for (int k = 0; k < 4; ++k) {
                    uint32_t off = 16 + k * 4;
                    uint32_t word =
                        (static_cast<uint32_t>(bytes[off + 0])) |
                        (static_cast<uint32_t>(bytes[off + 1]) << 8) |
                        (static_cast<uint32_t>(bytes[off + 2]) << 16) |
                        (static_cast<uint32_t>(bytes[off + 3]) << 24);
                    std::memcpy(&iv[k], &word, sizeof(uint32_t));
                }
            }

            ImGui::TableNextRow();

            // Address
            ImGui::TableSetColumnIndex(0);
            std::string a = addrString(addr);
            ImGui::TextUnformatted(a.c_str());

            // Data
            ImGui::TableSetColumnIndex(1);
            if (!hex.empty()) ImGui::TextUnformatted(hex.c_str());
            else              ImGui::TextUnformatted("");

            auto printFloat = [](float v) {
                char buf[32];
                // representación compacta y estable
                std::snprintf(buf, sizeof(buf), "%.6g", v);
                ImGui::TextUnformatted(buf);
                };

            // Floats
            for (int col = 0; col < 4; ++col) {
                ImGui::TableSetColumnIndex(2 + col);
                if (has32B) printFloat(f[col]); else ImGui::TextUnformatted("");
            }

            // Ints
            for (int col = 0; col < 4; ++col) {
                ImGui::TableSetColumnIndex(6 + col);
                if (has32B) ImGui::Text("%d", iv[col]); else ImGui::TextUnformatted("");
            }
        }

        ImGui::EndTable();
    }
}
