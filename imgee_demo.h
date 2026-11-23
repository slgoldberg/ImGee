// ============================================================================
// ImGee: Opt-In Interactive Showcase Window (imgee_demo.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE)
//
// Optional, 100% self-contained single-header interactive playground mirroring
// stock ImGui::ShowDemoWindow(bool* p_open). Not included by imgee.h unless
// explicitly requested (#include "imgee_demo.h" or #define IMGEE_INCLUDE_DEMO
// before #include "imgee.h") -- zero compile-time or binary cost otherwise.
//
// Entry Points:
//   ImGui::ShowImGeeDemoWindow(bool* p_open = nullptr);  // Own top-level window
//   ImGui::ShowImGeeDemoContent(const char* str_id = "##ImGeeDemo"); // Embedded
//
// Zero X-Plane / ImgWindow dependencies -- compiles standalone against pure
// upstream Dear ImGui (v1.84+ through v1.92+) and honors all IMGEE_DISABLE_*
// feature toggles cleanly.
// ============================================================================

#pragma once

#ifndef IMGEE_DEMO_H
#define IMGEE_DEMO_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#ifndef IMGEE_H
#include "imgee.h"
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace ImGeeDemoDetail {

struct DemoContextKey {
    ImGuiContext* ctx = nullptr;
    ImGuiID       id  = 0;
    bool operator<(const DemoContextKey& rhs) const {
        return (ctx != rhs.ctx) ? (ctx < rhs.ctx) : (id < rhs.id);
    }
};

inline bool BeginChildCompat(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags = 0) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19100)
    return ImGui::BeginChild(str_id, size, border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None, flags);
#elif defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
    return ImGui::BeginChild(str_id, size, border ? ImGuiChildFlags_Border : ImGuiChildFlags_None, flags);
#else
    return ImGui::BeginChild(str_id, size, border, flags);
#endif
}

inline int& GetForcedTabIndex() {
    static int s_ForcedTab = -1;
    return s_ForcedTab;
}

inline const char* DefaultMarkdownPlaygroundText() {
    return
        "# ImGee Markdown & GFM Table Playground\n\n"
        "Edit this Markdown live to test **bold**, *italic*, `inline code`, "
        "<badge=success>LIVE</badge> <pill=info>v1.4.0</pill> <kbd>Ctrl+F</kbd>, "
        "and procedural icons like :plane: and :gear:.\n\n"
        "> [!TIP]\n"
        "> Hover any table header or resize border below to see `v1.3.0` delayed "
        "table tooltip overrides (`tooltips=headers-only`, `grip-tooltip`, and `\\n` escapes).\n\n"
        "| Waypoint | Altitude | Speed | Status |\n"
        "| :--- | ---: | ---: | :---: |\n"
        "| **KLAX** | FL180 | 280 kt | <badge=success>ACTIVE</badge> |\n"
        "| **KSFO** | FL240 | 310 kt | <badge=info>NEXT</badge> |\n"
        "| **KSEA** | FL320 | 340 kt | <badge=warning>PLAN</badge> |\n"
        "<!-- table-preset: width=500 tip0=\"**Waypoint**\\nICAO fix identifier\" tip1=\"**Target Altitude**\\nFeet MSL or Flight Level\" tip3=off grip-tooltip=\"Drag to resize table width\\nDouble-click to toggle baseline\" -->\n\n"
        "```cpp\n"
        "// One-line interactive ImGee showcase:\n"
        "#include \"imgee_demo.h\"\n"
        "ImGui::ShowImGeeDemoWindow(&show_demo);\n"
        "```\n";
}

inline const char* DefaultDocViewerManualText() {
    return
        "# ImGee Quick Reference Manual\n\n"
        "Welcome to the embedded **DocumentViewer** showcase! Try **Zoom** (`50%..250%`), "
        "jump between sections via **Section:**, press **Ctrl+F** (or click the magnifying glass) "
        "for live **Find in Doc**, or follow an internal link like [Jump to UX Tenets](#ux-tenets).\n\n"
        "> [!NOTE]\n"
        "> Both header rows, the collapsible Find bar, and the footer stay crisp at `1.0x` "
        "while prose, code blocks, callouts, and tables zoom smoothly inside `BeginZoomedViewport`.\n\n"
        "## Live Search & Navigation\n\n"
        "In-document live search (`v1.2.0`) highlights matches cleanly behind text glyphs across "
        "headings, prose, GFM tables, fenced code blocks, and inline `<color=cyan>StyledText</color>` runs.\n\n"
        "* Press **Enter** / **Shift+Enter** to step continuously through matches.\n"
        "* Roll the mouse wheel **down** over `[ 1 / N ]` or the up/down carets to advance forward.\n"
        "* Press **Esc** or click outside to release input focus cleanly back to your host app.\n\n"
        "## Hover-Latching Menu Strips\n\n"
        "`BeginMenuStrip` / `BeginStripMenu` (`v1.3.0`) provides classic `Begin`/`End` scope "
        "APIs with zero C++ callbacks, `(ImGuiContext*, ImGuiID)` isolation, and frame-exact "
        "hover-switching across sibling menu triggers.\n\n"
        "```cpp\n"
        "if (ImGui::BeginMenuStrip(\"##Bar\")) {\n"
        "    if (ImGui::BeginStripMenu(\"View\", \"Switch perspective\")) {\n"
        "        ImGui::MenuItem(\"Cockpit\");\n"
        "        ImGui::EndStripMenu();\n"
        "    }\n"
        "    ImGui::EndMenuStrip();\n"
        "}\n"
        "```\n\n"
        "## UX Tenets\n\n"
        "ImGee enforces six human-factors tenets across every widget and extension:\n\n"
        "1. **Active Cursors:** Never leave interactive controls on an inert OS `Arrow`.\n"
        "2. **Natural List Order:** Wheel **down** always advances forward (`+1`).\n"
        "3. **Lens UI Focus Latching:** Wheel stepping inside open pulldowns latches tooltips to `*current_item` with `> 6px` hysteresis.\n"
        "4. **Clamping vs. Wrapping:** Quantitative scalars clamp; categorical presets wrap.\n"
        "5. **Scroll Pass-Through:** Closed inline dropdowns never hijack parent scrolling.\n"
        "6. **Timed Tooltips:** Stationary, dwell-delayed tooltips with strictly silent fallback when `TimedTooltip` is omitted.\n";
}

