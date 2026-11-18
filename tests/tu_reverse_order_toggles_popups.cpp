// ============================================================================
// Reverse Include-Order Verification: Toggles, Popups, Links & StyledText
// included BEFORE Markdown & TimedTooltip
// ============================================================================

#include "widgets/imgui_toggle_buttons.h"
#include "widgets/imgui_popup_menus.h"
#include "widgets/imgui_clickable_links.h"
#include "extensions/Typography/imgui_styled_text.h"
#include "extensions/Markdown/imgui_markdown_ext.h"
#include "extensions/TimedTooltip/imgui_tooltip_ext.h"
#include "imgee_test_common.h"

#if !defined(IMGUI_TIMED_TOOLTIP_TYPOGRAPHY_BRIDGE_DEFINED)
#error "Reverse include order (Typography BEFORE TimedTooltip) failed to activate TimedTooltip <-> Typography bridge"
#endif

namespace ImGeeTests {

void RunReverseOrderTogglesAndPopupsFrame() {
    // Verify ClickableLinks <-> Markdown bidirectional auto-link bridge activated
    IMGEE_TEST_CHECK(ImGui::GetDefaultMarkdownLinkCallback() != nullptr);
    // Verify Typography <-> TimedTooltip reverse-order base-font resolver bridge activated
    IMGEE_TEST_CHECK(ImGui::TimedTooltip::Detail::GetBaseFontResolver() != nullptr);

    static bool s_LandingLights = true;
    static int  s_SurfaceMode   = 0;
    static int  s_PulldownIdx   = 1;
    static const char* const kModes[] = { "Circle", "Spot", "Tower" };
    static const char* const kTips[]  = { "**Circle** view", "**Spot** view", "**Tower** view" };

    (void)ImGui::ToggleButton("Landing Lights", &s_LandingLights,
                              "Lights are **OFF**.", "Lights are **ON**.");
    (void)ImGui::ToggleButtons("Surface##pair", &s_SurfaceMode, "Solid", "Glass");
    (void)ImGui::SelectPulldownMD("##ViewPulldown", &s_PulldownIdx, kModes, 3, kTips,
                                  120.0f, ImGuiPopupAnchor_BelowItem, ImGuiPopupAlign_Left, true);
    ImGui::StyledTextWrapped("Reverse-order <b>StyledTextWrapped</b> via Markdown bridge.");
    ImGui::TimedTooltip::TextMD("Reverse-order **SelectPulldownMD** verified.");
}

} // namespace ImGeeTests
