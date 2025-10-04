#include "programs/cpu_tlp_shared_cache/views/PE3MemView.h"
#include "imgui.h"

void PE3MemView::render() {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextUnformatted("PE3 Memory View");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("Placeholder de memoria para PE3.");
}
