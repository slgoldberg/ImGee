// ============================================================================
// Reverse Include-Order Verification: Knobs BEFORE TimedTooltip & Markdown
// ============================================================================

#include "widgets/imgui_knobs.h"
#include "extensions/TimedTooltip/imgui_tooltip_ext.h"
#include "extensions/Markdown/imgui_markdown_ext.h"
#include "imgee_test_common.h"

namespace ImGeeTests {

void RunReverseOrderKnobsFrame() {
    static float s_Dimmer = 0.75f;
    static float s_Volume = 0.50f;
    (void)ImGui::DimmerKnob("Flood", &s_Dimmer, 0.05f, 1.0f, 0.75f);
    (void)ImGui::KnobWithReset("Volume", &s_Volume, 0.50f, 0.0f, 1.0f, "%.0f%%", 0.0f, 0.02f, "Reset **volume**");
    ImGui::TimedTooltip::TextMD("Reverse-order **Knob** + **Markdown** tooltip.");
}

} // namespace ImGeeTests
