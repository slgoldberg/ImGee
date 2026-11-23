// ============================================================================
// ImGee Extension: Standalone Markdown Document Viewer (imgui_doc_viewer.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE)
//
// Self-contained, 100% standalone composite Markdown manual & reference viewer
// combining a fixed 1.0x two-row header bar (responsive title, repeating
// [-]/[+] zoom steppers, mouse-wheel zoom slider, 1-click A/B zoom reset/undo,
// optional host window mode hooks, ATX Table-of-Contents section pulldown with
// 1-click scroll-to-top / return-to-reading-spot reset, zero-shift opacity and
// transparency controls, and 5-preset theme picker) with an Nx-zoomed
// ([0.50x .. 2.50x]) MarkdownExt body supporting internal #heading-slug
// navigation, image lightboxes, and crisp 1.0x footer chrome.
//
// Zero X-Plane / ImgWindow dependencies -- compiles standalone against pure
// upstream Dear ImGui (v1.84+ through v1.92+).
//
// Opt-Out Configuration:
//   #define IMGEE_DISABLE_DOC_VIEWER
// ============================================================================

#pragma once

#if !defined(IMGEE_DISABLE_EXTENSIONS) && \
    !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGEE_DISABLE_DOC_VIEWER) && \
    !defined(CONTRIB_DISABLE_DOC_VIEWER) && \
    !defined(IMGUI_DISABLE_DOC_VIEWER)

#ifndef IMGUI_DOC_VIEWER_H
#define IMGUI_DOC_VIEWER_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#if defined(__has_include)
    #if __has_include("imgui.h")
        #include "imgui.h"
    #else
        #include <imgui.h>
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

// Pull in sibling ImGee extensions & widgets when available (respecting all IMGEE_DISABLE_* flags)
#if defined(__has_include)
    #if __has_include("../TimedTooltip/imgui_tooltip_ext.h")
        #include "../TimedTooltip/imgui_tooltip_ext.h"
    #endif
    #if __has_include("../Markdown/imgui_markdown_ext.h")
        #include "../Markdown/imgui_markdown_ext.h"
    #endif
    #if __has_include("../Typography/imgui_font_roles.h")
        #include "../Typography/imgui_font_roles.h"
    #endif
    #if __has_include("../Viewport/imgui_zoomable_viewport.h")
        #include "../Viewport/imgui_zoomable_viewport.h"
    #endif
    #if __has_include("../../widgets/imgui_icons.h")
        #include "../../widgets/imgui_icons.h"
    #endif
    #if __has_include("../../widgets/imgui_popup_menus.h")
        #include "../../widgets/imgui_popup_menus.h"
    #endif
    #if __has_include("../../widgets/imgui_mouse_wheel_controls.h")
        #include "../../widgets/imgui_mouse_wheel_controls.h"
    #endif
    #if __has_include("../../widgets/imgui_toggle_buttons.h")
        #include "../../widgets/imgui_toggle_buttons.h"
    #endif
    #if __has_include("../../widgets/imgui_clickable_links.h")
        #include "../../widgets/imgui_clickable_links.h"
    #endif
    #if __has_include("../../themes/imgui_styles.h")
        #include "../../themes/imgui_styles.h"
    #endif
#else
    #include "../TimedTooltip/imgui_tooltip_ext.h"
    #include "../Markdown/imgui_markdown_ext.h"
    #include "../Typography/imgui_font_roles.h"
    #include "../Viewport/imgui_zoomable_viewport.h"
    #include "../../widgets/imgui_icons.h"
    #include "../../widgets/imgui_popup_menus.h"
    #include "../../widgets/imgui_mouse_wheel_controls.h"
    #include "../../widgets/imgui_toggle_buttons.h"
    #include "../../widgets/imgui_clickable_links.h"
    #include "../../themes/imgui_styles.h"
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
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

// ----------------------------------------------------------------------------
// 1. Public Types: TOC Entries, Theme Enum, Viewer Config & Persistent State
// ----------------------------------------------------------------------------

enum DocumentViewerTheme_ {
    DocumentViewerTheme_Charcoal = 0, // Sleek slate/graphite studio default
    DocumentViewerTheme_Rusty    = 1, // Warm aviation bronze / terracotta
    DocumentViewerTheme_Dark     = 2, // Stock Dear ImGui Dark
    DocumentViewerTheme_Classic  = 3, // Stock Dear ImGui Classic
    DocumentViewerTheme_Light    = 4, // High-contrast daytime Light
    DocumentViewerTheme_COUNT    = 5
};
typedef int DocumentViewerTheme;

struct DocumentTocEntry {
    int         level         = 1;     // ATX heading level (1 = #, 2 = ##, 3 = ###)
    std::string title;                 // Stripped human-readable heading title
    std::string display_label;         // Indented label shown in the Section Jump pulldown
    std::string slug;                  // GitHub-compatible #anchor slug (deduplicated with -1, -2...)
    size_t      byte_offset   = 0;     // Byte offset in source Markdown buffer
    float       content_y     = -1.0f; // Local scroll Y inside the zoomed viewport (-1 until rendered)
};

#if defined(IMGUI_MARKDOWN_H)
typedef MarkdownConfig DocumentViewerMarkdownConfig;
typedef MarkdownLinkCallback DocumentViewerLinkCallback;
#else
struct DocumentViewerMarkdownConfigPlaceholder {};
typedef DocumentViewerMarkdownConfigPlaceholder DocumentViewerMarkdownConfig;
typedef void DocumentViewerLinkCallback(const char* url, void* userData);
#endif

struct DocumentViewerConfig {
    // --- Title & Asset Discovery ---
    const char* title            = nullptr; // Full title (> 540px); auto-uses first # H1 if nullptr
    const char* medium_title     = nullptr; // Medium title (400..540px); auto-shortened if nullptr
    const char* compact_title    = nullptr; // Compact title (<= 400px); auto-shortened if nullptr
    bool        showTitleIcon    = true;    // Draw procedural book/manual vector icon beside title
    const char* baseDir          = nullptr; // Base directory for relative Markdown images (with images/ fallback)
    const char* externalPdfPath  = nullptr; // Optional companion PDF path/URL (adds [PDF] button in Row 1)

    // --- Markdown & Layout Behavior ---
    const DocumentViewerMarkdownConfig* markdownConfig = nullptr;
    bool   enableImageLightbox   = true;    // Click inline images to inspect in high-contrast lightbox
    bool   showImageCaptions     = true;    // Show [alt] / caption=... beneath Markdown figures
    bool   enableTableTooltips   = true;    // Delayed tristate sort & resize tooltips on GFM tables
    int    maxTocHeadingLevel    = 3;       // Maximum ATX heading depth included in TOC (1..3)
    float  tocWidth              = 220.0f;  // Target width for Row 2 Section Jump SelectPulldown
    bool   scaleRow2WithZoom     = false;   // Keep Row 2 at 1.0x chrome scale by default (true = scale with zoom)
    bool   showHeaderRow1          = true;    // Title + Zoom suite + optional host window buttons
    bool   showHeaderRow2          = true;    // TOC Section Jump + Opacity/Transparent + Theme picker
    bool   showScrollResetButton   = true;    // [Reset / Restore] 1-click jump-to-top / return-to-reading-spot button beside TOC
    bool   showOpacityControls     = true;    // Show Opacity/Saturation slider + Transparent/Custom toggle
    bool   showThemePicker         = true;    // Show 5-preset Theme MouseWheelCombo
    bool   showFooter              = true;    // Show compact 1.0x footer bar at bottom of viewer
    bool   showFooterCloseButton   = false;   // Show [Close] button on right side of footer
    const char* footerHint         = nullptr; // Optional left footer override text

    // --- Optional Host Window Chrome Hooks (All hidden by default in standalone mode) ---
    bool   showFitButton           = false;   // [Fit] proportional window resize button
    bool   showCursorToggle        = false;   // Dynamic cursor affordance toggle button
    bool   showPopOutButton        = false;   // [Pop-Out / Pop-In] OS window mode button
    bool   showVrButton            = false;   // [VR] move-into-VR button (shown when !isInVr)
    bool   showCloseButton         = false;   // [X] close button at right end of Row 1
    bool   showReloadButton        = false;   // [Reload] two-arc sync button beside TOC pulldown in Row 2
    bool   isPoppedOut           = false;   // Switches Transparent->Custom & Opacity->Saturation + Pop-In icon
    bool   isInVr                = false;   // Switches Pop button to Pop-In and hides [VR] button

    // --- Optional Host Event Callbacks ---
    void*  userData              = nullptr;
    void (*onZoomChanged)(float oldZoom, float newZoom, void* userData)       = nullptr;
    void (*onFitRequested)(float currentZoom, void* userData)                 = nullptr;
    void (*onCursorToggled)(bool cursorsEnabled, void* userData)              = nullptr;
    void (*onPopOutToggled)(bool wantPopOut, void* userData)                  = nullptr;
    void (*onVrRequested)(void* userData)                                     = nullptr;
    void (*onCloseRequested)(void* userData)                                  = nullptr;
    void (*onReloadRequested)(void* userData)                                 = nullptr;
    void (*onTransparencyToggled)(bool transparentHud, void* userData)        = nullptr;
    void (*onOpacityChanged)(float opacity, void* userData)                   = nullptr;
    void (*onThemeChanged)(int themeIndex, void* userData)                    = nullptr;
#if defined(IMGUI_MARKDOWN_H)
    MarkdownLinkCallback* externalLinkCallback                                = nullptr;
#endif
};

struct DocumentViewerState {
#if defined(IMGUI_ZOOMABLE_VIEWPORT_H) && !defined(IMGEE_DISABLE_ZOOMABLE_VIEWPORT) && !defined(CONTRIB_DISABLE_ZOOMABLE_VIEWPORT) && !defined(IMGUI_DISABLE_ZOOMABLE_VIEWPORT)
    ZoomableViewportState viewport;
#else
    struct FallbackViewportState {
        float  zoom            = 1.0f;
        float  default_zoom    = 1.0f;
        float  last_zoom       = 1.0f;
        float  min_zoom        = 0.50f;
        float  max_zoom        = 2.50f;
        float  wheel_step      = 0.05f;
        float  button_step     = 0.05f;
        bool   border          = false;
        ImVec2 content_padding = ImVec2(20.0f, 14.0f);
        float Clamp(float v) const { return (v < min_zoom) ? min_zoom : ((v > max_zoom) ? max_zoom : v); }
        bool SetZoom(float z) { float c = std::round(Clamp(z) * 1000.0f) / 1000.0f; bool ch = std::fabs(c - zoom) > 0.0005f; zoom = c; return ch; }
        bool StepZoom(float d) { return SetZoom(zoom + d); }
        bool ToggleResetZoom() {
            float b = Clamp(default_zoom);
            if (std::fabs(zoom - b) > 0.0005f) { last_zoom = zoom; zoom = b; return true; }
            float p = Clamp(last_zoom);
            if (std::fabs(p - b) > 0.0005f) { zoom = p; return true; }
            return false;
        }
    } viewport;
#endif

