// ============================================================================
// ImGui Contrib: Built-in Procedural Vector Icons Engine
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Description:
//   Provides a zero-dependency, zero-texture, zero-font procedural vector icon
//   engine for Dear ImGui. Renders crisp antialiased vector geometry directly
//   to ImDrawList, completely eliminating missing-glyph diamond boxes () when
//   extended Unicode blocks or custom font icons (e.g. FontAwesome) are missing.
//
// Features:
//   1. Multi-Tier Resolution:
//      - Custom Texture (via xyzzy / RegisterMemoryImage) -> ImGui::Image
//      - Icon Font (if configured by host) -> Font Glyph
//      - Default: Procedural Vector Paths (100% batched into ImDrawList)
//   2. Extensive Icon Catalog:
//      - undo / reset (↺), redo / restore (↻)
//      - external-link (↗), caret_up (▲), caret_down (▼), caret_left (◀), caret_right (▶)
//      - check (✓), cross / times (✗), plus (+), minus (-)
//      - gear / settings (⚙), info (ℹ), warning (⚠), star (★)
//      - search (🔍), plane / aircraft (✈), circle (●), square (■)
//   3. Forgiving Name Normalization:
//      Accepts "undo", ":undo:", "ICON_UNDO", "ICON_FA_UNDO", "fa-undo",
//      "reset", and raw Unicode symbols ("↺", "▲", etc.).
//   4. Turnkey Inline Widget:
//      ImGui::Icon(name, size, color)
// ============================================================================

#pragma once

#ifndef IMGUI_ICONS_H
#define IMGUI_ICONS_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#if !defined(IMGEE_DISABLE_WIDGETS) && \
    !defined(IMGEE_DISABLE_ICONS) && \
    !defined(CONTRIB_DISABLE_ICONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_ICONS) && \
    !defined(IMGUI_DISABLE_ICONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_ICONS)
#define CONTRIB_ICONS_AVAILABLE 1

#if defined(__has_include)
    #if __has_include(<imgui.h>)
        #include <imgui.h>
    #elif __has_include("imgui.h")
        #include "imgui.h"
    #endif
    #if __has_include("imgui_internal.h")
        #include "imgui_internal.h"
    #elif __has_include(<imgui_internal.h>)
        #include <imgui_internal.h>
    #endif
#else
    #include "imgui.h"
    #include "imgui_internal.h"
#endif

#include <string>
#include <cmath>
#include <algorithm>

#ifndef IMGUI_API
#define IMGUI_API
#endif

#ifndef IMGEE_GET_ITEM_ID_COMPAT_DEFINED
#define IMGEE_GET_ITEM_ID_COMPAT_DEFINED
namespace ImGui {
namespace Detail {
    inline ImGuiID GetLastItemIDCompat() {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM < 18300)
        return (GImGui && GImGui->CurrentWindow) ? GImGui->CurrentWindow->DC.LastItemId : 0;
#else
        return GetItemID();
#endif
    }
} // namespace Detail
} // namespace ImGui
#endif // IMGEE_GET_ITEM_ID_COMPAT_DEFINED

#ifndef IMGUI_LINK_CLICK_RESULT_DEFINED
#define IMGUI_LINK_CLICK_RESULT_DEFINED
namespace ImGui {
namespace Detail {
    struct LinkClickToggleState {
        ImGuiContext* context                 = nullptr;
        int           frame_count             = -1;
        ImGuiID       item_id                 = 0;
        ImVec2        item_min                = ImVec2(-1.0f, -1.0f);
        bool          toggle_mode             = false;
        bool          click_consumed          = false;
        bool          is_menu_item            = false;
        bool          menu_item_selected      = false;
        ImGuiContext* menu_active_context     = nullptr;
        int           menu_active_frame       = -1;
        bool          wheel_latched           = false;
        ImGuiContext* wheel_latch_context     = nullptr;
        int           wheel_latch_alive_frame = -1;
        ImVec2        wheel_latch_mouse_pos   = ImVec2(-1.0f, -1.0f);
    };

    inline LinkClickToggleState& GetLinkClickToggleState() {
        static LinkClickToggleState s_State;
        return s_State;
    }

    inline void RegisterItemTooltipToggle(bool is_toggle_eligible = true) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImGuiID cur_id = GetLastItemIDCompat();
        ImVec2 cur_min = GetItemRectMin();
        bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                         ((cur_id != 0 && st.item_id == cur_id) ||
                          (cur_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
        if (!same_item) {
            st.context            = cur_ctx;
            st.frame_count        = cur_frame;
            st.item_id            = cur_id;
            st.item_min           = cur_min;
            st.click_consumed     = false;
            st.is_menu_item       = false;
            st.menu_item_selected = false;
        }
        if (IsItemClicked()) {
            st.wheel_latched = false;
        }
        st.toggle_mode = st.click_consumed ? false : is_toggle_eligible;
    }

    inline void RegisterMenuItemSelection(bool is_selected, bool is_clicked = false, bool is_toggle_eligible = false) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        st.context             = cur_ctx;
        st.frame_count         = cur_frame;
        st.item_id             = GetLastItemIDCompat();
        st.item_min            = GetItemRectMin();
        st.click_consumed      = false;
        st.toggle_mode         = is_toggle_eligible;
        st.is_menu_item        = true;
        st.menu_item_selected  = is_selected;
        st.menu_active_context = cur_ctx;
        st.menu_active_frame   = cur_frame;
        if (is_clicked) {
            st.wheel_latched = false;
        }
    }

