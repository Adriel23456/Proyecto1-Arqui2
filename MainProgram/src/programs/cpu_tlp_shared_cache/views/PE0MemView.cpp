#include "programs/cpu_tlp_shared_cache/views/PE0MemView.h"
#include "imgui.h"

void PE0MemView::render() {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextUnformatted("PE0 Memory View");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("Placeholder de memoria para PE0.");
}
