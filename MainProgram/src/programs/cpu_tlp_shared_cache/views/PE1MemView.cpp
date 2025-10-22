#include "programs/cpu_tlp_shared_cache/views/PE1MemView.h"
#include <imgui.h>

void PE1MemView::render() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("##PE1MemScrollable", ImVec2(avail.x, avail.y), false);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "PE1 L1 Cache Memory");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(1, 5));

    m_table.render("##PE1_Cache_Table");

    ImGui::EndChild();
}

void PE1MemView::setBySetWay(int setIndex, int wayIndex, const std::string& tag,
    const std::array<uint8_t, 32>& data, const std::string& mesi) {
    m_table.setLineBySetWay(setIndex, wayIndex, tag, data, mesi);
}