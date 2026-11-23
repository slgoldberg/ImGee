# Zoomable Viewport Extension

`extensions/Viewport/imgui_zoomable_viewport.h` provides a lightweight two-layer zoom container (`ImGui::BeginZoomedViewport` / `ImGui::EndZoomedViewport`) so document viewers, help browsers, and inspector panels can keep outer window toolbars and footers sharp at `1.0x` while zooming inner scrollable content smoothly between `50%` and `250%`.

> **Author / Maintainer:** Steven L. Goldberg ([@slgoldberg](https://github.com/slgoldberg))  
> **License:** BSD 3-Clause (see [LICENSE](../../LICENSE))

---

## Why Two-Layer Viewport Zoom?

Calling `ImGui::SetWindowFontScale(zoom)` on a top-level window scales every button, combo box, tab bar, and footer inside that window. At `180%` zoom, toolbar controls grow oversized and squeeze out the content you actually wanted to read.

`BeginZoomedViewport` isolates content zoom inside a dedicated scrollable child region:
* **Crisp `1.0x` Outer Chrome:** Title bars, section pickers, sliders, and status footers outside the viewport stay at native `1.0x` UI metrics.
* **Proportional `Nx` Inner Content:** Text, GFM tables, callouts, and zoom-aware Markdown images inside the viewport scale together from `0.50x` (`50%`) to `2.50x` (`250%`), while `EndZoomedViewport()` restores `1.0x` cleanly before returning.
* **Toolbar Controls by Default (Zero OS Screen-Zoom Collisions):** `enable_ctrl_wheel` defaults to **`false`** because macOS Accessibility reserves `Ctrl + Scroll` for OS screen zoom and embedded/plugin host environments do not always forward modifier flags on wheel callbacks. Use `ZoomViewportControls(state)` (or a mouse-wheel slider in your `1.0x` header bar) for reliable universal zoom, and set `state.enable_ctrl_wheel = true` only when a standalone desktop app explicitly wants `Ctrl`/`Cmd` + wheel zoom.

---

## Quick Start

Always follow standard `ImGui::BeginChild` / `EndChild` discipline -- call `EndZoomedViewport()` unconditionally after `BeginZoomedViewport(...)`:

```cpp
#include "imgee.h"
// (Or standalone: #include "extensions/Viewport/imgui_zoomable_viewport.h")

static ImGui::ZoomableViewportState s_ZoomState;

// 1. Fixed 1.0x header bar with built-in [-][100%][+] controls:
ImGui::AlignTextToFramePadding();
ImGui::TextUnformatted("Release Notes");
ImGui::SameLine();
ImGui::ZoomViewportControls(s_ZoomState);

// 2. Scrollable Nx-zoomed content region (reserving 24px at the bottom for a 1.0x footer):
if (ImGui::BeginZoomedViewport("##NotesBody", s_ZoomState, ImVec2(0.0f, -24.0f))) {
    ImGui::MD::TextWrapped(releaseNotesMarkdown);
}
ImGui::EndZoomedViewport();

// 3. Fixed 1.0x footer:
ImGui::TextDisabled("Click 100%% in the header bar to toggle between default and custom zoom.");
```

> [!TIP]
> **Looking for a Complete Manual Window?**  
> See [`extensions/DocViewer/`](../DocViewer/README.md) (`ImGui::DocumentViewer`), which combines `BeginZoomedViewport` with an automatic Table-of-Contents section jumper, 1-click scroll-to-top / return-to-spot (`↺`/`↻`), translucent opacity controls, theme switching, and `#heading-slug` anchor navigation out of the box.

---

## API Reference

```cpp
namespace ImGui {
    struct ZoomableViewportState {
        float  zoom              = 1.0f;                 // Active zoom factor (clamped to [min_zoom .. max_zoom])
        float  default_zoom      = 1.0f;                 // Baseline 1.0x target for ResetZoom()
        float  last_zoom         = 1.0f;                 // Saved custom zoom for 1-click A/B ToggleResetZoom()
        float  min_zoom          = 0.50f;                // 50% floor
        float  max_zoom          = 2.50f;                // 250% ceiling
        float  wheel_step        = 0.05f;                // 5% per wheel notch (when enable_ctrl_wheel == true)
        float  button_step       = 0.10f;                // 10% per [-]/[+] button press
        bool   enable_ctrl_wheel = false;                // Opt-in Ctrl/Cmd + wheel zoom (false by default)
        bool   border            = false;                // Draw outer border around the content child
        ImVec2 content_padding   = ImVec2(-1.0f, -1.0f); // Custom inner padding (-1 = inherit WindowPadding)

        bool SetZoom(float target_zoom);
        bool StepZoom(float delta);
        bool ResetZoom();
        bool ToggleResetZoom();     // Swaps between default_zoom and last_zoom (A/B undo memory)
        int  GetZoomPercent() const;
    };

    bool BeginZoomedViewport(
        const char* str_id,
        ZoomableViewportState& state,
        const ImVec2& size = ImVec2(0.0f, 0.0f),
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar);

    void EndZoomedViewport(ZoomableViewportState& state);
    void EndZoomedViewport();

    // Compact [-][100%][+] toolbar cluster rendered at 1.0x:
    // - [-] and [+] enable ImGui ButtonRepeat while held down
    // - [ 100% ] clicks toggle default_zoom <-> last_zoom (A/B undo) and supports mouse-wheel zoom on hover
    bool ZoomViewportControls(ZoomableViewportState& state, const char* id_suffix = "##imgee_zoom_ctrl");
}
```

### Preprocessor Opt-Out
Define `#define IMGEE_DISABLE_ZOOMABLE_VIEWPORT` before including `imgee.h` to exclude this header.
