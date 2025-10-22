#include "programs/cpu_tlp_shared_cache/widgets/CacheMemTable.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>
#include <cstring>

void CacheMemTable::setLine(int index, const std::string& tag, const std::array<uint8_t, 32>& data, const std::string& mesi) {
    if (index < 0 || index >= kTotalLines) return;
    m_lines[index].tag = tag;
    m_lines[index].data = data;
    m_lines[index].mesi = mesi;
}

void CacheMemTable::setLineBySetWay(int set, int way, const std::string& tag, const std::array<uint8_t, 32>& data, const std::string& mesi) {
    if (set < 0 || set >= kSets || way < 0 || way >= kWays) return;
    int index = set * kWays + way;
    setLine(index, tag, data, mesi);
}

std::string CacheMemTable::dataToHex(const std::array<uint8_t, 32>& data) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');

    for (int i = 0; i < 32; ++i) {
        if (i > 0 && i % 8 == 0) {
            oss << "\n";
        }
        else if (i > 0 && i % 4 == 0) {
            oss << " ";
        }
        oss << std::setw(2) << static_cast<int>(data[i]);
    }

    return oss.str();
}

uint64_t CacheMemTable::extract8Bytes(const std::array<uint8_t, 32>& data, int segment) {
    if (segment < 0 || segment >= 4) return 0;

    uint64_t value = 0;
    int offset = segment * 8;

    for (int i = 0; i < 8; ++i) {
        value |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }

    return value;
}

std::string CacheMemTable::format8BytesAsHex(const std::array<uint8_t, 32>& data, int segment) {
    uint64_t value = extract8Bytes(data, segment);
    char buf[20];
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(value));
    return std::string(buf);
}

