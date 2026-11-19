// ============================================================================
// ImGee Extension: Two-Layer Zoomable Viewport (imgui_zoomable_viewport.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE)
//
// Standalone Dear ImGui helper for rendering unscaled 1.0x header/toolbar/
// footer chrome alongside a scrollable Nx-zoomed ([0.50x .. 2.50x]) inner
// content child window, with native Ctrl+MouseWheel (and Cmd+MouseWheel on
// macOS) zoom support and zero host-SDK dependencies.
//
// Usage:
//   static ImGui::ZoomableViewportState s_DocZoom;
//
//   // 1. Fixed 1.0x header / toolbar chrome:
//   ImGui::TextUnformatted("Flight Operations Manual");
//   ImGui::SameLine();
//   ImGui::ZoomViewportControls(s_DocZoom);
//
//   // 2. Scrollable Nx-zoomed content body (always pair Begin/End!):
//   if (ImGui::BeginZoomedViewport("##DocBody", s_DocZoom, ImVec2(0.0f, -28.0f))) {
//       ImGui::MarkdownExt(markdownText, markdownLen, &mdCfg);
//   }
//   ImGui::EndZoomedViewport();
//
//   // 3. Fixed 1.0x footer chrome:
//   ImGui::TextDisabled("Page 1 of 4");
// ============================================================================

#pragma once

#ifndef IMGUI_ZOOMABLE_VIEWPORT_H
#define IMGUI_ZOOMABLE_VIEWPORT_H

#if !defined(IMGEE_DISABLE_ZOOMABLE_VIEWPORT) && \
    !defined(CONTRIB_DISABLE_ZOOMABLE_VIEWPORT) && \
    !defined(IMGUI_DISABLE_ZOOMABLE_VIEWPORT)

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
#include <vector>

#ifndef IMGEE_INTERNAL_COMPAT_DEFINED
#define IMGEE_INTERNAL_COMPAT_DEFINED
namespace ImGui {
namespace ImGeeCompat {
    inline void PushFont(ImFont* font) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        ImGui::PushFont(font, font ? font->LegacySize : 0.0f);
#else
        ImGui::PushFont(font);
#endif
    }
    inline void Image(ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0)) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19190)
        if (border_col.w > 0.0f) {
            const float border_size = ImGui::GetStyle().ImageBorderSize;
            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, border_size > 1.0f ? border_size : 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, border_col);
            ImGui::ImageWithBg(user_texture_id, image_size, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0.0f);
            ImGui::ImageWithBg(user_texture_id, image_size, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
            ImGui::PopStyleVar();
        }
#else
        ImGui::Image(user_texture_id, image_size, uv0, uv1, tint_col, border_col);
#endif
    }
    inline void SetWindowFontScale(float scale) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200) && defined(IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window) {
            window->FontWindowScale = scale;
            ImGui::UpdateCurrentFontSize(0.0f);
        }
#else
        ImGui::SetWindowFontScale(scale);
#endif
    }
} // namespace ImGeeCompat
} // namespace ImGui
#endif // IMGEE_INTERNAL_COMPAT_DEFINED

namespace ImGui {

struct ZoomableViewportState {
    float  zoom              = 1.0f;                 // Active content zoom multiplier (1.0f == 100%)
    float  default_zoom      = 1.0f;                 // Baseline zoom restored by ResetZoom()
    float  last_zoom         = 1.0f;                 // A/B undo memory for ToggleResetZoom()
    float  min_zoom          = 0.50f;                // Minimum clamped zoom (default: 50%)
    float  max_zoom          = 2.50f;                // Maximum clamped zoom (default: 250%)
    float  wheel_step        = 0.05f;                // Zoom increment per mouse-wheel notch (5%)
    float  button_step       = 0.10f;                // Zoom increment per [-]/[+] button press (10%)
    bool   enable_ctrl_wheel = true;                 // Enable Ctrl+Wheel (and Cmd+Wheel on macOS)
    bool   border            = false;                // Draw outer border around the content child
    ImVec2 content_padding   = ImVec2(-1.0f, -1.0f); // Optional inner padding (-1 = inherit style.WindowPadding)

    // Internal scope state managed by BeginZoomedViewport / EndZoomedViewport:
    int    _pushed_style_vars       = 0;
    float  _saved_parent_font_scale = 1.0f;
    bool   _zoom_changed_this_frame = false;