    int         theme_index             = DocumentViewerTheme_Charcoal;
    bool        transparent_hud         = true;
    float       window_opacity          = 0.80f;
    float       prev_window_opacity     = 0.80f;
    bool        dynamic_cursors_enabled = true;
    bool        is_open                 = true;

    // Table of Contents, Scroll Reset & Anchor Jump State
    std::vector<DocumentTocEntry> toc;
    int         active_toc_index         = 0;
    int         pending_scroll_toc_index = -1;
    std::string pending_anchor_slug;
    float       last_scroll_y            = 0.0f;
    float       saved_scroll_y           = 0.0f;
    float       pending_raw_scroll_y     = -1.0f;

    // Internal Repeat & Cache Bookkeeping
    int         _dec_repeat_count     = 0;
    int         _inc_repeat_count     = 0;
    float       _slider_start_zoom    = 1.0f;
    uint64_t    _cached_doc_hash      = 0;
    size_t      _cached_doc_len       = 0;
    int         _cached_max_toc_level = 0;
    bool        _toc_initialized      = false;
    std::string _auto_doc_title;

    DocumentViewerState() {
        viewport.min_zoom        = 0.50f;
        viewport.max_zoom        = 2.50f;
        viewport.wheel_step      = 0.05f;
        viewport.button_step     = 0.05f;
        viewport.content_padding = ImVec2(20.0f, 14.0f);
    }

    void InvalidateTocCache() {
        _toc_initialized = false;
        _cached_doc_hash = 0;
        _cached_doc_len  = 0;
    }
};

// ----------------------------------------------------------------------------
// 2. Slug Generation, ATX TOC Parser, & Theme Application Helpers
// ----------------------------------------------------------------------------

namespace DocViewerDetail {

    inline uint64_t HashDocumentFnv1a(const char* data, size_t len) {
        uint64_t h = 14695981039346656037ull;
        if (!data) return h;
        for (size_t i = 0; i < len; ++i) {
            h ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
            h *= 1099511628211ull;
        }
        return h;
    }

    inline ImGuiID GetItemIDCompat() {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18600)
        return ImGui::GetItemID();
#elif defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18400)
        return GImGui ? GImGui->LastItemData.ID : 0;
#else
        return (GImGui && GImGui->CurrentWindow) ? GImGui->CurrentWindow->DC.LastItemId : 0;
#endif
    }

    inline void PushButtonRepeatCompat(bool repeat) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19070)
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, repeat);
#else
        ImGui::PushButtonRepeat(repeat);
#endif
    }

    inline void PopButtonRepeatCompat() {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19070)
        ImGui::PopItemFlag();
#else
        ImGui::PopButtonRepeat();
#endif
    }

    inline void SetToolbarCursor(const DocumentViewerState& state, ImGuiMouseCursor cursor) {
        if (state.dynamic_cursors_enabled) {
            ImGui::SetMouseCursor(cursor);
        }
    }

    inline void ShowDelayedTip(ImGuiID item_id, bool hovered, const char* md_fmt) {
        if (!md_fmt || md_fmt[0] == '\0') return;
#if defined(IMGUI_TOOLTIP_EXT_H) && defined(IMGUI_MARKDOWN_EXT_H) && \
    !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP) && \
    !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
        ImGui::TimedTooltip::TextMDProxy(item_id, hovered, "%s", md_fmt);
#elif defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGEE_DISABLE_TIMED_TOOLTIP) && !defined(CONTRIB_DISABLE_TIMED_TOOLTIP)
        ImGui::TimedTooltip::TextProxy(item_id, hovered, "%s", md_fmt);
#else
        (void)item_id;
        if (hovered) {
            ImGui::SetTooltip("%s", md_fmt);
        }
#endif
    }

    // Zero-bias stroke helper so procedural chrome glyphs stay centered on all ImGui versions
    inline void StrokePointsZeroBias(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col, float thickness, ImDrawFlags flags = 0) {
        if (!dl || !pts || count < 2) return;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->AddPolyline(pts, count, col, thickness, flags);
#else
        dl->AddPolyline(pts, count, col, flags, thickness);
#endif
    }

    inline void StrokeRectZeroBias(ImDrawList* dl, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, float thickness) {
        if (!dl) return;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->AddRect(p_min, p_max, col, rounding, thickness, 0);
#else
        dl->AddRect(p_min, p_max, col, rounding, 0, thickness);
#endif
    }

    enum ToolbarGlyph {
        ToolbarGlyph_None = 0,
        ToolbarGlyph_Book,
        ToolbarGlyph_CursorOn,
        ToolbarGlyph_CursorOff,
        ToolbarGlyph_PopOut,
        ToolbarGlyph_PopIn,
        ToolbarGlyph_Close,
        ToolbarGlyph_Reload
    };

    inline void DrawToolbarGlyph(ImDrawList* dl, const ImVec2& center, float sz, ImU32 col, ToolbarGlyph glyph) {
        if (!dl || glyph == ToolbarGlyph_None) return;
        const float r = sz * 0.5f;
        const float sw = ImMax(1.25f, sz * 0.11f);

        switch (glyph) {
            case ToolbarGlyph_Book: {
                const float w = r * 0.88f;
                const float h = r * 0.78f;
                StrokeRectZeroBias(dl, ImVec2(center.x - w, center.y - h), ImVec2(center.x + w, center.y + h), col, 1.5f, sw);
                const ImVec2 spine[2] = { ImVec2(center.x, center.y - h), ImVec2(center.x, center.y + h) };
                StrokePointsZeroBias(dl, spine, 2, col, sw);
                break;
            }
            case ToolbarGlyph_CursorOn:
            case ToolbarGlyph_CursorOff: {
                const ImVec2 poly[7] = {
                    ImVec2(center.x - r * 0.52f, center.y - r * 0.78f),
                    ImVec2(center.x - r * 0.52f, center.y + r * 0.54f),
                    ImVec2(center.x - r * 0.16f, center.y + r * 0.22f),
                    ImVec2(center.x + r * 0.08f, center.y + r * 0.78f),
                    ImVec2(center.x + r * 0.30f, center.y + r * 0.68f),
                    ImVec2(center.x + r * 0.06f, center.y + r * 0.14f),
                    ImVec2(center.x + r * 0.48f, center.y + r * 0.14f)
                };
                if (glyph == ToolbarGlyph_CursorOn) {
                    dl->AddTriangleFilled(poly[0], poly[1], poly[6], col);
                    const ImVec2 stem[4] = { poly[2], poly[3], poly[4], poly[5] };
                    dl->AddConvexPolyFilled(stem, 4, col);
                } else {
                    StrokePointsZeroBias(dl, poly, 7, col, sw, ImDrawFlags_Closed);
                }
                break;
            }
            case ToolbarGlyph_PopOut: {
                StrokeRectZeroBias(dl,
                    ImVec2(center.x - r * 0.72f, center.y - r * 0.40f),
                    ImVec2(center.x + r * 0.40f, center.y + r * 0.72f), col, 1.2f, sw);
                const ImVec2 diag[2] = {
                    ImVec2(center.x - r * 0.05f, center.y + r * 0.05f),
                    ImVec2(center.x + r * 0.72f, center.y - r * 0.72f)
                };
                const ImVec2 head[3] = {
                    ImVec2(center.x + r * 0.22f, center.y - r * 0.72f),
                    ImVec2(center.x + r * 0.72f, center.y - r * 0.72f),
                    ImVec2(center.x + r * 0.72f, center.y - r * 0.22f)
                };
                StrokePointsZeroBias(dl, diag, 2, col, sw);
                StrokePointsZeroBias(dl, head, 3, col, sw);
                break;
            }
            case ToolbarGlyph_PopIn: {
                StrokeRectZeroBias(dl,
                    ImVec2(center.x - r * 0.74f, center.y - r * 0.68f),
                    ImVec2(center.x + r * 0.74f, center.y + r * 0.68f), col, 1.5f, sw);
                const ImVec2 bar[2] = {
                    ImVec2(center.x - r * 0.74f, center.y - r * 0.28f),
                    ImVec2(center.x + r * 0.74f, center.y - r * 0.28f)
                };
                StrokePointsZeroBias(dl, bar, 2, col, sw);
                break;
            }
            case ToolbarGlyph_Close: {
                const float d = r * 0.56f;
                const ImVec2 a[2] = { ImVec2(center.x - d, center.y - d), ImVec2(center.x + d, center.y + d) };
                const ImVec2 b[2] = { ImVec2(center.x + d, center.y - d), ImVec2(center.x - d, center.y + d) };
                StrokePointsZeroBias(dl, a, 2, col, sw * 1.15f);
                StrokePointsZeroBias(dl, b, 2, col, sw * 1.15f);
                break;
            }
            case ToolbarGlyph_Reload: {
                // Distinct two-arc circular sync glyph (never visually confused with single-arrow ResetButton)
                const float ar = r * 0.62f;
                const float hw = ImMax(2.2f, r * 0.34f);
                for (int side = 0; side < 2; ++side) {
                    const float a0 = (side == 0) ? -2.62f : 0.52159265f;
                    const float a1 = (side == 0) ? -0.52f : 2.62159265f;
                    dl->PathClear();
                    dl->PathArcTo(center, ar, a0, a1, 12);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
                    dl->PathStroke(col, sw, 0);
#else
                    dl->PathStroke(col, 0, sw);
#endif
                    const float ca = std::cos(a1);
                    const float sa = std::sin(a1);
                    const ImVec2 tip(center.x + ca * ar, center.y + sa * ar);
                    // Clockwise tangent (-sin, cos) and inward/outward normal (cos, sin)
                    const ImVec2 tx(-sa, ca);
                    const ImVec2 nx(ca, sa);
                    const ImVec2 head[3] = {
                        ImVec2(tip.x - tx.x * hw + nx.x * hw * 0.68f, tip.y - tx.y * hw + nx.y * hw * 0.68f),
                        ImVec2(tip.x + tx.x * hw * 0.25f,             tip.y + tx.y * hw * 0.25f),
                        ImVec2(tip.x - tx.x * hw - nx.x * hw * 0.68f, tip.y - tx.y * hw - nx.y * hw * 0.68f)
                    };
                    StrokePointsZeroBias(dl, head, 3, col, sw);
                }
                break;
            }
            default:
                break;
        }
    }

    inline bool ToolbarGlyphButton(const char* str_id, ToolbarGlyph glyph, const DocumentViewerState& state, const char* md_tip, const ImVec2& size = ImVec2(24.0f, 0.0f), bool muted = false) {
        const float h = (size.y > 0.0f) ? size.y : ImGui::GetFrameHeight();
        const ImVec2 btn_sz((size.x > 0.0f) ? size.x : h, h);
        bool clicked = ImGui::Button(str_id, btn_sz);
        bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (hovered) {
            SetToolbarCursor(state, ImGuiMouseCursor_Hand);
        }
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mx = ImGui::GetItemRectMax();
        const ImVec2 center((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
        const ImU32 col = ImGui::GetColorU32(muted ? ImGuiCol_TextDisabled : ImGuiCol_Text);
        DrawToolbarGlyph(ImGui::GetWindowDrawList(), center, ImGui::GetFontSize() * 0.92f, col, glyph);
        if (md_tip && md_tip[0] != '\0') {
            ShowDelayedTip(GetItemIDCompat(), hovered, md_tip);
        }
        return clicked;
    }

    // Strips inline Markdown emphasis, backticks, HTML tags, and [label](url) links into clean plain text
    inline std::string StripHeadingInlineMarkup(const char* s, size_t len) {
        if (!s || len == 0) return std::string();
        std::string out;
        out.reserve(len);

        size_t i = 0;
        while (i < len) {
            // HTML tags <...>
            if (s[i] == '<') {
                size_t close_gt = i + 1;
                while (close_gt < len && s[close_gt] != '>') ++close_gt;
                if (close_gt < len) {
                    i = close_gt + 1;
                    continue;
                }
            }
            // Markdown link [label](url) -> keep label
            if (s[i] == '[') {
                size_t rb = i + 1;
                while (rb < len && s[rb] != ']') ++rb;
                if (rb + 1 < len && s[rb + 1] == '(') {
                    size_t rp = rb + 2;
                    while (rp < len && s[rp] != ')') ++rp;
                    if (rp < len) {
                        std::string inner = StripHeadingInlineMarkup(s + i + 1, rb - (i + 1));
                        out += inner;
                        i = rp + 1;
                        continue;
                    }
                }
            }
            // Strip emphasis/code delimiters
            if (s[i] == '*' || s[i] == '_' || s[i] == '`' || s[i] == '~') {
                ++i;
                continue;
            }
            out.push_back(s[i++]);
        }

        // Trim leading/trailing whitespace and collapse internal runs of spaces
        std::string collapsed;
        collapsed.reserve(out.size());
        bool prev_space = true;
        for (size_t k = 0; k < out.size(); ++k) {
            unsigned char ch = static_cast<unsigned char>(out[k]);
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                if (!prev_space) {
                    collapsed.push_back(' ');
                    prev_space = true;
                }
            } else {
                collapsed.push_back(out[k]);
                prev_space = false;
            }
        }
        if (!collapsed.empty() && collapsed.back() == ' ') {
            collapsed.pop_back();
        }
        return collapsed;
    }

    inline std::string UrlDecodeAnchor(const char* s, size_t len) {
        std::string out;
        out.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            if (s[i] == '%' && i + 2 < len &&
                std::isxdigit(static_cast<unsigned char>(s[i + 1])) &&
                std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
                auto hexVal = [](char c) -> int {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
                    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
                    return 0;
                };
                out.push_back(static_cast<char>((hexVal(s[i + 1]) << 4) | hexVal(s[i + 2])));
                i += 2;
            } else {
                out.push_back(s[i]);
            }
        }
        return out;
    }

    inline float CalcExpectedToggleWidth(const char* label) {
        const ImGuiStyle& style = ImGui::GetStyle();
#if defined(IMGUI_TOGGLE_BUTTONS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_TOGGLE_BUTTONS) && !defined(CONTRIB_DISABLE_TOGGLE_BUTTONS)
        const float font_sz = ImGui::GetFontSize();
        float scale_mult = ToggleButtonDetail::GetToggleConfig().scale;
        if (scale_mult <= 0.0f) scale_mult = 0.78f;
        const float h = std::floor(font_sz * scale_mult + 0.5f);
        const float pill_w = std::floor(h * 1.90f + 0.5f);
#else
        const float pill_w = ImGui::GetFrameHeight();
#endif
        const char* id_pos = label ? std::strstr(label, "##") : nullptr;
        const bool has_text = label && (id_pos != label) && (label[0] != '\0');
        const float text_w = has_text ? ImGui::CalcTextSize(label, id_pos, false).x : 0.0f;
        return pill_w + (has_text ? (style.ItemInnerSpacing.x + text_w) : 0.0f);
    }

    inline std::string AutoShortenTitle(const std::string& full_title, size_t max_chars) {
        if (full_title.size() <= max_chars) return full_title;
        // Prefer splitting before " -- " or ":" if present
        size_t dash_pos = full_title.find(" -- ");
        if (dash_pos != std::string::npos && dash_pos <= max_chars && dash_pos >= 4) {
            return full_title.substr(0, dash_pos);
        }
        size_t colon_pos = full_title.find(':');
        if (colon_pos != std::string::npos && colon_pos <= max_chars && colon_pos >= 4) {
            return full_title.substr(0, colon_pos);
        }
        if (max_chars <= 3) return full_title.substr(0, max_chars);
        size_t cut = max_chars - 3;
        while (cut > 4 && full_title[cut] != ' ') --cut;
        if (cut <= 4) cut = max_chars - 3;
        return full_title.substr(0, cut) + "...";
    }

    struct ActiveDocRenderContext {
        DocumentViewerState*        state          = nullptr;
        const DocumentViewerConfig* config         = nullptr;
        ImGuiWindow*                viewportWindow = nullptr;
        float                       topPaddingY    = 14.0f;
        size_t                      headingCursor  = 0;
#if defined(IMGUI_MARKDOWN_H)
        MarkdownFormalCallback*     baseFormatCb   = nullptr;
        MarkdownLinkCallback*       baseLinkCb     = nullptr;
        void*                       baseUserData   = nullptr;
#endif
    };

    inline ActiveDocRenderContext*& GetActiveRenderContext() {
        static ActiveDocRenderContext* s_Ctx = nullptr;
        return s_Ctx;
    }

} // namespace DocViewerDetail

