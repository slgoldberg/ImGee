// ============================================================================
// ImGui Contrib: Directional Popup Menus & Select Pulldown Widgets
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Description:
//   Provides first-class popup placement, anchoring, and selection widgets:
//   1. Directional Item Anchoring:
//      - OpenPopupAtItem() / BeginAnchoredPopup():
//        Anchors popups relative to the preceding item's bounding box
//        (ImGui::GetItemRectMin/Max).
//      - Supports ImGuiPopupAnchor_AboveItem (blossoms UP for status bars docked
//        at the screen bottom) or ImGuiPopupAnchor_BelowItem (standard dropdown).
//      - Font-scale invariant: anchors to live pixel metrics with sub-pixel
//        precision, eliminating hardcoded magic offsets and text-zoom drift.
//      - Muscle-memory toggle safety: clicking the opener button when the popup
//        is open cleanly toggles it closed without re-opening flicker.
//      - Headroom reporting: ImGui::GetLastPopupHeight(str_id) lets window
//        managers (e.g. WidgetImgWindow) query the exact pixel height needed
//        for XPLM window expansion without manual "boxel" guessing.
//
//   2. SelectPulldown() Widget:
//      - Compact 1-line inline summary dropdown for dataref selectors (IS_SELECT).
//      - Renders active option with down-caret indicator.
//      - Scroll pass-through when closed: mouse-wheel events are NOT captured,
//        allowing parent scrolling lists to scroll smoothly without mutating settings.
//      - Mouse-wheel cycling when open: spinning the wheel inside the open popup
//        cycles active options dynamically without closing the menu.
//      - "Lens UI" open-menu tooltip latching: per-item tooltips dynamically target
//        the newly selected item during wheel navigation until mouse moves deliberately (> 6 px).
//      - Predictable step direction and circular wrapping: configurable via 'wrap'
//        and 'natural_list_order' parameters.
//      - Automatic focus tracking and scroll-into-view (SetItemDefaultFocus / SetScrollHereY).
//      - Optional rich Markdown per-item tooltips and custom widths.
// ============================================================================

#pragma once

#ifndef IMGUI_POPUP_MENUS_H
#define IMGUI_POPUP_MENUS_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#if !defined(IMGEE_DISABLE_WIDGETS) && \
    !defined(IMGEE_DISABLE_POPUP_MENUS) && \
    !defined(CONTRIB_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_CONTRIB_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_DISABLE_POPUP_MENUS) && \
    !defined(IMGUI_DISABLE_CONTRIB_POPUP_MENUS)

#include "imgui.h"
#if defined(__has_include)
    #if __has_include("imgui_internal.h")
        #include "imgui_internal.h"
    #endif
#else
    #include "imgui_internal.h"
#endif
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cfloat>

#if !defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && \
    !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_CONTRIB_MOUSE_WHEEL_CONTROLS) && \
    !defined(IMGUI_DISABLE_MOUSE_WHEEL_CONTROLS) && \
    defined(__has_include)
    #if __has_include("imgui_mouse_wheel_controls.h")
        #include "imgui_mouse_wheel_controls.h"
    #elif __has_include("widgets/imgui_mouse_wheel_controls.h")
        #include "widgets/imgui_mouse_wheel_controls.h"
    #endif
#endif

#if !defined(IMGUI_TOOLTIP_EXT_H) && \
    !defined(IMGEE_DISABLE_EXTENSIONS) && \
    !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && \
    !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_CONTRIB_TIMED_TOOLTIP) && \
    !defined(IMGUI_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_TIMED_TOOLTIP) && \
    defined(__has_include)
    #if __has_include("../extensions/TimedTooltip/imgui_tooltip_ext.h")
        #include "../extensions/TimedTooltip/imgui_tooltip_ext.h"
    #elif __has_include("imgui_tooltip_ext.h")
        #include "imgui_tooltip_ext.h"
    #endif
#endif

#if !defined(IMGUI_MARKDOWN_EXT_H) && \
    !defined(IMGEE_DISABLE_EXTENSIONS) && \
    !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGEE_DISABLE_MARKDOWN) && \
    !defined(CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN) && \
    !defined(IMGUI_DISABLE_CONTRIB_MARKDOWN) && \
    !defined(IMGUI_DISABLE_MARKDOWN) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_MARKDOWN) && \
    defined(__has_include)
    #if __has_include("../extensions/Markdown/imgui_markdown_ext.h")
        #include "../extensions/Markdown/imgui_markdown_ext.h"
    #elif __has_include("imgui_markdown_ext.h")
        #include "imgui_markdown_ext.h"
    #endif
#endif

#ifndef IMGUI_API
#define IMGUI_API
#endif

// Optional compile-time icon strings for SelectPulldown carets:
#ifndef IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN
#define IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN nullptr // e.g. ICON_FA_CHEVRON_DOWN or "  v"
#endif

#ifndef IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP
#define IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP nullptr   // e.g. ICON_FA_CHEVRON_UP or "  ^"
#endif

enum ImGuiPopupAnchor {
    ImGuiPopupAnchor_BelowItem = 0, // Dropdown: popup top anchors below item
    ImGuiPopupAnchor_AboveItem = 1, // Status bar: popup bottom anchors above item
    ImGuiPopupAnchor_Auto      = 2  // Automatically selects Above or Below based on screen space
};

enum ImGuiPopupAlign {
    ImGuiPopupAlign_Left   = 0, // Align popup left edge with item left edge
    ImGuiPopupAlign_Right  = 1, // Align popup right edge with item right edge
    ImGuiPopupAlign_Center = 2, // Horizontally center popup over item
    ImGuiPopupAlign_Auto   = 3  // Left-align, but right-align if near display right boundary
};

namespace ImGui {

    using PopupAnchor = ImGuiPopupAnchor;
    using PopupAlign  = ImGuiPopupAlign;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_BelowItem = ::ImGuiPopupAnchor_BelowItem;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_AboveItem = ::ImGuiPopupAnchor_AboveItem;
    constexpr ImGuiPopupAnchor ImGuiPopupAnchor_Auto      = ::ImGuiPopupAnchor_Auto;
    constexpr ImGuiPopupAnchor PopupAnchor_BelowItem      = ::ImGuiPopupAnchor_BelowItem;
    constexpr ImGuiPopupAnchor PopupAnchor_AboveItem      = ::ImGuiPopupAnchor_AboveItem;
    constexpr ImGuiPopupAnchor PopupAnchor_Auto           = ::ImGuiPopupAnchor_Auto;

    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Left       = ::ImGuiPopupAlign_Left;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Right      = ::ImGuiPopupAlign_Right;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Center     = ::ImGuiPopupAlign_Center;
    constexpr ImGuiPopupAlign  ImGuiPopupAlign_Auto       = ::ImGuiPopupAlign_Auto;
    constexpr ImGuiPopupAlign  PopupAlign_Left            = ::ImGuiPopupAlign_Left;
    constexpr ImGuiPopupAlign  PopupAlign_Right           = ::ImGuiPopupAlign_Right;
    constexpr ImGuiPopupAlign  PopupAlign_Center          = ::ImGuiPopupAlign_Center;
    constexpr ImGuiPopupAlign  PopupAlign_Auto            = ::ImGuiPopupAlign_Auto;

