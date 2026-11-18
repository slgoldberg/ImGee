// ============================================================================
// Standalone Opt-Out & Fail-Fast Isolation Verification
// Verifies that IMGEE_DISABLE_* prevents disabled headers from being pulled in
// either standalone (via widget __has_include) or via imgee.h.
// ============================================================================

#define IMGEE_DISABLE_MARKDOWN
#define IMGEE_DISABLE_KNOBS
#include "widgets/imgui_copy_button.h"
#include "imgee.h"
#include "imgee_test_common.h"

#if defined(IMGUI_MARKDOWN_EXT_H)
#error "IMGEE_DISABLE_MARKDOWN failed to exclude imgui_markdown_ext.h"
#endif

#if defined(IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED)
#error "TimedTooltip::TextMD bridge must not exist when Markdown is disabled"
#endif

#if defined(IMGUI_KNOBS_H)
#error "IMGEE_DISABLE_KNOBS failed to exclude imgui_knobs.h"
#endif

#if !defined(IMGUI_TOOLTIP_EXT_H)
#error "TimedTooltip should remain enabled when only Markdown and Knobs are disabled"
#endif

namespace ImGeeTests {

int VerifyStandaloneOptOuts() {
    return 7;
}

} // namespace ImGeeTests