// Generates a GitHub-Flavored Markdown anchor slug (e.g. "## 2. Quick Start!" -> "2-quick-start")
inline std::string MakeMarkdownHeadingSlug(const char* text, size_t len) {
    std::string clean = DocViewerDetail::StripHeadingInlineMarkup(text, len);
    std::string slug;
    slug.reserve(clean.size());

    for (size_t i = 0; i < clean.size(); ++i) {
        unsigned char uc = static_cast<unsigned char>(clean[i]);
        if (std::isalnum(uc)) {
            slug.push_back(static_cast<char>(std::tolower(uc)));
        } else if (uc == '-' || uc == '_') {
            slug.push_back(static_cast<char>(uc));
        } else if (uc == ' ' || uc == '\t') {
            slug.push_back('-');
        } else if (uc >= 0x80) {
            // Preserve UTF-8 multibyte bytes so non-ASCII headings stay addressable
            slug.push_back(static_cast<char>(uc));
        }
    }

    // Trim leading/trailing hyphens if whole heading was punctuation-padded
    while (!slug.empty() && slug.front() == '-') slug.erase(slug.begin());
    while (!slug.empty() && slug.back() == '-') slug.pop_back();
    return slug;
}

inline std::string MakeMarkdownHeadingSlug(const std::string& text) {
    return MakeMarkdownHeadingSlug(text.c_str(), text.size());
}

// Scans ATX headings (# .. ###) while skipping fenced code blocks (``` / ~~~) and > blockquotes
inline void ParseMarkdownToc(
    const char* markdown,
    size_t markdown_len,
    std::vector<DocumentTocEntry>& out_toc,
    int max_heading_level = 3,
    std::string* out_first_h1 = nullptr)
{
    out_toc.clear();
    if (out_first_h1) out_first_h1->clear();
    if (!markdown || markdown_len == 0) return;

    const int clamped_max_level = (max_heading_level < 1) ? 1 : ((max_heading_level > 6) ? 6 : max_heading_level);
    std::unordered_map<std::string, int> slug_counts;

    bool   in_fence   = false;
    char   fence_char = '\0';
    size_t fence_len  = 0;

    size_t pos = 0;
    while (pos < markdown_len) {
        const size_t line_start = pos;
        size_t line_end = pos;
        while (line_end < markdown_len && markdown[line_end] != '\n' && markdown[line_end] != '\r') {
            ++line_end;
        }
        pos = line_end;
        if (pos < markdown_len && markdown[pos] == '\r') ++pos;
        if (pos < markdown_len && markdown[pos] == '\n') ++pos;

        // Count leading spaces (CommonMark allows 0..3 leading spaces on ATX headings and code fences)
        size_t p = line_start;
        size_t leading_spaces = 0;
        while (p < line_end && markdown[p] == ' ') {
            ++p;
            ++leading_spaces;
        }

        // Check fenced code blocks (both top-level and '>' blockquote-prefixed fences)
        size_t fp = p;
        if (!in_fence && leading_spaces <= 3 && fp < line_end && markdown[fp] == '>') {
            // Skip blockquote lines completely for top-level TOC
            continue;
        }

        if (leading_spaces <= 3 && fp < line_end && (markdown[fp] == '`' || markdown[fp] == '~')) {
            const char fc = markdown[fp];
            size_t run = 0;
            while (fp + run < line_end && markdown[fp + run] == fc) {
                ++run;
            }
            if (run >= 3) {
                if (!in_fence) {
                    // Backtick info strings cannot contain additional backticks on the same line
                    bool valid_open = true;
                    if (fc == '`') {
                        for (size_t k = fp + run; k < line_end; ++k) {
                            if (markdown[k] == '`') { valid_open = false; break; }
                        }
                    }
                    if (valid_open) {
                        in_fence = true;
                        fence_char = fc;
                        fence_len = run;
                        continue;
                    }
                } else if (fc == fence_char && run >= fence_len) {
                    bool only_ws = true;
                    for (size_t k = fp + run; k < line_end; ++k) {
                        if (markdown[k] != ' ' && markdown[k] != '\t') { only_ws = false; break; }
                    }
                    if (only_ws) {
                        in_fence = false;
                        fence_char = '\0';
                        fence_len = 0;
                        continue;
                    }
                }
            }
        }

        if (in_fence || leading_spaces > 3 || p >= line_end || markdown[p] != '#') {
            continue;
        }

        int h_level = 0;
        while (p < line_end && markdown[p] == '#' && h_level < 7) {
            ++h_level;
            ++p;
        }
        if (h_level < 1 || h_level > 6) {
            continue;
        }
        // ATX heading requires whitespace (or end-of-line) immediately after '#' run
        if (p < line_end && markdown[p] != ' ' && markdown[p] != '\t') {
            continue;
        }
        while (p < line_end && (markdown[p] == ' ' || markdown[p] == '\t')) {
            ++p;
        }

        // Trim optional CommonMark closing '#' sequence preceded by space/tab
        size_t h_end = line_end;
        while (h_end > p && (markdown[h_end - 1] == ' ' || markdown[h_end - 1] == '\t')) {
            --h_end;
        }
        size_t hash_tail = h_end;
        while (hash_tail > p && markdown[hash_tail - 1] == '#') {
            --hash_tail;
        }
        if (hash_tail < h_end && (hash_tail == p || markdown[hash_tail - 1] == ' ' || markdown[hash_tail - 1] == '\t')) {
            h_end = hash_tail;
            while (h_end > p && (markdown[h_end - 1] == ' ' || markdown[h_end - 1] == '\t')) {
                --h_end;
            }
        }

        std::string clean_title = DocViewerDetail::StripHeadingInlineMarkup(markdown + p, h_end - p);
        if (clean_title.empty()) {
            continue;
        }

        if (h_level == 1 && out_first_h1 && out_first_h1->empty()) {
            *out_first_h1 = clean_title;
        }

        if (h_level > clamped_max_level) {
            continue;
        }

        std::string base_slug = MakeMarkdownHeadingSlug(markdown + p, h_end - p);
        if (base_slug.empty()) {
            base_slug = "section";
        }
        int seen = slug_counts[base_slug]++;
        std::string unique_slug = (seen == 0) ? base_slug : (base_slug + "-" + std::to_string(seen));

        DocumentTocEntry entry;
        entry.level       = h_level;
        entry.title       = clean_title;
        entry.slug        = unique_slug;
        entry.byte_offset = line_start;
        entry.content_y   = -1.0f;
        if (h_level == 1) {
            entry.display_label = clean_title;
        } else if (h_level == 2) {
            entry.display_label = "  " + clean_title;
        } else {
            entry.display_label = "    " + clean_title;
        }
        out_toc.push_back(entry);
    }
}

