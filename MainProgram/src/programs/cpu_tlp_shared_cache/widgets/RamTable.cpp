#include "programs/cpu_tlp_shared_cache/widgets/RamTable.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>

std::string RamTable::addrString(uint64_t addr64) {
    char buf[32];
    // Formato completo de 64 bits: 0x%016llX
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(addr64));
    return std::string(buf);
}

std::string RamTable::toHex_(uint64_t v) {
    char buf[20];
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(v));
    return std::string(buf);
}

void RamTable::setDataByIndex(int rowIndex, uint64_t value) {
    if (rowIndex < 0 || rowIndex >= kRows) return;
    m_values[rowIndex] = value;
}

void RamTable::setDataByAddress(uint64_t address, uint64_t value) {
    // Verificar que la dirección esté alineada a 8 bytes
    if ((address % kStep) != 0) {
        std::cout << "[RAM] setDataByAddress: direccion no alineada a 8 bytes: "
            << addrString(address) << std::endl;
        return;
    }

    // Verificar que esté dentro del rango
    if (address > kMaxAddr) {
        std::cout << "[RAM] setDataByAddress: direccion fuera de rango: "
            << addrString(address) << std::endl;
        return;
    }

    // Calcular el índice de fila
    int rowIndex = static_cast<int>(address / kStep);
    setDataByIndex(rowIndex, value);
}

void RamTable::render(const char* id) {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    // 4 columnas: Address, Value (Hex), Decimal, Double
    if (ImGui::BeginTable(id, 4, flags, ImVec2(avail.x, avail.y))) {
        ImGui::TableSetupScrollFreeze(0, 1);

        const float charW = ImGui::CalcTextSize("W").x;
        const float addrMin = std::max(180.0f, charW * 20.0f); // "0x0000000000001FF8"
        const float hexMin = std::max(180.0f, charW * 20.0f);  // "0x0000000000000000"

        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, addrMin);
        ImGui::TableSetupColumn("Value (Hex)", ImGuiTableColumnFlags_WidthFixed, hexMin);
        ImGui::TableSetupColumn("Decimal (int64, 2's comp)", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Double (IEEE 754, 64-bit)", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < kRows; ++i) {
            // Dirección: cada fila está separada por 8 bytes
            uint64_t addr = kBaseAddr + static_cast<uint64_t>(i) * kStep;
            uint64_t value = m_values[i];

            ImGui::TableNextRow();

            // Col 0: Address (64 bits completos)
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", addrString(addr).c_str());

            // Col 1: Value (Hex) - Los 64 bits en hexadecimal
            ImGui::TableSetColumnIndex(1);
            {
                std::string hexStr = toHex_(value);
                ImGui::TextUnformatted(hexStr.c_str());

                // Context menu para copiar
                std::string ctxId = "##ctx_hex_" + std::to_string(i);
                if (ImGui::BeginPopupContextItem(ctxId.c_str())) {
                    if (ImGui::MenuItem("Copy HEX")) {
                        ImGui::SetClipboardText(hexStr.c_str());
                    }
                    ImGui::EndPopup();
                }
            }

            // Col 2: Decimal (int64 con complemento a 2)
            ImGui::TableSetColumnIndex(2);
            {
                int64_t asSigned = static_cast<int64_t>(value);
                ImGui::Text("%lld", static_cast<long long>(asSigned));
            }

            // Col 3: Double (IEEE 754, 64-bit)
            ImGui::TableSetColumnIndex(3);
            {
                double dbl = 0.0;
                uint64_t bits = value;
                std::memcpy(&dbl, &bits, sizeof(double));
                char dblStr[64];
                std::snprintf(dblStr, sizeof(dblStr), "%.17g", dbl);
                ImGui::TextUnformatted(dblStr);
            }
        }

        ImGui::EndTable();
    }
}