    namespace Detail {
        struct ContextScopedId {
            ImGuiContext* ctx = nullptr;
            ImGuiID       id  = 0;
            bool operator<(const ContextScopedId& rhs) const {
                return (ctx != rhs.ctx) ? (ctx < rhs.ctx) : (id < rhs.id);
            }
        };

        struct AnchoredPopupState {
            ImVec2 anchor_pos  = ImVec2(0.0f, 0.0f);
            ImVec2 item_min    = ImVec2(0.0f, 0.0f);
            ImVec2 item_max    = ImVec2(0.0f, 0.0f);
            ImVec2 last_size   = ImVec2(0.0f, 0.0f);
            ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto;
            ImGuiPopupAlign  align  = ImGuiPopupAlign_Auto;
            bool was_open = false;
            bool force_close = false;
            int  last_frame_seen = -1;
        };

        inline std::map<ContextScopedId, AnchoredPopupState>& GetPopupStateMap() {
            static std::map<ContextScopedId, AnchoredPopupState> s_Map;
            return s_Map;
        }

        inline AnchoredPopupState& GetOrCreatePopupState(ImGuiID id) {
            ContextScopedId key{ ImGui::GetCurrentContext(), id };
            auto& st = GetPopupStateMap()[key];
            const int cur_frame = ImGui::GetFrameCount();
            if (st.last_frame_seen > cur_frame) {
                st = AnchoredPopupState{};
            }
            st.last_frame_seen = cur_frame;
            return st;
        }

        inline AnchoredPopupState* FindPopupState(ImGuiID id) {
            ContextScopedId key{ ImGui::GetCurrentContext(), id };
            auto& m = GetPopupStateMap();
            auto it = m.find(key);
            if (it == m.end()) return nullptr;
            const int cur_frame = ImGui::GetFrameCount();
            if (it->second.last_frame_seen > cur_frame) {
                it->second = AnchoredPopupState{};
                it->second.last_frame_seen = cur_frame;
                return nullptr;
            }
            return &it->second;
        }

        inline void AddRectCompat(ImDrawList* dl, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f) {
            if (!dl) return;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
            dl->AddRect(p_min, p_max, col, rounding, thickness, flags);
#else
            dl->AddRect(p_min, p_max, col, rounding, flags, thickness);
#endif
        }

        inline const char*& GetDefaultPulldownCaretDownStorage() {
            static const char* s_Down = IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_DOWN;
            return s_Down;
        }

        inline const char*& GetDefaultPulldownCaretUpStorage() {
            static const char* s_Up = IMGUI_CONTRIB_DEFAULT_PULLDOWN_CARET_UP;
            return s_Up;
        }

        inline float GetPopupMouseWheel(bool consume = true) {
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_GET_MOUSE_WHEEL) && !defined(CONTRIB_DISABLE_GET_MOUSE_WHEEL)
            return WheelSliderDetail::GetMouseWheelEither(consume);
#else
            ImGuiIO& io = ImGui::GetIO();
            float wheel = io.MouseWheel;
            if (wheel == 0.0f) {
                wheel = io.MouseWheelH;
                if (consume && wheel != 0.0f) {
                    io.MouseWheelH = 0.0f;
                }
            } else if (consume) {
                io.MouseWheel = 0.0f;
            }
            return wheel;
#endif
        }

        inline bool CycleIndex(int* current_idx,
                               int min_idx,
                               int max_idx,
                               float clicks,
                               bool wrap = false,
                               bool natural_list_order = true)
        {
            if (!current_idx || max_idx <= min_idx || clicks == 0.0f)
                return false;

#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
            TimedTooltip::ClearActiveTooltip();
#endif

            int dir = (clicks < 0.0f) ? (natural_list_order ? 1 : -1)
                                      : (natural_list_order ? -1 : 1);

            int prev = *current_idx;
            int count = (max_idx - min_idx) + 1;

            if (wrap) {
                int offset = (*current_idx - min_idx + dir) % count;
                if (offset < 0) offset += count;
                *current_idx = min_idx + offset;
            } else {
                int next = *current_idx + dir;
                if (next < min_idx) next = min_idx;
                if (next > max_idx) next = max_idx;
                *current_idx = next;
            }

            return (*current_idx != prev);
        }

        inline void AttachTooltipProxy(ImGuiID id, const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
    #if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN) && !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN)
            TimedTooltip::TextMDProxy(id, is_hovered, "%s", text);
    #else
            TimedTooltip::TextProxy(id, is_hovered, "%s", text);
    #endif
#else
            // Strict silent fallback when TimedTooltip is disabled (UX Manifesto Tenet VI)
            (void)id;
            (void)is_hovered;
#endif
        }