    inline bool KeepWheelLatchAlive(bool wheel_stepped) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        ImVec2 mouse_pos = GetIO().MousePos;
        if (wheel_stepped) {
            st.wheel_latched           = true;
            st.wheel_latch_context     = cur_ctx;
            st.wheel_latch_alive_frame = cur_frame;
            st.wheel_latch_mouse_pos   = mouse_pos;
            return false;
        } else if (st.wheel_latched && st.wheel_latch_context == cur_ctx) {
            float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
            float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
            if (dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
                st.wheel_latched = false;
                return true;
            } else {
                st.wheel_latch_alive_frame = cur_frame;
            }
        }
        return false;
    }

    inline bool ResolveItemTooltipHover(ImGuiID raw_item_id, bool raw_hovered) {
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImGuiContext* cur_ctx = GetCurrentContext();
        int cur_frame = GetFrameCount();
        if (!st.wheel_latched || st.wheel_latch_context != cur_ctx) {
            return raw_hovered;
        }
        ImVec2 mouse_pos = GetIO().MousePos;
        float dx = mouse_pos.x - st.wheel_latch_mouse_pos.x;
        float dy = mouse_pos.y - st.wheel_latch_mouse_pos.y;
        bool menu_still_open = (st.menu_active_context == cur_ctx) && (cur_frame - st.menu_active_frame <= 1);
        if ((!menu_still_open && cur_frame - st.wheel_latch_alive_frame > 2) ||
            dx > 6.0f || dx < -6.0f || dy > 6.0f || dy < -6.0f) {
            st.wheel_latched = false;
            return raw_hovered;
        }
        if (menu_still_open) {
            ImVec2 cur_min = GetItemRectMin();
            bool same_item = (st.context == cur_ctx) && (st.frame_count == cur_frame) &&
                             ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                              (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y));
            if (same_item && st.is_menu_item) {
                return st.menu_item_selected;
            }
            return false;
        }
        return raw_hovered;
    }

    inline void MarkLastItemClickConsumed() {
        LinkClickToggleState& st = GetLinkClickToggleState();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            st.click_consumed = true;
            st.toggle_mode    = false;
        }
    }

    inline bool ShouldToggleTooltipOnClick(ImGuiID raw_item_id, bool is_item_active) {
        (void)is_item_active;
        LinkClickToggleState& st = GetLinkClickToggleState();
        ImVec2 cur_min = GetItemRectMin();
        if (st.context == GetCurrentContext() && st.frame_count == GetFrameCount()) {
            if ((raw_item_id != 0 && st.item_id == raw_item_id) ||
                (raw_item_id == 0 && st.item_id == 0 && st.item_min.x == cur_min.x && st.item_min.y == cur_min.y)) {
                return st.toggle_mode;
            }
        }
        return false;
    }
} // namespace Detail

    struct LinkClickResult {
        bool clicked;

        constexpr LinkClickResult(bool is_clicked = false) noexcept : clicked(is_clicked) {}

        operator bool() const noexcept {
            Detail::MarkLastItemClickConsumed();
            return clicked;
        }
    };
} // namespace ImGui
#endif // IMGUI_LINK_CLICK_RESULT_DEFINED

namespace ImGui {

    namespace Detail {
        template <typename T>
        inline T IconMax(T a, T b) { return (a > b) ? a : b; }
        template <typename T>
        inline T IconMin(T a, T b) { return (a < b) ? a : b; }

        // Zero-bias vector stroke helpers: ImDrawList::AddLine adds +0.5f to endpoints for 1px UI borders,
        // which shifts strokes relative to unshifted AddCircle / AddTriangle / PathArcTo geometry.
        inline void StrokeIconLine(ImDrawList* dl, const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness) {
            dl->PathClear();
            dl->PathLineTo(p1);
            dl->PathLineTo(p2);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            dl->PathStroke(col, thickness, 0);
#else
            dl->PathStroke(col, 0, thickness);
#endif
        }