inline void ApplyDocumentViewerTheme(
    int theme_index,
    bool transparent_hud = false,
    float window_opacity = 0.80f,
    ImGuiStyle* dst = nullptr)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    switch (theme_index) {
        case DocumentViewerTheme_Charcoal:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::ApplyDefaultTheme(style);
#else
            ImGui::StyleColorsDark(style);
#endif
            break;
        case DocumentViewerTheme_Rusty:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::StyleColorsRusty(style);
#else
            ImGui::StyleColorsDark(style);
#endif
            break;
        case DocumentViewerTheme_Dark:
            ImGui::StyleColorsDark(style);
            break;
        case DocumentViewerTheme_Classic:
            ImGui::StyleColorsClassic(style);
            break;
        case DocumentViewerTheme_Light:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::StyleColorsPaperLight(style);
#else
            ImGui::StyleColorsLight(style);
#endif
            break;
        default:
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
            ImGui::ApplyDefaultTheme(style);
#else
            ImGui::StyleColorsDark(style);
#endif
            break;
    }

    if (transparent_hud) {
        const float alpha = (window_opacity < 0.10f) ? 0.10f : ((window_opacity > 1.0f) ? 1.0f : window_opacity);
        style->Colors[ImGuiCol_WindowBg].w = alpha;
        style->Colors[ImGuiCol_Border].w   = (std::min)(1.0f, alpha * 0.8f + 0.2f);
    } else {
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
        style->Colors[ImGuiCol_Border].w   = 1.0f;
    }
}

inline bool JumpToDocumentHeading(DocumentViewerState& state, int toc_index) {
    if (toc_index < 0 || static_cast<size_t>(toc_index) >= state.toc.size()) {
        return false;
    }
    state.active_toc_index         = toc_index;
    state.pending_scroll_toc_index = toc_index;
    state.pending_anchor_slug.clear();
    return true;
}

namespace DocViewerDetail {
    inline std::string CollapseSlugHyphens(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        bool prev_hyphen = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '-') {
                if (!prev_hyphen) {
                    out.push_back('-');
                    prev_hyphen = true;
                }
            } else {
                out.push_back(s[i]);
                prev_hyphen = false;
            }
        }
        return out;
    }
} // namespace DocViewerDetail

inline bool JumpToDocumentAnchor(DocumentViewerState& state, const char* anchor_or_slug) {
    if (!anchor_or_slug || anchor_or_slug[0] == '\0') return false;
    const char* raw = (anchor_or_slug[0] == '#') ? (anchor_or_slug + 1) : anchor_or_slug;
    if (raw[0] == '\0') return false;

    const std::string decoded = DocViewerDetail::UrlDecodeAnchor(raw, std::strlen(raw));
    std::string target_slug = MakeMarkdownHeadingSlug(decoded);
    std::string raw_lower = decoded;
    for (size_t i = 0; i < raw_lower.size(); ++i) {
        raw_lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(raw_lower[i])));
    }

    // Pass 1: exact GFM slug match
    for (size_t i = 0; i < state.toc.size(); ++i) {
        if (state.toc[i].slug == raw_lower || (!target_slug.empty() && state.toc[i].slug == target_slug)) {
            state.active_toc_index         = static_cast<int>(i);
            state.pending_scroll_toc_index = static_cast<int>(i);
            state.pending_anchor_slug.clear();
            return true;
        }
    }

    // Pass 2: collapsed-hyphen fallback (so #3-missed-approach-holding matches 3-missed-approach--holding)
    const std::string collapsed_raw    = DocViewerDetail::CollapseSlugHyphens(raw_lower);
    const std::string collapsed_target = DocViewerDetail::CollapseSlugHyphens(target_slug);
    for (size_t i = 0; i < state.toc.size(); ++i) {
        const std::string collapsed_toc = DocViewerDetail::CollapseSlugHyphens(state.toc[i].slug);
        if (collapsed_toc == collapsed_raw || (!collapsed_target.empty() && collapsed_toc == collapsed_target)) {
            state.active_toc_index         = static_cast<int>(i);
            state.pending_scroll_toc_index = static_cast<int>(i);
            state.pending_anchor_slug.clear();
            return true;
        }
    }

    // Defer slug resolution in case TOC has not been parsed yet
    state.pending_anchor_slug = !target_slug.empty() ? target_slug : raw_lower;
    return true;
}

