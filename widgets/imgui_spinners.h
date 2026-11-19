// ============================================================================
// ImGui Contrib Widgets: Procedural Spinners & Countdown Dials (imgui_spinners.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Standalone C++ immediate-mode activity spinners and radial countdown timer
// dials for Dear ImGui. Procedurally rendered via ImDrawList antialiased paths
// with zero image textures, zero font dependencies, and smooth frame-rate
// independent animations.
// ============================================================================

#pragma once

#if !defined(IMGEE_DISABLE_WIDGETS) && \
    !defined(IMGEE_DISABLE_SPINNERS) && \
    !defined(IMGEE_DISABLE_SPINNER) && \
    !defined(CONTRIB_DISABLE_SPINNERS) && \
    !defined(CONTRIB_DISABLE_SPINNER) && \
    !defined(IMGUI_CONTRIB_DISABLE_SPINNERS) && \
    !defined(IMGUI_CONTRIB_DISABLE_SPINNER) && \
    !defined(IMGUI_DISABLE_CONTRIB_SPINNERS) && \
    !defined(IMGUI_DISABLE_CONTRIB_SPINNER) && \
    !defined(IMGUI_DISABLE_EXTRA_SPINNERS) && \
    !defined(IMGUI_DISABLE_EXTRA_SPINNER)
#ifndef IMGUI_SPINNERS_H
#define IMGUI_SPINNERS_H
#define IMGUI_EXTRA_SPINNERS_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include "imgui.h"
#if defined(__has_include)
    #if __has_include("imgui_internal.h")
        #include "imgui_internal.h"
    #elif __has_include(<imgui_internal.h>)
        #include <imgui_internal.h>
    #endif
#else
    #include "imgui_internal.h"
#endif
#include <cmath>
#include <algorithm>

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

    // -------------------------------------------------------------------------
    // 1. Procedural Activity Spinner
    // -------------------------------------------------------------------------
    // Renders an organic, smooth rotating activity arc for loading or busy states.
    // Animates automatically using ImGui::GetTime() * speed.
    inline IMGUI_API bool Spinner(const char* label, float radius = 0.0f, float thickness = -1.0f, ImU32 color = 0, float speed = 1.0f) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        if (radius <= 0.0f) {
            radius = GetFontSize() * 0.55f;
        }
        if (thickness <= 0.0f) {
            thickness = ImMax(2.0f, radius * 0.22f);
        }
        if (color == 0) {
            color = GetColorU32(ImGuiCol_CheckMark);
            if ((color & 0xFF000000) == 0) {
                color = GetColorU32(ImGuiCol_ButtonActive);
            }
        }

        const char* text_end = FindRenderedTextEnd(label);
        const bool has_label = (text_end > label);
        ImVec2 text_size = has_label ? CalcTextSize(label, text_end, true) : ImVec2(0.0f, 0.0f);

        ImVec2 pos = window->DC.CursorPos;
        float total_width = radius * 2.0f;
        if (has_label) {
            total_width += style.ItemInnerSpacing.x + text_size.x;
        }
        float total_height = ImMax(radius * 2.0f, text_size.y);

        const ImRect bb(pos, ImVec2(pos.x + total_width, pos.y + total_height));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;
        ButtonBehavior(bb, id, nullptr, nullptr);
        Detail::RegisterItemTooltipToggle(true);

        // Animate arc using sinusoidal expansion and contraction
        float t = (float)GetTime() * speed;
        float start_angle = t * 3.5f;
        float min_arc = 3.14159265f * 0.15f;
        float max_arc = 3.14159265f * 1.50f;
        float arc = min_arc + (max_arc - min_arc) * (0.5f + 0.5f * std::sin(t * 2.0f));

        ImVec2 center(pos.x + radius, pos.y + total_height * 0.5f);
        float track_radius = radius - thickness * 0.5f;

        // Background track (subtle translucent ring):
        ImU32 track_col = (color & 0x00FFFFFF) | 0x28000000;
        window->DrawList->AddCircle(center, track_radius, track_col, 32, thickness);

        // Animated foreground arc:
        window->DrawList->PathClear();
        window->DrawList->PathArcTo(center, track_radius, start_angle, start_angle + arc, 32);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        window->DrawList->PathStroke(color, thickness, false);
#else
        window->DrawList->PathStroke(color, false, thickness);
