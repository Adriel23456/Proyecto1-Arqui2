#include "programs/cpu_tlp_shared_cache/views/RAMView.h"
#include "imgui.h"

void RAMView::render() {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextUnformatted("RAM View");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("Placeholder de memoria RAM compartida / caché central.");
}