struct ImGeeDemoState {
    int        last_frame_seen         = -1;
    bool       initialized             = false;
    bool       has_host_style_snapshot = false;
    ImGuiStyle host_style_snapshot;

    // Shared theme & UX settings
    int   theme_index            = 0; // 0=Charcoal, 1=Rusty, 2=Dark, 3=Classic, 4=Paper Light
    bool  wheel_auto_defeat      = true;
    int   tooltip_delay_frames   = 18;
    bool  dynamic_cursors        = true;
    std::string last_action_note = "Ready -- hover controls or switch tabs to explore ImGee.";

    // Tab 2: Sliders, Toggles & Knobs
    float slider_gamma      = 2.20f;
    float slider_opacity    = 0.85f;
    int   slider_steps      = 12;
    int   combo_preset      = 0;
    bool  toggle_live       = true;
    bool  toggle_hud        = false;
    int   segmented_mode    = 1;
    float knob_gain         = 0.72f;
    float knob_trim         = 0.50f;
    float dimmer_panel      = 0.80f;

    // Tab 3: Menus, Pulldowns & Links
    int   pulldown_cam      = 0;
    bool  chk_link_val      = true;
    int   radio_link_val    = 1;
    int   strip_action_cnt  = 0;
    char  copy_input_buf[128] = {};

    // Tab 4: Tooltips, Badges & Spinners
    bool  timer_running     = true;
    float timer_elapsed     = 2.5f;
    float timer_duration    = 10.0f;

    // Tab 5: Vector Icons
    char   icon_filter[64]  = {};
    float  icon_size        = 18.0f;
    ImVec4 icon_tint        = ImVec4(0.92f, 0.94f, 0.98f, 1.0f);
    bool   icon_use_tint    = false;
    std::string last_icon_copied;

    // Tab 6: Markdown & GFM Tables
    char        md_editor_buf[4096] = {};
    bool        md_show_source      = true;
    std::string md_last_table_state = "(interact with the table header or borders to inspect live state)";

    // Tab 7: DocumentViewer
#if defined(IMGUI_DOC_VIEWER_H) && !defined(IMGEE_DISABLE_DOC_VIEWER) && !defined(CONTRIB_DISABLE_DOC_VIEWER)
    ImGui::DocumentViewerState doc_state;
#endif

    // Tab 8: File Dialog
    std::string picker_file;
    bool        open_dlg_visible = false;
    bool        save_dlg_visible = false;
    std::string chosen_open_path;
    std::string chosen_save_path;

    void ResetDemoValues(bool preserve_host_snapshot) {
        ImGuiStyle saved_host = host_style_snapshot;
        bool had_host = has_host_style_snapshot;
        int cur_frame = last_frame_seen;

        *this = ImGeeDemoState{};
        last_frame_seen = cur_frame;
        if (preserve_host_snapshot && had_host) {
            host_style_snapshot     = saved_host;
            has_host_style_snapshot = true;
        }
        std::snprintf(copy_input_buf, sizeof(copy_input_buf), "%s", "KSFO DCT RBL J1 KSEA");
        std::snprintf(md_editor_buf, sizeof(md_editor_buf), "%s", DefaultMarkdownPlaygroundText());
#if defined(IMGUI_DOC_VIEWER_H) && !defined(IMGEE_DISABLE_DOC_VIEWER) && !defined(CONTRIB_DISABLE_DOC_VIEWER)
        doc_state.theme_index = theme_index;
#endif
        initialized = true;
    }
};

inline std::map<DemoContextKey, ImGeeDemoState>& GetDemoStateMap() {
    static std::map<DemoContextKey, ImGeeDemoState> s_States;
    return s_States;
}

inline ImGeeDemoState& GetOrCreateDemoState(ImGuiID demo_id) {
    DemoContextKey key{ ImGui::GetCurrentContext(), demo_id };
    ImGeeDemoState& st = GetDemoStateMap()[key];
    const int cur_frame = ImGui::GetFrameCount();
    if (st.last_frame_seen > cur_frame || !st.initialized) {
        st.ResetDemoValues(false);
    }
    st.last_frame_seen = cur_frame;
    if (!st.has_host_style_snapshot) {
        st.host_style_snapshot     = ImGui::GetStyle();
        st.has_host_style_snapshot = true;
    }
    return st;
}

inline void ApplyDemoTheme(ImGeeDemoState& st, int idx) {
    if (idx < 0) idx = 0;
    if (idx > 4) idx = 4;
    st.theme_index = idx;
#if defined(IMGUI_DOC_VIEWER_H) && !defined(IMGEE_DISABLE_DOC_VIEWER) && !defined(CONTRIB_DISABLE_DOC_VIEWER)
    st.doc_state.theme_index = idx;
#endif
    switch (idx) {
        case 0:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::StyleColorsCharcoal();
#else
            ImGui::StyleColorsDark();
#endif
            break;
        case 1:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::StyleColorsRusty();
#else
            ImGui::StyleColorsDark();
#endif
            break;
        case 2:
            ImGui::StyleColorsDark();
            break;
        case 3:
            ImGui::StyleColorsClassic();
            break;
        case 4:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::StyleColorsPaperLight();
#else
            ImGui::StyleColorsLight();
#endif
            break;
        default:
            break;
    }
}

