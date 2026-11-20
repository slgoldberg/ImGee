# Zoomable Viewport Extension

`extensions/Viewport/imgui_zoomable_viewport.h` provides a lightweight two-layer zoom container (`ImGui::BeginZoomedViewport` / `ImGui::EndZoomedViewport`) so document viewers, checklists, and briefing panels can keep outer window toolbars and footers sharp at `1.0x` while zooming inner scrollable content smoothly between `50%` and `250%`.

> **Author / Maintainer:** Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))  
> **License:** BSD 3-Clause (see [LICENSE](../../LICENSE))

---

## Why Two-Layer Viewport Zoom?

Calling `ImGui::SetWindowFontScale(zoom)` on a top-level window scales every button, combo box, tab bar, and footer inside that window. At `180%` zoom, toolbar controls grow oversized and squeeze out the content you actually wanted to read.

`BeginZoomedViewport` isolates content zoom inside a dedicated scrollable child region:
* **Crisp `1.0x` Outer Chrome:** Title bars, section pickers, sliders, and status footers outside the viewport stay at native `1.0x` UI metrics.
* **Proportional `Nx` Inner Content:** Text, GFM tables, callouts, and zoom-aware Markdown images inside the viewport scale together from `0.50x` (`50%`) to `2.50x` (`250%`), while `EndZoomedViewport()` restores `1.0x` cleanly before returning.
* **Wheel Consumption Safety:** Holding `Ctrl+MouseWheel` (or `Cmd+MouseWheel` on macOS) inside the hovered viewport steps zoom cleanly and consumes `io.MouseWheel` immediately so inner `Ctrl+Wheel`-aware widgets and parent scrollbars never double-react on the same frame.

---

## Quick Start

Always follow standard `ImGui::BeginChild` / `EndChild` discipline -- call `EndZoomedViewport()` unconditionally after `BeginZoomedViewport(...)`:

```cpp
#include "imgee.h"
// (Or standalone: #include "extensions/Viewport/imgui_zoomable_viewport.h")

static ImGui::ZoomableViewportState s_ZoomState;

// 1. Fixed 1.0x header bar with built-in [-][100%][+][Reset] controls:
ImGui::TextUnformatted("Approach Briefing");
ImGui::SameLine();
ImGui::ZoomViewportControls(s_ZoomState);

// 2. Scrollable Nx-zoomed content region (reserving 28px at the bottom for a 1.0x footer):
if (ImGui::BeginZoomedViewport("##BriefingBody", s_ZoomState, ImVec2(0.0f, -28.0f))) {
    ImGui::MD::TextWrapped(briefingMarkdown);
}
ImGui::EndZoomedViewport();

// 3. Fixed 1.0x footer:
ImGui::TextDisabled("Hold Ctrl+Wheel (or Cmd+Wheel on macOS) over the document to zoom.");
```

> [!TIP]
> **Looking for a Complete Manual Window?**  
> See [`extensions/DocViewer/`](../DocViewer/README.md) (`ImGui::DocumentViewer`), which combines `BeginZoomedViewport` with an automatic Table-of-Contents section jumper, translucent HUD opacity controls, theme switching, and `#heading-slug` anchor navigation out of the box.

---

## API Reference

```cpp
namespace ImGui {
    using ImGuiZoomViewportFlags = int;
    enum ImGuiZoomViewportFlags_ {
        ImGuiZoomViewportFlags_None             = 0,
        ImGuiZoomViewportFlags_DisableCtrlWheel = 1 << 0 // Opt out of Ctrl/Cmd + MouseWheel zoom
    };

    struct ZoomableViewportState {
        float zoom         = 1.0f;  // Active zoom factor (clamped to [min_zoom .. max_zoom])
        float default_zoom = 1.0f;  // Baseline 1.0x target for 1-click Reset
        float last_zoom    = 1.0f;  // Saved custom zoom for 1-click A/B Undo/Restore
        float min_zoom     = 0.50f; // 50% floor
        float max_zoom     = 2.50f; // 250% ceiling
        float step         = 0.10f; // 10% increment per stepper click or Ctrl+Wheel notch

        bool SetZoom(float new_zoom);
        bool StepZoom(int delta_steps);
        bool ToggleResetZoom();     // Swaps between default_zoom and last_zoom (A/B undo memory)
        bool IsModified() const;
        bool CanRestore() const;
    };

    bool BeginZoomedViewport(
        const char* str_id,
        ZoomableViewportState& state,
        const ImVec2& size = ImVec2(0.0f, 0.0f),
        bool border = false,
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None,
        ImGuiZoomViewportFlags zoom_flags = ImGuiZoomViewportFlags_None);

    void EndZoomedViewport();

    // Compact [-][100%][+][Reset] toolbar helper rendered at 1.0x:
    bool ZoomViewportControls(ZoomableViewportState& state, const char* id_suffix = "##zoom_vp");
}
```

### Preprocessor Opt-Out
Define `#define IMGEE_DISABLE_ZOOMABLE_VIEWPORT` before including `imgee.h` to exclude this header.