    float Clamp(float candidate) const {
        const float lo = (min_zoom > 0.05f) ? min_zoom : 0.05f;
        const float hi = (max_zoom >= lo) ? max_zoom : lo;
        if (candidate < lo) return lo;
        if (candidate > hi) return hi;
        return candidate;
    }

    bool SetZoom(float target_zoom) {
        const float clamped = std::round(Clamp(target_zoom) * 1000.0f) / 1000.0f;
        if (std::fabs(clamped - zoom) > 0.0005f) {
            zoom = clamped;
            return true;
        }
        zoom = clamped;
        return false;
    }

    bool StepZoom(float delta) {
        return SetZoom(zoom + delta);
    }

    bool ResetZoom() {
        const float base = Clamp(default_zoom);
        if (std::fabs(zoom - base) > 0.0005f) {
            last_zoom = zoom;
            zoom = base;
            return true;
        }
        zoom = base;
        return false;
    }

    bool ToggleResetZoom() {
        const float base = Clamp(default_zoom);
        if (std::fabs(zoom - base) > 0.0005f) {
            last_zoom = zoom;
            zoom = base;
            return true;
        }
        const float prev = Clamp(last_zoom);
        if (std::fabs(prev - base) > 0.0005f) {
            zoom = prev;
            return true;
        }
        return false;
    }

    int GetZoomPercent() const {
        return static_cast<int>(std::round(zoom * 100.0f));
    }
};

namespace ZoomViewportDetail {
    inline std::vector<ZoomableViewportState*>& GetActiveViewportStack() {
        static std::vector<ZoomableViewportState*> s_Stack;
        return s_Stack;
    }

    inline bool IsZoomModifierHeld(const ImGuiIO& io) {
#if defined(__APPLE__)
        return io.KeyCtrl || io.KeySuper;
#else
        return io.KeyCtrl || (io.ConfigMacOSXBehaviors && io.KeySuper);
#endif
    }
} // namespace ZoomViewportDetail

inline bool SetViewportZoom(ZoomableViewportState& state, float target_zoom) {
    return state.SetZoom(target_zoom);
}

inline bool StepViewportZoom(ZoomableViewportState& state, float delta) {
    return state.StepZoom(delta);
}

inline bool ResetViewportZoom(ZoomableViewportState& state, bool allow_undo_toggle = false) {
    return allow_undo_toggle ? state.ToggleResetZoom() : state.ResetZoom();
}

// Begins a scrollable inner child viewport whose content is scaled by state.zoom,
// while leaving all surrounding parent window chrome at its native 1.0x scale.
// Always pair with ImGui::EndZoomedViewport() (unconditionally, matching ImGui::BeginChild).
inline bool BeginZoomedViewport(
    const char* str_id,
    ZoomableViewportState& state,
    const ImVec2& size = ImVec2(0.0f, 0.0f),
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar)
{
    state.zoom = state.Clamp(state.zoom);
    state._pushed_style_vars = 0;
    state._zoom_changed_this_frame = false;

    ImGuiWindow* parent_win = ImGui::GetCurrentWindow();
    state._saved_parent_font_scale = parent_win ? parent_win->FontWindowScale : 1.0f;

    const bool has_custom_padding = (state.content_padding.x >= 0.0f && state.content_padding.y >= 0.0f);
    if (has_custom_padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, state.content_padding);
        state._pushed_style_vars++;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const bool mod_held = state.enable_ctrl_wheel && ZoomViewportDetail::IsZoomModifierHeld(io);

    ImGuiWindowFlags effective_flags = window_flags;
    if (mod_held) {
        // Prevent vertical page scroll jump while Ctrl/Cmd+MouseWheel zooming
        effective_flags |= ImGuiWindowFlags_NoScrollWithMouse;
    }

    const bool use_window_padding = has_custom_padding || state.border;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19090)
    ImGuiChildFlags child_flags = state.border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
    if (use_window_padding) child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
    const bool visible = ImGui::BeginChild(str_id ? str_id : "##imgee_zoomed_viewport", size, child_flags, effective_flags);
#elif defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
    ImGuiChildFlags child_flags = state.border ? ImGuiChildFlags_Border : ImGuiChildFlags_None;
    if (use_window_padding) child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
    const bool visible = ImGui::BeginChild(str_id ? str_id : "##imgee_zoomed_viewport", size, child_flags, effective_flags);
#else
    if (use_window_padding) effective_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
    const bool visible = ImGui::BeginChild(str_id ? str_id : "##imgee_zoomed_viewport", size, state.border, effective_flags);
#endif