// 1-click A/B toggle between top of document (0.0f) and saved reading spot (saved_scroll_y)
inline bool ToggleDocumentScrollReset(DocumentViewerState& state) {
    const bool is_scrolled = (state.last_scroll_y > 4.0f);
    const bool can_restore = !is_scrolled && (state.saved_scroll_y > 4.0f);
    if (is_scrolled) {
        state.saved_scroll_y           = state.last_scroll_y;
        state.pending_raw_scroll_y     = 0.0f;
        state.pending_scroll_toc_index = -1;
        state.pending_anchor_slug.clear();
        state.active_toc_index         = 0;
        return true;
    }
    if (can_restore) {
        state.pending_raw_scroll_y     = state.saved_scroll_y;
        state.pending_scroll_toc_index = -1;
        state.pending_anchor_slug.clear();
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// 3. Composite DocumentViewer Widget
// ----------------------------------------------------------------------------

inline bool DocumentViewer(
    const char* str_id,
    const char* markdown_text,
    size_t markdown_len,
    DocumentViewerState& state,
    const DocumentViewerConfig* config = nullptr,
    const ImVec2& size = ImVec2(0.0f, 0.0f))
{
    static const DocumentViewerConfig s_DefaultConfig;
    const DocumentViewerConfig& cfg = config ? *config : s_DefaultConfig;

    if (!markdown_text) {
        markdown_text = "";
        markdown_len  = 0;
    }

    // Refresh cached TOC only when the document content or max heading level changes
    const int max_toc_lvl = (cfg.maxTocHeadingLevel < 1) ? 1 : ((cfg.maxTocHeadingLevel > 6) ? 6 : cfg.maxTocHeadingLevel);
    const uint64_t doc_hash = DocViewerDetail::HashDocumentFnv1a(markdown_text, markdown_len);
    if (!state._toc_initialized ||
        state._cached_doc_hash != doc_hash ||
        state._cached_doc_len != markdown_len ||
        state._cached_max_toc_level != max_toc_lvl)
    {
        ParseMarkdownToc(markdown_text, markdown_len, state.toc, max_toc_lvl, &state._auto_doc_title);
        state._cached_doc_hash      = doc_hash;
        state._cached_doc_len       = markdown_len;
        state._cached_max_toc_level = max_toc_lvl;
        state._toc_initialized      = true;
        if (state.active_toc_index >= static_cast<int>(state.toc.size())) {
            state.active_toc_index = 0;
        }
    }

    // Resolve any deferred #anchor jump now that state.toc is populated
    if (!state.pending_anchor_slug.empty()) {
        const std::string target = state.pending_anchor_slug;
        state.pending_anchor_slug.clear();
        bool resolved = false;
        for (size_t i = 0; i < state.toc.size(); ++i) {
            if (state.toc[i].slug == target) {
                state.active_toc_index         = static_cast<int>(i);
                state.pending_scroll_toc_index = static_cast<int>(i);
                resolved = true;
                break;
            }
        }
        if (!resolved) {
            const std::string collapsed_target = DocViewerDetail::CollapseSlugHyphens(target);
            for (size_t i = 0; i < state.toc.size(); ++i) {
                if (DocViewerDetail::CollapseSlugHyphens(state.toc[i].slug) == collapsed_target) {
                    state.active_toc_index         = static_cast<int>(i);
                    state.pending_scroll_toc_index = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    ImGui::PushID(str_id ? str_id : "##imgee_doc_viewer");

    const bool use_outer_child = (size.x != 0.0f || size.y != 0.0f);
    if (use_outer_child) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
        ImGui::BeginChild("##doc_viewer_frame", size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#else
        ImGui::BeginChild("##doc_viewer_frame", size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#endif
    }

    ImGuiWindow* host_win = ImGui::GetCurrentWindow();
    const float saved_host_scale = host_win ? host_win->FontWindowScale : 1.0f;

    // ========================================================================
    // A. HEADER CHROME ROW 1 (Fixed 1.0x Scale: Title + Zoom Suite + Host Hooks)
    // ========================================================================
    ImGui::ImGeeCompat::SetWindowFontScale(1.0f);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float win_w = ImGui::GetContentRegionAvail().x + style.WindowPadding.x * 2.0f;
    const float scrollbar_pad = (host_win && host_win->ScrollbarY) ? host_win->ScrollbarSizes.x : 0.0f;
    const float right_margin = use_outer_child ? 2.0f : (style.WindowPadding.x + scrollbar_pad);

    auto notifyZoomChange = [&](float old_z, float new_z) {
        if (std::fabs(new_z - old_z) > 0.0005f && cfg.onZoomChanged) {
            cfg.onZoomChanged(old_z, new_z, cfg.userData);
        }
    };

    if (cfg.showHeaderRow1) {
        const std::string base_title = (cfg.title && cfg.title[0] != '\0')
            ? std::string(cfg.title)
            : (!state._auto_doc_title.empty() ? state._auto_doc_title : std::string("Documentation"));
        const std::string med_title = (cfg.medium_title && cfg.medium_title[0] != '\0')
            ? std::string(cfg.medium_title)
            : DocViewerDetail::AutoShortenTitle(base_title, 22);
        const std::string cmp_title = (cfg.compact_title && cfg.compact_title[0] != '\0')
            ? std::string(cfg.compact_title)
            : DocViewerDetail::AutoShortenTitle(med_title, 12);

        const std::string& chosen_title = (win_w > 540.0f) ? base_title : ((win_w > 400.0f) ? med_title : cmp_title);

        ImGui::AlignTextToFramePadding();
        const float row1_start_x = ImGui::GetCursorPosX();
        if (cfg.showTitleIcon) {
            const ImVec2 icon_p = ImGui::GetCursorScreenPos();
            const float frame_h = ImGui::GetFrameHeight();
            const float icon_sz = ImGui::GetFontSize() * 1.05f;
            DocViewerDetail::DrawToolbarGlyph(
                ImGui::GetWindowDrawList(),
                ImVec2(icon_p.x + icon_sz * 0.5f, icon_p.y + frame_h * 0.5f),
                icon_sz,
                ImGui::GetColorU32(ImGuiCol_Text),
                DocViewerDetail::ToolbarGlyph_Book);
            ImGui::Dummy(ImVec2(icon_sz + 5.0f, frame_h));
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::AlignTextToFramePadding();
        }

        bool pushed_title_font = false;
#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
        if (ImFont* title_font = ImGui::GetFont(ImGui::ImFontRole::Title)) {
            ImGui::ImGeeCompat::PushFont(title_font);
            pushed_title_font = true;
        } else if (ImFont* bold_font = ImGui::GetFont(ImGui::ImFontRole::Bold)) {
            ImGui::ImGeeCompat::PushFont(bold_font);
            pushed_title_font = true;
        }
#endif
        ImGui::TextUnformatted(chosen_title.c_str());
        const float title_end_x = ImGui::GetCursorPosX() + ImGui::CalcTextSize(chosen_title.c_str()).x;
        if (pushed_title_font) {
            ImGui::PopFont();
        }
        (void)row1_start_x;
        (void)title_end_x;

        ImGui::SameLine();
        const float min_r1_offset = ImGui::GetCursorPosX();

        // Compute exact fixed width of Row 1 zoom + optional host buttons
        const bool has_pdf_btn   = (cfg.externalPdfPath && cfg.externalPdfPath[0] != '\0');
        const bool has_vr_btn    = (cfg.showVrButton && !cfg.isInVr);
        const float reset_btn_w  = ImGui::GetFrameHeight();
        float fixed_r1_w = 22.0f + 3.0f + 3.0f + 22.0f + 3.0f + reset_btn_w;
        if (cfg.showFitButton)    fixed_r1_w += 3.0f + 32.0f;
        if (has_pdf_btn)          fixed_r1_w += 6.0f + 38.0f;
        if (cfg.showCursorToggle) fixed_r1_w += 8.0f + 24.0f;
        if (cfg.showPopOutButton) fixed_r1_w += 4.0f + 24.0f;
        if (has_vr_btn)           fixed_r1_w += 4.0f + 28.0f;
        if (cfg.showCloseButton)  fixed_r1_w += 6.0f + 24.0f;

        const float avail_r1 = (std::max)(fixed_r1_w + 76.0f, ImGui::GetWindowWidth() - min_r1_offset - right_margin);
        const float zoom_slider_w = ImClamp(avail_r1 - fixed_r1_w, 76.0f, 126.0f);
        const float r1_suite_w = fixed_r1_w + zoom_slider_w;
        const float r1_target_x = ImGui::GetWindowWidth() - r1_suite_w - right_margin;
        ImGui::SameLine((std::max)(min_r1_offset, r1_target_x));

        // 1. [-] Repeating Zoom-Out Stepper
        DocViewerDetail::PushButtonRepeatCompat(true);
        bool dec_clicked = ImGui::Button("-##dv_zoom_dec", ImVec2(22.0f, 0.0f));
        DocViewerDetail::PopButtonRepeatCompat();
        bool dec_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (dec_hovered) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
        if (dec_clicked) {
            const bool is_init = ImGui::IsItemActivated() || ImGui::IsMouseClicked(0);
            bool allow_step = false;
            if (is_init) {
                state._dec_repeat_count = 0;
                allow_step = true;
            } else if ((++state._dec_repeat_count % 2) == 0) {
                allow_step = true;
            }
            if (allow_step) {
                const float old_z = state.viewport.zoom;
                if (state.viewport.StepZoom(-state.viewport.button_step)) {
                    notifyZoomChange(old_z, state.viewport.zoom);
                }
            }
        }
        DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), dec_hovered,
            "Zoom Out (-5%)\n\n*Click or hold down to step smoothly*");

        ImGui::SameLine(0.0f, 3.0f);

        // 2. Zoom Slider (50% .. 250%) + 2% Mouse-Wheel Stepping
        const float pre_slider_z = state.viewport.zoom;
        float pct = state.viewport.zoom * 100.0f;
        ImGui::SetNextItemWidth(zoom_slider_w);
        if (ImGui::SliderFloat("##dv_zoom_slider", &pct, state.viewport.min_zoom * 100.0f, state.viewport.max_zoom * 100.0f, "Zoom %.0f%%")) {
            state.viewport.SetZoom(pct / 100.0f);
        }
        if (ImGui::IsItemActivated()) {
            state._slider_start_zoom = pre_slider_z;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            notifyZoomChange(state._slider_start_zoom, state.viewport.zoom);
            state._slider_start_zoom = state.viewport.zoom;
        }
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_ADJUST_MOUSE_WHEEL)
        {
            float wheel_z = state.viewport.zoom;
            if (ImGui::AdjustItemOnMouseWheel(&wheel_z, state.viewport.min_zoom, state.viewport.max_zoom, 0.02f, state.dynamic_cursors_enabled)) {
                const float old_z = state.viewport.zoom;
                if (state.viewport.SetZoom(wheel_z)) {
                    notifyZoomChange(old_z, state.viewport.zoom);
                }
            }
        }
#else
        if (ImGui::IsItemHovered()) {
            DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_ResizeEW);
        }
#endif
        DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), ImGui::IsItemHovered(),
            "**Document Content Zoom**\n\nRoll mouse wheel (`2%`) or drag slider (`50%` to `250%`).\n*Toolbar chrome stays crisp at `1.0x`.*");

        ImGui::SameLine(0.0f, 3.0f);

        // 3. [+] Repeating Zoom-In Stepper
        DocViewerDetail::PushButtonRepeatCompat(true);
        bool inc_clicked = ImGui::Button("+##dv_zoom_inc", ImVec2(22.0f, 0.0f));
        DocViewerDetail::PopButtonRepeatCompat();
        bool inc_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (inc_hovered) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
        if (inc_clicked) {
            const bool is_init = ImGui::IsItemActivated() || ImGui::IsMouseClicked(0);
            bool allow_step = false;
            if (is_init) {
                state._inc_repeat_count = 0;
                allow_step = true;
            } else if ((++state._inc_repeat_count % 2) == 0) {
                allow_step = true;
            }
            if (allow_step) {
                const float old_z = state.viewport.zoom;
                if (state.viewport.StepZoom(+state.viewport.button_step)) {
                    notifyZoomChange(old_z, state.viewport.zoom);
                }
            }
        }
        DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), inc_hovered,
            "Zoom In (+5%)\n\n*Click or hold down to step smoothly*");

        ImGui::SameLine(0.0f, 3.0f);

        // 4. [Reset / Restore] Vector Button with 1-Click A/B Undo Memory
        const float base_z = state.viewport.Clamp(state.viewport.default_zoom);
        const float prev_z = state.viewport.Clamp(state.viewport.last_zoom);
        const bool is_zoom_mod = std::fabs(state.viewport.zoom - base_z) > 0.0005f;
        const bool can_zoom_restore = !is_zoom_mod && (std::fabs(prev_z - base_z) > 0.0005f);
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(IMGEE_DISABLE_RESET_BUTTON) && !defined(CONTRIB_DISABLE_RESET_BUTTON)
        bool rst_clicked = ImGui::ResetButton("##dv_zoom_reset", is_zoom_mod, 0.0f, false, nullptr, can_zoom_restore);
#else
        bool rst_clicked = ImGui::Button(can_zoom_restore ? "U##dv_zoom_reset" : "R##dv_zoom_reset", ImVec2(reset_btn_w, 0.0f));
#endif
        bool rst_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (rst_hovered) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
        if (rst_clicked) {
            const float old_z = state.viewport.zoom;
            if (state.viewport.ToggleResetZoom()) {
                notifyZoomChange(old_z, state.viewport.zoom);
            }
        }
        {
            char rst_tip[196];
            if (is_zoom_mod) {
                std::snprintf(rst_tip, sizeof(rst_tip), "**Reset Zoom:** Revert from **%d%%** back to **%d%%** baseline.",
                              static_cast<int>(std::round(state.viewport.zoom * 100.0f)),
                              static_cast<int>(std::round(base_z * 100.0f)));
            } else if (can_zoom_restore) {
                std::snprintf(rst_tip, sizeof(rst_tip), "**Restore Prior Zoom:** 1-click toggle back to **%d%%**.",
                              static_cast<int>(std::round(prev_z * 100.0f)));
            } else {
                std::snprintf(rst_tip, sizeof(rst_tip), "**Zoom Baseline:** Already at **%d%%**.",
                              static_cast<int>(std::round(base_z * 100.0f)));
            }
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), rst_hovered, rst_tip);
        }

        // 5. Optional Host Window Chrome Hooks
        if (cfg.showFitButton) {
            ImGui::SameLine(0.0f, 3.0f);
            if (ImGui::Button("Fit##dv_fit", ImVec2(32.0f, 0.0f))) {
                if (cfg.onFitRequested) cfg.onFitRequested(state.viewport.zoom, cfg.userData);
            }
            bool fit_hov = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if (fit_hov) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
            char fit_tip[196];
            std::snprintf(fit_tip, sizeof(fit_tip),
                          "**Fit Window to Baseline**\n\nResizes host window proportionally to current **%d%%** zoom.",
                          static_cast<int>(std::round(state.viewport.zoom * 100.0f)));
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), fit_hov, fit_tip);
        }

        if (has_pdf_btn) {
            ImGui::SameLine(0.0f, 6.0f);
            if (ImGui::Button("PDF##dv_pdf", ImVec2(38.0f, 0.0f))) {
#if defined(IMGUI_MARKDOWN_H)
                if (cfg.externalLinkCallback) {
                    MarkdownLinkCallbackData ld = {};
                    ld.text = "PDF";
                    ld.textLength = 3;
                    ld.link = cfg.externalPdfPath;
                    ld.linkLength = static_cast<int>(std::strlen(cfg.externalPdfPath));
                    ld.userData = cfg.userData;
                    ld.isImage = false;
                    cfg.externalLinkCallback(ld);
                } else
#endif
                {
#if defined(IMGUI_CLICKABLE_LINKS_READY) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_CLICKABLE_LINKS) && !defined(CONTRIB_DISABLE_CLICKABLE_LINKS)
                    ImGui::OpenURL(cfg.externalPdfPath);
#endif
                }
            }
            bool pdf_hov = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if (pdf_hov) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), pdf_hov,
                "**Open External PDF**\n\nLaunches the companion PDF manual in your system viewer.");
        }

        if (cfg.showCursorToggle) {
            ImGui::SameLine(0.0f, 8.0f);
            const char* cur_tip = state.dynamic_cursors_enabled
                ? "**Dynamic Cursors:** <pill color=#2ecc71>ACTIVE</pill>\n\nClick to switch to standard OS arrow cursor."
                : "**Dynamic Cursors:** <pill color=#7f8c8d>DISABLED</pill>\n\nClick to enable contextual hand & resize cursors.";
            if (DocViewerDetail::ToolbarGlyphButton(
                    "##dv_cursor_toggle",
                    state.dynamic_cursors_enabled ? DocViewerDetail::ToolbarGlyph_CursorOn : DocViewerDetail::ToolbarGlyph_CursorOff,
                    state, cur_tip, ImVec2(24.0f, 0.0f), !state.dynamic_cursors_enabled))
            {
                state.dynamic_cursors_enabled = !state.dynamic_cursors_enabled;
                if (cfg.onCursorToggled) cfg.onCursorToggled(state.dynamic_cursors_enabled, cfg.userData);
            }
        }

        if (cfg.showPopOutButton) {
            ImGui::SameLine(0.0f, 4.0f);
            const bool is_out = (cfg.isPoppedOut || cfg.isInVr);
            const char* pop_tip = is_out
                ? "**Move Back Into Application**\n\nDocks this manual back inside the main viewport."
                : "**Pop Out Window**\n\nDetaches this manual into an independent desktop OS window.";
            if (DocViewerDetail::ToolbarGlyphButton(
                    "##dv_popout",
                    is_out ? DocViewerDetail::ToolbarGlyph_PopIn : DocViewerDetail::ToolbarGlyph_PopOut,
                    state, pop_tip, ImVec2(24.0f, 0.0f)))
            {
                if (cfg.onPopOutToggled) cfg.onPopOutToggled(!is_out, cfg.userData);
            }
        }

        if (has_vr_btn) {
            ImGui::SameLine(0.0f, 4.0f);
            if (ImGui::Button("VR##dv_vr", ImVec2(28.0f, 0.0f))) {
                if (cfg.onVrRequested) cfg.onVrRequested(cfg.userData);
            }
            bool vr_hov = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if (vr_hov) DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), vr_hov,
                "**Move Into VR**\n\nAttaches this manual window into the VR workspace.");
        }

        if (cfg.showCloseButton) {
            ImGui::SameLine(0.0f, 6.0f);
            if (DocViewerDetail::ToolbarGlyphButton("##dv_close", DocViewerDetail::ToolbarGlyph_Close, state,
                                                    "**Close Manual**", ImVec2(24.0f, 0.0f))) {
                state.is_open = false;
                if (cfg.onCloseRequested) cfg.onCloseRequested(cfg.userData);
            }
        }
    }

    // ========================================================================
    // B. HEADER CHROME ROW 2 (TOC Section Jump + Invariant Opacity & Theme)
    // ========================================================================
    if (cfg.showHeaderRow2) {
        const float r2_scale = cfg.scaleRow2WithZoom ? state.viewport.zoom : 1.0f;
        ImGui::ImGeeCompat::SetWindowFontScale(r2_scale);

        const char* toggle_label = cfg.isPoppedOut ? "Custom##dv_trans" : "Transparent##dv_trans";
        const char* opac_label   = cfg.isPoppedOut ? "Saturation:" : "Opacity:";
        const float section_gap  = 10.0f * r2_scale;
        const float opac_slider_w = 88.0f * r2_scale;
        const float theme_combo_w = 138.0f * r2_scale;

        float right_r2_w = 0.0f;
        if (cfg.showOpacityControls) {
            const float toggle_w = DocViewerDetail::CalcExpectedToggleWidth(toggle_label);
            right_r2_w += toggle_w;
            if (state.transparent_hud) {
                const float opac_lbl_w = ImGui::CalcTextSize(opac_label).x;
                right_r2_w += opac_lbl_w + 4.0f * r2_scale + opac_slider_w + section_gap;
            }
        }
        if (cfg.showThemePicker) {
            if (right_r2_w > 0.0f) right_r2_w += section_gap;
            const float theme_lbl_w = ImGui::CalcTextSize("Theme:").x;
            right_r2_w += theme_lbl_w + 4.0f * r2_scale + theme_combo_w;
        }

        // 1. Left: Section Jump TOC SelectPulldown (+ 1-click Scroll Reset + optional Reload button)
        ImGui::AlignTextToFramePadding();
        const bool show_sec_lbl = (win_w > 620.0f);
        if (show_sec_lbl) {
            ImGui::TextDisabled("Section:");
            ImGui::SameLine(0.0f, 4.0f * r2_scale);
        }

        const float scroll_rst_reserve = cfg.showScrollResetButton ? (ImGui::GetFrameHeight() + 3.0f) : 0.0f;
        const float reload_reserve     = cfg.showReloadButton ? (24.0f + 4.0f) : 0.0f;
        const float max_toc_avail = ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - right_r2_w - right_margin - scroll_rst_reserve - reload_reserve - 12.0f;
        const float toc_w = ImClamp(cfg.tocWidth * r2_scale, 95.0f, (std::max)(95.0f, max_toc_avail));

        if (!state.toc.empty()) {
            std::vector<std::string> toc_labels;
            toc_labels.reserve(state.toc.size());
            for (size_t i = 0; i < state.toc.size(); ++i) {
                toc_labels.push_back(state.toc[i].display_label);
            }
            int sel = ImClamp(state.active_toc_index, 0, static_cast<int>(state.toc.size()) - 1);
            bool jumped = false;
#if defined(IMGUI_POPUP_MENUS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_POPUP_MENUS) && !defined(CONTRIB_DISABLE_POPUP_MENUS)
            jumped = ImGui::SelectPulldown("##dv_toc_jump", &sel, toc_labels, nullptr, toc_w);
#else
            std::vector<const char*> ptrs;
            ptrs.reserve(toc_labels.size());
            for (size_t i = 0; i < toc_labels.size(); ++i) ptrs.push_back(toc_labels[i].c_str());
            ImGui::SetNextItemWidth(toc_w);
            jumped = ImGui::Combo("##dv_toc_jump", &sel, ptrs.data(), static_cast<int>(ptrs.size()));
#endif
            if (jumped) {
                JumpToDocumentHeading(state, sel);
            }
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), ImGui::IsItemHovered(),
                "**Jump to Section**\n\nClick to open Table of Contents, then click or spin mouse wheel to jump.");
        } else {
            ImGui::TextDisabled("No sections");
        }

        if (cfg.showScrollResetButton) {
            ImGui::SameLine(0.0f, 3.0f);
            const bool is_scrolled = (state.last_scroll_y > 4.0f);
            const bool can_restore = !is_scrolled && (state.saved_scroll_y > 4.0f);
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(IMGEE_DISABLE_RESET_BUTTON) && !defined(CONTRIB_DISABLE_RESET_BUTTON)
            bool scroll_rst_clicked = ImGui::ResetButton("##dv_scroll_reset", is_scrolled, 0.0f, false, nullptr, can_restore);
#else
            const float fh = ImGui::GetFrameHeight();
            const bool actionable = (is_scrolled || can_restore);
            if (!actionable) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * 0.45f);
            bool scroll_rst_clicked = ImGui::Button(can_restore ? "U##dv_scroll_reset" : "R##dv_scroll_reset", ImVec2(fh, fh)) && actionable;
            if (!actionable) ImGui::PopStyleVar();