        inline void StrokeIconPolyline(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col, float thickness, ImDrawFlags flags = 0) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            dl->AddPolyline(pts, count, col, thickness, flags);
#else
            dl->AddPolyline(pts, count, col, flags, thickness);
#endif
        }
    }

    // -------------------------------------------------------------------------
    // 1. Icon Name Normalization
    // -------------------------------------------------------------------------

    /// Normalizes diverse icon strings ("ICON_FA_UNDO", ":undo:", "caret_up", "↺", "▲")
    /// into canonical lowercase hyphenated identifiers ("undo", "caret-up", etc.).
    inline std::string NormalizeIconName(const char* name) {
        if (!name || !*name) return "";

        std::string s(name);

        // Trim punctuation wrappers (":", "[", "]", "(", ")", "#", whitespace) first
        // so bracketed/padded raw UTF-8 glyphs like "[↺]" or " ▲ " normalize cleanly too:
        size_t start = 0;
        while (start < s.size() && (s[start] == ':' || s[start] == ' ' || s[start] == '\t' ||
                                    s[start] == '#' || s[start] == '[' || s[start] == ']' ||
                                    s[start] == '(' || s[start] == ')')) {
            start++;
        }
        size_t end = s.size();
        while (end > start && (s[end - 1] == ':' || s[end - 1] == ' ' || s[end - 1] == '\t' ||
                              s[end - 1] == '#' || s[end - 1] == '[' || s[end - 1] == ']' ||
                              s[end - 1] == '(' || s[end - 1] == ')')) {
            end--;
        }
        s = s.substr(start, end - start);

        // Check for raw UTF-8 Unicode glyphs:
        if (s == "\xE2\x86\xBA") return "undo";        // ↺ U+21BA
        if (s == "\xE2\x86\xBB") return "redo";        // ↻ U+21BB
        if (s == "\xE2\x86\x97") return "external-link";    // ↗ U+2197
        if (s == "\xE2\x86\x98") return "arrow-down-right"; // ↘ U+2198
        if (s == "\xE2\x86\x96") return "arrow-up-left";   // ↖ U+2196
        if (s == "\xE2\x86\x99") return "arrow-down-left"; // ↙ U+2199
        if (s == "\xE2\x96\xB2") return "caret-up";    // ▲ U+25B2
        if (s == "\xE2\x96\xBE") return "caret-down";  // ▾ U+25BE
        if (s == "\xE2\x96\xBC") return "caret-down";  // ▼ U+25BC
        if (s == "\xE2\x97\x80") return "caret-left";  // ◀ U+25C0
        if (s == "\xE2\x96\xB6") return "caret-right"; // ▶ U+25B6
        if (s == "\xE2\x9C\x93") return "check";       // ✓ U+2713
        if (s == "\xE2\x9C\x94") return "check";       // ✔ U+2714
        if (s == "\xE2\x9C\x97") return "cross";       // ✗ U+2717
        if (s == "\xE2\x9C\x98") return "cross";       // ✘ U+2718
        if (s == "\xE2\x9A\x99") return "gear";        // ⚙ U+2699
        if (s == "\xE2\x84\xB9") return "info";        // ℹ U+2139
        if (s == "\xE2\x9A\xA0") return "warning";     // ⚠ U+26A0
        if (s == "\xE2\x98\x85") return "star";        // ★ U+2605
        if (s == "\xE2\x9C\x88") return "plane";       // ✈ U+2708
        if (s == "\xF0\x9F\x90\x89") return "ouroboros";     // 🐉 U+1F409
        if (s == "\xF0\x9F\x90\xB2") return "ouroboros-alt"; // 🐲 U+1F432

        for (char& c : s) {
            c = (char)tolower((unsigned char)c);
            if (c == '_') c = '-';
        }

        // Strip conventional icon prefixes
        if (s.rfind("icon-fa-", 0) == 0)      s = s.substr(8);
        else if (s.rfind("icon-", 0) == 0)   s = s.substr(5);
        else if (s.rfind("fa-", 0) == 0)     s = s.substr(3);

        // Canonical aliases
        if (s == "reset" || s == "undo-alt" || s == "rotate-left" || s == "refresh" || s == "reload" || s == "sync" || s == "retweet") return "undo";
        if (s == "restore" || s == "redo-alt" || s == "rotate-right") return "redo";
        if (s == "external" || s == "open-in-new" || s == "link-external" || s == "external-link-alt" || s == "arrow-up-right") return "external-link";
        if (s == "folder-open" || s == "directory") return "folder";
        if (s == "lock-open" || s == "unlocked") return "unlock";
        if (s == "chevron-up" || s == "arrow-up" || s == "triangle-up" || s == "^") return "caret-up";
        if (s == "chevron-down" || s == "arrow-down" || s == "triangle-down" || s == "v") return "caret-down";
        if (s == "chevron-left" || s == "arrow-left" || s == "triangle-left" || s == "<") return "caret-left";
        if (s == "chevron-right" || s == "arrow-right" || s == "triangle-right" || s == ">") return "caret-right";
        if (s == "checkmark" || s == "ok" || s == "yes") return "check";
        if (s == "times" || s == "cancel" || s == "close" || s == "x") return "cross";
        if (s == "add") return "plus";
        if (s == "remove") return "minus";
        if (s == "settings" || s == "cog") return "gear";
        if (s == "info-circle" || s == "information") return "info";
        if (s == "bulb" || s == "idea" || s == "tip") return "lightbulb";
        if (s == "report") return "important";
        if (s == "octagon" || s == "stop") return "caution";
        if (s == "alert" || s == "exclamation-triangle" || s == "exclamation" || s == "exclamation-circle" || s == "warn") return "warning";
        if (s == "favorite") return "star";
        if (s == "magnifier" || s == "find") return "search";
        if (s == "aircraft" || s == "airplane") return "plane";
        if (s == "dot") return "circle";
        if (s == "ouroboros" || s == "dragon" || s == "gee-crest") return "ouroboros";
        if (s == "ouroboros-alt" || s == "dragon-alt") return "ouroboros-alt";

        return s;
    }

    /// Returns true if the specified string identifies a recognized vector icon.
    inline bool IsVectorIcon(const char* name) {
        if (!name || !*name) return false;
        std::string n = NormalizeIconName(name);
        return (n == "undo" || n == "redo" || n == "external-link" ||
                n == "caret-up" || n == "caret-down" || n == "caret-left" || n == "caret-right" ||
                n == "check" || n == "cross" || n == "plus" || n == "minus" ||
                n == "gear" || n == "info" || n == "lightbulb" || n == "important" ||
                n == "warning" || n == "caution" || n == "star" ||
                n == "search" || n == "plane" || n == "circle" || n == "square" ||
                n == "ouroboros" || n == "ouroboros-alt");
    }

    // -------------------------------------------------------------------------
    // 2. Procedural Vector Icon Renderer
    // -------------------------------------------------------------------------

    /// Actively draws a crisp procedural vector icon into draw_list inside the bounding
    /// box [pos.x .. pos.x + size, pos.y .. pos.y + size].
    /// Completely free of font atlas or texture dependencies!
    inline IMGUI_API bool DrawVectorIcon(ImDrawList* draw_list, ImVec2 pos, float size, ImU32 col, const char* name) {
        if (!draw_list || size <= 0.0f || !name || !*name) return false;

        std::string id = NormalizeIconName(name);
        if (id.empty()) return false;

        const float kPI = 3.14159265358979323846f;
        ImVec2 center(pos.x + size * 0.5f, pos.y + size * 0.5f);

        if (id == "caret-up") {
            float r = size * 0.40f;
            ImVec2 a(center.x, center.y - r * 0.85f);
            ImVec2 b(center.x - r * 0.95f, center.y + r * 0.75f);
            ImVec2 c(center.x + r * 0.95f, center.y + r * 0.75f);
            draw_list->AddTriangleFilled(a, b, c, col);
            return true;
        }

        if (id == "caret-down") {
            float r = size * 0.40f;
            ImVec2 a(center.x, center.y + r * 0.85f);
            ImVec2 b(center.x - r * 0.95f, center.y - r * 0.75f);
            ImVec2 c(center.x + r * 0.95f, center.y - r * 0.75f);
            draw_list->AddTriangleFilled(a, b, c, col);
            return true;
        }

        if (id == "caret-left") {
            float r = size * 0.40f;
            ImVec2 a(center.x - r * 0.85f, center.y);
            ImVec2 b(center.x + r * 0.75f, center.y - r * 0.95f);
            ImVec2 c(center.x + r * 0.75f, center.y + r * 0.95f);
            draw_list->AddTriangleFilled(a, b, c, col);
            return true;
        }

        if (id == "caret-right") {
            float r = size * 0.40f;
            ImVec2 a(center.x + r * 0.85f, center.y);
            ImVec2 b(center.x - r * 0.75f, center.y - r * 0.95f);
            ImVec2 c(center.x - r * 0.75f, center.y + r * 0.95f);
            draw_list->AddTriangleFilled(a, b, c, col);
            return true;
        }

        if (id == "undo") {
            // Counter-clockwise circular arc + tangent arrowhead (matching ICON_FA_UNDO / U+21BA):
            // In ImGui screen coordinates (+Y down):
            //   -0.62*PI is ~10:45 (top-left arrowhead), -0.50*PI is 12:00 (top),
            //    0.00*PI is  3:00 (right),              +0.50*PI is  6:00 (bottom),
            //   +0.72*PI is ~7:30 (bottom-left tail).
            // Sweeps counter-clockwise from bottom-left around the right to top-left!
            ImVec2 arc_center(pos.x + size * 0.52f, pos.y + size * 0.52f);
            float radius = size * 0.25f;
            float thickness = Detail::IconMax(1.2f, size * 0.095f);

            float a_head = -0.62f * kPI;
            float a_tail =  0.72f * kPI;
            draw_list->PathArcTo(arc_center, radius, a_head, a_tail, 16);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            draw_list->PathStroke(col, thickness, 0);
#else
            draw_list->PathStroke(col, 0, thickness);
#endif
            float cos_a = std::cos(a_head);
            float sin_a = std::sin(a_head);
            float tx =  sin_a;
            float ty = -cos_a;
            float nx =  cos_a;
            float ny =  sin_a;

            ImVec2 base(arc_center.x + radius * cos_a, arc_center.y + radius * sin_a);
            float arrow_len = radius * 0.95f;
            float arrow_w   = radius * 0.68f;
            ImVec2 tip(base.x + tx * (arrow_len * 0.65f), base.y + ty * (arrow_len * 0.65f));
            ImVec2 back(base.x - tx * (arrow_len * 0.35f), base.y - ty * (arrow_len * 0.35f));
            ImVec2 p1(back.x + nx * arrow_w, back.y + ny * arrow_w);
            ImVec2 p2(back.x - nx * arrow_w, back.y - ny * arrow_w);
            draw_list->AddTriangleFilled(tip, p1, p2, col);
            return true;
        }

        if (id == "redo") {
            // Clockwise circular arc + tangent arrowhead (horizontal mirror of undo)
            ImVec2 arc_center(pos.x + size * 0.48f, pos.y + size * 0.52f);
            float radius = size * 0.25f;
            float thickness = Detail::IconMax(1.2f, size * 0.095f);

            float a_tail = 0.28f * kPI;
            float a_head = 1.62f * kPI;
            draw_list->PathArcTo(arc_center, radius, a_tail, a_head, 16);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            draw_list->PathStroke(col, thickness, 0);
#else
            draw_list->PathStroke(col, 0, thickness);
#endif
            float cos_a = std::cos(a_head);
            float sin_a = std::sin(a_head);
            float tx = -sin_a;
            float ty =  cos_a;
            float nx =  cos_a;
            float ny =  sin_a;

            ImVec2 base(arc_center.x + radius * cos_a, arc_center.y + radius * sin_a);
            float arrow_len = radius * 0.95f;
            float arrow_w   = radius * 0.68f;
            ImVec2 tip(base.x + tx * (arrow_len * 0.65f), base.y + ty * (arrow_len * 0.65f));
            ImVec2 back(base.x - tx * (arrow_len * 0.35f), base.y - ty * (arrow_len * 0.35f));
            ImVec2 p1(back.x + nx * arrow_w, back.y + ny * arrow_w);
            ImVec2 p2(back.x - nx * arrow_w, back.y - ny * arrow_w);
            draw_list->AddTriangleFilled(tip, p1, p2, col);
            return true;
        }

        if (id == "check") {
            float thickness = Detail::IconMax(1.5f, size * 0.12f);
            float s = size * 0.70f;
            const ImVec2 pts[3] = {
                ImVec2(center.x - s * 0.38f, center.y + s * 0.05f),
                ImVec2(center.x - s * 0.08f, center.y + s * 0.38f),
                ImVec2(center.x + s * 0.42f, center.y - s * 0.38f)
            };
            Detail::StrokeIconPolyline(draw_list, pts, 3, col, thickness, 0);
            return true;
        }

        if (id == "cross") {
            float thickness = Detail::IconMax(1.5f, size * 0.12f);
            float r = size * 0.28f;
            Detail::StrokeIconLine(draw_list, ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), col, thickness);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x + r, center.y - r), ImVec2(center.x - r, center.y + r), col, thickness);
            return true;
        }

        if (id == "plus") {
            float thickness = Detail::IconMax(1.5f, size * 0.12f);
            float r = size * 0.32f;
            Detail::StrokeIconLine(draw_list, ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), col, thickness);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x, center.y - r), ImVec2(center.x, center.y + r), col, thickness);
            return true;
        }

        if (id == "minus") {
            float thickness = Detail::IconMax(1.5f, size * 0.12f);
            float r = size * 0.32f;
            Detail::StrokeIconLine(draw_list, ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), col, thickness);
            return true;
        }

        if (id == "gear") {
            float r_outer = size * 0.39f;
            float r_inner = size * 0.28f;
            float r_hole  = size * 0.13f;
            float r_mid   = (r_inner + r_hole) * 0.5f;
            float ring_th = r_inner - r_hole;
            const int teeth = 6;
            const float half_tip  = kPI / (teeth * 3.4f);
            const float half_base = half_tip + 0.18f;
            for (int t = 0; t < teeth; ++t) {
                float ac = (t * 2.0f * kPI) / teeth;
                float a0 = ac - half_tip;
                float a1 = ac + half_tip;
                ImVec2 tip(center.x + r_outer * std::cos(a0), center.y + r_outer * std::sin(a0));
                ImVec2 tip2(center.x + r_outer * std::cos(a1), center.y + r_outer * std::sin(a1));
                ImVec2 b0(center.x + r_mid * std::cos(ac - half_base), center.y + r_mid * std::sin(ac - half_base));
                ImVec2 b1(center.x + r_mid * std::cos(ac + half_base), center.y + r_mid * std::sin(ac + half_base));
                draw_list->AddQuadFilled(b0, tip, tip2, b1, col);
            }
            draw_list->AddCircle(center, r_mid, col, 20, ring_th);
            return true;
        }

        if (id == "info") {
            float r = size * 0.38f;
            float thickness = Detail::IconMax(1.2f, size * 0.09f);
            draw_list->AddCircle(center, r, col, 16, thickness);
            draw_list->AddCircleFilled(ImVec2(center.x, center.y - r * 0.42f), thickness * 0.95f, col, 8);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x, center.y - r * 0.12f), ImVec2(center.x, center.y + r * 0.50f), col, thickness * 1.3f);
            return true;
        }

        if (id == "lightbulb") {
            float thickness = Detail::IconMax(1.2f, size * 0.09f);
            ImVec2 bulb_c(center.x, center.y - size * 0.11f);
            float r = size * 0.29f;
            float neck_half_w = r * 0.56f;
            float neck_bot_y = center.y + size * 0.23f;

            // Single continuous closed symmetric silhouette: bulb dome + tapered neck + flat base
            draw_list->PathClear();
            const int arc_segs = 16;
            const float half_sweep = 0.74f * kPI;
            for (int i = 0; i <= arc_segs; ++i) {
                float frac = (static_cast<float>(i) / static_cast<float>(arc_segs)) - 0.5f;
                float a = 1.5f * kPI + frac * (2.0f * half_sweep);
                draw_list->PathLineTo(ImVec2(bulb_c.x + r * std::cos(a), bulb_c.y + r * std::sin(a)));
            }
            draw_list->PathLineTo(ImVec2(center.x + neck_half_w, neck_bot_y));
            draw_list->PathLineTo(ImVec2(center.x - neck_half_w, neck_bot_y));
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            draw_list->PathStroke(col, thickness, ImDrawFlags_Closed);
#else
            draw_list->PathStroke(col, ImDrawFlags_Closed, thickness);
#endif

            // Crisp zero-bias screw-cap bar beneath the flat base:
            float cap_half_w = neck_half_w * 0.72f;
            float cap_y = neck_bot_y + Detail::IconMax(1.8f, size * 0.135f);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x - cap_half_w, cap_y), ImVec2(center.x + cap_half_w, cap_y), col, thickness);
            return true;
        }

        if (id == "important") {
            float r = size * 0.38f;
            float thickness = Detail::IconMax(1.2f, size * 0.09f);
            draw_list->AddCircle(center, r, col, 16, thickness);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x, center.y - r * 0.48f), ImVec2(center.x, center.y + r * 0.12f), col, thickness * 1.35f);
            draw_list->AddCircleFilled(ImVec2(center.x, center.y + r * 0.46f), Detail::IconMax(1.0f, size * 0.065f), col, 8);
            return true;
        }

        if (id == "caution") {
            float r = size * 0.39f;
            float thickness = Detail::IconMax(1.2f, size * 0.09f);
            ImVec2 pts[8];
            for (int i = 0; i < 8; ++i) {
                float a = (i * kPI * 0.25f) + (kPI * 0.125f);
                pts[i] = ImVec2(center.x + r * std::cos(a), center.y + r * std::sin(a));
            }
            Detail::StrokeIconPolyline(draw_list, pts, 8, col, thickness, ImDrawFlags_Closed);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x, center.y - r * 0.46f), ImVec2(center.x, center.y + r * 0.10f), col, thickness * 1.35f);
            draw_list->AddCircleFilled(ImVec2(center.x, center.y + r * 0.44f), Detail::IconMax(1.0f, size * 0.065f), col, 8);
            return true;
        }

        if (id == "warning") {
            float h = size * 0.40f;
            float thickness = Detail::IconMax(1.3f, size * 0.09f);
            ImVec2 a(center.x, center.y - h);
            ImVec2 b(center.x - h * 1.05f, center.y + h * 0.85f);
            ImVec2 c(center.x + h * 1.05f, center.y + h * 0.85f);
            draw_list->AddTriangle(a, b, c, col, thickness);
            Detail::StrokeIconLine(draw_list, ImVec2(center.x, center.y - h * 0.30f), ImVec2(center.x, center.y + h * 0.25f), col, thickness);
            draw_list->AddCircleFilled(ImVec2(center.x, center.y + h * 0.55f), Detail::IconMax(1.0f, size * 0.065f), col, 8);
            return true;
        }

        if (id == "star") {
            const int points = 5;
            float r_out = size * 0.42f;
            float r_in  = size * 0.17f;
            ImVec2 pts[10];
            for (int p = 0; p < points * 2; ++p) {
                float a = (p * kPI) / points - (kPI * 0.5f);
                float r = (p % 2 == 0) ? r_out : r_in;
                pts[p] = ImVec2(center.x + r * std::cos(a), center.y + r * std::sin(a));
            }
            // A 5-pointed star is concave; decompose into 5 convex point kites + inner pentagon
            // so outer edges get a clean single-pass AA fringe and inner seams are covered.
            for (int i = 0; i < points; ++i) {
                const ImVec2 kite[4] = {
                    center,
                    pts[(2 * i + 9) % 10],
                    pts[2 * i],
                    pts[(2 * i + 1) % 10]
                };
                draw_list->AddConvexPolyFilled(kite, 4, col);
            }
            const ImVec2 inner_pent[5] = { pts[1], pts[3], pts[5], pts[7], pts[9] };
            draw_list->AddConvexPolyFilled(inner_pent, 5, col);
            return true;
        }

        if (id == "search") {
            float r = size * 0.22f;
            float thickness = Detail::IconMax(1.2f, size * 0.09f);
            ImVec2 ring_center(center.x - size * 0.06f, center.y - size * 0.06f);
            draw_list->AddCircle(ring_center, r, col, 16, thickness);
            ImVec2 h_start(ring_center.x + r * 0.707f, ring_center.y + r * 0.707f);
            ImVec2 h_end(center.x + size * 0.38f, center.y + size * 0.38f);
            Detail::StrokeIconLine(draw_list, h_start, h_end, col, thickness * 1.4f);
            return true;
        }

        if (id == "plane") {
            // Swept-wing aircraft silhouette
            float s = size * 0.42f;
            ImVec2 nose(center.x, center.y - s * 0.90f);
            ImVec2 tail(center.x, center.y + s * 0.90f);
            Detail::StrokeIconLine(draw_list, nose, tail, col, Detail::IconMax(1.5f, size * 0.12f));
            // Main wings
            const ImVec2 wing_pts[3] = {
                ImVec2(center.x - s * 0.95f, center.y + s * 0.10f),
                ImVec2(center.x, center.y - s * 0.20f),
                ImVec2(center.x + s * 0.95f, center.y + s * 0.10f)
            };
            Detail::StrokeIconPolyline(draw_list, wing_pts, 3, col, Detail::IconMax(1.3f, size * 0.10f), 0);
            // Tail stabilizer
            const ImVec2 tail_pts[3] = {
                ImVec2(center.x - s * 0.45f, center.y + s * 0.75f),
                tail,
                ImVec2(center.x + s * 0.45f, center.y + s * 0.75f)
            };
            Detail::StrokeIconPolyline(draw_list, tail_pts, 3, col, Detail::IconMax(1.1f, size * 0.08f), 0);
            return true;
        }

        if (id == "circle") {
            draw_list->AddCircleFilled(center, size * 0.30f, col, 16);
            return true;
        }

        if (id == "square") {
            float r = size * 0.30f;
            draw_list->AddRectFilled(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), col, size * 0.08f);
            return true;
        }

        if (id == "external-link") {
            float thickness = Detail::IconMax(1.2f, size * 0.095f);
            // Box with open top-right corner (single mitered polyline)
            const ImVec2 box_pts[5] = {
                ImVec2(pos.x + size * 0.40f, pos.y + size * 0.32f),
                ImVec2(pos.x + size * 0.12f, pos.y + size * 0.32f),
                ImVec2(pos.x + size * 0.12f, pos.y + size * 0.88f),
                ImVec2(pos.x + size * 0.68f, pos.y + size * 0.88f),
                ImVec2(pos.x + size * 0.68f, pos.y + size * 0.52f)
            };
            Detail::StrokeIconPolyline(draw_list, box_pts, 5, col, thickness, 0);

            // 45-degree breakout arrow + mitered arrowhead
            ImVec2 a_tail(pos.x + size * 0.42f, pos.y + size * 0.58f);
            ImVec2 a_tip(pos.x + size * 0.88f, pos.y + size * 0.12f);
            Detail::StrokeIconLine(draw_list, a_tail, a_tip, col, thickness);

            float fin = size * 0.28f;
            const ImVec2 head_pts[3] = {
                ImVec2(a_tip.x - fin, a_tip.y),
                a_tip,
                ImVec2(a_tip.x, a_tip.y + fin)
            };
            Detail::StrokeIconPolyline(draw_list, head_pts, 3, col, thickness, 0);
            return true;
        }

        if (id == "ouroboros" || id == "ouroboros-alt") {
            // House of Gee Ouroboros Chinese Dragons (born from swapped Lightbox arc/jaw radians!)
            const float ar = size * 0.38f;
            const float u  = ar / 5.4f;
            const float stroke_arc = Detail::IconMax(1.15f, 1.65f * u);
            const float stroke_jaw = Detail::IconMax(1.10f, 1.60f * u);
            draw_list->PathClear();
            if (id == "ouroboros") {
                draw_list->PathArcTo(center, ar, -0.59f, 4.11f, 20);
            } else {
                draw_list->PathArcTo(center, ar, -2.55f, 2.15f, 20);
            }
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            draw_list->PathStroke(col, stroke_arc, 0);
#else
            draw_list->PathStroke(col, 0, stroke_arc);
#endif
            if (id == "ouroboros") {
                const ImVec2 tip(center.x + std::cos(-2.55f) * ar, center.y + std::sin(-2.55f) * ar);
                const ImVec2 jaws[3] = {
                    ImVec2(tip.x + 3.6f * u, tip.y - 1.2f * u),
                    tip,
                    ImVec2(tip.x + 0.8f * u, tip.y + 3.4f * u)
                };
                Detail::StrokeIconPolyline(draw_list, jaws, 3, col, stroke_jaw, 0);
            } else {
                const ImVec2 tip(center.x + std::cos(-0.59f) * ar, center.y + std::sin(-0.59f) * ar);
                const ImVec2 jaws[3] = {
                    ImVec2(tip.x - 3.6f * u, tip.y - 1.2f * u),
                    tip,
                    ImVec2(tip.x - 0.8f * u, tip.y - 0.0f + 3.4f * u)
                };
                Detail::StrokeIconPolyline(draw_list, jaws, 3, col, stroke_jaw, 0);
            }
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------------
    // 3. Immediate-Mode Inline Icon Widget
    // -------------------------------------------------------------------------

    /// Submits an inline procedural vector icon into the current ImGui layout.
    /// Scales automatically to the active font size (GetFontSize()) if size <= 0.0f.
    /// Centers vertically with the text baseline.
    inline IMGUI_API void Icon(const char* name, float size = 0.0f, const ImVec4* color = nullptr) {
        if (!name || !*name) return;
        float font_size = GetFontSize();
        float sz = (size > 0.0f) ? size : (font_size * 0.90f);
        ImVec2 p = GetCursorScreenPos();
        ImU32 col = color ? GetColorU32(*color) : GetColorU32(ImGuiCol_Text);
        ImDrawList* draw_list = GetWindowDrawList();

        // Center vertically within the font line height:
        float offset_y = std::round((font_size - sz) * 0.5f);
        ImVec2 icon_pos(p.x, p.y + offset_y);

        Dummy(ImVec2(sz, font_size));
        Detail::RegisterItemTooltipToggle(true);
        DrawVectorIcon(draw_list, icon_pos, sz, col, name);
    }

    // -------------------------------------------------------------------------
    // 4. Immediate-Mode Procedural Vector Push-Buttons
    // -------------------------------------------------------------------------

#if !defined(IMGEE_DISABLE_ICON_BUTTON) && !defined(CONTRIB_DISABLE_ICON_BUTTON) && !defined(IMGUI_DISABLE_ICON_BUTTON)

    /// Renders a push button containing a centered procedural vector icon.
    /// - If size.x <= 0.0f, defaults to a square button matching ImGui::GetFrameHeight().
    /// - If size.y <= 0.0f, defaults to ImGui::GetFrameHeight().
    /// - Automatically sets ImGuiMouseCursor_Hand on hover.
    /// - Displays tooltip (if provided) when hovered.
    /// - Returns true on click.
    inline IMGUI_API bool IconButton(const char* str_id,
                                     const char* icon_name,
                                     const ImVec2& size = ImVec2(0.0f, 0.0f),
                                     const char* tooltip = nullptr,
                                     const ImVec4* icon_color = nullptr) {
        if (!str_id || !icon_name) return false;

        float frame_h = GetFrameHeight();
        ImVec2 btn_sz = size;
        if (btn_sz.x <= 0.0f) btn_sz.x = (btn_sz.y > 0.0f) ? btn_sz.y : frame_h;
        if (btn_sz.y <= 0.0f) btn_sz.y = frame_h;

        // Build invisible label (starting with ###) so ImGui draws no font text
        std::string id_str(str_id);
        std::string invisible_label;
        size_t id_pos_triple = id_str.find("###");
        if (id_pos_triple != std::string::npos) {
            invisible_label = id_str.substr(id_pos_triple);
        } else {
            size_t id_pos_double = id_str.find("##");
            if (id_pos_double != std::string::npos) {
                invisible_label = "###" + id_str.substr(id_pos_double + 2);
            } else {
                invisible_label = "###" + id_str;
            }
        }

        bool clicked = Button(invisible_label.c_str(), btn_sz);
        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            if (tooltip && *tooltip) {
                SetTooltip("%s", tooltip);
            }
        }

        ImVec2 p_min = GetItemRectMin();
        ImVec2 p_max = GetItemRectMax();
        float font_size = GetFontSize();
        float min_dim = Detail::IconMin(btn_sz.x, btn_sz.y);
        float icon_sz = Detail::IconMin(min_dim * 0.60f, font_size * 1.0f);
        if (icon_sz < 8.0f) icon_sz = Detail::IconMax(min_dim - 4.0f, 6.0f);

        ImVec2 center((p_min.x + p_max.x) * 0.5f, (p_min.y + p_max.y) * 0.5f);
        ImVec2 icon_pos(std::round(center.x - icon_sz * 0.5f), std::round(center.y - icon_sz * 0.5f));

        ImU32 col = icon_color ? GetColorU32(*icon_color) : GetColorU32(ImGuiCol_Text);
        ImDrawList* draw_list = GetWindowDrawList();
        DrawVectorIcon(draw_list, icon_pos, icon_sz, col, icon_name);

        return clicked;
    }

    /// Renders a push button containing both a procedural vector icon and text.
    /// - If icon_on_right == false (default): [Icon] [Text]
    /// - If icon_on_right == true:           [Text] [Icon]
    /// - If size <= (0, 0), auto-sizes to fit the icon, spacing, text, and frame padding.
    /// - Automatically sets ImGuiMouseCursor_Hand on hover.
    /// - Displays tooltip (if provided) when hovered.
    /// - Returns true on click.
    inline IMGUI_API bool ButtonIcon(const char* label,
                                     const char* icon_name,
                                     const ImVec2& size = ImVec2(0.0f, 0.0f),
                                     bool icon_on_right = false,
                                     const char* tooltip = nullptr,
                                     const ImVec4* icon_color = nullptr) {
        if (!label) return false;
        if (!icon_name || !*icon_name) {
            bool clicked = Button(label, size);
            if (IsItemHovered()) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
                if (tooltip && *tooltip) SetTooltip("%s", tooltip);
            }
            return clicked;
        }

        ImGuiStyle& style = GetStyle();
        float frame_h = GetFrameHeight();
        float font_size = GetFontSize();

        // Separate visible text and ID
        std::string full_str(label);
        std::string visible_text = full_str;
        std::string id_part = full_str;
        size_t id_pos_triple = full_str.find("###");
        if (id_pos_triple != std::string::npos) {
            visible_text = full_str.substr(0, id_pos_triple);
            id_part = full_str.substr(id_pos_triple + 3);
        } else {
            size_t id_pos_double = full_str.find("##");
            if (id_pos_double != std::string::npos) {
                visible_text = full_str.substr(0, id_pos_double);
                id_part = full_str.substr(id_pos_double + 2);
            }
        }

        float icon_sz = std::round(font_size * 0.90f);
        ImVec2 text_sz = visible_text.empty() ? ImVec2(0.0f, 0.0f) : CalcTextSize(visible_text.c_str());
        float spacing = visible_text.empty() ? 0.0f : style.ItemInnerSpacing.x;
        float content_w = visible_text.empty() ? icon_sz : (icon_sz + spacing + text_sz.x);

        ImVec2 btn_sz = size;
        if (btn_sz.x <= 0.0f) btn_sz.x = visible_text.empty() ? frame_h : (content_w + style.FramePadding.x * 2.0f);
        if (btn_sz.y <= 0.0f) btn_sz.y = frame_h;

        std::string invisible_label = "###" + id_part;
        bool clicked = Button(invisible_label.c_str(), btn_sz);
        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            if (tooltip && *tooltip) SetTooltip("%s", tooltip);
        }

        ImVec2 p_min = GetItemRectMin();
        ImVec2 p_max = GetItemRectMax();
        ImDrawList* draw_list = GetWindowDrawList();
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        ImU32 col = icon_color ? GetColorU32(*icon_color) : text_col;

        float start_x = std::round((p_min.x + p_max.x - content_w) * 0.5f);
        if (visible_text.empty()) {
            ImVec2 icon_pos(start_x, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
            DrawVectorIcon(draw_list, icon_pos, icon_sz, col, icon_name);
        } else if (!icon_on_right) {
            ImVec2 icon_pos(start_x, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
            DrawVectorIcon(draw_list, icon_pos, icon_sz, col, icon_name);
            ImVec2 text_pos(start_x + icon_sz + spacing, std::round((p_min.y + p_max.y - text_sz.y) * 0.5f));
            draw_list->AddText(text_pos, text_col, visible_text.c_str());
        } else {
            ImVec2 text_pos(start_x, std::round((p_min.y + p_max.y - text_sz.y) * 0.5f));
            draw_list->AddText(text_pos, text_col, visible_text.c_str());
            ImVec2 icon_pos(start_x + text_sz.x + spacing, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
            DrawVectorIcon(draw_list, icon_pos, icon_sz, col, icon_name);
        }

        return clicked;
    }

#endif // !CONTRIB_DISABLE_ICON_BUTTON

} // namespace ImGui

#endif // !CONTRIB_DISABLE_ICONS

#if defined(IMGUI_MARKDOWN_READY) && defined(CONTRIB_ICONS_AVAILABLE) && !defined(IMGUI_MARKDOWN_ICONS_BRIDGE_DEFINED)
#define IMGUI_MARKDOWN_ICONS_BRIDGE_DEFINED
namespace ImGui {
namespace MarkdownDetail {
    struct MarkdownVectorIconBridgeRunner {
        MarkdownVectorIconBridgeRunner() {
            GetDrawVectorIconFn() = &ImGui::DrawVectorIcon;
        }
    };
    static MarkdownVectorIconBridgeRunner s_MarkdownVectorIconBridgeRunner;
} // namespace MarkdownDetail
} // namespace ImGui
#endif

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_ICONS_H
