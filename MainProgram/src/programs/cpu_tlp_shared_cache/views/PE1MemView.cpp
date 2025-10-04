#include "programs/cpu_tlp_shared_cache/views/PE1MemView.h"
#include "imgui.h"

void PE1MemView::render() {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextUnformatted("PE1 Memory View");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("Placeholder de memoria para PE1.");
}