#endif
            bool scroll_rst_hov = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if (scroll_rst_hov && (is_scrolled || can_restore)) {
                DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
            }
            if (scroll_rst_clicked) {
                (void)ToggleDocumentScrollReset(state);
            }
            const char* scroll_rst_tip = is_scrolled
                ? "**Jump to Top:** Scroll back to the start of the document (saves your current reading spot)."
                : (can_restore
                    ? "**Return to Reading Spot:** 1-click jump back to your previous scroll position."
                    : "**At Top of Document**");
            DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), scroll_rst_hov, scroll_rst_tip);
        }

        if (cfg.showReloadButton) {
            ImGui::SameLine(0.0f, 4.0f);
            if (DocViewerDetail::ToolbarGlyphButton("##dv_reload", DocViewerDetail::ToolbarGlyph_Reload, state,
                                                    "**Reload Document**\n\nRe-reads Markdown source and refreshes images.",
                                                    ImVec2(24.0f, 0.0f))) {
                state.InvalidateTocCache();
                if (cfg.onReloadRequested) cfg.onReloadRequested(cfg.userData);
            }
        }

        // 2. Right: Zero-Shift Opacity/Saturation + Transparent/Custom Toggle + Theme Picker
        if (right_r2_w > 0.0f) {
            const float min_r2_x = ImGui::GetCursorPosX() + ImGui::GetFrameHeight() * 0.2f;
            ImGui::SameLine();
            const float after_toc_x = ImGui::GetCursorPosX();
            const float r2_target_x = ImGui::GetWindowWidth() - right_r2_w - right_margin;
            ImGui::SameLine((std::max)((std::max)(min_r2_x, after_toc_x), r2_target_x));

            if (cfg.showOpacityControls) {
                // Render Opacity/Saturation TO THE LEFT of the toggle so the toggle & Theme never shift!
                if (state.transparent_hud) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(opac_label);
                    ImGui::SameLine(0.0f, 4.0f * r2_scale);
                    ImGui::SetNextItemWidth(opac_slider_w);
                    bool opac_changed = false;
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_PERCENT_SLIDER)
                    opac_changed = ImGui::PercentSliderWithReset(
                        cfg.isPoppedOut ? "##dv_saturation" : "##dv_opacity",
                        &state.window_opacity, 0.80f, 0.10f, 1.00f, "%.0f%%", 0, 0.02f, nullptr, &state.prev_window_opacity);
#else
                    float op_pct = state.window_opacity * 100.0f;
                    if (ImGui::SliderFloat("##dv_opacity", &op_pct, 10.0f, 100.0f, "%.0f%%")) {
                        state.window_opacity = op_pct / 100.0f;
                        opac_changed = true;
                    }
#endif
                    if (opac_changed) {
                        ImGuiStyle& st = ImGui::GetStyle();
                        st.Colors[ImGuiCol_WindowBg].w = state.window_opacity;
                        st.Colors[ImGuiCol_Border].w   = (std::min)(1.0f, state.window_opacity * 0.8f + 0.2f);
                        if (cfg.onOpacityChanged) cfg.onOpacityChanged(state.window_opacity, cfg.userData);
                    }
                    {
                        char op_tip[240];
                        if (cfg.isPoppedOut) {
                            std::snprintf(op_tip, sizeof(op_tip),
                                "**Desktop Window Saturation**\n\nDrag or spin mouse wheel to tune backdrop contrast (**%.0f%%**).",
                                state.window_opacity * 100.0f);
                        } else {
                            std::snprintf(op_tip, sizeof(op_tip),
                                "**Continuous Backdrop Opacity**\n\nDrag or spin mouse wheel to tune glass transparency (**%.0f%%**).",
                                state.window_opacity * 100.0f);
                        }
                        DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), ImGui::IsItemHovered(), op_tip);
                    }
                    ImGui::SameLine(0.0f, section_gap);
                }

                const char* off_tip = cfg.isPoppedOut
                    ? "Custom HUD: <pill color=#7f8c8d>OFF</pill>\n\nStandard opaque window styling. Click to enable saturation control."
                    : "Transparent HUD: <pill color=#7f8c8d>OFF</pill>\n\nOpaque window background. Click to enable continuous glass opacity.";
                const char* on_tip = cfg.isPoppedOut
                    ? "Custom HUD: <pill color=#2ecc71>ON</pill>\n\nContinuous backdrop saturation active. Click to restore opaque background."
                    : "Transparent HUD: <pill color=#2ecc71>ON</pill>\n\nGlass HUD backdrop active. Click to restore opaque background.";
                const char* toggle_tips[2] = { off_tip, on_tip };

                bool trans_toggled = false;