#endif

        // Optional text label beside spinner:
        if (has_label) {
            ImVec2 text_pos(pos.x + radius * 2.0f + style.ItemInnerSpacing.x, pos.y + (total_height - text_size.y) * 0.5f);
            RenderText(text_pos, label, text_end);
        }

        return true;
    }

    inline IMGUI_API bool Spinner(const char* label, float radius, float thickness, const ImVec4& color, float speed = 1.0f) {
        return Spinner(label, radius, thickness, ColorConvertFloat4ToU32(color), speed);
    }

    // -------------------------------------------------------------------------
    // 2. Radial Countdown Dial
    // -------------------------------------------------------------------------
    // Renders a circular countdown dial representing fraction_remaining (0.0f .. 1.0f).
    // Begins at 12 o'clock (-90 deg) and sweeps clockwise (or counter-clockwise).
    inline IMGUI_API bool RadialTimer(const char* label, float fraction_remaining, float radius = 0.0f, float thickness = -1.0f,
                                      ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        if (radius <= 0.0f) {
            radius = GetFontSize() * 0.55f;
        }
        if (thickness <= 0.0f) {
            thickness = ImMax(2.0f, radius * 0.22f);
        }
        if (color == 0) {
            color = GetColorU32(ImGuiCol_PlotHistogram);
            if ((color & 0xFF000000) == 0) {
                color = GetColorU32(ImGuiCol_CheckMark);
            }
        }

        const char* text_end = FindRenderedTextEnd(label);
        const bool has_label = (text_end > label);
        ImVec2 text_size = has_label ? CalcTextSize(label, text_end, true) : ImVec2(0.0f, 0.0f);

        ImVec2 pos = window->DC.CursorPos;
        float total_width = radius * 2.0f;
        if (has_label) {
            total_width += style.ItemInnerSpacing.x + text_size.x;
        }
        float total_height = ImMax(radius * 2.0f, text_size.y);

        const ImRect bb(pos, ImVec2(pos.x + total_width, pos.y + total_height));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;
        ButtonBehavior(bb, id, nullptr, nullptr);
        Detail::RegisterItemTooltipToggle(true);

        ImVec2 center(pos.x + radius, pos.y + total_height * 0.5f);
        float track_radius = radius - thickness * 0.5f;

        // Background track:
        ImU32 track_col = (bg_color != 0) ? bg_color : ((color & 0x00FFFFFF) | 0x33000000);
        window->DrawList->AddCircle(center, track_radius, track_col, 32, thickness);

        // Progress arc starting at 12 o'clock (-PI / 2):
        float frac = ImClamp(fraction_remaining, 0.0f, 1.0f);
        if (frac > 0.0f) {
            const float kPi = 3.14159265358979323846f;
            float start_angle = -kPi * 0.5f;
            float sweep = frac * 2.0f * kPi;
            float end_angle = clockwise ? (start_angle + sweep) : (start_angle - sweep);

            window->DrawList->PathClear();
            window->DrawList->PathArcTo(center, track_radius, start_angle, end_angle, 32);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            window->DrawList->PathStroke(color, thickness, false);
#else
            window->DrawList->PathStroke(color, false, thickness);
#endif
        }

        // Optional text label beside timer dial:
        if (has_label) {
            ImVec2 text_pos(pos.x + radius * 2.0f + style.ItemInnerSpacing.x, pos.y + (total_height - text_size.y) * 0.5f);
            RenderText(text_pos, label, text_end);
        }

        return true;
    }

    inline IMGUI_API bool RadialTimer(const char* label, float fraction_remaining, float radius, float thickness,
                                      const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        ImU32 c = ColorConvertFloat4ToU32(color);
        ImU32 bg = bg_color ? ColorConvertFloat4ToU32(*bg_color) : 0;
        return RadialTimer(label, fraction_remaining, radius, thickness, c, bg, clockwise);
    }

    // -------------------------------------------------------------------------
    // 3. Countdown Timer (Seconds Helper)
    // -------------------------------------------------------------------------
    // Convenience wrapper accepting current remaining seconds and total max seconds.
    inline IMGUI_API bool CountdownTimer(const char* label, float current_sec, float max_sec, float radius = 0.0f, float thickness = -1.0f,
                                         ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        float frac = (max_sec > 0.0f) ? (current_sec / max_sec) : 0.0f;
        return RadialTimer(label, frac, radius, thickness, color, bg_color, clockwise);
    }

    inline IMGUI_API bool CountdownTimer(const char* label, float current_sec, float max_sec, float radius, float thickness,
                                         const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        float frac = (max_sec > 0.0f) ? (current_sec / max_sec) : 0.0f;
        return RadialTimer(label, frac, radius, thickness, color, bg_color, clockwise);
    }

    // Friendly C++ aliases:
    inline IMGUI_API bool ActivitySpinner(const char* label, float radius = 0.0f, float thickness = -1.0f, ImU32 color = 0, float speed = 1.0f) {
        return Spinner(label, radius, thickness, color, speed);
    }
    inline IMGUI_API bool ActivitySpinner(const char* label, float radius, float thickness, const ImVec4& color, float speed = 1.0f) {
        return Spinner(label, radius, thickness, color, speed);
    }
    inline IMGUI_API bool ProgressDial(const char* label, float fraction_remaining, float radius = 0.0f, float thickness = -1.0f,
                                      ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        return RadialTimer(label, fraction_remaining, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool ProgressDial(const char* label, float fraction_remaining, float radius, float thickness,
                                      const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        return RadialTimer(label, fraction_remaining, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool CountdownDial(const char* label, float current_sec, float max_sec, float radius = 0.0f, float thickness = -1.0f,
                                        ImU32 color = 0, ImU32 bg_color = 0, bool clockwise = true) {
        return CountdownTimer(label, current_sec, max_sec, radius, thickness, color, bg_color, clockwise);
    }
    inline IMGUI_API bool CountdownDial(const char* label, float current_sec, float max_sec, float radius, float thickness,
                                        const ImVec4& color, const ImVec4* bg_color = nullptr, bool clockwise = true) {
        return CountdownTimer(label, current_sec, max_sec, radius, thickness, color, bg_color, clockwise);
    }

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_SPINNERS_H
#endif // Collision guards