inline void RestoreHostStyle(ImGeeDemoState& st) {
    if (st.has_host_style_snapshot) {
        ImGui::GetStyle() = st.host_style_snapshot;
        st.last_action_note = "Restored original host ImGuiStyle snapshot.";
    }
}

inline void RenderDisabledBanner(const char* feature_name, const char* macro_name) {
    ImGui::TextDisabled("%s is disabled in this build (%s).", feature_name, macro_name);
}

inline void RenderTabOverviewAndThemes(ImGeeDemoState& st) {
    ImGui::TextWrapped(
        "ImGee v%s (Dear ImGui %s) -- header-only ergonomic widgets, stationary TimedTooltips, "
        "CommonMark + sortable GFM tables, procedural vector icons, curated themes, and a turnkey DocumentViewer.",
        IMGEE_VERSION, IMGUI_VERSION);
    ImGui::Spacing();

    // 1. Live Curated Theme Palette Swatches
    ImGui::TextUnformatted("Curated Style Palettes");
    ImGui::Separator();
    static const char* const kThemeLabels[5] = {
        "Charcoal", "Rusty", "ImGui Dark", "ImGui Classic",
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
        "Paper Light"
#else
        "ImGui Light"
#endif
    };
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine();
        const bool is_cur = (st.theme_index == i);
        if (is_cur) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(kThemeLabels[i])) {
            ApplyDemoTheme(st, i);
            st.last_action_note = std::string("Applied theme: ") + kThemeLabels[i];
        }
        if (ImGui::IsItemHovered() && st.dynamic_cursors) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (is_cur) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Host Style")) {
        RestoreHostStyle(st);
    }
    if (ImGui::IsItemHovered() && st.dynamic_cursors) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // Active palette color swatch strip
    {
        const ImGuiCol swatches[6] = {
            ImGuiCol_WindowBg, ImGuiCol_FrameBg, ImGuiCol_Button,
            ImGuiCol_ButtonActive, ImGuiCol_HeaderHovered, ImGuiCol_Text
        };
        const char* const swatchNames[6] = { "WindowBg", "FrameBg", "Button", "Active", "Header", "Text" };
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float sw_w = 22.0f;
        const float sw_h = 16.0f;
        for (int s = 0; s < 6; ++s) {
            if (s > 0) ImGui::SameLine(0.0f, 10.0f);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(sw_w, sw_h));
            dl->AddRectFilled(p, ImVec2(p.x + sw_w, p.y + sw_h), ImGui::GetColorU32(swatches[s]), 3.0f);
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::TextDisabled("%s", swatchNames[s]);
        }
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Inline StyledText & Semantic Typography");
    ImGui::Separator();
#if defined(IMGUI_STYLED_TEXT_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
    ImGui::StyledTextUnformatted(
        "Inline <b>bold</b>, <i>italic</i>, <mono>monospace</mono>, "
        "<color=cyan>tinted prose</color>, <badge=success>OK</badge>, "
        "<pill=info>v1.4.0</pill>, and <kbd>Ctrl+F</kbd> with baseline alignment.");
#else
    RenderDisabledBanner("Typography / StyledText", "IMGEE_DISABLE_TYPOGRAPHY");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("UX Tenets Runtime Controls");
    ImGui::Separator();
#if defined(IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED)
    if (ImGui::Checkbox("Mouse-Wheel Auto-Defeat inside Scrollable Viewports", &st.wheel_auto_defeat)) {
        ImGui::SetMouseWheelAutoDefeat(st.wheel_auto_defeat);
    }
#endif
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP)
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderInt("Demo TimedTooltip Dwell Delay (frames)", &st.tooltip_delay_frames, 4, 60);
#endif
    ImGui::Checkbox("Enable Dynamic OS Cursor Affordances (Hand / ResizeNS / ResizeEW)", &st.dynamic_cursors);
}

inline void RenderTabSlidersTogglesKnobs(ImGeeDemoState& st) {
    (void)st;
    ImGui::TextUnformatted("Mouse-Wheel Sliders & 1-Click Reset-with-Undo");
    ImGui::Separator();
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS)
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderWithReset("Display Gamma", &st.slider_gamma, 2.20f, 1.00f, 3.00f, "%.2f");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::PercentSliderWithReset("HUD Opacity", &st.slider_opacity, 0.85f, 0.10f, 1.00f, "%.0f%%");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderWithReset("Grid Divisions", &st.slider_steps, 12, 2, 32);

    static const char* const kPresets[] = { "Balanced", "High Contrast", "Night Vision", "Studio Warm" };
    ImGui::SetNextItemWidth(220.0f);
    ImGui::MouseWheelCombo("Preset (Wrap)", &st.combo_preset, kPresets, 4, -1, true);
#else
    RenderDisabledBanner("Mouse-Wheel Controls", "IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("Animated Toggle Switches & Segmented Selectors");
    ImGui::Separator();
#if defined(IMGUI_TOGGLE_BUTTONS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_TOGGLE_BUTTONS)
    ImGui::ToggleButton("Live Preview", &st.toggle_live,
                        "Preview is **OFF**.", "Preview is **ON**.");
    ImGui::SameLine(0.0f, 18.0f);
    ImGui::ToggleButton("Transparent HUD", &st.toggle_hud,
                        "Opaque backdrop.", "Translucent HUD active.");
    ImGui::Spacing();
    ImGui::ToggleButtons("Layout Density", &st.segmented_mode, "Compact", "Expanded");
#else
    RenderDisabledBanner("Toggle Buttons", "IMGEE_DISABLE_TOGGLE_BUTTONS");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("270-Degree Rotary Knobs & Two-Step OFF Dimmer");
    ImGui::Separator();
#if defined(IMGUI_KNOBS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_KNOBS)
    ImGui::KnobWithReset("Monitor Gain", &st.knob_gain, 0.72f, 0.0f, 1.0f, "%.0f%%");
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::Knob("Input Trim", &st.knob_trim, 0.0f, 1.0f, "%.2f", 22.0f, 0.01f,
                ImGuiKnobFlags_ShowTicks | ImGuiKnobFlags_ValueBelow);
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::DimmerKnob("Panel Dimmer", &st.dimmer_panel, 0.05f, 1.0f, 0.80f, "%.0f%%");
#else
    RenderDisabledBanner("Rotary Knobs", "IMGEE_DISABLE_KNOBS");
#endif
}

