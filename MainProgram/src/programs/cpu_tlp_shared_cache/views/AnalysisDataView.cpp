#include "programs/cpu_tlp_shared_cache/views/AnalysisDataView.h"
#include <imgui.h>

void AnalysisDataView::render() {
    // Usar TODO el espacio disponible (la tabla maneja su scroll)
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY |      // scroll vertical con rueda
        ImGuiTableFlags_ScrollX |      // scroll horizontal (por si se angosta)
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##analysis_data_table", 2, flags, ImVec2(avail.x, avail.y))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        // 70% / 30%
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 0.70f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.30f);
        ImGui::TableHeadersRow();

        auto row = [&](const char* name, uint64_t value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%llu", static_cast<unsigned long long>(value));
            };

        row("Number of cache misses:", m_cacheMisses);
        row("Number of invalidations of relevant lines:", m_invalidations);
        row("Number of read/write operations:", m_readWriteOps);
        row("Number of transactions in MESI:", m_transactionsMESI);
        row("Traffic of read/write in PE0:", m_trafficPE[0]);
        row("Traffic of read/write in PE1:", m_trafficPE[1]);
        row("Traffic of read/write in PE2:", m_trafficPE[2]);
        row("Traffic of read/write in PE3:", m_trafficPE[3]);

        ImGui::EndTable();
    }
}