#if defined(IMGUI_TOGGLE_BUTTONS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_TOGGLE_BUTTONS) && !defined(CONTRIB_DISABLE_TOGGLE_BUTTONS)
                trans_toggled = ImGui::ToggleButton(
                    toggle_label, &state.transparent_hud, ImVec2(0.0f, 0.0f), toggle_tips, true,
                    state.dynamic_cursors_enabled ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_None);
#else
                (void)toggle_tips;
                trans_toggled = ImGui::Checkbox(toggle_label, &state.transparent_hud);
#endif
                if (trans_toggled) {
                    ApplyDocumentViewerTheme(state.theme_index, state.transparent_hud, state.window_opacity);
                    if (cfg.onTransparencyToggled) cfg.onTransparencyToggled(state.transparent_hud, cfg.userData);
                }

                if (cfg.showThemePicker) {
                    ImGui::SameLine(0.0f, section_gap);
                }
            }

            if (cfg.showThemePicker) {
                static const char* const kThemeNames[DocumentViewerTheme_COUNT] = {
                    "Charcoal (Default)",
                    "Rusty (Bronze)",
                    "ImGui Dark",
                    "ImGui Classic",
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
                    "Paper Light"
#else
                    "ImGui Light"
#endif
                };
                static const char* const kThemeTips[DocumentViewerTheme_COUNT] = {
                    "**Charcoal (Default)**\nSleek modern slate and graphite studio palette.",
                    "**Rusty (Bronze)**\nWarm low-glare bronze and terracotta palette.",
                    "**ImGui Dark**\nStock Dear ImGui dark color scheme.",
                    "**ImGui Classic**\nClassic Dear ImGui indigo color scheme.",
#if defined(IMGUI_STYLES_H) && !defined(IMGEE_DISABLE_THEMES) && !defined(CONTRIB_DISABLE_THEMES)
                    "**Paper Light**\nClean, high-contrast publication light color scheme."
#else
                    "**ImGui Light**\nHigh-contrast daytime light color scheme."
#endif
                };

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Theme:");
                ImGui::SameLine(0.0f, 4.0f * r2_scale);
                ImGui::SetNextItemWidth(theme_combo_w);
                state.theme_index = ImClamp(state.theme_index, 0, DocumentViewerTheme_COUNT - 1);
                bool theme_changed = false;
#if defined(IMGUI_MOUSE_WHEEL_CONTROLS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_CONTROLS) && !defined(CONTRIB_DISABLE_MOUSE_WHEEL_COMBO)
                theme_changed = ImGui::MouseWheelCombo(
                    "##dv_theme", &state.theme_index, kThemeNames, DocumentViewerTheme_COUNT, -1, true, kThemeTips,
                    state.dynamic_cursors_enabled ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_None,
                    state.dynamic_cursors_enabled ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_None);
#else
                (void)kThemeTips;
                theme_changed = ImGui::Combo("##dv_theme", &state.theme_index, kThemeNames, DocumentViewerTheme_COUNT);
#endif
                if (theme_changed) {
                    ApplyDocumentViewerTheme(state.theme_index, state.transparent_hud, state.window_opacity);
                    if (cfg.onThemeChanged) cfg.onThemeChanged(state.theme_index, cfg.userData);
                }
                DocViewerDetail::ShowDelayedTip(DocViewerDetail::GetItemIDCompat(), ImGui::IsItemHovered(),
                    "**Color Theme Preset**\n\nClick dropdown or roll mouse wheel to cycle palettes while preserving opacity.");
            }
        }
    }

    if (cfg.showHeaderRow1 || cfg.showHeaderRow2) {
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        ImGui::Separator();
    }

    // ========================================================================
    // C. Nx ZOOMED MARKDOWN VIEWPORT BODY
    // ========================================================================
    float footer_reserve_h = 0.0f;
    const float footer_gap_y = 4.0f;
    float footer_content_h = 0.0f;
    if (cfg.showFooter) {
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        float footer_text_h = ImGui::GetTextLineHeight();
#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
        if (ImFont* sm = ImGui::GetFont(ImGui::ImFontRole::Small)) {
            ImGui::ImGeeCompat::PushFont(sm);
            footer_text_h = ImGui::GetTextLineHeight();
            ImGui::PopFont();
        }
#endif
        footer_content_h = cfg.showFooterCloseButton ? ImGui::GetFrameHeight() : footer_text_h;
        footer_reserve_h = footer_gap_y + 1.0f + footer_gap_y + footer_content_h;
    }

    const ImVec2 body_size(0.0f, (footer_reserve_h > 0.0f) ? -footer_reserve_h : 0.0f);
    const float pre_vp_zoom = state.viewport.zoom;

#if defined(IMGUI_ZOOMABLE_VIEWPORT_H) && !defined(IMGEE_DISABLE_ZOOMABLE_VIEWPORT) && !defined(CONTRIB_DISABLE_ZOOMABLE_VIEWPORT) && !defined(IMGUI_DISABLE_ZOOMABLE_VIEWPORT)
    const bool body_visible = ImGui::BeginZoomedViewport(
        "##dv_markdown_body", state.viewport, body_size, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    if (state.viewport._zoom_changed_this_frame) {
        notifyZoomChange(pre_vp_zoom, state.viewport.zoom);
    }
#else
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, state.viewport.content_padding);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
    const bool body_visible = ImGui::BeginChild(
        "##dv_markdown_body", body_size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar);
#else
    const bool body_visible = ImGui::BeginChild(
        "##dv_markdown_body", body_size, false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysVerticalScrollbar);
#endif
    ImGui::ImGeeCompat::SetWindowFontScale(state.viewport.zoom);
    (void)pre_vp_zoom;