inline void RenderTabMenusPulldownsLinks(ImGeeDemoState& st) {
    (void)st;
    ImGui::TextUnformatted("SelectPulldown (Scroll Pass-Through Closed, Lens UI Wheel Open)");
    ImGui::Separator();
#if defined(IMGUI_POPUP_MENUS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_POPUP_MENUS)
    static const char* const kCamModes[4] = { "Cockpit Pilot", "External Circle", "Control Tower", "Wing Spot" };
    static const char* const kCamTips[4]  = {
        "**Cockpit Pilot**\n3-D flight deck camera.",
        "**External Circle**\nExterior orbit tracking view.",
        "**Control Tower**\nNearest airport tower camera.",
        "**Wing Spot**\nFuselage-mounted wing camera."
    };
    ImGui::SelectPulldownMD("##demo_pulldown", &st.pulldown_cam, kCamModes, 4, kCamTips, 200.0f,
                            ImGuiPopupAnchor_BelowItem, ImGuiPopupAlign_Left, true);
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::TextDisabled("Active: %s", kCamModes[st.pulldown_cam]);

    ImGui::Spacing();
    ImGui::TextUnformatted("Hover-Latching Menu Strip (v1.3.0 -- Click Once, Glide Across Siblings)");
    ImGui::Separator();
    ImGui::MenuStripConfig stripCfg;
    stripCfg.anchor         = ImGuiPopupAnchor_AboveItem;
    stripCfg.align          = ImGuiPopupAlign_Left;
    stripCfg.dynamicCursors = st.dynamic_cursors;

    if (ImGui::BeginMenuStrip("##demo_bottom_strip", &stripCfg)) {
        if (ImGui::BeginStripMenu("View", "**View Modes**\nClick to open, then glide right.")) {
            if (ImGui::MenuItem("Cockpit")) { st.strip_action_cnt++; st.last_action_note = "Strip -> View / Cockpit"; }
            if (ImGui::MenuItem("Chase"))   { st.strip_action_cnt++; st.last_action_note = "Strip -> View / Chase"; }
            if (ImGui::MenuItem("Tower"))   { st.strip_action_cnt++; st.last_action_note = "Strip -> View / Tower"; }
            ImGui::EndStripMenu();
        }
        if (ImGui::BeginStripMenu("Tracking", "**Tracking Targets**\nSwitch active target.")) {
            if (ImGui::MenuItem("User Aircraft")) { st.strip_action_cnt++; st.last_action_note = "Strip -> Tracking / User"; }
            if (ImGui::MenuItem("Nearest AI"))    { st.strip_action_cnt++; st.last_action_note = "Strip -> Tracking / AI"; }
            ImGui::EndStripMenu();
        }
        if (ImGui::BeginStripMenu("Quick Actions", "**Quick Commands**\nOne-click tools.")) {
            if (ImGui::MenuItem("Reset Camera")) { st.strip_action_cnt++; st.last_action_note = "Strip -> Reset Camera"; }
            if (ImGui::MenuItem("Center View"))  { st.strip_action_cnt++; st.last_action_note = "Strip -> Center View"; }
            ImGui::EndStripMenu();
        }
        if (ImGui::BeginStripMenu("Locked", "Disabled trigger example.", false)) {
            ImGui::MenuItem("Unreachable");
            ImGui::EndStripMenu();
        }
        ImGui::EndMenuStrip();
    }
    const bool strip_open = ImGui::IsMenuStripOpen("##demo_bottom_strip");
    const ImVec2 pop_sz   = ImGui::GetMenuStripLastPopupSize("##demo_bottom_strip");
    ImGui::TextDisabled("Strip latched: %s  |  Last popup size: %.0fx%.0f px  |  Actions triggered: %d",
                        strip_open ? "YES" : "no", pop_sz.x, pop_sz.y, st.strip_action_cnt);
#else
    RenderDisabledBanner("Popup Menus & Menu Strip", "IMGEE_DISABLE_POPUP_MENUS");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("Clickable Links & Clipboard Copy Chasers");
    ImGui::Separator();
#if defined(IMGUI_CLICKABLE_LINKS_READY) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_CLICKABLE_LINKS)
    if (ImGui::ButtonLink("Execute Action")) {
        st.last_action_note = "ButtonLink clicked (dismisses any active tooltip).";
    }
    ImGui::SameLine(0.0f, 14.0f);
    ImGui::TextLinkUnformatted("(i) Click or hover for info");
#if defined(IMGUI_TOOLTIP_EXT_H) && defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(IMGEE_DISABLE_MARKDOWN)
    ImGui::TimedTooltip::TextMD("**Standalone Info Link**\nClicking an ignored `TextLink` toggles its `TimedTooltip` ON and OFF immediately.");
#endif
    ImGui::CheckboxLink("Enable Autothrottle", &st.chk_link_val);
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::RadioButtonLink("NAV 1", &st.radio_link_val, 0);
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::RadioButtonLink("NAV 2", &st.radio_link_val, 1);
#else
    RenderDisabledBanner("Clickable Links", "IMGEE_DISABLE_CLICKABLE_LINKS");
#endif

#if defined(IMGUI_COPY_BUTTON_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_COPY_BUTTON)
    ImGui::Spacing();
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("Route", st.copy_input_buf, sizeof(st.copy_input_buf));
    ImGui::SameLine();
    ImGui::CopyLastItemButton("##demo_copy_route");
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::CopyButton("##demo_copy_ver", "ImGee v" IMGEE_VERSION);
#else
    RenderDisabledBanner("Copy Buttons", "IMGEE_DISABLE_COPY_BUTTON");
#endif
}