    ZoomViewportDetail::GetActiveViewportStack().push_back(&state);

    if (visible && mod_held && std::fabs(io.MouseWheel) > 0.0001f) {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
            const float wheel = io.MouseWheel;
            const float notches = (std::fabs(wheel) < 1.0f) ? wheel : std::round(wheel);
            state._zoom_changed_this_frame = state.StepZoom(notches * state.wheel_step);
        }
    }

    ImGui::ImGeeCompat::SetWindowFontScale(state.zoom);
    return visible;
}

inline void EndZoomedViewport(ZoomableViewportState& state) {
    std::vector<ZoomableViewportState*>& stack = ZoomViewportDetail::GetActiveViewportStack();
    if (!stack.empty() && stack.back() == &state) {
        stack.pop_back();
    } else {
        for (size_t i = stack.size(); i > 0; --i) {
            if (stack[i - 1] == &state) {
                stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i - 1));
                break;
            }
        }
    }

    ImGui::EndChild();

    if (state._pushed_style_vars > 0) {
        ImGui::PopStyleVar(state._pushed_style_vars);
        state._pushed_style_vars = 0;
    }

    // Ensure parent window chrome scale remains pristine after exiting child scope
    ImGuiWindow* restored_win = ImGui::GetCurrentWindow();
    if (restored_win && std::fabs(restored_win->FontWindowScale - state._saved_parent_font_scale) > 0.0001f) {
        ImGui::ImGeeCompat::SetWindowFontScale(state._saved_parent_font_scale);
    }
}

inline void EndZoomedViewport() {
    std::vector<ZoomableViewportState*>& stack = ZoomViewportDetail::GetActiveViewportStack();
    if (!stack.empty() && stack.back() != nullptr) {
        ZoomableViewportState* top = stack.back();
        EndZoomedViewport(*top);
    } else {
        if (!stack.empty()) stack.pop_back();
        ImGui::EndChild();
    }
}

// Compact 1.0x toolbar control cluster: [-] [100%] [+]
// Clicking the percentage button toggles between default_zoom and the previous custom zoom.
inline bool ZoomViewportControls(ZoomableViewportState& state, const char* id_suffix = "##imgee_zoom_ctrl") {
    bool changed = false;
    ImGui::PushID(id_suffix ? id_suffix : "##imgee_zoom_ctrl");

    const float lo = (state.min_zoom > 0.05f) ? state.min_zoom : 0.05f;
    const float hi = (state.max_zoom >= lo) ? state.max_zoom : lo;
    const bool at_min = (state.zoom <= lo + 0.001f);
    const bool at_max = (state.zoom >= hi - 0.001f);

    const float btn_h = ImGui::GetFrameHeight();
    const ImVec2 step_sz(btn_h, btn_h);

    if (at_min) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.45f);
    if (ImGui::Button("-##zoom_out", step_sz) && !at_min) {
        changed |= state.StepZoom(-state.button_step);
    }
    if (!at_min && ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (at_min) ImGui::PopStyleVar();

    ImGui::SameLine(0.0f, 3.0f);

    char pct_label[32];
    std::snprintf(pct_label, sizeof(pct_label), "%d%%##zoom_reset", state.GetZoomPercent());
    const float pct_w = ImMax(52.0f, ImGui::CalcTextSize("250%").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    if (ImGui::Button(pct_label, ImVec2(pct_w, btn_h))) {
        changed |= state.ToggleResetZoom();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    ImGui::SameLine(0.0f, 3.0f);

    if (at_max) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.45f);
    if (ImGui::Button("+##zoom_in", step_sz) && !at_max) {
        changed |= state.StepZoom(state.button_step);
    }
    if (!at_max && ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (at_max) ImGui::PopStyleVar();

    ImGui::PopID();
    return changed;
}

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // !IMGEE_DISABLE_ZOOMABLE_VIEWPORT
#endif // IMGUI_ZOOMABLE_VIEWPORT_H