#endif

    if (body_visible) {
        ImGuiWindow* vp_win = ImGui::GetCurrentWindow();
        const float top_pad_y = vp_win ? vp_win->WindowPadding.y : 14.0f;

        bool applied_raw_scroll = false;
        if (state.pending_raw_scroll_y >= 0.0f) {
            const float raw_target_y = (std::max)(0.0f, state.pending_raw_scroll_y);
            ImGui::SetScrollY(raw_target_y);
            state.last_scroll_y = raw_target_y;
            state.pending_raw_scroll_y = -1.0f;
            applied_raw_scroll = true;
        }

        // Immediate jump if target TOC entry already has a measured content_y from a prior frame
        if (state.pending_scroll_toc_index >= 0 &&
            static_cast<size_t>(state.pending_scroll_toc_index) < state.toc.size())
        {
            const float target_y = state.toc[static_cast<size_t>(state.pending_scroll_toc_index)].content_y;
            if (target_y >= 0.0f) {
                ImGui::SetScrollY((std::max)(0.0f, target_y - top_pad_y));
                state.pending_scroll_toc_index = -1;
            }
        }

#if defined(IMGUI_MARKDOWN_EXT_H) && !defined(IMGEE_DISABLE_MARKDOWN) && !defined(CONTRIB_DISABLE_MARKDOWN)
        const MarkdownConfig* base_md = cfg.markdownConfig ? cfg.markdownConfig : ImGui::GetDefaultMarkdownConfig();
        MarkdownConfig md_cfg = base_md ? *base_md : MarkdownConfig();
        if (cfg.baseDir && cfg.baseDir[0] != '\0') {
            md_cfg.baseDir = cfg.baseDir;
        }
        if (!md_cfg.imageCallback) {
            md_cfg.imageCallback = &ImGui::MarkdownImageService::DefaultMarkdownImageCallback;
        }
        md_cfg.enableImageLightbox = cfg.enableImageLightbox;
        md_cfg.showImageCaptions   = cfg.showImageCaptions;
        md_cfg.enableTableTooltips = cfg.enableTableTooltips;
        md_cfg.wrapWidth           = 0.0f;

        // Wire semantic font roles automatically if caller did not populate MarkdownConfig fonts
#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
        if (!md_cfg.h1Font)         md_cfg.h1Font         = ImGui::GetFont(ImGui::ImFontRole::Heading1);
        if (!md_cfg.h2Font)         md_cfg.h2Font         = ImGui::GetFont(ImGui::ImFontRole::Heading2);
        if (!md_cfg.h3Font)         md_cfg.h3Font         = ImGui::GetFont(ImGui::ImFontRole::Heading3);
        if (!md_cfg.boldFont)       md_cfg.boldFont       = ImGui::GetFont(ImGui::ImFontRole::Bold);
        if (!md_cfg.italicFont)     md_cfg.italicFont     = ImGui::GetFont(ImGui::ImFontRole::Italic);
        if (!md_cfg.boldItalicFont) md_cfg.boldItalicFont = ImGui::GetFont(ImGui::ImFontRole::BoldItalic);
        if (!md_cfg.monoFont)       md_cfg.monoFont       = ImGui::GetFont(ImGui::ImFontRole::Monospace);
        if (!md_cfg.regularFont)    md_cfg.regularFont    = ImGui::GetFont(ImGui::ImFontRole::Regular);
#endif

        DocViewerDetail::ActiveDocRenderContext render_ctx;
        render_ctx.state          = &state;
        render_ctx.config         = &cfg;
        render_ctx.viewportWindow = vp_win;
        render_ctx.topPaddingY    = top_pad_y;
        render_ctx.headingCursor  = 0;
        render_ctx.baseFormatCb   = md_cfg.formatCallback ? md_cfg.formatCallback : &ImGui::defaultMarkdownFormatCallback;
        render_ctx.baseLinkCb     = cfg.externalLinkCallback ? cfg.externalLinkCallback
                                  : (md_cfg.linkCallback ? md_cfg.linkCallback : ImGui::GetDefaultMarkdownLinkCallback());
        render_ctx.baseUserData   = md_cfg.userData ? md_cfg.userData : cfg.userData;

        DocViewerDetail::ActiveDocRenderContext* prev_ctx = DocViewerDetail::GetActiveRenderContext();
        DocViewerDetail::GetActiveRenderContext() = &render_ctx;

        md_cfg.formatCallback = [](const MarkdownFormatInfo& info, bool start) {
            DocViewerDetail::ActiveDocRenderContext* rctx = DocViewerDetail::GetActiveRenderContext();
            if (rctx && rctx->baseFormatCb) {
                rctx->baseFormatCb(info, start);
            } else {
                ImGui::defaultMarkdownFormatCallback(info, start);
            }
            if (start && rctx && rctx->state &&
                info.type == MarkdownFormatType::HEADING &&
                info.level >= 1 && info.level <= rctx->state->_cached_max_toc_level &&
                ImGui::GetCurrentWindow() == rctx->viewportWindow &&
                ImGui::GetCurrentTable() == nullptr)
            {
                if (rctx->headingCursor < rctx->state->toc.size()) {
                    rctx->state->toc[rctx->headingCursor].content_y = ImGui::GetCursorPosY();
                    rctx->headingCursor++;
                }
            }
        };

        md_cfg.linkCallback = [](MarkdownLinkCallbackData data) {
            DocViewerDetail::ActiveDocRenderContext* rctx = DocViewerDetail::GetActiveRenderContext();
            if (!data.link || data.linkLength <= 0) return;

            // Intercept internal #heading-slug links directly inside the DocumentViewer!
            if (!data.isImage && data.link[0] == '#') {
                if (rctx && rctx->state) {
                    std::string anchor(data.link, static_cast<size_t>(data.linkLength));
                    ImGui::JumpToDocumentAnchor(*rctx->state, anchor.c_str());
                }
                return;
            }

            if (rctx && rctx->baseLinkCb) {
                MarkdownLinkCallbackData fwd = data;
                fwd.userData = rctx->baseUserData;
                rctx->baseLinkCb(fwd);
                return;
            }

#if defined(IMGUI_CLICKABLE_LINKS_READY) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_CLICKABLE_LINKS) && !defined(CONTRIB_DISABLE_CLICKABLE_LINKS)
            if (!data.isImage) {
                std::string url(data.link, static_cast<size_t>(data.linkLength));
                ImGui::OpenURL(url);
            }
#endif
        };

        ImGui::MarkdownExt(markdown_text, markdown_len, &md_cfg, true);

        DocViewerDetail::GetActiveRenderContext() = prev_ctx;
#else
        ImGui::TextWrapped("%.*s", static_cast<int>(markdown_len), markdown_text);
#endif

        // Same-frame forward jump resolution (e.g. first-frame JumpToDocumentHeading or clicking a #slug link!)
        if (state.pending_scroll_toc_index >= 0 &&
            static_cast<size_t>(state.pending_scroll_toc_index) < state.toc.size())
        {
            const float target_y = state.toc[static_cast<size_t>(state.pending_scroll_toc_index)].content_y;
            if (target_y >= 0.0f) {
                ImGui::SetScrollY((std::max)(0.0f, target_y - top_pad_y));
                state.pending_scroll_toc_index = -1;
            }
        }

        // Sync active_toc_index with manual user scrolling
        if (!applied_raw_scroll) {
            state.last_scroll_y = ImGui::GetScrollY();
            if (state.pending_scroll_toc_index < 0 && !state.toc.empty()) {
                const float max_scroll = ImGui::GetScrollMaxY();
                if (max_scroll > 4.0f && state.last_scroll_y >= max_scroll - 4.0f) {
                    state.active_toc_index = static_cast<int>(state.toc.size()) - 1;
                } else {
                    const float probe_y = state.last_scroll_y + top_pad_y + 12.0f;
                    int current_sec = 0;
                    for (size_t i = 0; i < state.toc.size(); ++i) {
                        if (state.toc[i].content_y >= 0.0f && state.toc[i].content_y <= probe_y) {
                            current_sec = static_cast<int>(i);
                        }
                    }
                    state.active_toc_index = current_sec;
                }
            }
        }
    }

#if defined(IMGUI_ZOOMABLE_VIEWPORT_H) && !defined(IMGEE_DISABLE_ZOOMABLE_VIEWPORT) && !defined(CONTRIB_DISABLE_ZOOMABLE_VIEWPORT) && !defined(IMGUI_DISABLE_ZOOMABLE_VIEWPORT)
    ImGui::EndZoomedViewport(state.viewport);
#else
    ImGui::EndChild();
    ImGui::PopStyleVar();
#endif
    const float child_bottom_y = ImGui::GetItemRectMax().y;

    // ========================================================================
    // D. OPTIONAL 1.0x FOOTER CHROME (Base-Font Isolated from TimedTooltip)
    // ========================================================================
    if (cfg.showFooter) {
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        const float footer_left_x = ImGui::GetCursorScreenPos().x;
        ImGui::SetCursorScreenPos(ImVec2(footer_left_x, child_bottom_y + footer_gap_y));
        ImGui::Separator();
        ImGui::SetCursorScreenPos(ImVec2(footer_left_x, child_bottom_y + footer_gap_y + 1.0f + footer_gap_y));

        bool pushed_small = false;
#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
        if (ImFont* sm = ImGui::GetFont(ImGui::ImFontRole::Small)) {
            ImGui::ImGeeCompat::PushFont(sm);
            pushed_small = true;
        }
#endif
        if (cfg.showFooterCloseButton) {
            ImGui::AlignTextToFramePadding();
        }
        if (cfg.footerHint && cfg.footerHint[0] != '\0') {
            ImGui::TextDisabled("%s", cfg.footerHint);
        } else if (!state.toc.empty() &&
                   state.active_toc_index >= 0 &&
                   static_cast<size_t>(state.active_toc_index) < state.toc.size())
        {
            ImGui::TextDisabled("%s  |  Click images to enlarge",
                                state.toc[static_cast<size_t>(state.active_toc_index)].title.c_str());
        } else {
            ImGui::TextDisabled("Click images to enlarge");
        }
        if (pushed_small) {
            ImGui::PopFont();
        }

        if (cfg.showFooterCloseButton) {
            const float close_w = 64.0f;
            const float close_x = ImGui::GetWindowWidth() - close_w - right_margin;
            ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + 8.0f, close_x));
            if (ImGui::Button("Close##dv_footer_close", ImVec2(close_w, footer_content_h))) {
                state.is_open = false;
                if (cfg.onCloseRequested) cfg.onCloseRequested(cfg.userData);
            }
            if (ImGui::IsItemHovered()) {
                DocViewerDetail::SetToolbarCursor(state, ImGuiMouseCursor_Hand);
            }
        }
    }

    // Restore caller's ambient window font scale cleanly
    if (host_win && std::fabs(host_win->FontWindowScale - saved_host_scale) > 0.0001f) {
        ImGui::ImGeeCompat::SetWindowFontScale(saved_host_scale);
    }

    if (use_outer_child) {
        ImGui::EndChild();
    }
    ImGui::PopID();

    return state.is_open;
}

inline bool DocumentViewer(
    const char* str_id,
    const std::string& markdown_text,
    DocumentViewerState& state,
    const DocumentViewerConfig* config = nullptr,
    const ImVec2& size = ImVec2(0.0f, 0.0f))
{
    return DocumentViewer(str_id, markdown_text.c_str(), markdown_text.size(), state, config, size);
}

inline bool DocumentViewer(
    const char* str_id,
    const char* markdown_text,
    DocumentViewerState& state,
    const DocumentViewerConfig* config = nullptr,
    const ImVec2& size = ImVec2(0.0f, 0.0f))
{
    const size_t len = markdown_text ? std::strlen(markdown_text) : 0;
    return DocumentViewer(str_id, markdown_text, len, state, config, size);
}

// Convenience overload with automatic per-ID persistent state
inline bool DocumentViewer(
    const char* str_id,
    const std::string& markdown_text,
    const DocumentViewerConfig* config = nullptr,
    const ImVec2& size = ImVec2(0.0f, 0.0f))
{
    static std::unordered_map<ImGuiID, DocumentViewerState> s_States;
    const ImGuiID key = ImGui::GetID(str_id ? str_id : "##imgee_doc_viewer");
    return DocumentViewer(str_id, markdown_text.c_str(), markdown_text.size(), s_States[key], config, size);
}

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_DOC_VIEWER_H
#endif // !IMGEE_DISABLE_DOC_VIEWER