inline void RenderTabTooltipsBadgesSpinners(ImGeeDemoState& st) {
    ImGui::TextUnformatted("Stationary TimedTooltips (Anti-Distraction Dwell)");
    ImGui::Separator();
    ImGui::Button("Hover for Rich Markdown TimedTooltip");
    if (ImGui::IsItemHovered() && st.dynamic_cursors) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
#if defined(IMGUI_TOOLTIP_EXT_H) && defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(IMGEE_DISABLE_MARKDOWN)
    {
        ImGui::TimedTooltip::TooltipConfig demoTipCfg = ImGui::TimedTooltip::GetConfig();
        demoTipCfg.hover_delay_cycles = st.tooltip_delay_frames;
        ImGui::TimedTooltip::TextMD(&demoTipCfg,
            "**Stationary TimedTooltip**\n\n"
            "Waits for a brief dwell (%d frames) before appearing, anchors cleanly without cursor-chasing jitter, "
            "clamps inside viewport bounds, and supports <badge=info>Markdown</badge> formatting.",
            st.tooltip_delay_frames);
    }
#elif defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP)
    {
        ImGui::TimedTooltip::TooltipConfig demoTipCfg = ImGui::TimedTooltip::GetConfig();
        demoTipCfg.hover_delay_cycles = st.tooltip_delay_frames;
        ImGui::TimedTooltip::Text(&demoTipCfg, "Stationary TimedTooltip (%d-frame dwell, plain text mode).", st.tooltip_delay_frames);
    }
#else
    ImGui::SameLine();
    RenderDisabledBanner("TimedTooltip", "IMGEE_DISABLE_TIMED_TOOLTIP");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("Status Badges, Pills & Keycaps");
    ImGui::Separator();
#if defined(IMGUI_BADGES_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_BADGES)
    ImGui::Badge("DEFAULT", ImGui::ImGuiBadgeVariant_Default);
    ImGui::SameLine();
    ImGui::Badge("PRIMARY", ImGui::ImGuiBadgeVariant_Primary);
    ImGui::SameLine();
    ImGui::Badge("SUCCESS", ImGui::ImGuiBadgeVariant_Success);
    ImGui::SameLine();
    ImGui::Badge("WARNING", ImGui::ImGuiBadgeVariant_Warning);
    ImGui::SameLine();
    ImGui::Badge("DANGER",  ImGui::ImGuiBadgeVariant_Danger);

    ImGui::Pill("v" IMGEE_VERSION, ImGui::ImGuiBadgeVariant_Primary);
    ImGui::SameLine();
    ImGui::PillNamed("check", "SYNCED", ImVec4(0.15f, 0.68f, 0.35f, 1.0f));
    ImGui::SameLine();
    ImGui::PillNamed("plane", "AIRBORNE", ImVec4(0.20f, 0.52f, 0.90f, 1.0f));

    ImGui::TextUnformatted("Shortcuts:");
    ImGui::SameLine();
    ImGui::Kbd("Ctrl+F");
    ImGui::SameLine();
    ImGui::Kbd("Shift+Enter");
    ImGui::SameLine();
    ImGui::Kbd("Esc");
#else
    RenderDisabledBanner("Badges, Pills & Kbd", "IMGEE_DISABLE_BADGES");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("Procedural Spinners & Radial Countdown Dials");
    ImGui::Separator();
#if defined(IMGUI_SPINNERS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_SPINNERS)
    if (st.timer_running) {
        st.timer_elapsed += ImGui::GetIO().DeltaTime;
        if (st.timer_elapsed > st.timer_duration) {
            st.timer_elapsed = 0.0f;
        }
    }
    const float remaining_sec  = (std::max)(0.0f, st.timer_duration - st.timer_elapsed);
    const float remaining_frac = (st.timer_duration > 0.0f) ? (remaining_sec / st.timer_duration) : 0.0f;

    ImGui::Spinner("Syncing navdata...");
    ImGui::SameLine(0.0f, 20.0f);
    ImGui::RadialTimer("Approach Leg", remaining_frac);
    ImGui::SameLine(0.0f, 20.0f);
    ImGui::CountdownTimer("Auto-Dismiss", remaining_sec, st.timer_duration);

    if (ImGui::Button(st.timer_running ? "Pause Timer" : "Resume Timer")) {
        st.timer_running = !st.timer_running;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Timer")) {
        st.timer_elapsed = 0.0f;
    }
#else
    RenderDisabledBanner("Spinners & Dials", "IMGEE_DISABLE_SPINNERS");
#endif
}