        // UX Rule #6 strict silent fallback (never falls back to instant-pop ImGui::SetTooltip)
        inline void AttachSilentTimedTooltipProxy(ImGuiID id, const char* text, bool is_hovered)
        {
            if (!text || text[0] == '\0') return;
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
    #if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN) && !defined(IMGUI_CONTRIB_DISABLE_MARKDOWN)
            TimedTooltip::TextMDProxy(id, is_hovered, "%s", text);
    #else
            TimedTooltip::TextProxy(id, is_hovered, "%s", text);
    #endif
#else
            (void)id;
            (void)is_hovered;
#endif
        }
    } // namespace Detail

    // ========================================================================
    // Global Caret Configuration & Procedural Drawing
    // ========================================================================

    /// Configure global default caret strings for SelectPulldown widgets (e.g. ICON_FA_CHEVRON_DOWN, ICON_FA_CHEVRON_UP).
    /// If set to nullptr or "", SelectPulldown automatically renders crisp procedural vector arrows!
    inline IMGUI_API void SetDefaultPulldownCaret(const char* down_caret, const char* up_caret = nullptr) {
        Detail::GetDefaultPulldownCaretDownStorage() = down_caret;
        Detail::GetDefaultPulldownCaretUpStorage() = up_caret;
    }

    /// Returns the global default down-caret string (or nullptr if vector fallback is active).
    inline IMGUI_API const char* GetDefaultPulldownCaretDown() {
        return Detail::GetDefaultPulldownCaretDownStorage();
    }

    /// Returns the global default up-caret string (or nullptr if vector fallback is active).
    inline IMGUI_API const char* GetDefaultPulldownCaretUp() {
        return Detail::GetDefaultPulldownCaretUpStorage();
    }

    /// Procedural vector arrow rendering helper (DPI-scaled equilateral triangle rendered via ImDrawList).
    /// Draws crisp vector arrows without requiring font icons or Unicode Geometric Shapes glyphs.
    inline IMGUI_API void DrawPulldownArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float font_size, float scale = 1.0f) {
        if (!draw_list) return;
        const float h = font_size;
        float r = h * 0.40f * scale;
        ImVec2 center(pos.x + h * 0.50f, pos.y + h * 0.50f * scale);

        ImVec2 a(0.0f, 0.0f), b(0.0f, 0.0f), c(0.0f, 0.0f);
        switch (dir) {
        case ImGuiDir_Up:
        case ImGuiDir_Down:
            if (dir == ImGuiDir_Up) r = -r;
            a = ImVec2(+0.000f * r, +0.750f * r);
            b = ImVec2(-0.866f * r, -0.750f * r);
            c = ImVec2(+0.866f * r, -0.750f * r);
            break;
        case ImGuiDir_Left:
        case ImGuiDir_Right:
            if (dir == ImGuiDir_Left) r = -r;
            a = ImVec2(+0.750f * r, +0.000f * r);
            b = ImVec2(-0.750f * r, +0.866f * r);
            c = ImVec2(-0.750f * r, -0.866f * r);
            break;
        default:
            return;
        }
        draw_list->AddTriangleFilled(ImVec2(center.x + a.x, center.y + a.y),
                                     ImVec2(center.x + b.x, center.y + b.y),
                                     ImVec2(center.x + c.x, center.y + c.y),
                                     col);
    }

    // ========================================================================
    // Headroom Queries (For Window Managers / Subclasses)
    // ========================================================================

    /// Returns the last measured content size of the specified popup window,
    /// or ImVec2(0, 0) if it has not yet been rendered.
    inline IMGUI_API ImVec2 GetLastPopupSize(const char* str_id) {
        if (!str_id || !*str_id) return ImVec2(0.0f, 0.0f);
        ImGuiID id = ImGui::GetID(str_id);
        if (Detail::AnchoredPopupState* st = Detail::FindPopupState(id)) {
            return st->last_size;
        }
        return ImVec2(0.0f, 0.0f);
    }

    /// Returns the last measured height in pixels of the specified popup window.
    inline IMGUI_API float GetLastPopupHeight(const char* str_id) {
        return GetLastPopupSize(str_id).y;
    }

    // ========================================================================
    // Anchored Popup Openers & Chasers
    // ========================================================================

    /// Open a popup anchored to the preceding item (Button, ButtonMD, Pill, Selectable, etc.).
    /// Call immediately after submitting the item:
    ///   if (ImGui::Button("View Mode")) {
    ///       ImGui::OpenPopupAtItem("##ViewMenu", ImGuiPopupAnchor_AboveItem);
    ///   }
    /// Returns true if the popup was opened, or false if it was toggled closed.
    inline IMGUI_API bool OpenPopupAtItem(const char* str_id,
                                          ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto,
                                          ImGuiPopupAlign align = ImGuiPopupAlign_Auto,
                                          ImGuiPopupFlags popup_flags = 0)
    {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        auto& state = Detail::GetOrCreatePopupState(id);

        // Muscle-memory toggle safety: if already open, toggle it closed cleanly
        if (ImGui::IsPopupOpen(str_id, popup_flags)) {
            state.force_close = true;
            state.was_open = false;
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        state.item_min = ImGui::GetItemRectMin();
        state.item_max = ImGui::GetItemRectMax();
        state.anchor   = anchor;
        state.align    = align;

        // Auto resolution based on screen position
        if (state.anchor == ImGuiPopupAnchor_Auto) {
            state.anchor = (state.item_max.y > io.DisplaySize.y * 0.70f)
                ? ImGuiPopupAnchor_AboveItem
                : ImGuiPopupAnchor_BelowItem;
        }
        if (state.align == ImGuiPopupAlign_Auto) {
            state.align = (state.item_max.x > io.DisplaySize.x * 0.75f)
                ? ImGuiPopupAlign_Right
                : ImGuiPopupAlign_Left;
        }

        // Calculate target anchor coordinates
        float target_y = (state.anchor == ImGuiPopupAnchor_AboveItem)
            ? (state.item_min.y - style.ItemSpacing.y)
            : (state.item_max.y + style.ItemSpacing.y);

        float target_x = state.item_min.x;
        if (state.align == ImGuiPopupAlign_Right) {
            target_x = state.item_max.x;
        } else if (state.align == ImGuiPopupAlign_Center) {
            target_x = (state.item_min.x + state.item_max.x) * 0.5f;
        }

        state.anchor_pos = ImVec2(target_x, target_y);
        state.was_open   = true;

        ImGui::OpenPopup(str_id, popup_flags);
        return true;
    }

    /// Open a popup anchored to an explicit screen position.
    inline IMGUI_API bool OpenPopupAtPos(const char* str_id,
                                         ImVec2 pos,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_Auto,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Auto,
                                         ImGuiPopupFlags popup_flags = 0)
    {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        auto& state = Detail::GetOrCreatePopupState(id);

        if (ImGui::IsPopupOpen(str_id, popup_flags)) {
            ImGui::CloseCurrentPopup();
            state.was_open = false;
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        state.item_min = pos;
        state.item_max = pos;
        state.anchor   = anchor;
        state.align    = align;

        if (state.anchor == ImGuiPopupAnchor_Auto) {
            state.anchor = (pos.y > io.DisplaySize.y * 0.70f)
                ? ImGuiPopupAnchor_AboveItem
                : ImGuiPopupAnchor_BelowItem;
        }
        if (state.align == ImGuiPopupAlign_Auto) {
            state.align = (pos.x > io.DisplaySize.x * 0.75f)
                ? ImGuiPopupAlign_Right
                : ImGuiPopupAlign_Left;
        }

        state.anchor_pos = pos;
        state.was_open   = true;

        ImGui::OpenPopup(str_id, popup_flags);
        return true;
    }

    /// Begin a popup window positioned according to its anchor coordinates.
    /// Call in place of standard ImGui::BeginPopup():
    ///   if (ImGui::BeginAnchoredPopup("##ViewMenu")) {
    ///       // ... menu items ...
    ///       ImGui::EndPopup();
    ///   }
    inline IMGUI_API bool BeginAnchoredPopup(const char* str_id, ImGuiWindowFlags flags = 0) {
        if (!str_id || !*str_id) return false;
        ImGuiID id = ImGui::GetID(str_id);
        Detail::AnchoredPopupState* st = Detail::FindPopupState(id);

        if (st && st->was_open) {
            ImVec2 pivot(0.0f, (st->anchor == ImGuiPopupAnchor_AboveItem) ? 1.0f : 0.0f);
            if (st->align == ImGuiPopupAlign_Right) pivot.x = 1.0f;
            else if (st->align == ImGuiPopupAlign_Center) pivot.x = 0.5f;

            if (st->last_size.y > 0.0f) {
                // Known size from previous opening: exact pivot placement
                ImGui::SetNextWindowPos(st->anchor_pos, ImGuiCond_Always, pivot);
            } else {
                // First appearance before measurement: estimate top-left to avoid jumping
                float est_h = ImGui::GetFontSize() * 3.5f;
                float start_y = (st->anchor == ImGuiPopupAnchor_AboveItem)
                    ? (st->anchor_pos.y - est_h)
                    : st->anchor_pos.y;
                ImGui::SetNextWindowPos(ImVec2(st->anchor_pos.x, start_y), ImGuiCond_Always, ImVec2(pivot.x, 0.0f));
            }
        }

        if (st && st->force_close) {
            st->force_close = false;
            st->was_open = false;
            if (ImGui::BeginPopup(str_id, flags)) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            return false;
        }

        bool open = ImGui::BeginPopup(str_id, flags);
        if (open) {
            if (st) {
                st->last_size = ImGui::GetWindowSize();
                st->last_frame_seen = ImGui::GetFrameCount();
            }
        } else {
            if (st) {
                st->was_open = false;
            }
        }
        return open;
    }

    // ========================================================================
    // Hover-Latching Menu Strip (v1.3.0)
    // ========================================================================

    struct MenuStripConfig {
        ImGuiPopupAnchor anchor            = ImGuiPopupAnchor_Auto; // AboveItem (status bar), BelowItem (menu bar), or Auto
        ImGuiPopupAlign  align             = ImGuiPopupAlign_Auto;  // Left, Right, Center, or Auto
        bool             autoSameLine      = true;                  // Automatically places sibling triggers on one row
        float            itemSpacingX      = -1.0f;                 // < 0 uses style.ItemSpacing.x
        bool             showCaret         = true;                  // Draws procedural vector caret matching anchor direction
        bool             flipCaretOnOpen   = true;                  // Flips caret direction while this menu is open
        bool             matchTriggerWidth = true;                  // Enforce popup min-width >= trigger button width
        bool             dynamicCursors    = true;                  // Active ImGuiMouseCursor_Hand on strip triggers
        ImGuiWindowFlags popupFlags        = 0;                     // Extra flags forwarded to BeginAnchoredPopup
    };

    struct MenuStripState {
        bool    is_latched                 = false;                 // True while any menu in this strip is open
        ImGuiID active_menu_id             = 0;                     // ID of currently open menu (0 if closed)
        int     active_menu_index          = -1;                    // 0-based index of currently open menu (-1 if closed)
        ImVec2  last_popup_size            = ImVec2(0.0f, 0.0f);    // Last measured size of active popup (headroom query)

        struct TriggerSlot {
            ImGuiID menu_id  = 0;
            ImVec2  rect_min = ImVec2(0.0f, 0.0f);
            ImVec2  rect_max = ImVec2(0.0f, 0.0f);
            bool    enabled  = true;
        };

        // Internal frame-sync bookkeeping (managed by BeginMenuStrip / BeginStripMenu / EndMenuStrip)
        ImGuiID _hovered_menu_id_prev      = 0;
        ImGuiID _hovered_menu_id_curr      = 0;
        ImGuiID _suppress_click_menu_id    = 0;
        ImGuiID _opened_while_mouse_down_id = 0;
        ImGuiID _pre_switch_target_id      = 0;
        ImVec2  _active_popup_min          = ImVec2(0.0f, 0.0f);
        ImVec2  _active_popup_max          = ImVec2(0.0f, 0.0f);
        bool    _any_open_this_frame       = false;
        int     _item_count                = 0;
        int     _last_frame_seen           = -1;
        std::vector<TriggerSlot> _slots_prev;
        std::vector<TriggerSlot> _slots_curr;
    };

    namespace Detail {
        struct ActiveMenuStripScope {
            ImGuiID         strip_id = 0;
            MenuStripConfig config;
            MenuStripState* state    = nullptr;
        };

        inline std::map<ContextScopedId, MenuStripState>& GetMenuStripStateMap() {
            static std::map<ContextScopedId, MenuStripState> s_Map;
            return s_Map;
        }

        inline std::map<ImGuiContext*, std::vector<ActiveMenuStripScope>>& GetActiveMenuStripStackMap() {
            static std::map<ImGuiContext*, std::vector<ActiveMenuStripScope>> s_Stacks;
            return s_Stacks;
        }

        inline MenuStripState& GetOrCreateMenuStripState(ImGuiID strip_id) {
            ContextScopedId key{ ImGui::GetCurrentContext(), strip_id };
            auto& st = GetMenuStripStateMap()[key];
            const int cur_frame = ImGui::GetFrameCount();
            if (st._last_frame_seen > cur_frame) {
                st = MenuStripState{};
            }
            st._last_frame_seen = cur_frame;
            return st;
        }

        inline MenuStripState* FindMenuStripState(ImGuiID strip_id) {
            ContextScopedId key{ ImGui::GetCurrentContext(), strip_id };
            auto& m = GetMenuStripStateMap();
            auto it = m.find(key);
            if (it == m.end()) return nullptr;
            const int cur_frame = ImGui::GetFrameCount();
            if (it->second._last_frame_seen > cur_frame) {
                it->second = MenuStripState{};
                it->second._last_frame_seen = cur_frame;
                return nullptr;
            }
            return &it->second;
        }
    } // namespace Detail

    /// Opens a hover-latching horizontal menu strip scope.
    /// Passing state = nullptr uses internal (ImGuiContext*, ImGuiID)-keyed storage automatically.
    /// Always pair with EndMenuStrip().
    inline IMGUI_API bool BeginMenuStrip(const char* str_id,
                                         const MenuStripConfig* config = nullptr,
                                         MenuStripState* state = nullptr)
    {
        const char* safe_id = (str_id && str_id[0] != '\0') ? str_id : "##imgee_menu_strip";
        const ImGuiID strip_id = ImGui::GetID(safe_id);
        ImGui::PushID(static_cast<int>(strip_id));

        MenuStripState* st = state ? state : &Detail::GetOrCreateMenuStripState(strip_id);
        const int cur_frame = ImGui::GetFrameCount();
        if (st->_last_frame_seen > cur_frame) {
            *st = MenuStripState{};
        }
        st->_last_frame_seen = cur_frame;

        // Release stale click guards once LMB is no longer held or releasing
        if (!ImGui::IsMouseDown(0) && !ImGui::IsMouseReleased(0)) {
            st->_suppress_click_menu_id     = 0;
            st->_opened_while_mouse_down_id = 0;
        }

        // Determine which enabled trigger slot the mouse is actually over on this frame
        // (ignoring any slot hidden underneath the currently open popup rect).
        ImGuiID live_mouse_slot_id = 0;
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            const ImGuiIO& io = ImGui::GetIO();
            const bool has_pop = (st->_active_popup_max.x > st->_active_popup_min.x &&
                                  st->_active_popup_max.y > st->_active_popup_min.y);
            const bool in_pop = has_pop &&
                                (io.MousePos.x >= st->_active_popup_min.x && io.MousePos.x <= st->_active_popup_max.x &&
                                 io.MousePos.y >= st->_active_popup_min.y && io.MousePos.y <= st->_active_popup_max.y);
            if (!in_pop) {
                for (size_t si = 0; si < st->_slots_prev.size(); ++si) {
                    const MenuStripState::TriggerSlot& sl = st->_slots_prev[si];
                    if (sl.enabled &&
                        io.MousePos.x >= sl.rect_min.x && io.MousePos.x <= sl.rect_max.x &&
                        io.MousePos.y >= sl.rect_min.y && io.MousePos.y <= sl.rect_max.y)
                    {
                        live_mouse_slot_id = sl.menu_id;
                        break;
                    }
                }
            }
        }

        // Muscle-memory toggle guard: if the user pressed LMB directly over the currently open
        // trigger, Dear ImGui's popup dismissal will close the popup before Button() fires on
        // release. Record _suppress_click_menu_id so the release click does NOT bounce it back open.
        // Clicking a *sibling* trigger leaves _suppress_click_menu_id == 0 so it switches cleanly!
        if (st->active_menu_id != 0 && ImGui::IsMouseClicked(0)) {
            const bool clicking_active_trigger = (live_mouse_slot_id != 0)
                ? (live_mouse_slot_id == st->active_menu_id)
                : (st->_hovered_menu_id_prev == st->active_menu_id);
            if (clicking_active_trigger) {
                st->_suppress_click_menu_id = st->active_menu_id;
                st->is_latched = false;
            }
        }

        st->_hovered_menu_id_curr = 0;
        st->_pre_switch_target_id = 0;
        st->_any_open_this_frame  = false;
        st->_item_count           = 0;
        st->_slots_curr.clear();

        // Pre-pass hover-switch detection: if latched and mouse has moved onto an enabled sibling
        // trigger, suppress rendering the outgoing popup on this frame so forward glides (A -> B)
        // and direct sibling clicks transition cleanly with zero 1-frame dual-popup overlap!
        if (st->is_latched && st->active_menu_id != 0 &&
            live_mouse_slot_id != 0 && live_mouse_slot_id != st->active_menu_id)
        {
            st->_pre_switch_target_id = live_mouse_slot_id;
        }

        Detail::ActiveMenuStripScope scope;
        scope.strip_id = strip_id;
        if (config) scope.config = *config;
        scope.state = st;
        Detail::GetActiveMenuStripStackMap()[ImGui::GetCurrentContext()].push_back(scope);
        return true;
    }

    /// Submits a menu strip trigger button and opens its anchored popup scope if open.
    /// anchor_override / align_override default to -1 (inherit MenuStripConfig::anchor / align).
    /// Returns true while the menu popup is open (caller MUST call EndStripMenu() iff true).
    inline IMGUI_API bool BeginStripMenu(const char* label,
                                         const char* tooltip_md = nullptr,
                                         bool enabled = true,
                                         float width = 0.0f,
                                         int anchor_override = -1,
                                         int align_override = -1)
    {
        auto& stack = Detail::GetActiveMenuStripStackMap()[ImGui::GetCurrentContext()];
        if (stack.empty() || !stack.back().state) return false;

        Detail::ActiveMenuStripScope& scope = stack.back();
        MenuStripState* st = scope.state;
        const MenuStripConfig& cfg = scope.config;
        const int item_idx = st->_item_count++;

        if (cfg.autoSameLine && item_idx > 0) {
            ImGui::SameLine(0.0f, cfg.itemSpacingX);
        }

        const char* safe_label = label ? label : "";
        const ImGuiID menu_id = ImGui::GetID(safe_label);
        char popup_id[64];
        std::snprintf(popup_id, sizeof(popup_id), "##MenuStrip_%08X", static_cast<unsigned int>(menu_id));

        const ImGuiPopupAnchor resolved_anchor = (anchor_override >= 0)
            ? static_cast<ImGuiPopupAnchor>(anchor_override)
            : cfg.anchor;
        const ImGuiPopupAlign resolved_align = (align_override >= 0)
            ? static_cast<ImGuiPopupAlign>(align_override)
            : cfg.align;

        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiStyle& style = ImGui::GetStyle();
        const float font_size = ImGui::GetFontSize();
        const float btn_h = ImGui::GetFrameHeight();
        const float pad_x = style.FramePadding.x;

        // Determine visible label text (strip ##ID suffix)
        const char* label_end = std::strstr(safe_label, "##");
        if (!label_end) label_end = safe_label + std::strlen(safe_label);
        const ImVec2 text_sz = (label_end > safe_label)
            ? ImGui::CalcTextSize(safe_label, label_end, false)
            : ImVec2(0.0f, font_size);

        const float caret_w = cfg.showCaret ? (font_size * 0.72f) : 0.0f;
        const float caret_gap = (cfg.showCaret && label_end > safe_label) ? (pad_x * 0.65f) : 0.0f;
        const float min_btn_w = pad_x * 2.0f + text_sz.x + caret_gap + caret_w;
        float btn_w = min_btn_w;
        if (width > 0.0f) {
            btn_w = width;
        } else if (width < 0.0f) {
            btn_w = (std::max)(min_btn_w, ImGui::GetContentRegionAvail().x + width);
        }

        const bool was_open_before = ImGui::IsPopupOpen(popup_id);
        const bool show_active_style = (st->_pre_switch_target_id != 0)
            ? (st->_pre_switch_target_id == menu_id)
            : (was_open_before || (st->is_latched && st->active_menu_id == menu_id));

        if (!enabled) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18500)
            ImGui::BeginDisabled(true);
#else
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * 0.5f);
#endif
        }

        if (show_active_style) {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ButtonActive]);
        }

        char btn_id[80];
        std::snprintf(btn_id, sizeof(btn_id), "##StripBtn_%08X", static_cast<unsigned int>(menu_id));
        const bool raw_clicked = ImGui::Button(btn_id, ImVec2(btn_w, btn_h));
        const bool clicked = enabled && raw_clicked;

        if (show_active_style) {
            ImGui::PopStyleColor(2);
        }

        if (!enabled) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18500)
            ImGui::EndDisabled();
