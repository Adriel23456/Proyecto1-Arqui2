#include "programs/cpu_tlp_shared_cache/views/GeneralView.h"
#include "imgui.h"

void GeneralView::render() {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextUnformatted("General View");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("Vista general del sistema TLP con Shared Cache (topología, métricas y estado).");
}