inline void RenderTabVectorIcons(ImGeeDemoState& st) {
#if defined(CONTRIB_ICONS_AVAILABLE) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_ICONS)
    static const char* const kAllIcons[] = {
        "undo", "redo", "external-link",
        "caret-up", "caret-down", "caret-left", "caret-right",
        "check", "cross", "plus", "minus",
        "gear", "info", "lightbulb", "important",
        "warning", "caution", "star",
        "search", "plane", "circle", "square",
        "ouroboros", "ouroboros-alt"
    };
    const size_t kIconCount = sizeof(kAllIcons) / sizeof(kAllIcons[0]);

    ImGui::SetNextItemWidth(170.0f);
    ImGui::InputTextWithHint("##icon_filter", "Filter icons...", st.icon_filter, sizeof(st.icon_filter));
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::SetNextItemWidth(110.0f);
    ImGui::SliderFloat("Size", &st.icon_size, 12.0f, 36.0f, "%.0f px");
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::Checkbox("Custom Tint", &st.icon_use_tint);
    if (st.icon_use_tint) {
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::ColorEdit4("##icon_tint", &st.icon_tint.x, ImGuiColorEditFlags_NoInputs);
    }

    ImGui::Spacing();
    ImGui::IconButton("##demo_icon_btn_plus", "plus", ImVec2(0.0f, 0.0f), "IconButton example (`plus`)");
    ImGui::SameLine();
    ImGui::ButtonIcon("Settings##demo_btn_icon", "gear", ImVec2(0.0f, 0.0f), false, "ButtonIcon example (`gear`)");
    ImGui::SameLine();
    ImGui::ButtonIcon("Flight Plan##demo_btn_plane", "plane", ImVec2(0.0f, 0.0f), false, "ButtonIcon example (`plane`)");
    if (!st.last_icon_copied.empty()) {
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("Copied: %s", st.last_icon_copied.c_str());
    }

    ImGui::Separator();

    // Case-insensitive substring filter
    std::string filt(st.icon_filter);
    for (size_t i = 0; i < filt.size(); ++i) {
        filt[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(filt[i])));
    }

    const float cell_w = 128.0f;
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const int cols = (std::max)(1, static_cast<int>(avail_w / cell_w));
    int shown = 0;
    for (size_t i = 0; i < kIconCount; ++i) {
        const char* nm = kAllIcons[i];
        if (!filt.empty() && std::strstr(nm, filt.c_str()) == nullptr) {
            continue;
        }
        if (shown > 0 && (shown % cols) != 0) {
            ImGui::SameLine(0.0f, 8.0f);
        }
        ImGui::PushID(static_cast<int>(i));
        const ImVec2 btn_sz(cell_w - 8.0f, (std::max)(34.0f, st.icon_size + 14.0f));
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        if (ImGui::Button("##tile", btn_sz)) {
            std::string snippet = std::string(":") + nm + ":";
            ImGui::SetClipboardText(snippet.c_str());
            st.last_icon_copied = snippet;
            st.last_action_note = std::string("Copied icon shortcode ") + snippet;
        }
        if (ImGui::IsItemHovered() && st.dynamic_cursors) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 icon_col = st.icon_use_tint ? ImGui::GetColorU32(st.icon_tint) : ImGui::GetColorU32(ImGuiCol_Text);
        const ImVec2 icon_pos(p0.x + 8.0f, p0.y + (btn_sz.y - st.icon_size) * 0.5f);
        ImGui::DrawVectorIcon(dl, icon_pos, st.icon_size, icon_col, nm);
        const float text_x = p0.x + 14.0f + st.icon_size;
        const float text_y = p0.y + (btn_sz.y - ImGui::GetTextLineHeight()) * 0.5f;
        const ImVec4 clip(text_x, p0.y, p0.x + btn_sz.x - 4.0f, p0.y + btn_sz.y);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(text_x, text_y),
                    ImGui::GetColorU32(ImGuiCol_Text), nm, nullptr, 0.0f, &clip);
        ImGui::PopID();
        ++shown;
    }
#else
    (void)st;
    RenderDisabledBanner("Procedural Vector Icons", "IMGEE_DISABLE_ICONS");
#endif
}

inline void RenderTabMarkdownAndTables(ImGeeDemoState& st) {
#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
    ImGui::Checkbox("Show Editable Source", &st.md_show_source);
    ImGui::SameLine(0.0f, 12.0f);
    if (ImGui::Button("Reset Sample Markdown")) {
        std::snprintf(st.md_editor_buf, sizeof(st.md_editor_buf), "%s", DefaultMarkdownPlaygroundText());
    }
    if (ImGui::IsItemHovered() && st.dynamic_cursors) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::TextDisabled("Table State: %s", st.md_last_table_state.c_str());

    const float avail_h = (std::max)(180.0f, ImGui::GetContentRegionAvail().y - 4.0f);
    if (st.md_show_source) {
        const float half_w = (std::max)(160.0f, ImGui::GetContentRegionAvail().x * 0.44f);
        ImGui::InputTextMultiline("##md_src", st.md_editor_buf, sizeof(st.md_editor_buf), ImVec2(half_w, avail_h));
        ImGui::SameLine(0.0f, 8.0f);
    }

    if (BeginChildCompat("##md_preview_pane", ImVec2(0.0f, avail_h), true)) {
        ImGui::MarkdownConfig md_cfg;
        md_cfg.userData = &st;
        md_cfg.tableStateCallback = [](const ImGui::MarkdownTableState& tblState, void* ud) {
            ImGeeDemoState* dst = static_cast<ImGeeDemoState*>(ud);
            if (dst && tblState.isCommitted) {
                char buf[160];
                std::snprintf(buf, sizeof(buf), "width=%.0f sort=%s dir=%d",
                              tblState.totalWidth,
                              tblState.sortColumnName.empty() ? "none" : tblState.sortColumnName.c_str(),
                              tblState.sortDirection);
                dst->md_last_table_state = buf;
            }
        };
        ImGui::MarkdownExt(st.md_editor_buf, std::strlen(st.md_editor_buf), &md_cfg, true);
    }
    ImGui::EndChild();
#else
    (void)st;
    RenderDisabledBanner("Markdown & GFM Tables", "IMGEE_DISABLE_MARKDOWN");
#endif
}