void CacheMemTable::render(const char* id) {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // ========== CONTROLES DE NAVEGACIÓN ==========
    // ===== 1. MÁS ESPACIO VERTICAL PARA EL NAVEGADOR =====
    const float CTRL_HEIGHT = 200.0f;
    const float TABLE_HEIGHT = avail.y - CTRL_HEIGHT - 10.0f;

    ImGui::BeginChild("##CacheControls", ImVec2(avail.x, CTRL_HEIGHT), true);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cache Line Navigator");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(1, 8));  // Más espacio vertical

        // ===== SELECTOR DE SET =====
        ImGui::Text("Set:");
        ImGui::SameLine();

        const float btnW = 60.0f;      // Botones más anchos
        const float btnH = 32.0f;      // Botones más altos
        const float spacing = 8.0f;    // Más espacio entre botones

        for (int s = 0; s < kSets; ++s) {
            if (s > 0) ImGui::SameLine(0, spacing);

            if (m_selectedSet == s) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.8f, 1.0f));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            }

            char label[16];
            std::snprintf(label, sizeof(label), "%d##set", s);
            if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                m_selectedSet = s;
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::Dummy(ImVec2(1, 10));  // Más espacio entre filas

        // ===== SELECTOR DE WAY =====
        ImGui::Text("Way:");
        ImGui::SameLine();

        for (int w = 0; w < kWays; ++w) {
            if (w > 0) ImGui::SameLine(0, spacing);

            if (m_selectedWay == w) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.9f, 0.6f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 1.0f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.8f, 0.5f, 1.0f));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            }

            char label[16];
            std::snprintf(label, sizeof(label), "%d##way", w);
            if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                m_selectedWay = w;
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine(0, 25.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f),
            "Viewing: Set %d, Way %d", m_selectedSet, m_selectedWay);
    }
    ImGui::EndChild();

    ImGui::Dummy(ImVec2(1, 5));

    // ========== TABLA DE LA LÍNEA SELECCIONADA ==========
    ImGui::BeginChild("##CacheTableArea", ImVec2(avail.x, TABLE_HEIGHT), false);
    {
        ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        // 5 columnas: Tag, Data, MESI, Decimal, Double
        if (ImGui::BeginTable(id, 5, flags, ImVec2(0, 0))) {
            ImGui::TableSetupScrollFreeze(0, 1);

            const float charW = ImGui::CalcTextSize("W").x;

            // ===== AJUSTAR TAMAÑOS DE COLUMNAS =====
            const float tagWidth = charW * 22.0f;      // 2. Tag perfecto - sin cambios
            const float dataWidth = charW * 27.0f;     // 3. Data contraído 30% (de 38 a ~27)
            const float mesiWidth = charW * 6.0f;      // 4. MESI perfecto - sin cambios
            // 5 y 6. Decimal y Double extendidos con factores de stretch mayores

            ImGui::TableSetupColumn("Tag (56 bits)", ImGuiTableColumnFlags_WidthFixed, tagWidth);
            ImGui::TableSetupColumn("Data (Hex, 32B)", ImGuiTableColumnFlags_WidthFixed, dataWidth);
            ImGui::TableSetupColumn("MESI", ImGuiTableColumnFlags_WidthFixed, mesiWidth);
            ImGui::TableSetupColumn("Decimal (int64, 2's comp)", ImGuiTableColumnFlags_WidthStretch, 1.5f);  // 5. Extendido de 1.0f a 1.5f
            ImGui::TableSetupColumn("Double (IEEE 754, 64-bit)", ImGuiTableColumnFlags_WidthStretch, 1.5f);  // 6. Extendido de 1.0f a 1.5f
            ImGui::TableHeadersRow();

            // Obtener la línea actual
            const CacheLine& line = getCurrentLine();

            // ===== FILA ÚNICA: LA LÍNEA SELECCIONADA =====
            // Como la DATA tiene 4 segmentos de 8 bytes, renderizamos 4 sub-filas
            for (int seg = 0; seg < 4; ++seg) {
                ImGui::TableNextRow();

                // ===== COLUMNA 0: TAG (solo en la primera sub-fila) =====
                ImGui::TableSetColumnIndex(0);
                if (seg == 0) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Set %d / Way %d", m_selectedSet, m_selectedWay);
                    ImGui::Text("%s", line.tag.c_str());
                }
                else {
                    ImGui::Dummy(ImVec2(0, 0));
                }

                // ===== COLUMNA 1: DATA (segmento actual) =====
                ImGui::TableSetColumnIndex(1);
                {
                    // Mostrar solo los 8 bytes de este segmento
                    std::ostringstream oss;
                    oss << std::hex << std::uppercase << std::setfill('0');

                    int offset = seg * 8;
                    for (int i = 0; i < 8; ++i) {
                        if (i > 0 && i % 4 == 0) oss << " ";
                        oss << std::setw(2) << static_cast<int>(line.data[offset + i]);
                    }

                    ImGui::Text("[%d] %s", seg, oss.str().c_str());
                }

                // ===== COLUMNA 2: MESI (solo en la primera sub-fila) =====
                ImGui::TableSetColumnIndex(2);
                if (seg == 0) {
                    ImVec4 color;
                    if (line.mesi == "M") color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    else if (line.mesi == "E") color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                    else if (line.mesi == "S") color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
                    else color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

                    ImGui::TextColored(color, "%s", line.mesi.c_str());
                }
                else {
                    ImGui::Dummy(ImVec2(0, 0));
                }

                // ===== COLUMNA 3: DECIMAL (segmento actual) =====
                ImGui::TableSetColumnIndex(3);
                {
                    uint64_t bits = extract8Bytes(line.data, seg);
                    int64_t asSigned = static_cast<int64_t>(bits);
                    ImGui::Text("%lld", static_cast<long long>(asSigned));
                }

                // ===== COLUMNA 4: DOUBLE (segmento actual) =====
                ImGui::TableSetColumnIndex(4);
                {
                    uint64_t bits = extract8Bytes(line.data, seg);
                    double dbl = 0.0;
                    std::memcpy(&dbl, &bits, sizeof(double));

                    char dblStr[64];
                    std::snprintf(dblStr, sizeof(dblStr), "%.17g", dbl);
                    ImGui::TextUnformatted(dblStr);
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}