#else
            ImGui::PopStyleVar();
#endif
        }

        const ImVec2 item_min = ImGui::GetItemRectMin();
        const ImVec2 item_max = ImGui::GetItemRectMax();
        {
            MenuStripState::TriggerSlot slot;
            slot.menu_id  = menu_id;
            slot.rect_min = item_min;
            slot.rect_max = item_max;
            slot.enabled  = enabled;
            st->_slots_curr.push_back(slot);
        }

        // Guard against accidentally hover-switching to a sibling trigger hidden behind the open popup
        const bool has_popup_rect = (st->_active_popup_max.x > st->_active_popup_min.x &&
                                     st->_active_popup_max.y > st->_active_popup_min.y);
        const bool mouse_in_active_popup = has_popup_rect &&
                                           (io.MousePos.x >= st->_active_popup_min.x && io.MousePos.x <= st->_active_popup_max.x &&
                                            io.MousePos.y >= st->_active_popup_min.y && io.MousePos.y <= st->_active_popup_max.y);

        ImGuiHoveredFlags hov_flags = ImGuiHoveredFlags_AllowWhenBlockedByPopup;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18500)
        if (!enabled) hov_flags |= ImGuiHoveredFlags_AllowWhenDisabled;
#endif
        const bool raw_hovered = ImGui::IsItemHovered(hov_flags) && !mouse_in_active_popup;
        const bool btn_hovered = enabled && raw_hovered;

        if (btn_hovered) {
            st->_hovered_menu_id_curr = menu_id;
            if (cfg.dynamicCursors) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }

        // Draw crisp trigger label + optional procedural vector caret + Paper Light active ring
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 txt_col = ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        if (show_active_style) {
            const ImVec4 win_bg = style.Colors[ImGuiCol_WindowBg];
            const bool is_light = (win_bg.x * 0.299f + win_bg.y * 0.587f + win_bg.z * 0.114f) > 0.55f;
            if (is_light) {
                Detail::AddRectCompat(dl, item_min, item_max, IM_COL32(37, 99, 215, 190), style.FrameRounding, 0, 1.0f);
            }
        }

        const float text_x = item_min.x + pad_x;
        const float text_y = item_min.y + (btn_h - text_sz.y) * 0.5f;
        const float clip_right = cfg.showCaret ? (item_max.x - pad_x - caret_w - caret_gap * 0.5f) : (item_max.x - pad_x);
        if (label_end > safe_label && clip_right > text_x) {
            const ImVec4 clip_rect(text_x, item_min.y, clip_right, item_max.y);
            dl->AddText(ImGui::GetFont(), font_size, ImVec2(text_x, text_y), txt_col,
                        safe_label, label_end, 0.0f, &clip_rect);
        }

        if (cfg.showCaret) {
            ImGuiPopupAnchor eff_anchor = resolved_anchor;
            if (eff_anchor == ImGuiPopupAnchor_Auto) {
                eff_anchor = (item_max.y > io.DisplaySize.y * 0.70f)
                    ? ImGuiPopupAnchor_AboveItem
                    : ImGuiPopupAnchor_BelowItem;
            }
            const bool points_up = (eff_anchor == ImGuiPopupAnchor_AboveItem)
                ? (cfg.flipCaretOnOpen ? !show_active_style : true)
                : (cfg.flipCaretOnOpen ?  show_active_style : false);
            const float arrow_x = item_max.x - pad_x - font_size * 0.85f;
            const float arrow_y = item_min.y + (btn_h - font_size) * 0.5f;
            DrawPulldownArrow(dl, ImVec2(arrow_x, arrow_y), txt_col,
                              points_up ? ImGuiDir_Up : ImGuiDir_Down, font_size, 0.85f);
        }

        // Open / Hover-Switch / Toggle-Close state transitions
        auto openThisMenu = [&]() {
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
            TimedTooltip::ClearActiveTooltip();
#endif
            if (OpenPopupAtItem(popup_id, resolved_anchor, resolved_align)) {
                st->is_latched              = true;
                st->active_menu_id          = menu_id;
                st->active_menu_index       = item_idx;
                st->_suppress_click_menu_id = 0;
            }
        };

        const bool sibling_direct_click = enabled && btn_hovered && ImGui::IsMouseClicked(0) &&
                                          (st->active_menu_id != 0) && (st->active_menu_id != menu_id);
        const bool hover_switch = enabled && st->is_latched && btn_hovered &&
                                  !was_open_before && (st->active_menu_id != menu_id);

        if (clicked) {
            if (st->_suppress_click_menu_id == menu_id) {
                // Swallow mouse-release after muscle-memory toggle close on mouse-down
                st->_suppress_click_menu_id = 0;
                st->is_latched = false;
                if (was_open_before) {
                    Detail::AnchoredPopupState& pst = Detail::GetOrCreatePopupState(ImGui::GetID(popup_id));
                    pst.force_close = true;
                    pst.was_open = false;
                }
            } else if (st->_opened_while_mouse_down_id == menu_id) {
                // Already switched/opened on mouse-down of this exact click: keep it open on release!
                st->_opened_while_mouse_down_id = 0;
            } else if (was_open_before) {
                // Single-frame toggle close
                OpenPopupAtItem(popup_id, resolved_anchor, resolved_align);
                st->is_latched = false;
                st->active_menu_id = 0;
                st->active_menu_index = -1;
            } else {
                openThisMenu();
            }
        } else if (sibling_direct_click || hover_switch) {
            openThisMenu();
            if (ImGui::IsMouseDown(0)) {
                st->_opened_while_mouse_down_id = menu_id;
            }
        }

        // Stationary TimedTooltip on closed triggers (suppressed while any menu in the strip is open)
        if (tooltip_md && tooltip_md[0] != '\0') {
            const bool allow_tip = !st->is_latched && !was_open_before && raw_hovered;
            Detail::AttachSilentTimedTooltipProxy(menu_id, tooltip_md, allow_tip);
        }

        if (cfg.matchTriggerWidth) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(btn_w, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
        }

        // If a sibling trigger is taking over on this frame, skip rendering the outgoing popup
        if (st->_pre_switch_target_id != 0 && st->_pre_switch_target_id != menu_id) {
            return false;
        }

        const bool popup_open = BeginAnchoredPopup(popup_id, cfg.popupFlags);
        if (popup_open) {
            const ImVec2 p_pos = ImGui::GetWindowPos();
            const ImVec2 p_sz  = ImGui::GetWindowSize();
            st->_any_open_this_frame = true;
            st->is_latched           = true;
            st->active_menu_id       = menu_id;
            st->active_menu_index    = item_idx;
            st->last_popup_size      = p_sz;
            st->_active_popup_min    = p_pos;
            st->_active_popup_max    = ImVec2(p_pos.x + p_sz.x, p_pos.y + p_sz.y);
            return true;
        }
        return false;
    }

    inline IMGUI_API void EndStripMenu() {
        ImGui::EndPopup();
    }

    inline IMGUI_API void EndMenuStrip() {
        auto& stack = Detail::GetActiveMenuStripStackMap()[ImGui::GetCurrentContext()];
        if (!stack.empty()) {
            MenuStripState* st = stack.back().state;
            if (st) {
                st->_hovered_menu_id_prev = st->_hovered_menu_id_curr;
                st->_slots_prev.swap(st->_slots_curr);
                st->_slots_curr.clear();
                st->_pre_switch_target_id = 0;
                if (!ImGui::IsMouseDown(0)) {
                    st->_suppress_click_menu_id     = 0;
                    st->_opened_while_mouse_down_id = 0;
                }
                if (!st->_any_open_this_frame) {
                    st->is_latched        = false;
                    st->active_menu_id    = 0;
                    st->active_menu_index = -1;
                    st->_active_popup_min = ImVec2(0.0f, 0.0f);
                    st->_active_popup_max = ImVec2(0.0f, 0.0f);
                }
            }
            stack.pop_back();
        }
        ImGui::PopID();
    }

    /// Returns true if any menu inside the specified menu strip is currently open.
    inline IMGUI_API bool IsMenuStripOpen(const char* str_id) {
        if (!str_id || !*str_id) return false;
        const ImGuiID strip_id = ImGui::GetID(str_id);
        const MenuStripState* st = Detail::FindMenuStripState(strip_id);
        return st ? st->is_latched : false;
    }

    /// Returns the last measured pixel size of the active popup inside the specified menu strip.
    inline IMGUI_API ImVec2 GetMenuStripLastPopupSize(const char* str_id) {
        if (!str_id || !*str_id) return ImVec2(0.0f, 0.0f);
        const ImGuiID strip_id = ImGui::GetID(str_id);
        const MenuStripState* st = Detail::FindMenuStripState(strip_id);
        return st ? st->last_popup_size : ImVec2(0.0f, 0.0f);
    }

    // ========================================================================
    // SelectPulldown Widget (For Settings & Inline Datarefs)
    // ========================================================================

    /// Renders a compact 1-line dataref / radio pulldown selector.
    /// - When closed: Renders active item with caret (vector arrow or custom icon font).
    ///   Mouse-wheel events are NOT captured, allowing parent container scrolling without accidental edits.
    /// - When open: Mouse-wheel cycles active selection dynamically without closing popup.
    /// - "Lens UI" tooltip latching: When wheel is spun inside popup, per-item tooltips target
    ///   the newly selected item (*current_item) until mouse moves deliberately (> 6 px).
    /// - Supports directional anchoring (ImGuiPopupAnchor_BelowItem or ImGuiPopupAnchor_AboveItem).
    /// - Optional per-item tooltips displayed on option hover (using TimedTooltip and Markdown if available).
    /// - Supports circular wrapping (wrap = true) and natural list ordering (natural_list_order).
    inline IMGUI_API bool SelectPulldown(const char* label,
                                         int* current_item,
                                         const char* const items[],
                                         int items_count,
                                         const char* const tooltips[] = nullptr,
                                         float width = 0.0f,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                         bool wrap = false,
                                         bool natural_list_order = true)
    {
        if (!current_item || !items || items_count <= 0) return false;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float font_size = ImGui::GetFontSize();
        float btn_h = ImGui::GetFrameHeight();
        float pad_x = style.FramePadding.x;

        ImGuiID id = ImGui::GetID(label);
        char popup_id[64];
        std::snprintf(popup_id, sizeof(popup_id), "##SelectPulldown_%08X", (unsigned int)id);

        static ImGuiID s_WheelLatchPulldownID = 0;
        static ImVec2  s_WheelLatchMousePos = ImVec2(-1.0f, -1.0f);

        int cur_idx = *current_item;
        if (cur_idx < 0) cur_idx = 0;
        if (cur_idx >= items_count) cur_idx = items_count - 1;

        const char* preview_text = items[cur_idx];
        if (!preview_text) preview_text = "";

        bool is_open = ImGui::IsPopupOpen(popup_id);

        // Reset wheel latch if popup closed
        if (!is_open && s_WheelLatchPulldownID == id) {
            s_WheelLatchPulldownID = 0;
        }

        // Determine caret direction & custom icon string:
        // Anchored above (e.g. status bar): closed points UP (▲), open points DOWN (▼).
        // Anchored below (e.g. dropdown): closed points DOWN (▼), open points UP (▲).
        bool points_up = (anchor == ImGuiPopupAnchor_AboveItem) ? !is_open : is_open;

        const char* down_caret = GetDefaultPulldownCaretDown();
        const char* up_caret   = GetDefaultPulldownCaretUp();
        const char* custom_icon = points_up ? (up_caret ? up_caret : down_caret) : down_caret;
        bool has_custom_icon = (custom_icon != nullptr && custom_icon[0] != '\0');

        // Calculate button width:
        ImVec2 text_sz = ImGui::CalcTextSize(preview_text, nullptr, true);
        float arrow_w = has_custom_icon 
            ? ImGui::CalcTextSize(custom_icon, nullptr, true).x 
            : (font_size * 0.85f);
        float arrow_zone_w = arrow_w + pad_x;
        float min_btn_w = text_sz.x + pad_x * 2.0f + arrow_zone_w;

        float btn_w = min_btn_w;
        if (width > 0.0f) {
            btn_w = width;
        } else if (width < 0.0f) {
            btn_w = (std::max)(min_btn_w, ImGui::GetContentRegionAvail().x + width);
        }

        if (is_open) {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
        }

        std::string btn_id = std::string("##") + popup_id;
        bool clicked = ImGui::Button(btn_id.c_str(), ImVec2(btn_w, btn_h));
        ImGuiHoveredFlags hover_flags = is_open ? ImGuiHoveredFlags_AllowWhenBlockedByPopup : ImGuiHoveredFlags_None;
        bool btn_hovered = ImGui::IsItemHovered(hover_flags);
        if (btn_hovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        if (is_open) {
            ImGui::PopStyleColor();
        }

        // Draw caret / arrow docked at the right edge of the button, and clip preview
        // text strictly to the left of the caret zone so narrow explicit widths never collide:
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 icon_col = ImGui::GetColorU32(ImGuiCol_Text);

        float icon_left_x = item_max.x - pad_x;
        if (has_custom_icon) {
            ImVec2 icon_sz = ImGui::CalcTextSize(custom_icon, nullptr, true);
            icon_left_x = item_max.x - pad_x - icon_sz.x;
            ImVec2 icon_pos(icon_left_x, item_min.y + (btn_h - icon_sz.y) * 0.5f);
            draw_list->AddText(icon_pos, icon_col, custom_icon);
        } else {
            ImGuiDir arrow_dir = points_up ? ImGuiDir_Up : ImGuiDir_Down;
            icon_left_x = item_max.x - pad_x - font_size * 0.85f;
            ImVec2 arrow_pos(icon_left_x, item_min.y + (btn_h - font_size) * 0.5f);
            DrawPulldownArrow(draw_list, arrow_pos, icon_col, arrow_dir, font_size, 1.0f);
        }

        const char* preview_end = std::strstr(preview_text, "##");
        if (!preview_end) {
            preview_end = preview_text + std::strlen(preview_text);
        }
        float text_clip_max_x = (std::max)(item_min.x + pad_x, icon_left_x - pad_x * 0.5f);
        if (preview_end > preview_text && text_clip_max_x > item_min.x + pad_x) {
            ImVec4 clip_rect(item_min.x + pad_x, item_min.y, text_clip_max_x, item_max.y);
            ImVec2 text_pos(item_min.x + pad_x, item_min.y + (btn_h - text_sz.y) * 0.5f);
            draw_list->AddText(ImGui::GetFont(), font_size, text_pos, icon_col,
                               preview_text, preview_end, 0.0f, &clip_rect);
        }

        bool value_changed = false;
        if (clicked) {
            OpenPopupAtItem(popup_id, anchor, align);
        }

        // Enforce popup minimum width to match button width
        ImGui::SetNextWindowSizeConstraints(ImVec2(btn_w, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        // Render options inside open popup
        if (BeginAnchoredPopup(popup_id)) {
            // Break the wheel-selection tooltip latch only when the mouse moves deliberately (> 6 px),
            // ignoring tiny 1-2 px physical mouse jiggles while spinning a notched scroll wheel:
            if (s_WheelLatchPulldownID == id) {
                if (std::abs(io.MousePos.x - s_WheelLatchMousePos.x) > 6.0f ||
                    std::abs(io.MousePos.y - s_WheelLatchMousePos.y) > 6.0f) {
                    s_WheelLatchPulldownID = 0;
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
                    TimedTooltip::ClearActiveTooltip();
#endif
                }
            }

            bool popup_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) ||
                                 ImGui::IsWindowHovered() ||
                                 ImGui::IsAnyItemHovered();
            bool wheel_stepped = false;

            // In-menu wheel cycling: allow spinning while hovering popup items OR the opener button!
            if (popup_hovered || btn_hovered) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                float clicks = Detail::GetPopupMouseWheel(true);
                if (clicks != 0.0f) {
                    s_WheelLatchPulldownID = id;
                    s_WheelLatchMousePos = io.MousePos;
#if defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && !defined(IMGUI_CONTRIB_DISABLE_TIMED_TOOLTIP)
                    TimedTooltip::ClearActiveTooltip();
#endif
                    if (Detail::CycleIndex(current_item, 0, items_count - 1, clicks, wrap, natural_list_order)) {
                        value_changed = true;
                        wheel_stepped = true;
                    }
                }
            }

            for (int i = 0; i < items_count; i++) {
                ImGui::PushID(i);
                const char* item_text = items[i] ? items[i] : "*Unknown item*";
                bool is_selected = (i == *current_item);
                if (ImGui::Selectable(item_text, is_selected)) {
                    s_WheelLatchPulldownID = 0;
                    if (*current_item != i) {
                        *current_item = i;
                        value_changed = true;
                    }
                }
                bool item_hovered = ImGui::IsItemHovered();
                if (item_hovered) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                if (tooltips && tooltips[i] && tooltips[i][0] != '\0') {
                    bool tooltip_target = (s_WheelLatchPulldownID == id)
                                          ? ((i == *current_item) && !wheel_stepped)
                                          : item_hovered;
                    Detail::AttachTooltipProxy(ImGui::GetItemID(), tooltips[i], tooltip_target);
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                    if (wheel_stepped) {
                        ImGui::SetScrollHereY();
                    }
                }
                ImGui::PopID();
            }

            ImGui::EndPopup();
        }

        return value_changed;
    }

    /// std::vector overload of SelectPulldown
    inline IMGUI_API bool SelectPulldown(const char* label,
                                         int* current_item,
                                         const std::vector<std::string>& items,
                                         const std::vector<std::string>* tooltips = nullptr,
                                         float width = 0.0f,
                                         ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                         ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                         bool wrap = false,
                                         bool natural_list_order = true)
    {
        if (!current_item || items.empty()) return false;
        std::vector<const char*> item_ptrs;
        item_ptrs.reserve(items.size());
        for (const auto& s : items) item_ptrs.push_back(s.c_str());

        std::vector<const char*> tip_ptrs;
        if (tooltips && tooltips->size() == items.size()) {
            tip_ptrs.reserve(tooltips->size());
            for (const auto& t : *tooltips) tip_ptrs.push_back(t.c_str());
        }

        return SelectPulldown(label, current_item, item_ptrs.data(), (int)item_ptrs.size(),
                              tip_ptrs.empty() ? nullptr : tip_ptrs.data(), width, anchor, align,
                              wrap, natural_list_order);
    }

    // ========================================================================
    // Explicit Markdown Shorthands & Sub-Namespace Aliases
    // ========================================================================

    /// Explicit Markdown alias: ImGui::SelectPulldownMD(...)
    inline IMGUI_API bool SelectPulldownMD(const char* label,
                                           int* current_item,
                                           const char* const items[],
                                           int items_count,
                                           const char* const tooltips[] = nullptr,
                                           float width = 0.0f,
                                           ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                           ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                           bool wrap = false,
                                           bool natural_list_order = true)
    {
        return SelectPulldown(label, current_item, items, items_count, tooltips, width, anchor, align, wrap, natural_list_order);
    }

    /// Explicit Markdown alias (std::vector overload): ImGui::SelectPulldownMD(...)
    inline IMGUI_API bool SelectPulldownMD(const char* label,
                                           int* current_item,
                                           const std::vector<std::string>& items,
                                           const std::vector<std::string>* tooltips = nullptr,
                                           float width = 0.0f,
                                           ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                           ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                           bool wrap = false,
                                           bool natural_list_order = true)
    {
        return SelectPulldown(label, current_item, items, tooltips, width, anchor, align, wrap, natural_list_order);
    }

    namespace Markdown {
        /// Sub-namespace alias: ImGui::Markdown::SelectPulldown(...) / ImGui::MD::SelectPulldown(...)
        inline bool SelectPulldown(const char* label,
                                   int* current_item,
                                   const char* const items[],
                                   int items_count,
                                   const char* const tooltips[] = nullptr,
                                   float width = 0.0f,
                                   ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                   ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                   bool wrap = false,
                                   bool natural_list_order = true)
        {
            return ImGui::SelectPulldown(label, current_item, items, items_count, tooltips, width, anchor, align, wrap, natural_list_order);
        }

        inline bool SelectPulldown(const char* label,
                                   int* current_item,
                                   const std::vector<std::string>& items,
                                   const std::vector<std::string>* tooltips = nullptr,
                                   float width = 0.0f,
                                   ImGuiPopupAnchor anchor = ImGuiPopupAnchor_BelowItem,
                                   ImGuiPopupAlign align = ImGuiPopupAlign_Left,
                                   bool wrap = false,
                                   bool natural_list_order = true)
        {
            return ImGui::SelectPulldown(label, current_item, items, tooltips, width, anchor, align, wrap, natural_list_order);
        }
    } // namespace Markdown

    namespace MD = Markdown;

} // namespace ImGui

#endif // !CONTRIB_DISABLE_POPUP_MENUS

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_POPUP_MENUS_H