inline void RenderTabDocumentViewer(ImGeeDemoState& st) {
#if defined(IMGUI_DOC_VIEWER_H) && !defined(IMGEE_DISABLE_DOC_VIEWER) && !defined(CONTRIB_DISABLE_DOC_VIEWER)
    st.doc_state.theme_index = st.theme_index;
    st.doc_state.dynamic_cursors_enabled = st.dynamic_cursors;

    ImGui::DocumentViewerConfig doc_cfg;
    doc_cfg.title                = "ImGee Quick Reference";
    doc_cfg.showOpacityControls  = false;
    doc_cfg.showSearchButton     = true;
    doc_cfg.enableSearchShortcut = true;
    doc_cfg.userData             = &st;
    doc_cfg.onThemeChanged       = [](int newThemeIdx, void* ud) {
        ImGeeDemoState* dst = static_cast<ImGeeDemoState*>(ud);
        if (dst) {
            dst->theme_index = newThemeIdx;
            dst->last_action_note = "Synchronized Demo theme from DocumentViewer.";
        }
    };

    const char* manual = DefaultDocViewerManualText();
    const float h = (std::max)(220.0f, ImGui::GetContentRegionAvail().y - 2.0f);
    (void)ImGui::DocumentViewer("##demo_doc_viewer", manual, std::strlen(manual), st.doc_state, &doc_cfg, ImVec2(0.0f, h));
#else
    (void)st;
    RenderDisabledBanner("DocumentViewer", "IMGEE_DISABLE_DOC_VIEWER");
#endif
}

inline void RenderTabFileDialogAndDiagnostics(ImGeeDemoState& st) {
    (void)st;
    ImGui::TextUnformatted("Non-Blocking File Picker & In-Viewport Dialogs");
    ImGui::Separator();
#if defined(IMGUI_FILE_DIALOG_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_FILE_DIALOG)
    ImGui::SetNextItemWidth(240.0f);
    if (ImGui::FilePickerCombo("Header Picker (.)", &st.picker_file, ".", ".h;.md")) {
        st.last_action_note = std::string("FilePickerCombo selected: ") + st.picker_file;
    }
    if (ImGui::Button("Open File Dialog...")) {
        st.open_dlg_visible = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save File Dialog...")) {
        st.save_dlg_visible = true;
    }
    if (ImGui::OpenFileDialog("ImGee Open File Demo", &st.open_dlg_visible, &st.chosen_open_path, ".", ".h;.md;.txt")) {
        st.last_action_note = std::string("Opened: ") + st.chosen_open_path;
    }
    if (ImGui::SaveFileDialog("ImGee Save File Demo", &st.save_dlg_visible, &st.chosen_save_path, ".", "sample.md", ".md;.txt")) {
        st.last_action_note = std::string("Save path: ") + st.chosen_save_path;
    }
    if (!st.chosen_open_path.empty()) {
        ImGui::TextDisabled("Last Open: %s", st.chosen_open_path.c_str());
    }
    if (!st.chosen_save_path.empty()) {
        ImGui::TextDisabled("Last Save: %s", st.chosen_save_path.c_str());
    }
#else
    RenderDisabledBanner("File Dialog", "IMGEE_DISABLE_FILE_DIALOG");
#endif

    ImGui::Spacing();
    ImGui::TextUnformatted("Build & Subsystem Diagnostics");
    ImGui::Separator();
    ImGui::BulletText("ImGee Version: %s (%d)", IMGEE_VERSION, IMGEE_VERSION_NUM);
    ImGui::BulletText("Dear ImGui Version: %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    const ImGee::HostBackend& hb = ImGee::GetHostBackend();
    ImGui::BulletText("HostBackend: CreateTexture=%s, DeleteTexture=%s, ResolveNamed=%s",
                      hb.CreateTextureRGBA8 ? "wired" : "null",
                      hb.DeleteTexture      ? "wired" : "null",
                      hb.ResolveNamedTexture ? "wired" : "null");
    ImGui::BulletText("Subsystems: TimedTooltip=%s, Markdown=%s, Typography=%s, DocViewer=%s, Icons=%s, Themes=%s",
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP)
                      "ON",
#else
                      "OFF",
#endif
#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN)
                      "ON",
#else
                      "OFF",
#endif
#if defined(IMGUI_STYLED_TEXT_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY)
                      "ON",
#else
                      "OFF",
#endif
#if defined(IMGUI_DOC_VIEWER_H) && !defined(IMGEE_DISABLE_DOC_VIEWER)
                      "ON",
#else
                      "OFF",
#endif
#if defined(CONTRIB_ICONS_AVAILABLE) && !defined(IMGEE_DISABLE_ICONS)
                      "ON",
#else
                      "OFF",
#endif
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES)
                      "ON"
#else
                      "OFF"
#endif
    );
}

} // namespace ImGeeDemoDetail

namespace ImGui {

/// Renders the interactive ImGee showcase body directly inside the caller's active window.
/// Ideal for hosts that already own ImGui::Begin()/End() (e.g. X-Plane ImgWindow / FlyWithLua).
inline void ShowImGeeDemoContent(const char* str_id = "##ImGeeDemo") {
    const char* safe_id = (str_id && str_id[0] != '\0') ? str_id : "##ImGeeDemo";
    const ImGuiID demo_id = ImGui::GetID(safe_id);
    ImGui::PushID(static_cast<int>(demo_id));

    ImGeeDemoDetail::ImGeeDemoState& st = ImGeeDemoDetail::GetOrCreateDemoState(demo_id);

    // 1. Top Strip: Live Hover-Latching Menu Strip + Theme Picker + Version Pill
#if defined(IMGUI_POPUP_MENUS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_POPUP_MENUS)
    {
        ImGui::MenuStripConfig topStripCfg;
        topStripCfg.anchor         = ImGuiPopupAnchor_BelowItem;
        topStripCfg.align          = ImGuiPopupAlign_Left;
        topStripCfg.dynamicCursors = st.dynamic_cursors;

        if (ImGui::BeginMenuStrip("##imgee_demo_top_strip", &topStripCfg)) {
            if (ImGui::BeginStripMenu("Demo", "**Demo Controls**\nReset sample values or restore your host style.")) {
                if (ImGui::MenuItem("Reset Demo State")) {
                    st.ResetDemoValues(true);
                    st.last_action_note = "Reset all interactive demo values.";
                }
                if (ImGui::MenuItem("Restore Host Style", nullptr, false, st.has_host_style_snapshot)) {
                    ImGeeDemoDetail::RestoreHostStyle(st);
                }
                ImGui::EndStripMenu();
            }
            if (ImGui::BeginStripMenu("Theme", "**Style Preset**\nApply curated ImGee or stock Dear ImGui themes.")) {
                static const char* const kMenuThemes[5] = {
                    "Charcoal (Default)", "Rusty (Bronze)", "ImGui Dark", "ImGui Classic",
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
                    "Paper Light"
#else
                    "ImGui Light"
#endif
                };
                for (int ti = 0; ti < 5; ++ti) {
                    if (ImGui::MenuItem(kMenuThemes[ti], nullptr, st.theme_index == ti)) {
                        ImGeeDemoDetail::ApplyDemoTheme(st, ti);
                        st.last_action_note = std::string("Applied theme: ") + kMenuThemes[ti];
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Restore Host Style", nullptr, false, st.has_host_style_snapshot)) {
                    ImGeeDemoDetail::RestoreHostStyle(st);
                }
                ImGui::EndStripMenu();
            }
            if (ImGui::BeginStripMenu("UX Toggles", "**Human-Factors Toggles**\nTest UX Manifesto behaviors live.")) {
                if (ImGui::MenuItem("Dynamic Cursors", nullptr, &st.dynamic_cursors)) {
                    st.last_action_note = st.dynamic_cursors ? "Dynamic cursors ON." : "Dynamic cursors OFF.";
                }
#if defined(IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED)
                if (ImGui::MenuItem("Mouse-Wheel Auto-Defeat", nullptr, &st.wheel_auto_defeat)) {
                    ImGui::SetMouseWheelAutoDefeat(st.wheel_auto_defeat);
                }
#endif
                ImGui::EndStripMenu();
            }
            ImGui::EndMenuStrip();
        }
        ImGui::SameLine(0.0f, 14.0f);
    }
#endif

    // Synchronized Theme Picker
    {
        static const char* const kThemeNames[5] = {
            "Charcoal", "Rusty", "ImGui Dark", "ImGui Classic",
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            "Paper Light"
#else
            "ImGui Light"
#endif
        };
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Theme:");
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::SetNextItemWidth(124.0f);
        int chosen_theme = st.theme_index;
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_COMBO)
        if (ImGui::MouseWheelCombo("##demo_theme_combo", &chosen_theme, kThemeNames, 5, -1, true)) {
            ImGeeDemoDetail::ApplyDemoTheme(st, chosen_theme);
        }
#else
        if (ImGui::Combo("##demo_theme_combo", &chosen_theme, kThemeNames, 5)) {
            ImGeeDemoDetail::ApplyDemoTheme(st, chosen_theme);
        }
#endif
    }

    ImGui::SameLine(0.0f, 12.0f);
#if defined(IMGUI_BADGES_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_BADGES)
    ImGui::Pill("v" IMGEE_VERSION, ImGui::ImGuiBadgeVariant_Primary);
    ImGui::SameLine(0.0f, 8.0f);
#endif
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("ImGui %s", IMGUI_VERSION);

    ImGui::Separator();

    // 2. Main 8-Tab Showcase Area
    const float footer_h = ImGui::GetTextLineHeightWithSpacing() + 6.0f;
    const float tabs_h   = (std::max)(200.0f, ImGui::GetContentRegionAvail().y - footer_h);
    const int forced_tab = ImGeeDemoDetail::GetForcedTabIndex();

    if (ImGeeDemoDetail::BeginChildCompat("##imgee_demo_tabs_region", ImVec2(0.0f, tabs_h), false)) {
        if (ImGui::BeginTabBar("##imgee_demo_tabbar", ImGuiTabBarFlags_FittingPolicyScroll)) {
            auto tabFlags = [&](int idx) -> ImGuiTabItemFlags {
                return (forced_tab == idx) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            };
            if (ImGui::BeginTabItem("Overview & Themes", nullptr, tabFlags(0))) {
                ImGeeDemoDetail::RenderTabOverviewAndThemes(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sliders, Toggles & Knobs", nullptr, tabFlags(1))) {
                ImGeeDemoDetail::RenderTabSlidersTogglesKnobs(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Menus, Pulldowns & Links", nullptr, tabFlags(2))) {
                ImGeeDemoDetail::RenderTabMenusPulldownsLinks(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tooltips, Badges & Spinners", nullptr, tabFlags(3))) {
                ImGeeDemoDetail::RenderTabTooltipsBadgesSpinners(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Vector Icons", nullptr, tabFlags(4))) {
                ImGeeDemoDetail::RenderTabVectorIcons(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Markdown & GFM Tables", nullptr, tabFlags(5))) {
                ImGeeDemoDetail::RenderTabMarkdownAndTables(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("DocumentViewer & Find", nullptr, tabFlags(6))) {
                ImGeeDemoDetail::RenderTabDocumentViewer(st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("File Dialog & Diagnostics", nullptr, tabFlags(7))) {
                ImGeeDemoDetail::RenderTabFileDialogAndDiagnostics(st);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    // 3. Compact Status Footer
    ImGui::Separator();
    ImGui::TextDisabled("%s", st.last_action_note.c_str());

    ImGui::PopID();
}

/// Opens a standalone top-level interactive ImGee showcase window (1:1 mirror of ImGui::ShowDemoWindow).
inline void ShowImGeeDemoWindow(bool* p_open = nullptr) {
    if (p_open && !*p_open) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(880.0f, 660.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("ImGee Interactive Showcase (v" IMGEE_VERSION ")", p_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ShowImGeeDemoContent("##ImGeeDemoWindowBody");
    }
    ImGui::End();
}

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGEE_DEMO_H
