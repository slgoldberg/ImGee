// ============================================================================
// ImGee Extension: Markdown (extensions/Markdown/imgui_markdown_ext.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// Original Upstream: Juliette Foucaut (@juliettef) & Doug Binks (@dougbinks)
// License: BSD 3-Clause (see LICENSE)
//
// Contributors:
//   - Steven L. Goldberg: Lookahead line-wrapping, typographic delimiter
//     protection, hanging indents, dynamic font scaling, pre-calculation,
//     and parameterized styling tags.
// ============================================================================

#if !defined(IMGEE_DISABLE_EXTENSIONS) && \
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
    !defined(IMGWINDOW_DISABLE_CONTRIB_MARKDOWN)
#ifndef IMGUI_MARKDOWN_EXT_H
#define IMGUI_MARKDOWN_EXT_H

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
    #endif
#else
    #include "imgui_internal.h"
#endif
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

#ifndef IMGUI_PRINTFARGS
#if defined(__clang__) || defined(__GNUC__)
#define IMGUI_PRINTFARGS(FMT, X) __attribute__((format(printf, FMT, X)))
#else
#define IMGUI_PRINTFARGS(FMT, X)
#endif
#endif
#include "imgui_markdown.h"
#include "imgui_markdown_image_service.h"
#if !defined(IMGEE_DISABLE_ZOOMABLE_VIEWPORT) && \
    !defined(CONTRIB_DISABLE_ZOOMABLE_VIEWPORT) && \
    !defined(IMGUI_DISABLE_ZOOMABLE_VIEWPORT)
#if defined(__has_include)
    #if __has_include("../Viewport/imgui_zoomable_viewport.h")
        #include "../Viewport/imgui_zoomable_viewport.h"
    #endif
#else
    #include "../Viewport/imgui_zoomable_viewport.h"
#endif
#endif
#if !defined(IMGEE_DISABLE_BADGES) && \
    !defined(IMGEE_DISABLE_WIDGETS) && \
    !defined(CONTRIB_DISABLE_BADGES) && \
    !defined(CONTRIB_DISABLE_BADGE) && \
    !defined(CONTRIB_DISABLE_WIDGETS) && \
    !defined(IMGUI_CONTRIB_DISABLE_BADGES) && \
    !defined(IMGUI_DISABLE_CONTRIB_BADGES) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_WIDGETS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)
#if defined(__has_include)
    #if __has_include("../../widgets/imgui_badges.h")
        #include "../../widgets/imgui_badges.h"
        #define CONTRIB_HAS_BADGES 1
    #endif
#else
    #include "../../widgets/imgui_badges.h"
    #define CONTRIB_HAS_BADGES 1
#endif
#endif

#if defined(CONTRIB_ICONS_AVAILABLE)
    #ifndef CONTRIB_HAS_ICONS
        #define CONTRIB_HAS_ICONS 1
    #endif
#elif !defined(IMGEE_DISABLE_ICONS) && \
    !defined(IMGEE_DISABLE_WIDGETS) && \
    !defined(CONTRIB_DISABLE_ICONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_ICONS) && \
    !defined(IMGUI_DISABLE_ICONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_ICONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_WIDGETS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)
#if defined(__has_include)
    #if __has_include("../../widgets/imgui_icons.h")
        #include "../../widgets/imgui_icons.h"
    #elif __has_include("imgui_icons.h")
        #include "imgui_icons.h"
    #endif
#else
    #include "../../widgets/imgui_icons.h"
#endif
#if defined(CONTRIB_ICONS_AVAILABLE) && !defined(CONTRIB_HAS_ICONS)
    #define CONTRIB_HAS_ICONS 1
#endif
#endif

#if !defined(IMGEE_DISABLE_TYPOGRAPHY) && \
    !defined(IMGEE_DISABLE_EXTENSIONS) && \
    !defined(CONTRIB_DISABLE_TYPOGRAPHY) && \
    !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_TYPOGRAPHY) && \
    !defined(IMGUI_DISABLE_CONTRIB_TYPOGRAPHY) && \
    !defined(IMGUI_DISABLE_TYPOGRAPHY) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_TYPOGRAPHY) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB)
#if defined(__has_include)
    #if __has_include("../Typography/imgui_font_roles.h")
        #include "../Typography/imgui_font_roles.h"
        #define CONTRIB_HAS_TYPOGRAPHY 1
    #endif
#else
    #include "../Typography/imgui_font_roles.h"
    #define CONTRIB_HAS_TYPOGRAPHY 1
#endif
#endif

namespace ImGui {

    // -------------------------------------------------------------------------
    // ImGui Markdown Extensions (Tags & Styling Widgets)
    // -------------------------------------------------------------------------
    
    // 1. Config Registry (shared with imgui_markdown.h)
#ifndef IMGUI_MARKDOWN_CONFIG_REGISTRY_DEFINED
#define IMGUI_MARKDOWN_CONFIG_REGISTRY_DEFINED
    inline const MarkdownConfig*& GetDefaultMarkdownConfig() {
        static const MarkdownConfig* s_DefaultConfig = nullptr;
        return s_DefaultConfig;
    }

    inline void SetDefaultMarkdownConfig(const MarkdownConfig* config) {
        GetDefaultMarkdownConfig() = config;
    }

    inline const MarkdownConfig*& GetActiveMarkdownConfig() {
        static const MarkdownConfig* s_ActiveConfig = nullptr;
        return s_ActiveConfig;
    }
#endif

    // 2. Color Helper for Built-in Styling Tags
    // (ParseMarkdownColor is defined in imgui_markdown.h in namespace ImGui)

    // 3. Custom Markdown Styling Tags Pre-Processor
    struct MarkdownTagStyle {
        std::string clean_text;
        std::string color_param; // Legacy single-color parameter
        std::string bg_color;    // Background color override or primary color
        std::string fg_color;    // Foreground / text color override
        std::string icon_name;   // Named texture icon identifier (e.g. "avionics_beacon")
        bool is_bold = false;
        bool is_italic = false;
        bool is_mono = false;
        bool is_strike = false;
        bool is_underline = false;
    };

    inline MarkdownTagStyle ParseMarkdownTagStyle(const std::string& inner_text, const std::string& param, bool is_capsule_tag = false, bool strip_inner_markdown = true) {
        MarkdownTagStyle style;
        style.clean_text = inner_text;

        auto strip_quotes = [](std::string s) -> std::string {
            if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
                return s.substr(1, s.size() - 2);
            }
            return s;
        };

        // 1. Check if param contains 'bold', 'italic', 'mono', or color tokens
        std::string p = param;
        for (char& c : p) {
            if (c == ',' || c == ';') c = ' ';
        }
        size_t pos = 0;
        bool found_explicit_style = false;
        while (pos < p.length()) {
            while (pos < p.length() && (p[pos] == ' ' || p[pos] == '\t')) ++pos;
            if (pos >= p.length()) break;
            size_t end = pos;
            while (end < p.length() && p[end] != ' ' && p[end] != '\t') ++end;
            std::string token = p.substr(pos, end - pos);
            std::string t = token;
            for (char& c : t) c = (char)tolower((unsigned char)c);
            if (t == "bolditalic" || t == "bi" || t == "bold-italic" || t == "style=bolditalic" || t == "font=bolditalic") {
                style.is_bold = true;
                style.is_italic = true;
                found_explicit_style = true;
            } else if (t == "bold" || t == "b" || t == "font=bold" || t == "style=bold") {
                style.is_bold = true;
                found_explicit_style = true;
            } else if (t == "italic" || t == "italics" || t == "i" || t == "font=italic" || t == "style=italic") {
                style.is_italic = true;
                found_explicit_style = true;
            } else if (t == "mono" || t == "monospace" || t == "code" || t == "m" || t == "font=mono" || t == "style=mono") {
                style.is_mono = true;
                found_explicit_style = true;
            } else if (t == "strike" || t == "strikethrough" || t == "del" || t == "s") {
                style.is_strike = true;
                found_explicit_style = true;
            } else if (t == "underline" || t == "u" || t == "ins") {
                style.is_underline = true;
                found_explicit_style = true;
            } else if (t.rfind("icon=", 0) == 0) {
                style.icon_name = strip_quotes(token.substr(5));
                found_explicit_style = true;
            } else if (t.rfind("img=", 0) == 0) {
                style.icon_name = strip_quotes(token.substr(4));
                found_explicit_style = true;
            } else if (t.rfind("image=", 0) == 0) {
                style.icon_name = strip_quotes(token.substr(6));
                found_explicit_style = true;
            } else {
                if (t.rfind("fgcolor=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(8));
                } else if (t.rfind("fgcol=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(6));
                } else if (t.rfind("fg=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(3));
                } else if (t.rfind("text=", 0) == 0) {
                    style.fg_color = strip_quotes(token.substr(5));
                } else if (t.rfind("bgcolor=", 0) == 0 || t.rfind("bgcol=", 0) == 0 || t.rfind("bg=", 0) == 0) {
                    size_t eq = token.find('=');
                    std::string val = strip_quotes(token.substr(eq + 1));
                    if (!style.bg_color.empty() && style.fg_color.empty()) {
                        // Shift previously assigned bare color to foreground text
                        style.fg_color = style.bg_color;
                    }
                    style.bg_color = val;
                } else if (t.rfind("color=", 0) == 0 || t.rfind("col=", 0) == 0) {
                    size_t eq = token.find('=');
                    std::string val = strip_quotes(token.substr(eq + 1));
                    if (is_capsule_tag) {
                        if (style.bg_color.empty()) style.bg_color = val;
                        else style.fg_color = val;
                    } else {
                        if (style.fg_color.empty()) style.fg_color = val;
                        else style.bg_color = val;
                    }
                } else {
                    // Bare token (e.g. "green", "#ff0000", "danger")
                    std::string val = strip_quotes(token);
                    if (!val.empty()) {
                        if (is_capsule_tag) {
                            if (style.bg_color.empty()) {
                                style.bg_color = val;
                            } else if (style.fg_color.empty()) {
                                style.fg_color = val;
                            }
                        } else {
                            if (style.fg_color.empty()) {
                                style.fg_color = val;
                            } else if (style.bg_color.empty()) {
                                style.bg_color = val;
                            }
                        }
                    }
                }
            }
            pos = end;
        }
        if (is_capsule_tag) {
            if (!style.bg_color.empty()) {
                style.color_param = style.bg_color;
            } else if (!style.fg_color.empty()) {
                style.color_param = style.fg_color;
            } else if (!found_explicit_style && !param.empty()) {
                style.color_param = strip_quotes(param);
                style.bg_color = style.color_param;
            }
        } else {
            if (!style.fg_color.empty()) {
                style.color_param = style.fg_color;
            } else if (!style.bg_color.empty()) {
                style.color_param = style.bg_color;
            } else if (!found_explicit_style && !param.empty()) {
                style.color_param = strip_quotes(param);
                style.fg_color = style.color_param;
            }
        }

        // 2. Normalize any UTF-8 non-breaking spaces (\xC2\xA0, e.g. macOS Option+Space)
        // to standard ASCII ' ' (0x20) so fonts baking only 7-bit ASCII (0x20..0x7E)
        // still measure and render the exact advance width of a space glyph:
        for (size_t i = 0; i + 1 < style.clean_text.length(); ++i) {
            if ((unsigned char)style.clean_text[i] == 0xC2 &&
                (unsigned char)style.clean_text[i + 1] == 0xA0) {
                style.clean_text.replace(i, 2, " ");
            }
        }

        // 2b. Extract embedded vector icon tag if inner_text contains an injected <icon ... /> tag (only for dedicated capsule widgets like pill/badge/btn):
        if (is_capsule_tag) {
            size_t icon_tag = style.clean_text.find("<icon ");
            if (icon_tag != std::string::npos) {
                size_t tag_end = style.clean_text.find(">", icon_tag);
                if (tag_end != std::string::npos) {
                    std::string inside = style.clean_text.substr(icon_tag + 6, tag_end - (icon_tag + 6));
                    while (!inside.empty() && (inside.back() == '/' || inside.back() == ' ' || inside.back() == '\t')) inside.pop_back();
                    while (!inside.empty() && (inside.front() == ' ' || inside.front() == '\t')) inside.erase(0, 1);
                    if (style.icon_name.empty() && !inside.empty()) {
#if defined(CONTRIB_HAS_ICONS)
                        style.icon_name = NormalizeIconName(inside.c_str());
#else
                        style.icon_name = inside;
#endif
                    }
                    style.clean_text.erase(icon_tag, tag_end - icon_tag + 1);
                    while (!style.clean_text.empty() && (style.clean_text.front() == ' ' || style.clean_text.front() == '\t')) style.clean_text.erase(0, 1);
                    while (!style.clean_text.empty() && (style.clean_text.back() == ' ' || style.clean_text.back() == '\t')) style.clean_text.pop_back();
                }
            }
        }

        // 3. Locate non-whitespace core to check for markdown bold/italic/code syntax
        // (**text**, __text__, *text*, _text_, `text`) WITHOUT discarding intentional
        // leading/trailing padding spaces (e.g., "<pill> " ICON_FA_RETWEET " </pill>"):
        if (strip_inner_markdown) {
            size_t start_idx = 0;
            while (start_idx < style.clean_text.length() && style.clean_text[start_idx] == ' ') ++start_idx;
            size_t end_idx = style.clean_text.length();
            while (end_idx > start_idx && style.clean_text[end_idx - 1] == ' ') --end_idx;

            std::string lead_spaces = style.clean_text.substr(0, start_idx);
            std::string core = style.clean_text.substr(start_idx, end_idx - start_idx);
            std::string trail_spaces = style.clean_text.substr(end_idx);

            if ((core.length() >= 6 && core.rfind("***", 0) == 0 && core.compare(core.length() - 3, 3, "***") == 0) ||
                (core.length() >= 6 && core.rfind("___", 0) == 0 && core.compare(core.length() - 3, 3, "___") == 0))
            {
                style.is_bold = true;
                style.is_italic = true;
                core = core.substr(3, core.length() - 6);
            }
            else if ((core.length() >= 4 && core.rfind("**", 0) == 0 && core.compare(core.length() - 2, 2, "**") == 0) ||
                (core.length() >= 4 && core.rfind("__", 0) == 0 && core.compare(core.length() - 2, 2, "__") == 0))
            {
                style.is_bold = true;
                core = core.substr(2, core.length() - 4);
            }
            else if ((core.length() >= 2 && core.front() == '*' && core.back() == '*') ||
                     (core.length() >= 2 && core.front() == '_' && core.back() == '_'))
            {
                style.is_italic = true;
                core = core.substr(1, core.length() - 2);
            }
            else if (core.length() >= 2 && core.front() == '`' && core.back() == '`')
            {
                style.is_mono = true;
                core = core.substr(1, core.length() - 2);
            }
            else if (core.length() >= 4 && core.rfind("~~", 0) == 0 && core.compare(core.length() - 2, 2, "~~") == 0)
            {
                style.is_strike = true;
                core = core.substr(2, core.length() - 4);
            }
            else if (core.length() >= 4 && core.rfind("$$", 0) == 0 && core.compare(core.length() - 2, 2, "$$") == 0)
            {
                style.is_strike = true;
                core = core.substr(2, core.length() - 4);
            }
            else if (core.length() >= 4 && core.rfind("++", 0) == 0 && core.compare(core.length() - 2, 2, "++") == 0)
            {
                style.is_underline = true;
                core = core.substr(2, core.length() - 4);
            }
            style.clean_text = lead_spaces + core + trail_spaces;
        }

        return style;
    }

    inline void PushMarkdownTagFont(const MarkdownConfig* config, const MarkdownTagStyle& style) {
        if (!config) config = GetActiveMarkdownConfig();
        if (!config) config = GetDefaultMarkdownConfig();
        if (style.is_mono) {
            ImFont* mono = (config && config->monoFont) ? config->monoFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!mono) {
                if (style.is_bold) mono = ImGui::GetFont(ImFontRole::MonospaceMedium);
                if (!mono) mono = ImGui::GetFont(ImFontRole::Monospace);
            }
#endif
            if (mono) {
                MarkdownDetail::PushMarkdownFont(mono);
                return;
            }
        }

        ImFont* fontToPush = nullptr;
        if (style.is_bold && style.is_italic) {
            fontToPush = (config && config->boldItalicFont) ? config->boldItalicFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPush) fontToPush = ImGui::GetFont(ImFontRole::BoldItalic);
#endif
        } else if (style.is_bold) {
            fontToPush = (config && config->boldFont) ? config->boldFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPush) fontToPush = ImGui::GetFont(ImFontRole::Bold);
#endif
        } else if (style.is_italic) {
            fontToPush = (config && config->italicFont) ? config->italicFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPush) fontToPush = ImGui::GetFont(ImFontRole::Italic);
#endif
        }
        if (fontToPush) {
            MarkdownDetail::PushMarkdownFont(fontToPush);
            return;
        }

        if (!config || !config->formatCallback) {
            return;
        }
        if (style.is_bold && style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 3; // Bold-Italic
            config->formatCallback(info, true);
        } else if (style.is_bold) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 2; // Bold
            config->formatCallback(info, true);
        } else if (style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 1; // Italic
            config->formatCallback(info, true);
        }
    }

    inline void PopMarkdownTagFont(const MarkdownConfig* config, const MarkdownTagStyle& style) {
        if (!config) config = GetActiveMarkdownConfig();
        if (!config) config = GetDefaultMarkdownConfig();
        if (style.is_mono) {
            ImFont* mono = (config && config->monoFont) ? config->monoFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!mono) {
                if (style.is_bold) mono = ImGui::GetFont(ImFontRole::MonospaceMedium);
                if (!mono) mono = ImGui::GetFont(ImFontRole::Monospace);
            }
#endif
            if (mono) {
                ImGui::PopFont();
                return;
            }
        }

        ImFont* fontToPop = nullptr;
        if (style.is_bold && style.is_italic) {
            fontToPop = (config && config->boldItalicFont) ? config->boldItalicFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPop) fontToPop = ImGui::GetFont(ImFontRole::BoldItalic);
#endif
        } else if (style.is_bold) {
            fontToPop = (config && config->boldFont) ? config->boldFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPop) fontToPop = ImGui::GetFont(ImFontRole::Bold);
#endif
        } else if (style.is_italic) {
            fontToPop = (config && config->italicFont) ? config->italicFont : nullptr;
#ifndef CONTRIB_DISABLE_TYPOGRAPHY
            if (!fontToPop) fontToPop = ImGui::GetFont(ImFontRole::Italic);
#endif
        }
        if (fontToPop) {
            ImGui::PopFont();
            return;
        }

        if (!config || !config->formatCallback) {
            return;
        }
        if (style.is_bold && style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 3; // Bold-Italic
            config->formatCallback(info, false);
        } else if (style.is_bold) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 2; // Bold
            config->formatCallback(info, false);
        } else if (style.is_italic) {
            MarkdownFormatInfo info;
            info.config = config;
            info.type = MarkdownFormatType::EMPHASIS;
            info.level = 1; // Italic
            config->formatCallback(info, false);
        }
    }

    using CustomTagCallback = std::function<void(const std::string& inner_text, const std::string& param)>;

    inline void EnsureTagFitsOnLine(float required_width) {
        const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
        if (config && config->wrapWidth < 0.0f) return;
        float lineStartX = 0.0f;
        float avail = 0.0f;
        MarkdownDetail::ResolveLineFloatGeometry(config ? config->wrapWidth : 0.0f, lineStartX, avail);
        float cursorX = ImGui::GetCursorScreenPos().x;
        float scale = (ImGui::GetFontSize() > 0.0f) ? (ImGui::GetFontSize() / 14.0f) : 1.0f;
        // Only wrap down if we are mid-line (cursor is not at the start) and the tag doesn't fit on this line:
        if ((cursorX - lineStartX) > (10.0f * scale) && required_width > avail) {
            ImGui::NewLine();
            MarkdownDetail::PrepareLineStartForFloats();
        }
    }

    // Pre-processes Markdown text to convert procedural vector icon shortcuts:
    // 1. Single-glyph backtick rescue: `↺` -> <icon undo />, `▲` -> <icon caret-up />
    // 2. Steven's raw-fallback link macro: #[label](ICON_NAME) -> <icon ICON_NAME />
    // 3. Emoji shortcodes: :icon_name: -> <icon icon_name />
    // 4. Raw 3-byte Unicode glyphs: ↺ -> <icon undo />, ▲ -> <icon caret-up />, etc.
    // Preserves fenced code blocks (``` or ~~~).
    inline bool PreprocessMarkdownIconsAndShortcodes(const char* text, size_t len, std::string& out) {
        if (!text || len == 0) return false;
#if !defined(CONTRIB_HAS_ICONS)
        (void)out;
        return false;
#else
        out.reserve(len + 128);
        bool any_replaced = false;
        bool in_fence = false;
        char fence_char = '\0';
        size_t fence_len = 0;

        size_t i = 0;
        while (i < len) {
            // Check for start/end of fenced code blocks (``` or ~~~), including inside '>' blockquotes:
            if (i == 0 || text[i-1] == '\n' || text[i-1] == '\r') {
                size_t fscan = i;
                while (fscan < len && (text[fscan] == ' ' || text[fscan] == '\t' || text[fscan] == '>')) ++fscan;
                if (fscan < len && (text[fscan] == '`' || text[fscan] == '~')) {
                    char fc = text[fscan];
                    size_t fl = 0;
                    while (fscan + fl < len && text[fscan + fl] == fc) ++fl;
                    if (fl >= 3) {
                        if (!in_fence) {
                            in_fence = true;
                            fence_char = fc;
                            fence_len = fl;
                        } else if (fc == fence_char && fl >= fence_len) {
                            in_fence = false;
                            fence_char = '\0';
                            fence_len = 0;
                        }
                        while (i < fscan + (size_t)fl) {
                            out.push_back(text[i++]);
                        }
                        continue;
                    }
                }
            }

            if (in_fence) {
                out.push_back(text[i++]);
                continue;
            }

            // 1. Backtick handling:
            //    - If single-glyph or bracketed single-glyph backtick rescue
            //      (e.g. `↺`, `[↺]`, `▲`, `▼`, `^`, `[^]`, `v`, `[v]`):
            //      convert to `<icon ... />` (preserving `[` / `]` if bracketed).
            //    - Otherwise (inline code span like `:plane:`, `<icon ... />`, `#[^](ICON)`):
            //      PRESERVE VERBATIM! Do NOT expand shortcodes or macros inside code spans.
            if (text[i] == '`') {
                size_t backtick_count = 0;
                while (i + backtick_count < len && text[i + backtick_count] == '`') {
                    ++backtick_count;
                }
                if (backtick_count <= 2) {
                    size_t k = i + backtick_count;
                    size_t close_pos = std::string::npos;
                    while (k < len && text[k] != '\n' && text[k] != '\r') {
                        if (text[k] == '`') {
                            size_t match_len = 0;
                            while (k + match_len < len && text[k + match_len] == '`') ++match_len;
                            if (match_len == backtick_count) {
                                close_pos = k;
                                break;
                            }
                            k += match_len;
                        } else {
                            ++k;
                        }
                    }

                    if (close_pos != std::string::npos) {
                        if (backtick_count == 1) {
                            std::string inner(text + i + 1, close_pos - (i + 1));
                            bool has_brackets = (inner.length() >= 3 && inner.front() == '[' && inner.back() == ']');
                            std::string query = has_brackets ? inner.substr(1, inner.length() - 2) : inner;
                            if (has_brackets) {
                                while (!query.empty() && (query.front() == ' ' || query.front() == '\t')) query.erase(query.begin());
                                while (!query.empty() && (query.back() == ' ' || query.back() == '\t')) query.pop_back();
                            }
                            std::string norm = NormalizeIconName(query.c_str());
                            if (IsVectorIcon(norm.c_str())) {
                                bool is_raw_glyph = (query.length() == 3 && (unsigned char)query[0] == 0xE2) ||
                                                    (query.length() == 4 && (unsigned char)query[0] == 0xF0 &&
                                                     (unsigned char)query[1] == 0x9F && (unsigned char)query[2] == 0x90 &&
                                                     ((unsigned char)query[3] == 0x89 || (unsigned char)query[3] == 0xB2));
                                if (is_raw_glyph || query == "^" || query == "v") {
                                    if (has_brackets) out.push_back('[');
                                    out.append("<icon ");
                                    out.append(norm);
                                    out.append(" />");
                                    if (has_brackets) out.push_back(']');
                                    any_replaced = true;
                                    i = close_pos + 1;
                                    continue;
                                }
                            }
                        }

                        // Preserving inline code verbatim
                        size_t span_len = (close_pos + backtick_count) - i;
                        out.append(text + i, span_len);
                        i += span_len;
                        continue;
                    }
                }
            }

            // 2. Steven's raw-fallback link macro: #[label](ICON_NAME)
            //    e.g. #[^](ICON_CARET_UP) or #[↺](ICON_UNDO)
            if (text[i] == '#' && i + 1 < len && text[i + 1] == '[') {
                size_t close_bracket = std::string::npos;
                for (size_t k = i + 2; k < len && text[k] != '\n' && text[k] != '\r'; ++k) {
                    if (text[k] == ']') { close_bracket = k; break; }
                }
                if (close_bracket != std::string::npos && close_bracket + 1 < len && text[close_bracket + 1] == '(') {
                    size_t close_paren = std::string::npos;
                    for (size_t k = close_bracket + 2; k < len && text[k] != '\n' && text[k] != '\r'; ++k) {
                        if (text[k] == ')') { close_paren = k; break; }
                    }
                    if (close_paren != std::string::npos) {
                        std::string target(text + close_bracket + 2, close_paren - (close_bracket + 2));
                        std::string target_icon = target;
                        std::string target_extra = "";
                        size_t sp = target.find(' ');
                        if (sp != std::string::npos) {
                            std::string w1 = target.substr(0, sp);
                            std::string w2 = target.substr(sp + 1);
                            if (IsVectorIcon(NormalizeIconName(w1.c_str()).c_str())) {
                                target_icon = NormalizeIconName(w1.c_str());
                                target_extra = w2;
                            } else if (IsVectorIcon(NormalizeIconName(w2.c_str()).c_str())) {
                                target_icon = NormalizeIconName(w2.c_str());
                                target_extra = w1;
                            }
                        } else {
                            target_icon = NormalizeIconName(target.c_str());
                        }

                        if (IsVectorIcon(target_icon.c_str())) {
                            out.append("<icon ");
                            out.append(target_icon);
                            if (!target_extra.empty()) {
                                out.push_back(' ');
                                out.append(target_extra);
                            }
                            out.append(" />");
                            any_replaced = true;
                            i = close_paren + 1;
                            continue;
                        }
                    }
                }
            }

            // 3. Emoji shortcodes: :icon_name:
            //    e.g. :undo:, :reset:, :caret_up:, :gear:, :check:
            if (text[i] == ':') {
                bool is_boundary = (i == 0 || (!isalnum((unsigned char)text[i-1]) && text[i-1] != ':' && text[i-1] != '\\'));
                if (is_boundary) {
                    size_t next_colon = std::string::npos;
                    for (size_t k = i + 1; k < len && text[k] != '\n' && text[k] != '\r' && text[k] != ' ' && text[k] != '\t' && (k - i) < 32; ++k) {
                        if (text[k] == ':') { next_colon = k; break; }
                    }
                    if (next_colon != std::string::npos && next_colon > i + 1) {
                        std::string candidate(text + i + 1, next_colon - (i + 1));
                        std::string norm = NormalizeIconName(candidate.c_str());
                        if (IsVectorIcon(norm.c_str())) {
                            out.append("<icon ");
                            out.append(norm);
                            out.append(" />");
                            any_replaced = true;
                            i = next_colon + 1;
                            continue;
                        }
                    }
                }
            }

            // 4. Raw 3-byte UTF-8 Unicode glyphs outside code blocks:
            if (i + 2 < len && (unsigned char)text[i] == 0xE2) {
                unsigned char c1 = (unsigned char)text[i + 1];
                unsigned char c2 = (unsigned char)text[i + 2];
                const char* icon_name = nullptr;
                if (c1 == 0x86 && c2 == 0xBA) icon_name = "undo";
                else if (c1 == 0x86 && c2 == 0xBB) icon_name = "redo";
                else if (c1 == 0x96 && c2 == 0xB2) icon_name = "caret-up";
                else if (c1 == 0x96 && (c2 == 0xBC || c2 == 0xBE)) icon_name = "caret-down";
                else if (c1 == 0x97 && c2 == 0x80) icon_name = "caret-left";
                else if (c1 == 0x96 && c2 == 0xB6) icon_name = "caret-right";
                else if (c1 == 0x9C && (c2 == 0x93 || c2 == 0x94)) icon_name = "check";
                else if (c1 == 0x9C && (c2 == 0x97 || c2 == 0x98)) icon_name = "cross";
                else if (c1 == 0x9A && c2 == 0x99) icon_name = "gear";
                else if (c1 == 0x84 && c2 == 0xB9) icon_name = "info";
                else if (c1 == 0x9A && c2 == 0xA0) icon_name = "warning";
                else if (c1 == 0x98 && c2 == 0x85) icon_name = "star";
                else if (c1 == 0x9C && c2 == 0x88) icon_name = "plane";

                if (icon_name) {
                    out.append("<icon ");
                    out.append(icon_name);
                    out.append(" />");
                    any_replaced = true;
                    i += 3;
                    continue;
                }
            }

            // 4b. Raw 4-byte UTF-8 House of Gee Ouroboros glyphs outside code blocks:
            if (i + 3 < len && (unsigned char)text[i] == 0xF0 &&
                (unsigned char)text[i + 1] == 0x9F && (unsigned char)text[i + 2] == 0x90) {
                unsigned char c3 = (unsigned char)text[i + 3];
                const char* dragon_icon = (c3 == 0x89) ? "ouroboros" : ((c3 == 0xB2) ? "ouroboros-alt" : nullptr);
                if (dragon_icon) {
                    out.append("<icon ");
                    out.append(dragon_icon);
                    out.append(" />");
                    any_replaced = true;
                    i += 4;
                    continue;
                }
            }

            out.push_back(text[i++]);
        }

        return any_replaced;
#endif
    }

    // Pre-measures the total rendered horizontal advance width of an inline Markdown line,
    // accounting for font headers (#, ##, ###), bold/italic/code spans, and inline capsules (<pill>, <badge>, <kbd>).
    inline float MeasureMarkdownLineWidth(const char* text, size_t len, const MarkdownConfig* config = nullptr) {
        if (!text || len == 0) return 0.0f;
        if (!config) config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();

#if defined(CONTRIB_HAS_ICONS)
        std::string processed_icons;
        if (PreprocessMarkdownIconsAndShortcodes(text, len, processed_icons)) {
            text = processed_icons.c_str();
            len = processed_icons.length();
        }
#endif

        // Check for leading header (#, ##, ###)
        int heading_level = 0;
        size_t i = 0;
        while (i < len && text[i] == ' ') ++i;
        size_t h_start = i;
        while (i < len && text[i] == '#') {
            ++heading_level;
            ++i;
        }
        if (heading_level > 0 && i < len && (text[i] == ' ' || text[i] == '\t')) {
            ++i; // skip space after '#'
            // CommonMark 4.2: Strip optional trailing closing '#' sequence
            const char* h_end = text + len;
            TrimTrailingHeadingHashes(text + i, h_end);
            len = (size_t)(h_end - text);
        } else {
            heading_level = 0;
            i = h_start;
        }

        ImFont* ambient_font = ImGui::GetFont();
        const float ambient_size = ImGui::GetFontSize();
        const float ambient_native = MarkdownDetail::GetNativeFontPixelSize(ambient_font);
        const float zoom_ratio = (ambient_size > 0.0f && ambient_native > 0.0f)
                                     ? (ambient_size / ambient_native)
                                     : ImGui::MarkdownDetail::GetActiveContentZoom();

        ImFont* base_font = (config && config->regularFont) ? config->regularFont : ambient_font;
        if (heading_level == 1 && config) {
            if (config->h1Font) base_font = config->h1Font;
            else if (config->headingFormats[0].font) base_font = config->headingFormats[0].font;
        } else if (heading_level == 2 && config) {
            if (config->h2Font) base_font = config->h2Font;
            else if (config->headingFormats[1].font) base_font = config->headingFormats[1].font;
        } else if (heading_level >= 3 && config) {
            if (config->h3Font) base_font = config->h3Font;
            else if (heading_level == 3 && config->headingFormats[2].font) base_font = config->headingFormats[2].font;
            else if (heading_level >= 4 && config->headingFormats[3].font) base_font = config->headingFormats[3].font;
        }

        ImFont* heading_italic = nullptr;
        if (heading_level == 1 && config && config->h1ItalicFont) heading_italic = config->h1ItalicFont;
        else if (heading_level == 2 && config && config->h2ItalicFont) heading_italic = config->h2ItalicFont;
        else if (heading_level >= 3 && config && config->h3ItalicFont) heading_italic = config->h3ItalicFont;
        if (!heading_italic && config) heading_italic = config->headingItalicFont;

        float body_italic_size = 0.0f;
        if (config && config->italicFont) {
            const float italic_native = MarkdownDetail::GetNativeFontPixelSize(config->italicFont);
            if (italic_native > 0.0f) body_italic_size = italic_native * zoom_ratio;
        }

        float font_size = ambient_size;
        if (base_font && base_font != ambient_font) {
            const float base_native = MarkdownDetail::GetNativeFontPixelSize(base_font);
            if (base_native > 0.0f) font_size = base_native * zoom_ratio;
        }
        if (font_size <= 0.0f) font_size = 14.0f * zoom_ratio;
        const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
        const float line_avail_w = MarkdownDetail::ResolveFullLineAvailWidth(config);

        struct ScopedMeasureConfig {
            const MarkdownConfig* prev;
            explicit ScopedMeasureConfig(const MarkdownConfig* cur) : prev(GetActiveMarkdownConfig()) {
                if (cur) GetActiveMarkdownConfig() = cur;
            }
            ~ScopedMeasureConfig() {
                GetActiveMarkdownConfig() = prev;
            }
        } measure_cfg_scope(config);

        float total_w = 0.0f;
        std::string seg;

        const float clamp_ovr = MarkdownDetail::GetActiveCaptionFontSizeOverride();
        auto flush_seg = [&](ImFont* seg_font) {
            if (!seg.empty()) {
                ImFont* f = seg_font ? seg_font : base_font;
                if (!f) f = ImGui::GetFont();
                float seg_sz = font_size;
                if (f != base_font) {
                    const float f_native = (clamp_ovr > 0.0f) ? clamp_ovr : MarkdownDetail::GetNativeFontPixelSize(f);
                    if (f_native > 0.0f) seg_sz = f_native * zoom_ratio;
                }
                ImVec2 sz = f->CalcTextSizeA(seg_sz, FLT_MAX, 0.0f, seg.c_str(), seg.c_str() + seg.size());
                total_w += sz.x;
                seg.clear();
            }
        };

        ImFont* cur_font = base_font;
        bool in_code_span = false;

        while (i < len) {
            char c = text[i];

            if (c == '`') {
                flush_seg(cur_font);
                in_code_span = !in_code_span;
                cur_font = (in_code_span && config && config->monoFont) ? config->monoFont : base_font;
                ++i;
                continue;
            }

            if (in_code_span) {
                seg.push_back(c);
                ++i;
                continue;
            }

            // Markdown bold/italic/strikethrough/underline delimiters
            if (c == '*' || c == '_' || c == '~' || c == '+') {
                if (i + 1 < len && text[i + 1] == c) {
                    flush_seg(cur_font);
                    if (heading_level > 0) {
                        cur_font = base_font;
                    } else {
                        cur_font = (cur_font == base_font && config && config->boldFont) ? config->boldFont : base_font;
                    }
                    i += 2;
                    continue;
                } else if (c == '*' || c == '_') {
                    flush_seg(cur_font);
                    if (heading_level > 0) {
                        if (heading_italic) cur_font = heading_italic;
                        else if (body_italic_size > 0.0f && body_italic_size >= (font_size - 0.5f) && config && config->italicFont) cur_font = config->italicFont;
                        else cur_font = base_font;
                    } else {
                        cur_font = (cur_font == base_font && config && config->italicFont) ? config->italicFont : base_font;
                    }
                    ++i;
                    continue;
                }
            }

            // Inline images ![alt](url){attrs} and links [label](url):
            const bool is_img_start = (c == '!' && i + 1 < len &&
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                                       text[i + 1] == '{'
#else
                                       (text[i + 1] == '[' || text[i + 1] == '{')
#endif
                                       );
            const bool is_link_start = (!is_img_start && !heading_level &&
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                                        c == '{'
#else
                                        (c == '[' || c == '{')
#endif
                                        );
            if (is_img_start || is_link_start) {
                const size_t open_brk = is_img_start ? (i + 1) : i;
                const char brk_close = (text[open_brk] == '{') ? '}' : ']';
                size_t close_brk = std::string::npos;
                for (size_t k = open_brk + 1; k < len; ++k) {
                    if (text[k] == '\n' || text[k] == '\r') break;
                    if (text[k] == brk_close) { close_brk = k; break; }
                }
                if (close_brk != std::string::npos && close_brk + 1 < len && text[close_brk + 1] == '(') {
                    int paren_depth = 1;
                    size_t close_paren = std::string::npos;
                    for (size_t k = close_brk + 2; k < len; ++k) {
                        if (text[k] == '\n' || text[k] == '\r') break;
                        if (text[k] == '(') ++paren_depth;
                        else if (text[k] == ')') {
                            --paren_depth;
                            if (paren_depth == 0) { close_paren = k; break; }
                        }
                    }
                    if (close_paren != std::string::npos) {
                        flush_seg(cur_font);
                        const char* label_ptr = text + open_brk + 1;
                        const size_t label_len = close_brk - (open_brk + 1);
                        const char* url_ptr = text + close_brk + 2;
                        const size_t url_len = close_paren - (close_brk + 2);
                        size_t next_idx = close_paren + 1;

                        if (is_img_start) {
                            size_t p_attr = next_idx;
                            while (p_attr < len && (text[p_attr] == ' ' || text[p_attr] == '\t')) ++p_attr;
                            std::string combined_url(url_ptr, url_len);
                            if (p_attr < len && text[p_attr] == '{') {
                                char aq = '\0';
                                size_t close_brace = std::string::npos;
                                for (size_t k = p_attr + 1; k < len; ++k) {
                                    if (text[k] == '\n' || text[k] == '\r') break;
                                    if (aq != '\0') {
                                        if (text[k] == aq && text[k - 1] != '\\') aq = '\0';
                                        continue;
                                    }
                                    if (text[k] == '"' || text[k] == '\'') { aq = text[k]; continue; }
                                    if (text[k] == '}') { close_brace = k; break; }
                                }
                                if (close_brace != std::string::npos) {
                                    combined_url += ' ';
                                    combined_url.append(text + p_attr + 1, close_brace - (p_attr + 1));
                                    next_idx = close_brace + 1;
                                }
                            }

                            MarkdownImageCallback* imageCb = (config && config->imageCallback) ? config->imageCallback : GetDefaultMarkdownImageCallback();
                            MarkdownImageData imgData;
                            if (imageCb) {
                                imgData = imageCb({ label_ptr, (int)label_len, combined_url.c_str(), (int)combined_url.size(), config ? config->userData : nullptr, true });
                            }
                            if (imgData.isValid) {
                                float img_w = imgData.size.x;
                                if (line_avail_w > 1.0f && img_w > line_avail_w) img_w = line_avail_w;
                                total_w += img_w;
                            } else {
                                std::string disp = MarkdownDetail::CleanDisplayImageUrl(url_ptr, (int)url_len);
                                std::string ph = imgData.isLoading
                                                     ? ("( Loading " + disp + "... )")
                                                     : ("( Image " + disp + " not loaded )");
                                ImFont* pf = base_font ? base_font : ImGui::GetFont();
                                total_w += pf->CalcTextSizeA(font_size, FLT_MAX, 0.0f, ph.c_str(), ph.c_str() + ph.size()).x;
                            }
                        } else {
                            total_w += MeasureMarkdownLineWidth(label_ptr, label_len, config);
                        }

                        i = next_idx;
                        continue;
                    }
                }
            }

            // Inline tags: <pill...>, <badge...>, <kbd...>, <backdrop...>, etc.
            if (c == '<') {
                size_t tag_close = std::string::npos;
                for (size_t k = i + 1; k < len; ++k) {
                    if (text[k] == '>') { tag_close = k; break; }
                    if (text[k] == '\n' || text[k] == '<') break;
                }
                if (tag_close != std::string::npos) {
                    size_t name_start = i + 1;
                    size_t name_end = tag_close;
                    for (size_t k = name_start; k < tag_close; ++k) {
                        if (text[k] == '=' || text[k] == ' ' || text[k] == '\t' || text[k] == ':') {
                            name_end = k;
                            break;
                        }
                    }
                    std::string tag_name = std::string(text + name_start, name_end - name_start);
                    for (char& ch : tag_name) ch = (char)tolower((unsigned char)ch);

                    std::string close_tag = "</" + tag_name + ">";
                    size_t close_pos = std::string::npos;
                    for (size_t k = tag_close + 1; k + close_tag.size() <= len; ++k) {
                        if (text[k] == '<') {
                            std::string candidate(text + k, close_tag.size());
                            for (char& ch : candidate) ch = (char)tolower((unsigned char)ch);
                            if (candidate == close_tag) {
                                close_pos = k;
                                break;
                            }
                        }
                    }

                    if (close_pos != std::string::npos) {
                        flush_seg(cur_font);
                        if (tag_name == "icon") {
                            total_w += font_size * 0.90f + 2.0f * scale;
                            i = close_pos + close_tag.size();
                            continue;
                        }
                        std::string inner(text + tag_close + 1, close_pos - (tag_close + 1));
                        std::string params = (name_end < tag_close) ? std::string(text + name_end, tag_close - name_end) : "";
                        bool is_literal_code_tag = (tag_name == "mono" || tag_name == "code");
                        MarkdownTagStyle style = ParseMarkdownTagStyle(inner, params, false, !is_literal_code_tag);
                        if (is_literal_code_tag) style.is_mono = true;
                        ImFont* tf = cur_font ? cur_font : base_font;
                        if (style.is_mono) {
                            ImFont* mono = (config && config->monoFont) ? config->monoFont : nullptr;
#if defined(CONTRIB_HAS_TYPOGRAPHY) || defined(IMGUI_FONT_ROLES_H)
                            if (!mono) {
                                if (style.is_bold) mono = ImGui::GetFont(ImFontRole::MonospaceMedium);
                                if (!mono) mono = ImGui::GetFont(ImFontRole::Monospace);
                            }
#endif
                            if (mono) tf = mono;
                        }
#if defined(CONTRIB_HAS_TYPOGRAPHY) || defined(IMGUI_FONT_ROLES_H)
                        if (tag_name == "small") tf = ImGui::GetFont(ImFontRole::Small);
                        else if (tag_name == "tiny") tf = ImGui::GetFont(ImFontRole::Tiny);
                        else if (tag_name == "big" || tag_name == "large") tf = ImGui::GetFont(ImFontRole::Title);
                        else if (tag_name == "huge") tf = ImGui::GetFont(ImFontRole::BoldLarge);
                        else if (tag_name == "regular" || tag_name == "prop" || tag_name == "sans") tf = ImGui::GetFont(ImFontRole::Regular);
#endif
                        if (!tf) tf = ImGui::GetFont();
                        float tf_size = font_size;
                        if (tf != base_font) {
                            const float tf_native = (clamp_ovr > 0.0f) ? clamp_ovr : MarkdownDetail::GetNativeFontPixelSize(tf);
                            if (tf_native > 0.0f) tf_size = tf_native * zoom_ratio;
                        }
                        ImVec2 inner_sz = tf->CalcTextSizeA(tf_size, FLT_MAX, 0.0f, style.clean_text.c_str(), style.clean_text.c_str() + style.clean_text.size());
                        float icon_w = 0.0f;
                        float gap_x = 0.0f;
                        if (!style.icon_name.empty()) {
                            ImVec2 isz(0.0f, 0.0f);
                            if (Detail::GetNamedTextureResolver()) {
                                Detail::GetNamedTextureResolver()(style.icon_name.c_str(), &isz);
                            }
                            if (isz.y > 0.0f) {
                                float aspect = (isz.x > 0.0f) ? (isz.x / isz.y) : 1.0f;
                                icon_w = font_size * aspect;
                            } else {
                                icon_w = font_size;
                            }
                            if (!style.clean_text.empty()) gap_x = ImMax(4.0f * scale, 3.0f);
                        }
                        float pad_x = 0.0f;
                        if (tag_name == "pill") {
                            pad_x = !style.clean_text.empty() ? ImMax(7.0f * scale, 6.0f) : ImMax(5.0f * scale, 4.0f);
                        } else if (tag_name == "badge" || tag_name == "tag") {
                            pad_x = !style.clean_text.empty() ? ImMax(5.0f * scale, 4.0f) : ImMax(4.0f * scale, 3.0f);
                        } else if (tag_name == "kbd" || tag_name == "key" || tag_name == "kdb") {
                            pad_x = ImMax(5.5f * scale, 5.0f);
                        } else if (tag_name == "btn") {
                            pad_x = ImMax(ImGui::GetStyle().FramePadding.x, 8.0f * scale);
                        } else if (tag_name == "mono" || tag_name == "code" || tag_name == "backdrop" || tag_name == "bg" || tag_name == "highlight") {
                            pad_x = 3.0f * scale;
                        }
                        total_w += icon_w + gap_x + inner_sz.x + pad_x * 2.0f;
                        i = close_pos + close_tag.size();
                        continue;
                    } else {
                        // Check if self-closing: e.g. <pill icon=foo /> or <img ... /> or <icon ... />
                        bool is_self_closing = (tag_close > name_start && text[tag_close - 1] == '/');
                        if (is_self_closing || tag_name == "img" || tag_name == "icon") {
                            flush_seg(cur_font);
                            if (tag_name == "icon") {
                                total_w += font_size * 0.90f + 2.0f * scale;
                                i = tag_close + 1;
                                continue;
                            }
                            size_t p_end = is_self_closing ? (tag_close - 1) : tag_close;
                            std::string params = (name_end < p_end) ? std::string(text + name_end, p_end - name_end) : "";
                            if (tag_name == "img") {
                                MarkdownImageCallback* imageCb = (config && config->imageCallback) ? config->imageCallback : GetDefaultMarkdownImageCallback();
                                MarkdownImageData imgData;
                                if (imageCb) {
                                    imgData = imageCb({ "", 0, params.c_str(), (int)params.size(), config ? config->userData : nullptr, true });
                                }
                                if (imgData.isValid) {
                                    float img_w = imgData.size.x;
                                    if (line_avail_w > 1.0f && img_w > line_avail_w) img_w = line_avail_w;
                                    total_w += img_w;
                                } else {
                                    std::string disp = MarkdownDetail::CleanDisplayImageUrl(params.c_str(), (int)params.size());
                                    std::string ph = imgData.isLoading
                                                         ? ("( Loading " + disp + "... )")
                                                         : ("( Image " + disp + " not loaded )");
                                    ImFont* pf = base_font ? base_font : ImGui::GetFont();
                                    total_w += pf->CalcTextSizeA(font_size, FLT_MAX, 0.0f, ph.c_str(), ph.c_str() + ph.size()).x;
                                }
                                i = tag_close + 1;
                                continue;
                            }
                            MarkdownTagStyle style = ParseMarkdownTagStyle("", params);
                            float item_w = 0.0f;
                            if (!style.icon_name.empty()) {
                                ImVec2 isz(0.0f, 0.0f);
                                if (Detail::GetNamedTextureResolver()) {
                                    Detail::GetNamedTextureResolver()(style.icon_name.c_str(), &isz);
                                }
                                if (isz.y > 0.0f) {
                                    float aspect = (isz.x > 0.0f) ? (isz.x / isz.y) : 1.0f;
                                    item_w = font_size * aspect;
                                } else {
                                    item_w = font_size;
                                }
                            }
                            float pad_x = 0.0f;
                            if (tag_name == "pill") {
                                pad_x = ImMax(5.0f * scale, 4.0f);
                            } else if (tag_name == "badge" || tag_name == "tag") {
                                pad_x = ImMax(4.0f * scale, 3.0f);
                            }
                            total_w += item_w + pad_x * 2.0f;
                            i = tag_close + 1;
                            continue;
                        }
                    }
                }
            }

            seg.push_back(c);
            ++i;
        }
        flush_seg(cur_font);
        return total_w;
    }

    namespace Detail {
        enum class CalloutKind {
            Note = 0,
            Tip,
            Important,
            Warning,
            Caution
        };

        // Reuse canonical nesting-safe underlay helper from ImGui::MarkdownDetail:
        using ImDrawListUnderlayScope = ImGui::MarkdownDetail::ImDrawListUnderlayScope;

        // Standalone procedural glyph fallback when widgets/imgui_icons.h is disabled or absent:
        inline void DrawMinimalCalloutGlyph(ImDrawList* dl, const ImVec2& pos, float size, ImU32 col, CalloutKind kind) {
            if (!dl || size <= 0.0f) return;
            ImVec2 center(pos.x + size * 0.5f, pos.y + size * 0.5f);
            float r = size * 0.38f;
            float thickness = ImMax(1.2f, size * 0.095f);

            if (kind == CalloutKind::Warning) {
                float h = size * 0.40f;
                ImVec2 a(center.x, center.y - h);
                ImVec2 b(center.x - h * 1.05f, center.y + h * 0.85f);
                ImVec2 c(center.x + h * 1.05f, center.y + h * 0.85f);
                dl->AddTriangle(a, b, c, col, thickness);
                ImGui::MarkdownDetail::StrokeLineCompat(dl, ImVec2(center.x, center.y - h * 0.28f), ImVec2(center.x, center.y + h * 0.24f), col, thickness);
                dl->AddCircleFilled(ImVec2(center.x, center.y + h * 0.54f), ImMax(1.0f, size * 0.065f), col, 8);
                return;
            }

            dl->AddCircle(center, r, col, 16, thickness);
            if (kind == CalloutKind::Note) {
                dl->AddCircleFilled(ImVec2(center.x, center.y - r * 0.42f), thickness * 0.95f, col, 8);
                ImGui::MarkdownDetail::StrokeLineCompat(dl, ImVec2(center.x, center.y - r * 0.10f), ImVec2(center.x, center.y + r * 0.50f), col, thickness * 1.25f);
            } else if (kind == CalloutKind::Tip) {
                float s = size * 0.46f;
                const ImVec2 pts[3] = {
                    ImVec2(center.x - s * 0.38f, center.y + s * 0.04f),
                    ImVec2(center.x - s * 0.06f, center.y + s * 0.36f),
                    ImVec2(center.x + s * 0.42f, center.y - s * 0.32f)
                };
                ImGui::MarkdownDetail::StrokePolylineCompat(dl, pts, 3, col, thickness, 0);
            } else if (kind == CalloutKind::Caution) {
                float x_r = r * 0.46f;
                ImGui::MarkdownDetail::StrokeLineCompat(dl, ImVec2(center.x - x_r, center.y - x_r), ImVec2(center.x + x_r, center.y + x_r), col, thickness);
                ImGui::MarkdownDetail::StrokeLineCompat(dl, ImVec2(center.x + x_r, center.y - x_r), ImVec2(center.x - x_r, center.y + x_r), col, thickness);
            } else {
                // Important: circle with exclamation mark
                ImGui::MarkdownDetail::StrokeLineCompat(dl, ImVec2(center.x, center.y - r * 0.48f), ImVec2(center.x, center.y + r * 0.12f), col, thickness * 1.25f);
                dl->AddCircleFilled(ImVec2(center.x, center.y + r * 0.46f), ImMax(1.0f, size * 0.065f), col, 8);
            }
        }

        inline bool TryMatchCalloutTypeToken(const std::string& raw_tok, CalloutKind& out_kind, std::string& out_canonical_name) {
            std::string t = raw_tok;
            for (char& c : t) c = (char)tolower((unsigned char)c);
            if (t == "note" || t == "info") {
                out_kind = CalloutKind::Note;
                out_canonical_name = "note";
                return true;
            }
            if (t == "tip" || t == "hint") {
                out_kind = CalloutKind::Tip;
                out_canonical_name = "tip";
                return true;
            }
            if (t == "important") {
                out_kind = CalloutKind::Important;
                out_canonical_name = "important";
                return true;
            }
            if (t == "warning" || t == "warn") {
                out_kind = CalloutKind::Warning;
                out_canonical_name = "warning";
                return true;
            }
            if (t == "caution" || t == "danger" || t == "error") {
                out_kind = CalloutKind::Caution;
                out_canonical_name = "caution";
                return true;
            }
            return false;
        }

        inline bool TryParseGfmAlertMarker(const std::string& stripped_line, std::string& out_canonical_type, std::string& out_inline_body) {
            size_t pos = 0;
            int lead_cols = 0;
            while (pos < stripped_line.size() && (stripped_line[pos] == ' ' || stripped_line[pos] == '\t')) {
                lead_cols += (stripped_line[pos] == '\t') ? 4 : 1;
                if (lead_cols > 3) return false;
                ++pos;
            }
            if (pos + 4 > stripped_line.size() || stripped_line[pos] != '[' || stripped_line[pos + 1] != '!') {
                return false;
            }
            size_t close_b = stripped_line.find(']', pos + 2);
            if (close_b == std::string::npos || close_b == pos + 2) {
                return false;
            }
            std::string raw_type = stripped_line.substr(pos + 2, close_b - (pos + 2));
            while (!raw_type.empty() && (raw_type.front() == ' ' || raw_type.front() == '\t')) raw_type.erase(0, 1);
            while (!raw_type.empty() && (raw_type.back() == ' ' || raw_type.back() == '\t')) raw_type.pop_back();
            for (char& c : raw_type) c = (char)tolower((unsigned char)c);

            // Strictly match the five GFM callout types (+ [!INFO] alias) so arbitrary '[!...]' text stays quoted prose
            if (raw_type != "note" && raw_type != "info" && raw_type != "tip" &&
                raw_type != "important" && raw_type != "warning" && raw_type != "caution") {
                return false;
            }
            out_canonical_type = (raw_type == "info") ? "note" : raw_type;

            size_t rem = close_b + 1;
            while (rem < stripped_line.size() && (stripped_line[rem] == ' ' || stripped_line[rem] == '\t')) {
                ++rem;
            }
            out_inline_body = (rem < stripped_line.size()) ? stripped_line.substr(rem) : std::string();
            while (!out_inline_body.empty() && (out_inline_body.back() == ' ' || out_inline_body.back() == '\t')) {
                out_inline_body.pop_back();
            }
            return true;
        }

        inline void ParseCalloutTagParams(const std::string& param, CalloutKind& out_kind, bool& out_has_custom_title, std::string& out_custom_title) {
            out_kind = CalloutKind::Note;
            out_has_custom_title = false;
            out_custom_title.clear();

            size_t i = 0;
            size_t n = param.size();
            while (i < n) {
                while (i < n && (param[i] == ' ' || param[i] == '\t' || param[i] == ',' || param[i] == ';')) ++i;
                if (i >= n) break;

                size_t k_start = i;
                while (i < n && param[i] != ' ' && param[i] != '\t' && param[i] != '=' && param[i] != ',' && param[i] != ';') ++i;
                std::string key = param.substr(k_start, i - k_start);
                std::string key_lower = key;
                for (char& c : key_lower) c = (char)tolower((unsigned char)c);

                if (i < n && param[i] == '=') {
                    ++i; // skip '='
                    while (i < n && (param[i] == ' ' || param[i] == '\t')) ++i;
                    std::string val;
                    if (i < n && (param[i] == '"' || param[i] == '\'')) {
                        char q = param[i++];
                        size_t v_start = i;
                        while (i < n && param[i] != q) ++i;
                        val = param.substr(v_start, i - v_start);
                        if (i < n && param[i] == q) ++i;
                    } else {
                        size_t v_start = i;
                        while (i < n && param[i] != ' ' && param[i] != '\t' && param[i] != ',' && param[i] != ';') ++i;
                        val = param.substr(v_start, i - v_start);
                    }

                    if (key_lower == "type" || key_lower == "kind" || key_lower == "variant") {
                        std::string dummy;
                        TryMatchCalloutTypeToken(val, out_kind, dummy);
                    } else if (key_lower == "title" || key_lower == "header" || key_lower == "label") {
                        out_has_custom_title = true;
                        out_custom_title = val;
                    }
                } else {
                    // Bare token: e.g. <callout warning> or <callout="tip"> (where outer '=' was stripped by Markdown::Render)
                    std::string stripped_key = key;
                    if (stripped_key.size() >= 2 &&
                        ((stripped_key.front() == '"' && stripped_key.back() == '"') ||
                         (stripped_key.front() == '\'' && stripped_key.back() == '\''))) {
                        stripped_key = stripped_key.substr(1, stripped_key.size() - 2);
                    }
                    std::string dummy;
                    TryMatchCalloutTypeToken(stripped_key, out_kind, dummy);
                }
            }
        }
    } // namespace Detail

    // Fence-aware and indented-code-aware preprocessor converting standard Markdown blockquotes
    // ('> ...', bare '>', tight '>text', and nested '> > ...') and GitHub-Flavored Markdown
    // callout blocks ('> [!NOTE]', '[!INFO]', '[!TIP]', '[!IMPORTANT]', '[!WARNING]', '[!CAUTION]',
    // with either multi-line or same-line inline bodies) into <blockquote> / <callout> blocks.
    // Peels exactly one '>' level per MarkdownExt pass so nested quotes & callouts compose cleanly.
    inline bool PreprocessMarkdownBlockquotesAndAlerts(const char* text, size_t len, std::string& out, const MarkdownConfig* config = nullptr) {
        if (!text || len == 0 || memchr(text, '>', len) == nullptr) {
            return false;
        }

        const bool gt_is_emphasis = (config && config->emphasisChars && std::strchr(config->emphasisChars, '>') != nullptr);

        out.reserve(len + 128);
        bool any_replaced = false;
        bool in_fence = false;
        char fence_char = '\0';
        int fence_len = 0;

        bool in_raw_container = false;
        std::string raw_container_tag;
        int raw_container_depth = 0;

        bool prev_blank_or_doc_start = true;
        bool in_indented_code_block = false;

        bool in_quote = false;
        bool quote_is_callout = false;
        std::string callout_type = "note";
        std::vector<std::string> quote_lines;
        bool quote_in_fence = false;
        char quote_fence_char = '\0';
        int quote_fence_len = 0;

        auto flushQuoteOrCallout = [&]() {
            if (!in_quote) return;
            if (quote_is_callout) {
                out.append("<callout type=");
                out.append(callout_type);
                out.append(">\n");
                for (const std::string& ql : quote_lines) {
                    out.append(ql);
                    out.push_back('\n');
                }
                out.append("</callout>\n");
            } else {
                out.append("<blockquote>\n");
                for (const std::string& ql : quote_lines) {
                    out.append(ql);
                    out.push_back('\n');
                }
                out.append("</blockquote>\n");
            }
            in_quote = false;
            quote_is_callout = false;
            callout_type = "note";
            quote_lines.clear();
            quote_in_fence = false;
            quote_fence_char = '\0';
            quote_fence_len = 0;
        };

        auto matchRawContainerName = [](const char* p, size_t rem_len, std::string& out_name) -> bool {
            static const char* const kNames[] = { "blockquote", "callout", "alert", "notice" };
            for (int idx = 0; idx < 4; ++idx) {
                const char* nm = kNames[idx];
                size_t nl = std::strlen(nm);
                if (rem_len > nl && TableDetail::StrCaseEqualN(p, nm, nl)) {
                    char after = p[nl];
                    if (after == '>' || after == ' ' || after == '\t' || after == '=' || after == ':' || after == '/') {
                        out_name = nm;
                        return true;
                    }
                }
            }
            return false;
        };

        auto scanRawContainerLine = [&matchRawContainerName](const char* lptr, size_t start_k, size_t line_len,
                                                             const std::string& tag_name, int& io_depth) {
            const std::string close_pat = "</" + tag_name + ">";
            int inline_backticks = 0;
            size_t k = start_k;
            while (k < line_len) {
                if (lptr[k] == '`') {
                    int bc = 0;
                    while (k + (size_t)bc < line_len && lptr[k + (size_t)bc] == '`') ++bc;
                    if (inline_backticks == 0) {
                        // CommonMark: only enter an inline code span if a matching run of 'bc' backticks closes on this line
                        size_t probe = k + (size_t)bc;
                        bool has_closing_run = false;
                        while (probe < line_len) {
                            if (lptr[probe] == '`') {
                                int pbc = 0;
                                while (probe + (size_t)pbc < line_len && lptr[probe + (size_t)pbc] == '`') ++pbc;
                                if (pbc == bc) {
                                    has_closing_run = true;
                                    break;
                                }
                                probe += (size_t)pbc;
                            } else {
                                ++probe;
                            }
                        }
                        if (has_closing_run) {
                            inline_backticks = bc;
                        }
                    } else if (bc == inline_backticks) {
                        inline_backticks = 0;
                    }
                    k += (size_t)bc;
                    continue;
                }

                if (inline_backticks > 0) {
                    ++k;
                    continue;
                }

                if (lptr[k] == '<') {
                    if (k + close_pat.size() <= line_len &&
                        TableDetail::StrCaseEqualN(lptr + k, close_pat.c_str(), close_pat.size())) {
                        --io_depth;
                        if (io_depth <= 0) {
                            io_depth = 0;
                            return;
                        }
                        k += close_pat.size();
                        continue;
                    }

                    std::string nested_nm;
                    if (matchRawContainerName(lptr + k + 1, line_len - (k + 1), nested_nm) && nested_nm == tag_name) {
                        size_t nk = k + 1 + nested_nm.size();
                        size_t nested_gt = std::string::npos;
                        while (nk < line_len) {
                            if (lptr[nk] == '>') { nested_gt = nk; break; }
                            if (lptr[nk] == '<') break;
                            ++nk;
                        }
                        if (nested_gt != std::string::npos) {
                            size_t ns = nested_gt;
                            while (ns > k + 1 && (lptr[ns - 1] == ' ' || lptr[ns - 1] == '\t')) --ns;
                            if (lptr[ns - 1] != '/') {
                                ++io_depth;
                            }
                            k = nested_gt + 1;
                            continue;
                        }
                    }
                }
                ++k;
            }
        };

        size_t i = 0;
        while (i < len) {
            size_t line_start = i;
            while (i < len && text[i] != '\n' && text[i] != '\r') ++i;
            size_t line_end = i;
            if (i < len && text[i] == '\r') ++i;
            if (i < len && text[i] == '\n') ++i;
            size_t next_line = i;

            const char* lptr = text + line_start;
            size_t line_len = line_end - line_start;

            size_t lead_idx = 0;
            int lead_cols = 0;
            while (lead_idx < line_len && (lptr[lead_idx] == ' ' || lptr[lead_idx] == '\t')) {
                lead_cols += (lptr[lead_idx] == '\t') ? 4 : 1;
                ++lead_idx;
            }
            bool is_blank_line = (lead_idx >= line_len);

            // 1. Track fenced code blocks (``` or ~~~ with 0..3 leading spaces), even inside
            //    explicit raw <blockquote/callout/alert/notice> containers so literal closing
            //    tags or '>' lines inside code fences never close the container or turn into quotes:
            if (!in_indented_code_block && lead_cols <= 3 && lead_idx < line_len &&
                (lptr[lead_idx] == '`' || lptr[lead_idx] == '~')) {
                char fc = lptr[lead_idx];
                int fl = 0;
                while (lead_idx + (size_t)fl < line_len && lptr[lead_idx + (size_t)fl] == fc) ++fl;
                if (fl >= 3) {
                    if (!in_fence) {
                        if (!in_raw_container) {
                            flushQuoteOrCallout();
                        }
                        in_fence = true;
                        fence_char = fc;
                        fence_len = fl;
                    } else if (fc == fence_char && fl >= fence_len) {
                        in_fence = false;
                        fence_char = '\0';
                        fence_len = 0;
                    }
                    out.append(lptr, next_line - line_start);
                    prev_blank_or_doc_start = false;
                    continue;
                }
            }

            if (in_fence) {
                out.append(lptr, next_line - line_start);
                continue;
            }

            // 2. Track explicit raw <blockquote/callout/alert/notice> blocks (skipping inline
            //    backtick spans) so their inner '>' lines are handled on the tag's own recursive
            //    MarkdownExt pass rather than prematurely split:
            if (in_raw_container) {
                scanRawContainerLine(lptr, 0, line_len, raw_container_tag, raw_container_depth);
                if (raw_container_depth <= 0) {
                    in_raw_container = false;
                    raw_container_depth = 0;
                }
                out.append(lptr, next_line - line_start);
                prev_blank_or_doc_start = false;
                continue;
            }

            // 3. Track CommonMark 4-space indented code blocks outside blockquotes:
            if (!in_quote) {
                if (in_indented_code_block) {
                    if (is_blank_line || lead_cols >= 4) {
                        out.append(lptr, next_line - line_start);
                        continue;
                    }
                    in_indented_code_block = false;
                } else if (prev_blank_or_doc_start && !is_blank_line && lead_cols >= 4) {
                    in_indented_code_block = true;
                    out.append(lptr, next_line - line_start);
                    prev_blank_or_doc_start = false;
                    continue;
                }
            }

            // 4. Detect start of raw explicit <blockquote/callout/alert/notice> container on this line:
            if (lead_cols <= 3 && lead_idx < line_len && lptr[lead_idx] == '<') {
                std::string matched_tag;
                if (matchRawContainerName(lptr + lead_idx + 1, line_len - (lead_idx + 1), matched_tag)) {
                    flushQuoteOrCallout();
                    size_t nk = lead_idx + 1 + matched_tag.size();
                    size_t open_gt = std::string::npos;
                    while (nk < line_len) {
                        if (lptr[nk] == '>') { open_gt = nk; break; }
                        if (lptr[nk] == '<') break;
                        ++nk;
                    }
                    if (open_gt != std::string::npos) {
                        size_t ns = open_gt;
                        while (ns > lead_idx + 1 && (lptr[ns - 1] == ' ' || lptr[ns - 1] == '\t')) --ns;
                        if (lptr[ns - 1] != '/') {
                            int depth = 1;
                            scanRawContainerLine(lptr, open_gt + 1, line_len, matched_tag, depth);
                            if (depth > 0) {
                                in_raw_container = true;
                                raw_container_tag = matched_tag;
                                raw_container_depth = depth;
                            }
                        }
                    }
                    out.append(lptr, next_line - line_start);
                    prev_blank_or_doc_start = false;
                    continue;
                }
            }

            // 5. Blockquote / GFM Alert line ('>' with 0..3 leading spaces):
            //    When host config registers '>' in emphasisChars (e.g. ">>Important:>>"),
            //    require '>' to be followed by EOL, ' ', or '\t' so tight ">>text" / ">text"
            //    remains inline emphasis while "> text", bare ">", and "> [!NOTE]" still work.
            if (lead_cols <= 3 && lead_idx < line_len && lptr[lead_idx] == '>' &&
                (!gt_is_emphasis || lead_idx + 1 >= line_len || lptr[lead_idx + 1] == ' ' || lptr[lead_idx + 1] == '\t')) {
                size_t content_p = lead_idx + 1;
                if (content_p < line_len && (lptr[content_p] == ' ' || lptr[content_p] == '\t')) {
                    ++content_p;
                }
                std::string stripped(lptr + content_p, line_len - content_p);

                bool is_quote_fence_line = false;
                {
                    size_t q_lead_cols = 0;
                    size_t q_lead_idx = 0;
                    while (q_lead_idx < stripped.size() && (stripped[q_lead_idx] == ' ' || stripped[q_lead_idx] == '\t')) {
                        q_lead_cols += (stripped[q_lead_idx] == '\t') ? 4u : 1u;
                        ++q_lead_idx;
                    }
                    if (q_lead_cols <= 3 && q_lead_idx < stripped.size() && (stripped[q_lead_idx] == '`' || stripped[q_lead_idx] == '~')) {
                        char qfc = stripped[q_lead_idx];
                        int qfl = 0;
                        while (q_lead_idx + (size_t)qfl < stripped.size() && stripped[q_lead_idx + (size_t)qfl] == qfc) ++qfl;
                        if (qfl >= 3) {
                            if (!quote_in_fence) {
                                bool valid_open = true;
                                if (qfc == '`') {
                                    for (size_t qk = q_lead_idx + (size_t)qfl; qk < stripped.size(); ++qk) {
                                        if (stripped[qk] == '`') { valid_open = false; break; }
                                    }
                                }
                                if (valid_open) {
                                    quote_in_fence = true;
                                    quote_fence_char = qfc;
                                    quote_fence_len = qfl;
                                    is_quote_fence_line = true;
                                }
                            } else if (qfc == quote_fence_char && qfl >= quote_fence_len) {
                                size_t q_trail = q_lead_idx + (size_t)qfl;
                                while (q_trail < stripped.size() && (stripped[q_trail] == ' ' || stripped[q_trail] == '\t')) ++q_trail;
                                if (q_trail == stripped.size()) {
                                    quote_in_fence = false;
                                    quote_fence_char = '\0';
                                    quote_fence_len = 0;
                                    is_quote_fence_line = true;
                                }
                            }
                        }
                    }
                }

                std::string detected_alert_type;
                std::string inline_alert_body;
                if (!quote_in_fence && !is_quote_fence_line &&
                    Detail::TryParseGfmAlertMarker(stripped, detected_alert_type, inline_alert_body)) {
                    flushQuoteOrCallout();
                    in_quote = true;
                    quote_is_callout = true;
                    callout_type = detected_alert_type;
                    if (!inline_alert_body.empty()) {
                        quote_lines.push_back(inline_alert_body);
                    }
                } else {
                    if (!in_quote) {
                        in_quote = true;
                        quote_is_callout = false;
                    }
                    quote_lines.push_back(stripped);
                }
                any_replaced = true;
                prev_blank_or_doc_start = false;
                continue;
            }

            // Non-quote line ends any open quote/callout block:
            if (in_quote) {
                flushQuoteOrCallout();
            }
            out.append(lptr, next_line - line_start);
            prev_blank_or_doc_start = is_blank_line;
        }

        if (in_quote) {
            flushQuoteOrCallout();
        }
        return any_replaced;
    }

    // Pre-processes Markdown text to convert block-level centering markers (-> text <- or =text=)
    // into canonical <center>text</center> tags outside of fenced code blocks.
    inline bool PreprocessMarkdownCenteredBlocks(const char* text, size_t len, std::string& out) {
        if (!text || len == 0) return false;
        bool has_candidates = false;
        for (size_t i = 0; i + 1 < len; ++i) {
            if ((text[i] == '-' && text[i + 1] == '>') || (text[i] == '=' && (i == 0 || text[i-1] == '\n' || text[i-1] == '\r'))) {
                has_candidates = true;
                break;
            }
        }
        if (!has_candidates) {
            return false;
        }

        out.reserve(len + 64);
        bool any_replaced = false;
        bool in_fence = false;
        char fence_char = '\0';
        size_t fence_len = 0;

        size_t i = 0;
        while (i < len) {
            size_t line_start = i;
            while (i < len && text[i] != '\n' && text[i] != '\r') {
                ++i;
            }
            size_t line_end = i;
            if (i < len && text[i] == '\r') ++i;
            if (i < len && text[i] == '\n') ++i;
            size_t next_line = i;

            size_t line_len = line_end - line_start;
            const char* lptr = text + line_start;

            size_t lead = 0;
            while (lead < line_len && (lptr[lead] == ' ' || lptr[lead] == '\t')) ++lead;

            if (lead < line_len && (lptr[lead] == '`' || lptr[lead] == '~')) {
                char fc = lptr[lead];
                size_t fl = 0;
                while (lead + fl < line_len && lptr[lead + fl] == fc) ++fl;
                if (fl >= 3) {
                    if (!in_fence) {
                        in_fence = true;
                        fence_char = fc;
                        fence_len = fl;
                    } else if (fc == fence_char && fl >= fence_len) {
                        in_fence = false;
                        fence_char = '\0';
                        fence_len = 0;
                    }
                }
            }

            if (!in_fence && line_len >= 5) {
                size_t start_idx = lead;
                // 1. Check for "-> " ... " <-"
                if (start_idx + 2 < line_len && lptr[start_idx] == '-' && lptr[start_idx + 1] == '>' &&
                    (lptr[start_idx + 2] == ' ' || lptr[start_idx + 2] == '\t')) {
                    size_t trail = line_len;
                    while (trail > start_idx + 3 && (lptr[trail - 1] == ' ' || lptr[trail - 1] == '\t')) {
                        --trail;
                    }
                    if (trail >= start_idx + 5 && lptr[trail - 2] == '<' && lptr[trail - 1] == '-' &&
                        (lptr[trail - 3] == ' ' || lptr[trail - 3] == '\t')) {
                        size_t content_start = start_idx + 3;
                        while (content_start < trail - 2 && (lptr[content_start] == ' ' || lptr[content_start] == '\t')) {
                            ++content_start;
                        }
                        size_t content_end = trail - 2;
                        while (content_end > content_start && (lptr[content_end - 1] == ' ' || lptr[content_end - 1] == '\t')) {
                            --content_end;
                        }
                        if (content_end > content_start) {
                            out.append(lptr, lead);
                            out.append("<center>");
                            out.append(lptr + content_start, content_end - content_start);
                            out.append("</center>\n");
                            any_replaced = true;
                            continue;
                        }
                    }
                }
                // 2. Check for "= " ... " =" or "=Text="
                else if (start_idx + 1 < line_len && lptr[start_idx] == '=') {
                    size_t trail = line_len;
                    while (trail > start_idx + 1 && (lptr[trail - 1] == ' ' || lptr[trail - 1] == '\t')) {
                        --trail;
                    }
                    if (trail >= start_idx + 3 && lptr[trail - 1] == '=') {
                        bool all_equals = true;
                        for (size_t k = start_idx; k < trail; ++k) {
                            if (lptr[k] != '=') { all_equals = false; break; }
                        }
                        if (!all_equals) {
                            size_t content_start = start_idx + 1;
                            while (content_start < trail - 1 && (lptr[content_start] == ' ' || lptr[content_start] == '\t')) {
                                ++content_start;
                            }
                            size_t content_end = trail - 1;
                            while (content_end > content_start && (lptr[content_end - 1] == ' ' || lptr[content_end - 1] == '\t')) {
                                --content_end;
                            }
                            if (content_end > content_start) {
                                out.append(lptr, lead);
                                out.append("<center>");
                                out.append(lptr + content_start, content_end - content_start);
                                out.append("</center>\n");
                                any_replaced = true;
                                continue;
                            }
                        }
                    }
                }
            }

            out.append(lptr, next_line - line_start);
        }

        return any_replaced;
    }

    // Forward declarations
    inline void MarkdownExt(const char* markdown_text, size_t markdown_len, const MarkdownConfig* config_override = nullptr, bool create_float_scope = true);
    inline void MarkdownExt(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr, bool create_float_scope = true);
    inline void RenderInlineTaggedText(const std::string& text, const MarkdownConfig* config);

    inline std::unordered_map<std::string, CustomTagCallback>& GetCustomMarkdownTags() {
        static std::unordered_map<std::string, CustomTagCallback> s_CustomTags;
        static bool s_InitializedDefaults = false;
        if (!s_InitializedDefaults) {
            s_InitializedDefaults = true;
#ifndef IMGUI_DISABLE_MARKDOWN_DEFAULT_TAGS
            // Built-in Tag 1: <color=...>, <col=...>, <style=...>
            auto colorHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, false);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                const float rendered_w = MeasureMarkdownLineWidth(style.clean_text.c_str(), style.clean_text.length(), config);
                const float font_size = ImGui::GetFontSize();
                const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;

                bool has_bg = false;
                ImVec4 bg_col(0.0f, 0.0f, 0.0f, 1.0f);
                float pad_x = 0.0f;
                if (!style.bg_color.empty() && ParseMarkdownColor(style.bg_color, bg_col)) {
                    has_bg = true;
                    pad_x = 3.0f * scale;
                }

                const float req_w = (std::max)(text_size.x, rendered_w) + (has_bg ? pad_x * 2.0f : 0.0f);
                EnsureTagFitsOnLine(req_w);

                if (has_bg) {
                    const ImVec2 text_pos = ImGui::GetCursorScreenPos();
                    const float pad_y = 1.0f * scale;
                    const float rounding = 3.0f * scale;
                    const ImU32 u_bg = ImGui::GetColorU32(bg_col);
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(
                        ImVec2(text_pos.x - pad_x, text_pos.y - pad_y),
                        ImVec2(text_pos.x + rendered_w + pad_x, text_pos.y + font_size + pad_y),
                        u_bg, rounding);
                }

                ImVec4 col(1.0f, 1.0f, 1.0f, 1.0f);
                bool has_col = false;
                if (!style.fg_color.empty()) {
                    has_col = ParseMarkdownColor(style.fg_color, col);
                } else if (!has_bg && !style.color_param.empty()) {
                    has_col = ParseMarkdownColor(style.color_param, col);
                } else if (has_bg) {
                    // When a background is explicitly drawn but no foreground is specified,
                    // calculate auto-contrast text so text is always clearly legible!
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) {
                        col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Dark text on bright background
                        has_col = true;
                    } else if (lum < 0.35f) {
                        col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White text on dark background
                        has_col = true;
                    }
                }
                if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);

                bool no_wrap = (config && config->wrapWidth < 0.0f);
                float avail_w = (config && config->wrapWidth > 0.0f) ? config->wrapWidth : ImGui::GetContentRegionAvail().x;
                if (no_wrap || text_size.x <= avail_w) {
                    RenderInlineTaggedText(style.clean_text, config);
                } else {
                    float wrap_pos = (config && config->wrapWidth > 0.0f) ? (ImGui::GetCursorPos().x + config->wrapWidth) : 0.0f;
                    ImGui::PushTextWrapPos(wrap_pos);
                    RenderInlineTaggedText(style.clean_text, config);
                    ImGui::PopTextWrapPos();
                }

                if (style.is_strike) {
                    DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), true);
                }
                if (style.is_underline) {
                    DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), false);
                }

                if (has_col) ImGui::PopStyleColor();

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["color"] = colorHandler;
            s_CustomTags["col"]   = colorHandler;
            s_CustomTags["style"] = colorHandler;

            // Built-in Tag 2: <backdrop=...>, <bg=...>, <highlight=...>
            // Draws an exact filled rectangular pill behind the text bounding box with rounded corners
            auto backdropHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, true);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 bg_col(1.0f, 0.90f, 0.20f, 0.65f); // default soft yellow highlight
                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;
                ParseMarkdownColor(bg_str, bg_col);

                float rendered_w = MeasureMarkdownLineWidth(style.clean_text.c_str(), style.clean_text.length(), config);
                float font_size = ImGui::GetFontSize();
                ImVec2 text_size(rendered_w, font_size);
                float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                float pad_x = 3.0f * scale;
                float pad_y = 1.0f * scale;
                float rounding = 3.0f * scale;

                EnsureTagFitsOnLine(text_size.x + pad_x * 2.0f);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImU32 col_u32 = ImGui::GetColorU32(bg_col);
                // Optically balanced rounded rectangle framing the text:
                draw_list->AddRectFilled(
                    ImVec2(pos.x - pad_x, pos.y - pad_y),
                    ImVec2(pos.x + text_size.x + pad_x, pos.y + text_size.y + pad_y),
                    col_u32, rounding
                );

                if (!style.fg_color.empty()) {
                    ImVec4 fg_col;
                    if (ParseMarkdownColor(style.fg_color, fg_col)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, fg_col);
                        RenderInlineTaggedText(style.clean_text, config);
                        ImGui::PopStyleColor();
                    } else {
                        RenderInlineTaggedText(style.clean_text, config);
                    }
                } else {
                    // Auto-contrast text color if background is dark
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum < 0.35f) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        RenderInlineTaggedText(style.clean_text, config);
                        ImGui::PopStyleColor();
                    } else {
                        RenderInlineTaggedText(style.clean_text, config);
                    }
                }

                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["backdrop"]  = backdropHandler;
            s_CustomTags["bg"]        = backdropHandler;
            s_CustomTags["highlight"] = backdropHandler;

            // Built-in Tag 3: <badge=...>, <tag=...>
            // Delegates directly to ImGui::Badge standalone C++ widget
            auto badgeHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, true);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f); // default charcoal
                ImVec4 text_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;

                if (bg_str == "danger" || bg_str == "red") {
                    bg_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                } else if (bg_str == "warning" || bg_str == "yellow") {
                    bg_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if (bg_str == "success" || bg_str == "green") {
                    bg_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                } else if (bg_str == "info" || bg_str == "blue") {
                    bg_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                } else if (!bg_str.empty()) {
                    ParseMarkdownColor(bg_str, bg_col);
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                bool has_explicit_fg = !style.fg_color.empty();
                if (has_explicit_fg) {
                    ParseMarkdownColor(style.fg_color, text_col);
                }

                ImTextureID icon_tex = (ImTextureID)0;
                ImVec2 icon_size(0.0f, 0.0f);
                std::string vec_icon_name;
                if (!style.icon_name.empty()) {
                    if (Detail::GetNamedTextureResolver()) {
                        icon_tex = Detail::GetNamedTextureResolver()(style.icon_name.c_str(), &icon_size);
                    }
                    if (!icon_tex) {
                        auto imageCb = GetActiveMarkdownConfig() && GetActiveMarkdownConfig()->imageCallback 
                                       ? GetActiveMarkdownConfig()->imageCallback 
                                       : GetDefaultMarkdownImageCallback();
                        if (imageCb) {
                            MarkdownImageData imgData = imageCb({ "", 0, style.icon_name.c_str(), (int)style.icon_name.length(), nullptr, true });
                            if (imgData.isValid) {
                                icon_tex = imgData.user_texture_id;
                                icon_size = imgData.size;
                            }
                        }
                    }
#if defined(CONTRIB_HAS_ICONS)
                    if (!icon_tex) {
                        std::string norm = NormalizeIconName(style.icon_name.c_str());
                        if (IsVectorIcon(norm.c_str())) {
                            vec_icon_name = norm;
                        }
                    }
#endif
                }

                const float font_size = ImGui::GetFontSize();
                const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                const bool has_icon = (icon_tex != (ImTextureID)0 || !vec_icon_name.empty());
                ImVec2 actual_icon_size = icon_size;
                if (has_icon) {
                    if (actual_icon_size.y <= 0.0f) {
                        float target_h = !vec_icon_name.empty() ? (font_size * 0.90f) : font_size;
                        float aspect = (actual_icon_size.x > 0.0f && actual_icon_size.y > 0.0f) ? (actual_icon_size.x / actual_icon_size.y) : 1.0f;
                        actual_icon_size = ImVec2(target_h * aspect, target_h);
                    } else if (actual_icon_size.x <= 0.0f) {
                        actual_icon_size = ImVec2(actual_icon_size.y, actual_icon_size.y);
                    }
                }

                ImVec2 text_size = !style.clean_text.empty() ? ImGui::CalcTextSize(style.clean_text.c_str()) : ImVec2(0.0f, 0.0f);
                float pad_x = !style.clean_text.empty() ? ImMax(5.0f * scale, 4.0f) : ImMax(4.0f * scale, 3.0f);
                float gap_x = (has_icon && !style.clean_text.empty()) ? ImMax(4.0f * scale, 3.0f) : 0.0f;
                float required_w = (has_icon ? actual_icon_size.x : 0.0f) + gap_x + text_size.x + pad_x * 2.0f;
                EnsureTagFitsOnLine(required_w);

#if defined(CONTRIB_HAS_BADGES)
                ImGui::Badge(style.clean_text.empty() ? nullptr : style.clean_text.c_str(),
                             bg_col,
                             has_explicit_fg ? &text_col : nullptr,
                             -1.0f,
                             icon_tex,
                             actual_icon_size,
                             vec_icon_name.empty() ? nullptr : vec_icon_name.c_str());
#else
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                ImGui::Text("[%s]", style.clean_text.c_str());
                ImGui::PopStyleColor();
#endif
                if (!style.clean_text.empty()) {
                    MarkdownDetail::HighlightSearchMatchesInCapsule(
                        style.clean_text.c_str(), style.clean_text.size(),
                        ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                }

                PopMarkdownTagFont(config, style);
            };

            // Built-in Tag 3b: <pill=...>
            // Delegates directly to ImGui::Pill standalone C++ widget
            auto pillHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, true);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 bg_col = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
                ImVec4 text_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;

                if (bg_str == "danger" || bg_str == "red") {
                    bg_col = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
                } else if (bg_str == "warning" || bg_str == "yellow") {
                    bg_col = ImVec4(0.95f, 0.75f, 0.10f, 1.0f);
                    text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if (bg_str == "success" || bg_str == "green") {
                    bg_col = ImVec4(0.15f, 0.70f, 0.25f, 1.0f);
                } else if (bg_str == "info" || bg_str == "blue") {
                    bg_col = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
                } else if (!bg_str.empty()) {
                    ParseMarkdownColor(bg_str, bg_col);
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum > 0.65f) text_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                }

                bool has_explicit_fg = !style.fg_color.empty();
                if (has_explicit_fg) {
                    ParseMarkdownColor(style.fg_color, text_col);
                }

                ImTextureID icon_tex = (ImTextureID)0;
                ImVec2 icon_size(0.0f, 0.0f);
                std::string vec_icon_name;
                if (!style.icon_name.empty()) {
                    if (Detail::GetNamedTextureResolver()) {
                        icon_tex = Detail::GetNamedTextureResolver()(style.icon_name.c_str(), &icon_size);
                    }
                    if (!icon_tex) {
                        auto imageCb = GetActiveMarkdownConfig() && GetActiveMarkdownConfig()->imageCallback 
                                       ? GetActiveMarkdownConfig()->imageCallback 
                                       : GetDefaultMarkdownImageCallback();
                        if (imageCb) {
                            MarkdownImageData imgData = imageCb({ "", 0, style.icon_name.c_str(), (int)style.icon_name.length(), nullptr, true });
                            if (imgData.isValid) {
                                icon_tex = imgData.user_texture_id;
                                icon_size = imgData.size;
                            }
                        }
                    }
#if defined(CONTRIB_HAS_ICONS)
                    if (!icon_tex) {
                        std::string norm = NormalizeIconName(style.icon_name.c_str());
                        if (IsVectorIcon(norm.c_str())) {
                            vec_icon_name = norm;
                        }
                    }
#endif
                }

                const float font_size = ImGui::GetFontSize();
                const float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                const bool has_icon = (icon_tex != (ImTextureID)0 || !vec_icon_name.empty());
                ImVec2 actual_icon_size = icon_size;
                if (has_icon) {
                    if (actual_icon_size.y <= 0.0f) {
                        float target_h = !vec_icon_name.empty() ? (font_size * 0.90f) : font_size;
                        float aspect = (actual_icon_size.x > 0.0f && actual_icon_size.y > 0.0f) ? (actual_icon_size.x / actual_icon_size.y) : 1.0f;
                        actual_icon_size = ImVec2(target_h * aspect, target_h);
                    } else if (actual_icon_size.x <= 0.0f) {
                        actual_icon_size = ImVec2(actual_icon_size.y, actual_icon_size.y);
                    }
                }

                ImVec2 text_size = !style.clean_text.empty() ? ImGui::CalcTextSize(style.clean_text.c_str()) : ImVec2(0.0f, 0.0f);
                float pad_x = !style.clean_text.empty() ? ImMax(7.0f * scale, 6.0f) : ImMax(5.0f * scale, 4.0f);
                float gap_x = (has_icon && !style.clean_text.empty()) ? ImMax(4.0f * scale, 3.0f) : 0.0f;
                float required_w = (has_icon ? actual_icon_size.x : 0.0f) + gap_x + text_size.x + pad_x * 2.0f;
                EnsureTagFitsOnLine(required_w);

#if defined(CONTRIB_HAS_BADGES)
                ImGui::Pill(style.clean_text.empty() ? nullptr : style.clean_text.c_str(),
                            bg_col,
                            has_explicit_fg ? &text_col : nullptr,
                            icon_tex,
                            actual_icon_size,
                            vec_icon_name.empty() ? nullptr : vec_icon_name.c_str());
#else
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                ImGui::Text("(%s)", style.clean_text.c_str());
                ImGui::PopStyleColor();
#endif
                if (!style.clean_text.empty()) {
                    MarkdownDetail::HighlightSearchMatchesInCapsule(
                        style.clean_text.c_str(), style.clean_text.size(),
                        ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                }

                PopMarkdownTagFont(config, style);
            };

            // Built-in Tag 3c: <kbd>, <key>
            // Delegates directly to ImGui::Kbd standalone C++ widget
            auto kbdHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                float rendered_w = ImGui::CalcTextSize(style.clean_text.c_str(), style.clean_text.c_str() + style.clean_text.size(), false).x;
                float font_size = ImGui::GetFontSize();
                ImVec2 text_size(rendered_w, font_size);
                float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                float pad_x = ImMax(5.5f * scale, 5.0f);
                float required_w = text_size.x + pad_x * 2.0f;
                EnsureTagFitsOnLine(required_w);

#if defined(CONTRIB_HAS_BADGES)
                ImGui::Kbd(style.clean_text.c_str());
#else
                ImGui::Text("[%s]", style.clean_text.c_str());
#endif
                if (!style.clean_text.empty()) {
                    MarkdownDetail::HighlightSearchMatchesInCapsule(
                        style.clean_text.c_str(), style.clean_text.size(),
                        ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                }

                PopMarkdownTagFont(config, style);
            };

            s_CustomTags["badge"] = badgeHandler;
            s_CustomTags["tag"]   = badgeHandler;
            s_CustomTags["pill"]  = pillHandler;
            s_CustomTags["kbd"]   = kbdHandler;
            s_CustomTags["key"]   = kbdHandler;
            s_CustomTags["kdb"]   = kbdHandler; // Common typo alias for <kbd>

            // Built-in Tag 4: <btn=...>
            s_CustomTags["btn"] = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, true);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                float rendered_w = ImGui::CalcTextSize(style.clean_text.c_str(), nullptr, true).x;
                float font_size = ImGui::GetFontSize();
                ImVec2 text_size(rendered_w, font_size);
                float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                float pad_x = ImMax(ImGui::GetStyle().FramePadding.x, 8.0f * scale);
                float pad_y = ImMax(ImGui::GetStyle().FramePadding.y, 2.0f * scale);
                EnsureTagFitsOnLine(text_size.x + pad_x * 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));
                ImGui::SmallButton(style.clean_text.c_str());
                ImGui::PopStyleVar();
                PopMarkdownTagFont(config, style);
            };

            // Built-in Tag 5: <mono>, <code>
            // Renders monospace code span with subtle rounded background pill
            auto monoCodeHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param, true, false);
                style.is_mono = true;
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();

                ImFont* mono_font = (config && config->monoFont) ? config->monoFont : nullptr;
                if (mono_font) {
                    ImGui::ImGeeCompat::PushFont(mono_font);
                } else {
                    PushMarkdownTagFont(config, style);
                }

                // Default: subtle translucent gray rounded pill behind code
                ImVec4 bg_col(0.5f, 0.5f, 0.5f, 0.24f);
                std::string bg_str = !style.bg_color.empty() ? style.bg_color : style.color_param;
                if (!bg_str.empty()) {
                    ParseMarkdownColor(bg_str, bg_col);
                }

                float rendered_w = ImGui::CalcTextSize(style.clean_text.c_str(), style.clean_text.c_str() + style.clean_text.size(), false).x;
                float font_size = ImGui::GetFontSize();
                ImVec2 text_size(rendered_w, font_size);
                float scale = (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
                float pad_x = 3.0f * scale;
                float pad_y = 1.0f * scale;
                float rounding = 3.0f * scale;

                ImVec4 fg_col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_explicit_fg = false;
                if (!style.fg_color.empty()) {
                    has_explicit_fg = ParseMarkdownColor(style.fg_color, fg_col);
                } else if (!bg_str.empty()) {
                    float lum = bg_col.x * 0.299f + bg_col.y * 0.587f + bg_col.z * 0.114f;
                    if (lum < 0.35f) {
                        fg_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        has_explicit_fg = true;
                    } else if (lum > 0.75f) {
                        fg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                        has_explicit_fg = true;
                    }
                }

#if defined(CONTRIB_HAS_ICONS)
                bool has_icon = (!style.icon_name.empty() && IsVectorIcon(style.icon_name.c_str()));
#else
                bool has_icon = false;
#endif
                float icon_sz = font_size * 0.90f;
                bool has_text = !style.clean_text.empty();
                float gap = (has_icon && has_text) ? (4.0f * scale) : 0.0f;
                float content_w = (has_icon ? icon_sz : 0.0f) + gap + text_size.x;

                EnsureTagFitsOnLine(content_w + pad_x * 2.0f);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImU32 col_u32 = ImGui::GetColorU32(bg_col);
                ImU32 fg_u32 = ImGui::GetColorU32(fg_col);
                (void)fg_u32;

                draw_list->AddRectFilled(
                    ImVec2(pos.x - pad_x, pos.y - pad_y),
                    ImVec2(pos.x + content_w + pad_x, pos.y + font_size + pad_y),
                    col_u32, rounding
                );

                float cur_x = pos.x;
#if defined(CONTRIB_HAS_ICONS)
                if (has_icon) {
                    ImVec2 icon_pos(cur_x, pos.y + (font_size - icon_sz) * 0.5f);
                    ImGui::DrawVectorIcon(draw_list, icon_pos, icon_sz, fg_u32, style.icon_name.c_str());
                    cur_x += icon_sz + gap;
                }
#endif

                if (has_text) {
                    ImGui::SetCursorScreenPos(ImVec2(cur_x, pos.y));
                    if (has_explicit_fg) {
                        ImGui::PushStyleColor(ImGuiCol_Text, fg_col);
                        MarkdownDetail::RenderTextWithSearchHighlight(style.clean_text.c_str(), style.clean_text.c_str() + style.clean_text.size());
                        ImGui::PopStyleColor();
                    } else {
                        MarkdownDetail::RenderTextWithSearchHighlight(style.clean_text.c_str(), style.clean_text.c_str() + style.clean_text.size());
                    }
                    if (has_icon) {
                        ImGui::SetCursorScreenPos(ImVec2(pos.x + content_w, pos.y));
                    }
                } else if (has_icon) {
                    ImGui::Dummy(ImVec2(content_w, font_size));
                }

                if (mono_font) {
                    ImGui::PopFont();
                } else {
                    PopMarkdownTagFont(config, style);
                }
            };
            s_CustomTags["mono"] = monoCodeHandler;
            s_CustomTags["code"] = monoCodeHandler;

            // Built-in Tag 6: <u>, <ins> (Underline)
            auto underlineHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : style.color_param;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                EnsureTagFitsOnLine(text_size.x);

                bool no_wrap = (config && config->wrapWidth < 0.0f);
                float avail_w = (config && config->wrapWidth > 0.0f) ? config->wrapWidth : ImGui::GetContentRegionAvail().x;
                if (no_wrap || text_size.x <= avail_w) {
                    RenderInlineTaggedText(style.clean_text, config);
                } else {
                    float wrap_pos = (config && config->wrapWidth > 0.0f) ? (ImGui::GetCursorPos().x + config->wrapWidth) : 0.0f;
                    ImGui::PushTextWrapPos(wrap_pos);
                    RenderInlineTaggedText(style.clean_text, config);
                    ImGui::PopTextWrapPos();
                }

                DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), false);

                if (has_col) ImGui::PopStyleColor();
                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["u"]   = underlineHandler;
            s_CustomTags["ins"] = underlineHandler;

            // Built-in Tag 7: <s>, <del>, <strike> (Strikethrough)
            auto strikeHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);

                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : style.color_param;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                EnsureTagFitsOnLine(text_size.x);

                bool no_wrap = (config && config->wrapWidth < 0.0f);
                float avail_w = (config && config->wrapWidth > 0.0f) ? config->wrapWidth : ImGui::GetContentRegionAvail().x;
                if (no_wrap || text_size.x <= avail_w) {
                    RenderInlineTaggedText(style.clean_text, config);
                } else {
                    float wrap_pos = (config && config->wrapWidth > 0.0f) ? (ImGui::GetCursorPos().x + config->wrapWidth) : 0.0f;
                    ImGui::PushTextWrapPos(wrap_pos);
                    RenderInlineTaggedText(style.clean_text, config);
                    ImGui::PopTextWrapPos();
                }

                DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), true);

                if (has_col) ImGui::PopStyleColor();
                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["s"]      = strikeHandler;
            s_CustomTags["del"]    = strikeHandler;
            s_CustomTags["strike"] = strikeHandler;

            // Built-in Tag 8: <center>, <centre>
            auto centerHandler = [](const std::string& inner_text, const std::string& /*param*/) {
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                const float avail_w = MarkdownDetail::ResolveFullLineAvailWidth(config);

                std::istringstream stream(inner_text);
                std::string line;
                bool first = true;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    size_t first_non = line.find_first_not_of(" \t");
                    if (first_non == std::string::npos) {
                        if (!first) ImGui::NewLine();
                        first = false;
                        continue;
                    }
                    std::string trimmed = line.substr(first_non);
                    size_t last_non = trimmed.find_last_not_of(" \t");
                    if (last_non != std::string::npos) {
                        trimmed = trimmed.substr(0, last_non + 1);
                    }

                    float line_avail_w = avail_w;
                    float left_in = 0.0f, right_in = 0.0f;
                    if (MarkdownDetail::QueryFloatBandAtY(ImGui::GetCursorScreenPos().y, ImGui::GetTextLineHeight(), left_in, right_in)) {
                        MarkdownDetail::PrepareLineStartForFloats();
                        line_avail_w = ImMax(10.0f, avail_w - left_in - right_in);
                    }

                    MarkdownDetail::ScopedFullLineWidth center_full_w_scope(line_avail_w, true);
                    float line_w = MeasureMarkdownLineWidth(trimmed.c_str(), trimmed.size(), config);
                    float offset_x = (line_avail_w - line_w) * 0.5f;
                    if (offset_x > 0.0f) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);
                    }

                    MarkdownExt(trimmed.c_str(), trimmed.size(), config, false);
                    first = false;
                }
            };
            s_CustomTags["center"] = centerHandler;
            s_CustomTags["centre"] = centerHandler;

            // Built-in Tag 8b: <blockquote>, <callout>, <alert>, <notice>
            // Nesting-safe GFM Callout Cards & Standard Blockquotes (zero ImDrawListSplitter!)
            auto renderBlockquoteOrCallout = [](const std::string& inner_text, const std::string& param, bool is_callout) {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                if (!win) return;

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                if (win->DC.IsSameLine) {
                    ImGui::NewLine();
                }
#endif
                const MarkdownConfig* active_cfg = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                MarkdownConfig inner_cfg = active_cfg ? *active_cfg : MarkdownConfig{};

                float scale = ImGui::GetVisualScale();
                if (scale <= 0.0f) scale = 1.0f;

                const ImVec4 win_bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
                const bool light_theme = (win_bg.x * 0.299f + win_bg.y * 0.587f + win_bg.z * 0.114f) > 0.55f;

                Detail::CalloutKind kind = Detail::CalloutKind::Note;
                bool has_custom_title = false;
                std::string custom_title;
                if (is_callout) {
                    Detail::ParseCalloutTagParams(param, kind, has_custom_title, custom_title);
                }

                ImU32 accent_col = 0;
                ImU32 bg_col     = 0;
                ImU32 border_col = 0;
                const char* icon_name = "info";
                std::string header_label = "NOTE";

                if (is_callout) {
                    switch (kind) {
                        case Detail::CalloutKind::Tip:
                            accent_col   = light_theme ? IM_COL32(22, 138, 72, 255)  : IM_COL32(56, 204, 118, 255);
                            bg_col       = light_theme ? IM_COL32(22, 163, 74, 24)   : IM_COL32(24, 98, 56, 48);
                            border_col   = light_theme ? IM_COL32(22, 138, 72, 68)   : IM_COL32(56, 204, 118, 64);
                            icon_name    = "lightbulb";
                            header_label = "TIP";
                            break;
                        case Detail::CalloutKind::Important:
                            accent_col   = light_theme ? IM_COL32(126, 58, 212, 255) : IM_COL32(178, 108, 255, 255);
                            bg_col       = light_theme ? IM_COL32(139, 92, 246, 24)  : IM_COL32(88, 38, 145, 48);
                            border_col   = light_theme ? IM_COL32(126, 58, 212, 68)  : IM_COL32(178, 108, 255, 64);
                            icon_name    = "important";
                            header_label = "IMPORTANT";
                            break;
                        case Detail::CalloutKind::Warning:
                            accent_col   = light_theme ? IM_COL32(180, 105, 8, 255)  : IM_COL32(245, 166, 35, 255);
                            bg_col       = light_theme ? IM_COL32(234, 150, 12, 28)  : IM_COL32(125, 72, 16, 52);
                            border_col   = light_theme ? IM_COL32(180, 105, 8, 72)   : IM_COL32(245, 166, 35, 68);
                            icon_name    = "warning";
                            header_label = "WARNING";
                            break;
                        case Detail::CalloutKind::Caution:
                            accent_col   = light_theme ? IM_COL32(198, 38, 38, 255)  : IM_COL32(244, 78, 78, 255);
                            bg_col       = light_theme ? IM_COL32(220, 38, 38, 24)   : IM_COL32(130, 30, 30, 52);
                            border_col   = light_theme ? IM_COL32(198, 38, 38, 72)   : IM_COL32(244, 78, 78, 68);
                            icon_name    = "caution";
                            header_label = "CAUTION";
                            break;
                        case Detail::CalloutKind::Note:
                        default:
                            accent_col   = light_theme ? IM_COL32(37, 99, 215, 255)  : IM_COL32(96, 148, 255, 255);
                            bg_col       = light_theme ? IM_COL32(59, 130, 246, 24)  : IM_COL32(42, 72, 150, 48);
                            border_col   = light_theme ? IM_COL32(37, 99, 215, 68)   : IM_COL32(96, 148, 255, 64);
                            icon_name    = "info";
                            header_label = "NOTE";
                            break;
                    }
                    if (has_custom_title) {
                        header_label = custom_title;
                    }
                } else {
                    // Standard '> ...' blockquote: quieter neutral slate bar & whisper background tint
                    accent_col = light_theme ? IM_COL32(120, 132, 152, 195) : IM_COL32(135, 146, 168, 185);
                    bg_col     = light_theme ? IM_COL32(100, 116, 139, 18)  : IM_COL32(145, 158, 182, 22);
                    border_col = 0;
                }

                const float bar_w    = is_callout ? ImMax(3.0f, std::round(4.0f * scale)) : ImMax(2.5f, std::round(3.5f * scale));
                const float pad_x    = is_callout ? std::round(12.0f * scale) : std::round(11.0f * scale);
                const float pad_y    = std::round(6.0f * scale);
                const float inset_x  = std::round(1.0f * scale);
                const float rounding = is_callout ? std::round(4.0f * scale)  : std::round(3.0f * scale);

                float raw_avail_w = (active_cfg && active_cfg->wrapWidth > 0.0f)
                                        ? active_cfg->wrapWidth
                                        : MarkdownDetail::ResolveFullLineAvailWidth(active_cfg);
                {
                    MarkdownDetail::FloatLayoutScope* fl_scope = MarkdownDetail::GetActiveFloatScope();
                    const float cur_y = ImGui::GetCursorScreenPos().y;
                    if (fl_scope && !fl_scope->boxes.empty() && fl_scope->deepestMaxY > cur_y + 0.5f) {
                        const float remaining_span = ImMax(ImGui::GetTextLineHeight() * 2.0f, fl_scope->deepestMaxY - cur_y);
                        float fl_left = 0.0f, fl_right = 0.0f;
                        if (MarkdownDetail::QueryFloatBandAtY(cur_y, remaining_span, fl_left, fl_right)) {
                            if ((raw_avail_w - fl_left - fl_right) < 220.0f * scale) {
                                MarkdownDetail::ClearActiveFloats();
                            } else {
                                fl_scope->justPlacedFloat = false;
                                const float natural_start_x = MarkdownDetail::GetWindowNaturalLineStartX(win);
                                const float semantic_indent = ImMax(0.0f, natural_start_x - fl_scope->containerLeftX);
                                const float target_start_x = fl_scope->containerLeftX + fl_left + semantic_indent;
                                if (ImGui::GetCursorScreenPos().x < target_start_x - 0.5f) {
                                    ImGui::SetCursorScreenPos(ImVec2(target_start_x, cur_y));
                                }
                                raw_avail_w -= (fl_left + fl_right);
                            }
                        }
                    }
                }
                const bool shrink_to_content = (active_cfg && active_cfg->wrapWidth < 0.0f && ImGui::GetContentRegionAvail().x > 16384.0f);
                const float outer_w = ImMax(60.0f * scale, raw_avail_w - inset_x * 2.0f);
                const float inner_avail_w = ImMax(40.0f * scale, outer_w - (bar_w + pad_x * 2.0f));
                inner_cfg.wrapWidth = shrink_to_content ? -1.0f : inner_avail_w;

                // Strip only leading/trailing blank lines (preserving column indentation on content lines!)
                size_t b_start = 0;
                while (b_start < inner_text.size() && (inner_text[b_start] == '\r' || inner_text[b_start] == '\n')) {
                    ++b_start;
                }
                size_t b_end = inner_text.size();
                while (b_end > b_start && (inner_text[b_end - 1] == '\r' || inner_text[b_end - 1] == '\n' || inner_text[b_end - 1] == ' ' || inner_text[b_end - 1] == '\t')) {
                    --b_end;
                }
                std::string clean_body = (b_end > b_start) ? inner_text.substr(b_start, b_end - b_start) : std::string();

                ImGui::BeginGroup();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                Detail::ImDrawListUnderlayScope underlay(dl);

                ImVec2 box_start = ImGui::GetCursorScreenPos();
                box_start.x += inset_x;

                ImGui::SetCursorScreenPos(ImVec2(box_start.x + bar_w + pad_x, box_start.y + pad_y));
                ImGui::BeginGroup();

                const bool show_header = is_callout && (!has_custom_title || !header_label.empty());
                if (show_header) {
                    ImFont* hdr_bold = inner_cfg.boldFont;
#if defined(CONTRIB_HAS_TYPOGRAPHY) || defined(IMGUI_FONT_ROLES_H)
                    if (!hdr_bold) hdr_bold = ImGui::GetFont(ImFontRole::Bold);
#endif
                    if (hdr_bold) ImGui::ImGeeCompat::PushFont(hdr_bold);

                    float hdr_font_sz = ImGui::GetFontSize();
                    if (hdr_font_sz <= 0.0f) hdr_font_sz = 14.0f * scale;
                    float icon_sz = std::round(hdr_font_sz * 0.95f);
                    ImVec2 hdr_pos = ImGui::GetCursorScreenPos();
                    float icon_y = hdr_pos.y + std::round((hdr_font_sz - icon_sz) * 0.5f);

#if defined(CONTRIB_HAS_ICONS) || defined(CONTRIB_ICONS_AVAILABLE)
                    if (!ImGui::DrawVectorIcon(dl, ImVec2(hdr_pos.x, icon_y), icon_sz, accent_col, icon_name)) {
                        Detail::DrawMinimalCalloutGlyph(dl, ImVec2(hdr_pos.x, icon_y), icon_sz, accent_col, kind);
                    }
#else
                    (void)icon_name;
                    Detail::DrawMinimalCalloutGlyph(dl, ImVec2(hdr_pos.x, icon_y), icon_sz, accent_col, kind);
#endif
                    ImGui::Dummy(ImVec2(icon_sz, hdr_font_sz));
                    ImGui::SameLine(0.0f, std::round(6.0f * scale));

                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(accent_col));
                    RenderInlineTaggedText(header_label, &inner_cfg);
                    ImGui::PopStyleColor();

                    if (hdr_bold) ImGui::PopFont();

                    if (!clean_body.empty()) {
                        ImGui::Dummy(ImVec2(0.0f, std::round(3.0f * scale)));
                    }
                }

                if (!clean_body.empty()) {
                    bool pushed_quote_tint = false;
                    if (!is_callout) {
                        ImVec4 base_txt = ImGui::GetStyle().Colors[ImGuiCol_Text];
                        ImVec4 muted_txt(base_txt.x, base_txt.y, base_txt.z, base_txt.w * 0.90f);
                        ImGui::PushStyleColor(ImGuiCol_Text, muted_txt);
                        pushed_quote_tint = true;
                    }
                    MarkdownExt(clean_body.c_str(), clean_body.size(), &inner_cfg);
                    if (pushed_quote_tint) {
                        ImGui::PopStyleColor();
                    }
                } else if (!show_header) {
                    // Empty blockquote ('>'): preserve one minimal line height
                    ImGui::Dummy(ImVec2(1.0f, ImMax(8.0f, ImGui::GetTextLineHeight() * 0.6f)));
                }

                ImGui::EndGroup();
                ImVec2 inner_max = ImGui::GetItemRectMax();

                const float content_w = (inner_max.x - box_start.x) + pad_x;
                const float final_w = shrink_to_content ? ImMax(content_w, 60.0f * scale) : outer_w;
                const ImVec2 box_min = box_start;
                const ImVec2 box_max(box_start.x + final_w, inner_max.y + pad_y);

                // Extend outer group cleanly to box_max using a real Dummy item (zero IsSetPos warning!)
                ImGui::SetCursorScreenPos(ImVec2(box_start.x - inset_x, inner_max.y));
                ImGui::Dummy(ImVec2(final_w + inset_x, pad_y));

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18200)
                const ImDrawFlags round_left_flags = ImDrawFlags_RoundCornersLeft;
#else
                const ImDrawFlags round_left_flags = static_cast<ImDrawFlags>(1 | 4);
#endif
                underlay.BeginBackground();
                dl->AddRectFilled(box_min, box_max, bg_col, rounding);
                dl->AddRectFilled(box_min, ImVec2(box_min.x + bar_w, box_max.y), accent_col, rounding, round_left_flags);
                if (border_col != 0) {
                    ImGui::MarkdownDetail::AddRectCompat(dl, box_min, box_max, border_col, rounding, 0, ImMax(1.0f, std::round(1.0f * scale)));
                }
                underlay.EndUnderlay();

                ImGui::EndGroup();
            };

            auto blockquoteHandler = [renderBlockquoteOrCallout](const std::string& inner_text, const std::string& param) {
                renderBlockquoteOrCallout(inner_text, param, false);
            };
            auto calloutHandler = [renderBlockquoteOrCallout](const std::string& inner_text, const std::string& param) {
                renderBlockquoteOrCallout(inner_text, param, true);
            };
            s_CustomTags["blockquote"] = blockquoteHandler;
            s_CustomTags["callout"]    = calloutHandler;
            s_CustomTags["alert"]      = calloutHandler;
            s_CustomTags["notice"]     = calloutHandler;

            // Built-in Tag 9: <img ... /> or <img ...>
            auto imgHandler = [](const std::string& inner_text, const std::string& param) {
                (void)inner_text;
                const MarkdownConfig* active_cfg = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                MarkdownConfig fallback_cfg;
                const MarkdownConfig& cfg_ref = active_cfg ? *active_cfg : fallback_cfg;

                auto imageCb = cfg_ref.imageCallback
                               ? cfg_ref.imageCallback
                               : GetDefaultMarkdownImageCallback();
                if (!imageCb) {
                    ImGui::TextDisabled("( Image service disabled )");
                    return;
                }

                MarkdownLinkCallbackData cbData;
                cbData.text = "";
                cbData.textLength = 0;
                cbData.link = param.c_str();
                cbData.linkLength = (int)param.length();
                cbData.userData = cfg_ref.userData;
                cbData.isImage = true;

                MarkdownImageData imgData = imageCb(cbData);
                MarkdownDetail::RenderMarkdownImageElement(cfg_ref, imgData, "", 0, param.c_str(), (int)param.length(), false);
            };
            s_CustomTags["img"] = imgHandler;
            s_CustomTags["clear"] = [](const std::string&, const std::string& param) {
                const MarkdownImageFloat mode = MarkdownDetail::ParseClearFloatMode(param.c_str(), param.size(), false);
                MarkdownDetail::ClearActiveFloats(mode);
            };
            s_CustomTags["br"] = [](const std::string&, const std::string& param) {
                const MarkdownImageFloat mode = MarkdownDetail::ParseClearFloatMode(param.c_str(), param.size(), true);
                if (static_cast<unsigned char>(mode) != 255) {
                    if (!MarkdownDetail::ClearActiveFloats(mode)) ImGui::NewLine();
                } else {
                    ImGui::NewLine();
                }
            };
#if defined(CONTRIB_HAS_ICONS)
            // Built-in Tag 10: <icon ... /> or <icon>...</icon>
            // Renders zero-dependency, zero-font procedural vector icons
            auto iconHandler = [](const std::string& inner_text, const std::string& param) {
                std::string raw = !inner_text.empty() ? inner_text : param;
                std::string icon_name = "";
                std::string col_str = "";
                float custom_size = 0.0f;

                if (!inner_text.empty() && !param.empty()) {
                    col_str = param;
                    icon_name = inner_text;
                } else {
                    std::istringstream iss(raw);
                    std::string token;
                    while (iss >> token) {
                        if (token.find("color=") == 0 || token.find("col=") == 0) {
                            size_t eq = token.find('=');
                            col_str = token.substr(eq + 1);
                        } else if (token.find("name=") == 0 || token.find("icon=") == 0) {
                            size_t eq = token.find('=');
                            icon_name = token.substr(eq + 1);
                        } else if (token.find("size=") == 0) {
                            size_t eq = token.find('=');
                            custom_size = (float)atof(token.substr(eq + 1).c_str());
                        } else {
                            std::string norm = NormalizeIconName(token.c_str());
                            if (IsVectorIcon(norm.c_str()) && icon_name.empty()) {
                                icon_name = norm;
                            } else {
                                ImVec4 test_c;
                                if (ParseMarkdownColor(token, test_c) && col_str.empty()) {
                                    col_str = token;
                                } else if (icon_name.empty()) {
                                    icon_name = norm;
                                }
                            }
                        }
                    }
                }

                if (icon_name.empty()) return;

                const float font_size = ImGui::GetFontSize();
                const float icon_sz = (custom_size > 0.0f) ? custom_size : (font_size * 0.90f);

                EnsureTagFitsOnLine(icon_sz + 2.0f);

                ImVec4 c(1.0f, 1.0f, 1.0f, 1.0f);
                bool has_col = false;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, c);
                }

                ImGui::Icon(icon_name.c_str(), icon_sz, has_col ? &c : nullptr);
            };
            s_CustomTags["icon"] = iconHandler;
#endif
#endif

#if defined(CONTRIB_HAS_TYPOGRAPHY) || defined(IMGUI_FONT_ROLES_H)
            auto sizedTextHandler = [](const std::string& inner_text, const std::string& param, ImFontRole role) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();

                ImGui::PushFontRole(role);

                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : (!style.color_param.empty() ? style.color_param : style.bg_color);
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }

                ImVec2 text_size = ImGui::CalcTextSize(style.clean_text.c_str());
                EnsureTagFitsOnLine(text_size.x);

                bool no_wrap = (config && config->wrapWidth < 0.0f);
                float avail_w = (config && config->wrapWidth > 0.0f) ? config->wrapWidth : ImGui::GetContentRegionAvail().x;
                if (no_wrap || text_size.x <= avail_w) {
                    RenderInlineTaggedText(style.clean_text, config);
                } else {
                    float wrap_pos = (config && config->wrapWidth > 0.0f) ? (ImGui::GetCursorPos().x + config->wrapWidth) : 0.0f;
                    ImGui::PushTextWrapPos(wrap_pos);
                    RenderInlineTaggedText(style.clean_text, config);
                    ImGui::PopTextWrapPos();
                }

                if (style.is_strike) {
                    DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), true);
                }
                if (style.is_underline) {
                    DrawTextDecorationLine(ImGui::GetColorU32(has_col ? col : ImGui::GetStyle().Colors[ImGuiCol_Text]), false);
                }

                if (has_col) ImGui::PopStyleColor();
                ImGui::PopFontRole();
            };

            s_CustomTags["small"]   = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Small); };
            s_CustomTags["tiny"]    = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Tiny); };
            s_CustomTags["big"]     = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Title); };
            s_CustomTags["large"]   = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Title); };
            s_CustomTags["huge"]    = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::BoldLarge); };
            s_CustomTags["regular"] = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Regular); };
            s_CustomTags["prop"]    = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Regular); };
            s_CustomTags["sans"]    = [sizedTextHandler](const std::string& inner, const std::string& param) { sizedTextHandler(inner, param, ImFontRole::Regular); };
#endif

            // Inline formatting tags: <b>, <strong>, <i>, <em>, <bi>
            // (<u>/<ins> and <s>/<del>/<strike> are already registered above with color/font-aware underlineHandler & strikeHandler)
            auto bHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                style.is_bold = true;
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : style.color_param;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }
                RenderInlineTaggedText(style.clean_text, config);
                if (has_col) ImGui::PopStyleColor();
                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["b"]      = bHandler;
            s_CustomTags["strong"] = bHandler;

            auto iHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                style.is_italic = true;
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : style.color_param;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }
                RenderInlineTaggedText(style.clean_text, config);
                if (has_col) ImGui::PopStyleColor();
                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["i"]  = iHandler;
            s_CustomTags["em"] = iHandler;

            auto biHandler = [](const std::string& inner_text, const std::string& param) {
                auto style = ParseMarkdownTagStyle(inner_text, param);
                style.is_bold = true;
                style.is_italic = true;
                const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
                PushMarkdownTagFont(config, style);
                ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
                bool has_col = false;
                std::string col_str = !style.fg_color.empty() ? style.fg_color : style.color_param;
                if (!col_str.empty()) {
                    has_col = ParseMarkdownColor(col_str, col);
                    if (has_col) ImGui::PushStyleColor(ImGuiCol_Text, col);
                }
                RenderInlineTaggedText(style.clean_text, config);
                if (has_col) ImGui::PopStyleColor();
                PopMarkdownTagFont(config, style);
            };
            s_CustomTags["bi"] = biHandler;
        }
        return s_CustomTags;
    }

    // Transforms inline markdown emphasis (*, **, ***, _, __, ___, ~~, ++, `)
    // outside of XML tags into corresponding tags (`<i>`, `<b>`, `<bi>`, `<s>`, `<u>`, `<code>`).
    inline std::string ExpandInlineMarkdownEmphasisToTags(const std::string& input) {
        if (input.empty()) return "";

        std::string out;
        out.reserve(input.length() + 32);

        size_t i = 0;
        size_t len = input.length();

        while (i < len) {
            // Skip existing XML tags verbatim:
            if (input[i] == '<') {
                size_t tag_end = input.find('>', i);
                if (tag_end != std::string::npos) {
                    out.append(input, i, tag_end - i + 1);
                    i = tag_end + 1;
                    continue;
                }
            }

            // Inline code span `...`
            if (input[i] == '`') {
                size_t close_pos = input.find('`', i + 1);
                if (close_pos != std::string::npos && close_pos > i + 1) {
                    out.append("<code>");
                    out.append(input, i + 1, close_pos - (i + 1));
                    out.append("</code>");
                    i = close_pos + 1;
                    continue;
                }
            }

            // Strikethrough ~~...~~
            if (input[i] == '~' && i + 1 < len && input[i + 1] == '~') {
                size_t close_pos = input.find("~~", i + 2);
                if (close_pos != std::string::npos && close_pos > i + 2) {
                    out.append("<s>");
                    out.append(input, i + 2, close_pos - (i + 2));
                    out.append("</s>");
                    i = close_pos + 2;
                    continue;
                }
            }

            // Underline ++...++
            if (input[i] == '+' && i + 1 < len && input[i + 1] == '+') {
                size_t close_pos = input.find("++", i + 2);
                if (close_pos != std::string::npos && close_pos > i + 2) {
                    out.append("<u>");
                    out.append(input, i + 2, close_pos - (i + 2));
                    out.append("</u>");
                    i = close_pos + 2;
                    continue;
                }
            }

            // Triple delimiter: ***...*** or ___...___ (Bold-Italic)
            if ((input[i] == '*' && i + 2 < len && input[i + 1] == '*' && input[i + 2] == '*') ||
                (input[i] == '_' && i + 2 < len && input[i + 1] == '_' && input[i + 2] == '_')) {
                std::string delim(3, input[i]);
                size_t close_pos = input.find(delim, i + 3);
                if (close_pos != std::string::npos && close_pos > i + 3) {
                    out.append("<bi>");
                    out.append(input, i + 3, close_pos - (i + 3));
                    out.append("</bi>");
                    i = close_pos + 3;
                    continue;
                }
            }

            // Double delimiter: **...** or __...__ (Bold)
            if ((input[i] == '*' && i + 1 < len && input[i + 1] == '*') ||
                (input[i] == '_' && i + 1 < len && input[i + 1] == '_')) {
                std::string delim(2, input[i]);
                size_t close_pos = input.find(delim, i + 2);
                if (close_pos != std::string::npos && close_pos > i + 2) {
                    out.append("<b>");
                    out.append(input, i + 2, close_pos - (i + 2));
                    out.append("</b>");
                    i = close_pos + 2;
                    continue;
                }
            }

            // Single delimiter: *...* or _..._ (Italic)
            if (input[i] == '*' || input[i] == '_') {
                char delim = input[i];
                size_t close_pos = input.find(delim, i + 1);
                if (close_pos != std::string::npos && close_pos > i + 1) {
                    bool valid = true;
                    if (close_pos + 1 < len && input[close_pos + 1] == delim) valid = false;
                    if (valid) {
                        out.append("<i>");
                        out.append(input, i + 1, close_pos - (i + 1));
                        out.append("</i>");
                        i = close_pos + 1;
                        continue;
                    }
                }
            }

            out.push_back(input[i++]);
        }

        return out;
    }

    /** Recursively renders text containing embedded inline XML/HTML tags with full nested styling */
    inline void RenderInlineTaggedText(const std::string& input_text, const MarkdownConfig* config) {
        (void)config;
        std::string preprocessed_icons;
        const std::string& icon_ready = PreprocessMarkdownIconsAndShortcodes(input_text.c_str(), input_text.length(), preprocessed_icons)
                                            ? preprocessed_icons
                                            : input_text;
        std::string text = (icon_ready.find('*') != std::string::npos || icon_ready.find('_') != std::string::npos ||
                            icon_ready.find('~') != std::string::npos || icon_ready.find('+') != std::string::npos ||
                            icon_ready.find('`') != std::string::npos)
                           ? ExpandInlineMarkdownEmphasisToTags(icon_ready)
                           : icon_ready;

        if (text.find('<') == std::string::npos) {
            MarkdownDetail::RenderTextWithSearchHighlight(text.c_str(), text.c_str() + text.size());
            return;
        }

        size_t pos = 0;
        size_t len = text.length();
        auto& tags = GetCustomMarkdownTags();
        bool first = true;

        while (pos < len) {
            size_t open = text.find('<', pos);
            if (open == std::string::npos) {
                std::string rem = text.substr(pos);
                if (!rem.empty()) {
                    if (!first) ImGui::SameLine(0.0f, 0.0f);
                    MarkdownDetail::RenderTextWithSearchHighlight(rem.c_str(), rem.c_str() + rem.size());
                }
                break;
            }

            if (open > pos) {
                std::string pre = text.substr(pos, open - pos);
                if (!pre.empty()) {
                    if (!first) ImGui::SameLine(0.0f, 0.0f);
                    MarkdownDetail::RenderTextWithSearchHighlight(pre.c_str(), pre.c_str() + pre.size());
                    first = false;
                }
            }

            size_t close_header = text.find('>', open + 1);
            if (close_header == std::string::npos) {
                std::string rem = text.substr(open);
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                MarkdownDetail::RenderTextWithSearchHighlight(rem.c_str(), rem.c_str() + rem.size());
                break;
            }

            std::string header = text.substr(open + 1, close_header - (open + 1));
            bool isSelfClosing = (!header.empty() && header.back() == '/');
            if (isSelfClosing) {
                header.pop_back();
                while (!header.empty() && (header.back() == ' ' || header.back() == '\t')) header.pop_back();
            }

            size_t sp = header.find_first_of(" \t=");
            std::string tag_name = (sp != std::string::npos) ? header.substr(0, sp) : header;
            std::string tag_param = (sp != std::string::npos) ? header.substr(sp + 1) : "";
            for (char& c : tag_name) c = (char)tolower((unsigned char)c);

            if (isSelfClosing) {
                auto it = tags.find(tag_name);
                if (it != tags.end()) {
                    if (!first) ImGui::SameLine(0.0f, 0.0f);
                    it->second("", tag_param);
                    first = false;
                }
                pos = close_header + 1;
                continue;
            }

            std::string close_tag = "</" + tag_name + ">";
            size_t close_pos = std::string::npos;
            for (size_t k = close_header + 1; k + close_tag.length() <= len; ++k) {
                if (text[k] == '<' && text[k + 1] == '/') {
                    bool match = true;
                    for (size_t m = 0; m < tag_name.length(); ++m) {
                        if (tolower((unsigned char)text[k + 2 + m]) != tag_name[m]) {
                            match = false;
                            break;
                        }
                    }
                    if (match && text[k + 2 + tag_name.length()] == '>') {
                        close_pos = k;
                        break;
                    }
                }
            }

            if (close_pos == std::string::npos) {
                std::string chunk = text.substr(open, close_header + 1 - open);
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                MarkdownDetail::RenderTextWithSearchHighlight(chunk.c_str(), chunk.c_str() + chunk.size());
                first = false;
                pos = close_header + 1;
                continue;
            }

            std::string inner = text.substr(close_header + 1, close_pos - (close_header + 1));
            auto it = tags.find(tag_name);
            if (it != tags.end()) {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                it->second(inner, tag_param);
                first = false;
            } else {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                MarkdownDetail::RenderTextWithSearchHighlight(inner.c_str(), inner.c_str() + inner.size());
                first = false;
            }
            pos = close_pos + tag_name.length() + 3;
        }
    }

    // Registration APIs (supporting both 2-arg parameterized and 1-arg simple callbacks):
    // Explicit host registrations always overwrite built-in default tags (such as "callout" or "blockquote").
    inline void RegisterMarkdownTag(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
        std::string norm_name = tag_name;
        for (char& c : norm_name) c = (char)tolower((unsigned char)c);
        GetCustomMarkdownTags()[norm_name] = callback;
    }

    inline void RegisterMarkdownTag(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
        RegisterMarkdownTag(tag_name, [callback](const std::string& text, const std::string&) {
            callback(text);
        });
    }

    // Backward-compatibility aliases
    inline void RegisterMarkdownWidget(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
        RegisterMarkdownTag(tag_name, callback);
    }
    inline void RegisterMarkdownWidget(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
        RegisterMarkdownTag(tag_name, callback);
    }

    // Declarative style tag registration: combines colors, fonts, emphasis, and backgrounds into a reusable semantic tag.
    // Examples:
    //   RegisterMarkdownStyleTag("cam", "green bold mono");
    //   RegisterMarkdownStyleTag("opt", "gold mono bg=#332a00");
    inline void RegisterMarkdownStyleTag(const std::string& tag_name, const std::string& default_style_params) {
        RegisterMarkdownTag(tag_name, [default_style_params](const std::string& inner_text, const std::string& param) {
            std::string combined = default_style_params;
            if (!param.empty()) {
                combined += " " + param;
            }
            auto& tags = GetCustomMarkdownTags();
            auto it = tags.find("style");
            if (it != tags.end()) {
                it->second(inner_text, combined);
            }
        });
    }

    // Declarative markdown template/alias tag registration: expands any arbitrary markdown or XML string template.
    // %s is replaced by the inner text anywhere in the template (can appear multiple times).
    // Examples:
    //   RegisterMarkdownTagAlias("cam", "<style green bold mono>%s</style>");
    //   RegisterMarkdownTagAlias("axis", "**Axis: <style green bold>%s</style>**");
    inline void RegisterMarkdownTagAlias(const std::string& tag_name, const std::string& format_template) {
        RegisterMarkdownTag(tag_name, [format_template](const std::string& inner_text, const std::string& /*param*/) {
            std::string expanded = format_template;
            size_t pos = 0;
            while ((pos = expanded.find("%s", pos)) != std::string::npos) {
                expanded.replace(pos, 2, inner_text);
                pos += inner_text.length();
            }
            const MarkdownConfig* config = GetActiveMarkdownConfig() ? GetActiveMarkdownConfig() : GetDefaultMarkdownConfig();
            RenderInlineTaggedText(expanded, config);
        });
    }

    // 4. Core Markdown Extension Engine (Unified Single-Pass Pipeline)
    inline void ResolveMarkdownConfigFonts(MarkdownConfig& cfg) {
        // 1. Inherit any unset font slots from GetDefaultMarkdownConfig()
        const MarkdownConfig* def = GetDefaultMarkdownConfig();
        if (def && def != &cfg) {
            if (!cfg.regularFont)    cfg.regularFont    = def->regularFont;
            if (!cfg.boldFont)       cfg.boldFont       = def->boldFont;
            if (!cfg.italicFont)     cfg.italicFont     = def->italicFont;
            if (!cfg.boldItalicFont) cfg.boldItalicFont = def->boldItalicFont;
            if (!cfg.monoFont)       cfg.monoFont       = def->monoFont;
            if (!cfg.h1Font)         cfg.h1Font         = def->h1Font;
            if (!cfg.h2Font)         cfg.h2Font         = def->h2Font;
            if (!cfg.h3Font)         cfg.h3Font         = def->h3Font;
            for (int hi = 0; hi < MarkdownConfig::NUMHEADINGS; ++hi) {
                if (!cfg.headingFormats[hi].font) {
                    cfg.headingFormats[hi].font = def->headingFormats[hi].font;
                }
            }
        }
        // 2. Auto-wire remaining unset font slots from ImFontRole when Typography is enabled
#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGEE_DISABLE_TYPOGRAPHY) && !defined(CONTRIB_DISABLE_TYPOGRAPHY)
        ImFont* ambient = ImGui::GetCurrentContext() ? ImGui::GetFont() : nullptr;
        auto resolveRole = [&](ImFontRole role) -> ImFont* {
            ImFont* explicit_slot = Detail::GetFontRoleSlot(role);
            if (explicit_slot) return explicit_slot;
            ImFont* resolved = ImGui::GetFont(role);
            if (role == ImFontRole::Regular) return resolved;
            return (resolved && resolved != ambient) ? resolved : nullptr;
        };
        if (!cfg.regularFont)    cfg.regularFont    = resolveRole(ImFontRole::Regular);
        if (!cfg.boldFont)       cfg.boldFont       = resolveRole(ImFontRole::Bold);
        if (!cfg.italicFont)     cfg.italicFont     = resolveRole(ImFontRole::Italic);
        if (!cfg.boldItalicFont) cfg.boldItalicFont = resolveRole(ImFontRole::BoldItalic);
        if (!cfg.monoFont)       cfg.monoFont       = resolveRole(ImFontRole::Monospace);
        if (!cfg.h1Font)         cfg.h1Font         = resolveRole(ImFontRole::Heading1);
        if (!cfg.h2Font)         cfg.h2Font         = resolveRole(ImFontRole::Heading2);
        if (!cfg.h3Font)         cfg.h3Font         = resolveRole(ImFontRole::Heading3);
        if (!cfg.headingFormats[0].font && cfg.h1Font) cfg.headingFormats[0].font = cfg.h1Font;
        if (!cfg.headingFormats[1].font && cfg.h2Font) cfg.headingFormats[1].font = cfg.h2Font;
        if (!cfg.headingFormats[2].font && cfg.h3Font) cfg.headingFormats[2].font = cfg.h3Font;
#endif
    }

    inline void MarkdownExt(const char* markdown_text, size_t markdown_len, const MarkdownConfig* config_override, bool create_float_scope) {
        if (!markdown_text) return;
        const MarkdownConfig* config = config_override ? config_override : GetDefaultMarkdownConfig();
        static const MarkdownConfig s_FallbackConfig;
        if (!config) {
            config = &s_FallbackConfig;
        }

        MarkdownConfig local_config = *config;
        ResolveMarkdownConfigFonts(local_config);

        std::string processed_quotes;
        bool has_quotes = PreprocessMarkdownBlockquotesAndAlerts(markdown_text, markdown_len, processed_quotes, &local_config);
        const char* text_q = has_quotes ? processed_quotes.c_str() : markdown_text;
        size_t len_q = has_quotes ? processed_quotes.length() : markdown_len;

        std::string processed_center;
        bool has_blocks = PreprocessMarkdownCenteredBlocks(text_q, len_q, processed_center);
        const char* text_mid = has_blocks ? processed_center.c_str() : text_q;
        size_t len_mid = has_blocks ? processed_center.length() : len_q;

        std::string processed_icons;
        bool has_icons = PreprocessMarkdownIconsAndShortcodes(text_mid, len_mid, processed_icons);
        const char* actual_text = has_icons ? processed_icons.c_str() : text_mid;
        size_t actual_len = has_icons ? processed_icons.length() : len_mid;

        const MarkdownConfig* prev_active = GetActiveMarkdownConfig();
        GetActiveMarkdownConfig() = &local_config;

        local_config.tagCallback = [](const MarkdownTagData& data) {
            std::string tag_name(data.tagName, data.tagNameLength);
            for (char& c : tag_name) c = (char)tolower((unsigned char)c);
            std::string tag_param(data.tagParam ? data.tagParam : "", data.tagParamLength);
            std::string inner_text(data.innerText, data.innerTextLength);
            
            const MarkdownConfig* prev = GetActiveMarkdownConfig();
            if (data.config) GetActiveMarkdownConfig() = data.config;

            auto& tags = GetCustomMarkdownTags();
            auto it = tags.find(tag_name);
            if (it != tags.end()) {
                it->second(inner_text, tag_param);
            } else {
                MarkdownDetail::RenderTextWithSearchHighlight(inner_text.c_str(), inner_text.c_str() + inner_text.size());
            }

            GetActiveMarkdownConfig() = prev;
        };

        MarkdownDetail::RenderInternal(actual_text, actual_len, local_config, create_float_scope);

        GetActiveMarkdownConfig() = prev_active;
    }

    inline void MarkdownExt(const std::string& markdown_text, const MarkdownConfig* config_override, bool create_float_scope) {
        MarkdownExt(markdown_text.c_str(), markdown_text.length(), config_override, create_float_scope);
    }

} // namespace ImGui

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
        ImGuiID cur_id = GetItemID();
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
        st.item_id             = GetItemID();
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

    namespace Markdown {

        using Config = MarkdownConfig;

        // Tag registration convenience aliases
        inline void RegisterTag(const std::string& tag_name, std::function<void(const std::string& inner_text, const std::string& param)> callback) {
            RegisterMarkdownTag(tag_name, callback);
        }
        inline void RegisterTag(const std::string& tag_name, std::function<void(const std::string& inner_text)> callback) {
            RegisterMarkdownTag(tag_name, callback);
        }
        inline void RegisterStyleTag(const std::string& tag_name, const std::string& default_style_params) {
            RegisterMarkdownStyleTag(tag_name, default_style_params);
        }
        inline void RegisterTagAlias(const std::string& tag_name, const std::string& format_template) {
            RegisterMarkdownTagAlias(tag_name, format_template);
        }

        namespace Detail {
            inline void RenderWithWrapMode(const char* text, size_t len, float default_wrap_width, const MarkdownConfig* config_override) {
                if (!text) return;
                const MarkdownConfig* base = config_override ? config_override : GetDefaultMarkdownConfig();
                static const MarkdownConfig s_FallbackConfig;
                if (!base) {
                    base = &s_FallbackConfig;
                }
                MarkdownConfig local_cfg = *base;
                // If config_override explicitly specifies a positive wrapWidth, honor it unless default_wrap_width > 0.0f:
                if (default_wrap_width > 0.0f) {
                    local_cfg.wrapWidth = default_wrap_width;
                } else if (!config_override || config_override->wrapWidth == 0.0f) {
                    if (default_wrap_width < 0.0f && ImGui::GetCurrentWindow() && ImGui::GetCurrentWindow()->DC.TextWrapPos >= 0.0f) {
                        // Honor active ImGui::PushTextWrapPos() stack just like standard ImGui::Text / TextUnformatted:
                        float wp = ImGui::GetCurrentWindow()->DC.TextWrapPos;
                        local_cfg.wrapWidth = (wp == 0.0f) ? 0.0f : ImMax(10.0f, wp - ImGui::GetCursorPos().x);
                    } else {
                        local_cfg.wrapWidth = default_wrap_width;
                    }
                }
                ImGui::BeginGroup();
                MarkdownExt(text, len, &local_cfg);
                ImGui::EndGroup();
            }
        } // namespace Detail

        // -------------------------------------------------------------------------
        // Standard Widget API (mirroring standard ImGui vocabulary)
        // -------------------------------------------------------------------------
        // - MD::TextUnformatted / MD::Text: Do NOT auto-wrap by default (wrapWidth = -1.0f,
        //   single line per '\n') unless an explicit wrap_width > 0.0f, wrap_size.x > 0.0f,
        //   or config_override->wrapWidth > 0.0f is supplied.
        // - MD::TextWrapped: Automatically wraps to GetContentRegionAvail().x (wrapWidth = 0.0f),
        //   or to an explicit wrap_width / wrap_size.x if supplied.

        inline void TextUnformatted(const char* text, const char* text_end = nullptr, const MarkdownConfig* config_override = nullptr) {
            if (!text) return;
            size_t len = text_end ? (size_t)(text_end - text) : std::strlen(text);
            Detail::RenderWithWrapMode(text, len, -1.0f, config_override);
        }

        inline void TextUnformatted(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_width > 0.0f) ? wrap_width : -1.0f, config_override);
        }

        inline void Text(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_size.x > 0.0f) ? wrap_size.x : -1.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), 0.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_width > 0.0f) ? wrap_width : 0.0f, config_override);
        }

        inline void TextWrapped(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            Detail::RenderWithWrapMode(markdown_text.c_str(), markdown_text.length(), (wrap_size.x > 0.0f) ? wrap_size.x : 0.0f, config_override);
        }

        // Clickable Markdown Text Links (shows Hand cursor on hover; toggles TimedTooltip on click
        // unless the return value is tested in an `if (...)` block):
        inline LinkClickResult TextUnformattedLink(const char* text, const char* text_end = nullptr, const MarkdownConfig* config_override = nullptr) {
            TextUnformatted(text, text_end, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextUnformattedLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            return TextUnformattedLink(markdown_text.c_str(), markdown_text.c_str() + markdown_text.length(), config_override);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline LinkClickResult TextLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextLink(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, wrap_width, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextLink(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_size.x, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_width, config_override);
        }

        inline LinkClickResult TextLinkContrib(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextLink(markdown_text, wrap_size.x, config_override);
        }
#pragma pop_macro("TextLink")

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, float wrap_width, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, wrap_width, config_override);
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            bool clicked = ImGui::IsItemClicked();
            ImGui::Detail::RegisterItemTooltipToggle(true);
            return LinkClickResult(clicked);
        }

        inline LinkClickResult TextWrappedLink(const std::string& markdown_text, const ImVec2& wrap_size, const MarkdownConfig* config_override = nullptr) {
            return TextWrappedLink(markdown_text, wrap_size.x, config_override);
        }

        // Backward-compatibility and convenience aliases
        inline void TextMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            Text(markdown_text, config_override);
        }

#if !defined(CONTRIB_DISABLE_TEXT_WRAPPED_MD) && !defined(IMGUI_DISABLE_TEXT_WRAPPED_MD) && !defined(CONTRIB_DISABLE_TEXTWRAPPEDMD)
        inline void TextWrappedMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
            TextWrapped(markdown_text, config_override);
        }
#endif

        // -------------------------------------------------------------------------
        // Layout Calculation API
        // -------------------------------------------------------------------------

        // Mathematically calculates the exact ImVec2 dimensions of a parsed markdown string 
        // WITHOUT rendering it to the screen. Perfect for pre-calculating X-Plane OS window boundaries!
        inline ImVec2 CalcMarkdownSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            (void)hash_id;
            ImVec2 calculated_size(0, 0);

            // Save ambient context font metrics and tooltip click-toggle state so dry-run measurement cannot leak side effects
            float saved_font_size = GImGui ? GImGui->FontSize : 14.0f;
            float saved_drawlist_size = GImGui ? GImGui->DrawListSharedData.FontSize : 14.0f;
            float saved_win_scale = (GImGui && GImGui->CurrentWindow) ? GImGui->CurrentWindow->FontWindowScale : 1.0f;
            float caller_scale = ImGui::GetFontScale(false);
            ImFont* saved_font = GImGui ? GImGui->Font : nullptr;
            const ImGui::Detail::LinkClickToggleState saved_toggle_state = ImGui::Detail::GetLinkClickToggleState();
            
            // Use static measurement window IDs so we NEVER create zombie windows or leak into g.WindowsFocusOrder!
            static const char* s_measure_windows[4] = {
                "##md_measure_0", "##md_measure_1", "##md_measure_2", "##md_measure_3"
            };
            const char* measure_name = s_measure_windows[pass & 3];
            
            float effective_scale = (font_scale > 0.0f)
                                        ? font_scale
                                        : ((caller_scale > 0.0f) ? caller_scale : ((saved_win_scale > 0.0f) ? saved_win_scale : 1.0f));
            float s = effective_scale;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f * s, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * s, 1.0f * s));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f * s);
            
            ImGui::SetNextWindowPos(ImVec2(-10000.0f, -10000.0f));
            ImGui::Begin(measure_name, nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
#if defined(IMGUI_INTERNAL_H) || defined(IMGUI_CONTRIB_INTERNAL_INCLUDED)
            if (ImGuiWindow* m_win = ImGui::GetCurrentWindow()) {
                m_win->HiddenFramesCannotSkipItems = 2;
                m_win->SkipItems = false;
            }
#endif
            ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
            // Keep outer measurement window at 1.0x so child window scale never compounds
            // (Dear ImGui multiplies child FontWindowScale by parent FontWindowScale).
            ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        
            const MarkdownConfig* default_cfg = GetDefaultMarkdownConfig();
            MarkdownConfig measure_cfg = md_config ? *md_config : (default_cfg ? *default_cfg : MarkdownConfig{});
            if (wrap_width > 0.0f) {
                measure_cfg.wrapWidth = wrap_width;
            } else if (wrap_width < 0.0f) {
                measure_cfg.wrapWidth = -1.0f;
            } else if (!md_config) {
                measure_cfg.wrapWidth = 0.0f;
            }

            // Feed it a massive constraint if 0.0f so it doesn't word-wrap infinitely to 1 char.
            // For wrap_width > 0, make child_width generous so the measurement harness never clips text!
            float child_width = (wrap_width > 0.0f) ? (wrap_width + 100.0f * s) : 99999.0f;
            // Pass 10000.0f Y to ensure we never clip vertically during the dry run.
            // We MUST NOT pass AlwaysAutoResize, or it will infinitely expand X and never word-wrap!
#if IMGUI_VERSION_NUM >= 18989
            ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 10000.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
#else
            ImGui::BeginChild("##md_measure_child", ImVec2(child_width, 10000.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
#endif
#if defined(IMGUI_INTERNAL_H) || defined(IMGUI_CONTRIB_INTERNAL_INCLUDED)
            if (ImGuiWindow* m_child = ImGui::GetCurrentWindow()) {
                m_child->HiddenFramesCannotSkipItems = 2;
                m_child->SkipItems = false;
            }
#endif
            ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
            ImGui::ImGeeCompat::SetWindowFontScale(effective_scale);
            
            ImGui::BeginGroup();
            MarkdownExt(markdown_text, &measure_cfg);
            ImGui::EndGroup();
        
            calculated_size = ImGui::GetItemRectSize();
        
            ImGui::EndChild();
            ImGui::End();
            ImGui::PopStyleVar(3);

            // Restore ambient context font metrics completely
            if (GImGui) {
                GImGui->FontSize = saved_font_size;
                GImGui->DrawListSharedData.FontSize = saved_drawlist_size;
                if (GImGui->CurrentWindow) {
                    GImGui->CurrentWindow->FontWindowScale = saved_win_scale;
                }
                if (saved_font && saved_font != GImGui->Font) {
                    GImGui->Font = saved_font;
                }
            }
            ImGui::Detail::GetLinkClickToggleState() = saved_toggle_state;
        
            return calculated_size;
        }

        inline ImVec2 CalcSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(hash_id, pass, markdown_text, wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcTextSize(ImGuiID hash_id, int pass, const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(hash_id, pass, markdown_text, wrap_width, font_scale, md_config);
        }

        // Convenient Dear ImGui-style overloads without internal hash_id / pass arguments:
        inline ImVec2 CalcMarkdownSize(const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, markdown_text, wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcMarkdownSize(const char* markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, std::string(markdown_text ? markdown_text : ""), wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcSize(const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, markdown_text, wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcSize(const char* markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, std::string(markdown_text ? markdown_text : ""), wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcTextSize(const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, markdown_text, wrap_width, font_scale, md_config);
        }

        inline ImVec2 CalcTextSize(const char* markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
            return CalcMarkdownSize(0, 0, std::string(markdown_text ? markdown_text : ""), wrap_width, font_scale, md_config);
        }

        namespace Detail {
            inline void RenderMarkdownOverlayPreservingDC(const ImVec2& screen_pos, const std::string& md_text) {
                if (md_text.empty()) return;
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                if (!window) return;

                const ImVec2 saved_cursor_pos            = window->DC.CursorPos;
                const ImVec2 saved_cursor_pos_prev_line  = window->DC.CursorPosPrevLine;
                const ImVec2 saved_cursor_max_pos        = window->DC.CursorMaxPos;
                const ImVec2 saved_curr_line_size        = window->DC.CurrLineSize;
                const ImVec2 saved_prev_line_size        = window->DC.PrevLineSize;
                const float  saved_curr_line_base_offset = window->DC.CurrLineTextBaseOffset;
                const float  saved_prev_line_base_offset = window->DC.PrevLineTextBaseOffset;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                const bool   saved_is_same_line          = window->DC.IsSameLine;
                const bool   saved_is_set_pos            = window->DC.IsSetPos;
#endif
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM >= 18400)
                const ImGuiLastItemData saved_last_item  = GImGui->LastItemData;
#else
                const ImGuiID              saved_last_id   = window->DC.LastItemId;
                const ImGuiItemStatusFlags saved_last_st   = window->DC.LastItemStatusFlags;
                const ImRect               saved_last_rect = window->DC.LastItemRect;
#endif
                const ImGui::Detail::LinkClickToggleState saved_toggle_state = ImGui::Detail::GetLinkClickToggleState();

                const MarkdownConfig* base_cfg = GetDefaultMarkdownConfig();
                MarkdownConfig nowrap_cfg = base_cfg ? *base_cfg : MarkdownConfig{};
                nowrap_cfg.wrapWidth = -1.0f;

                ImGui::SetCursorScreenPos(screen_pos);
                Markdown::TextUnformatted(md_text, &nowrap_cfg);

                window->DC.CursorPos              = saved_cursor_pos;
                window->DC.CursorPosPrevLine      = saved_cursor_pos_prev_line;
                window->DC.CursorMaxPos           = saved_cursor_max_pos;
                window->DC.CurrLineSize           = saved_curr_line_size;
                window->DC.PrevLineSize           = saved_prev_line_size;
                window->DC.CurrLineTextBaseOffset = saved_curr_line_base_offset;
                window->DC.PrevLineTextBaseOffset = saved_prev_line_base_offset;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                window->DC.IsSameLine             = saved_is_same_line;
                window->DC.IsSetPos               = saved_is_set_pos;
#endif
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM >= 18400)
                GImGui->LastItemData              = saved_last_item;
#else
                window->DC.LastItemId             = saved_last_id;
                window->DC.LastItemStatusFlags    = saved_last_st;
                window->DC.LastItemRect           = saved_last_rect;
#endif
                ImGui::Detail::GetLinkClickToggleState() = saved_toggle_state;
            }
        } // namespace Detail

        /// Renders an interactive Dear ImGui push button supporting procedural vector icons,
        /// pills, badges, rich Markdown formatting, Unicode glyphs, hand cursor on hover,
        /// and optional rich Markdown tooltips.
        ///
        /// Supported syntaxes in label:
        /// - Standalone Pills/Badges: "<pill orange> :retweet: </pill>", "<badge green> OK </badge>"
        /// - Rich Markdown text: "**Bold** Label", "<col red>Warning</col>", "<code>Code</code>"
        /// - XML tags: "<icon caret-up />", "<icon undo color=#ffaa00 />", "<icon gear size=16 />"
        /// - Shortcodes: ":undo:", ":gear: Settings", ":check: OK"
        /// - Unicode glyphs: "▲##up", "↺##reset", "⚙ Settings"
        /// - Steven's macros: "#[^](ICON_CARET_UP)"
        /// - Bare icon names: "ICON_FA_UNDO", "caret-up", "undo"
        inline bool Button(const std::string& label,
                           const ImVec2& size_arg = ImVec2(0.0f, 0.0f),
                           const std::string& tooltip_markdown = "",
                           float tooltip_wrap_width = 400.0f,
                           bool tooltip_allowed = true) {
            // Separate display part and ID part
            std::string display_part = label;
            std::string id_part = label;
            size_t id_pos_triple = label.find("###");
            if (id_pos_triple != std::string::npos) {
                display_part = label.substr(0, id_pos_triple);
                id_part = label.substr(id_pos_triple + 3);
            } else {
                size_t id_pos_double = label.find("##");
                if (id_pos_double != std::string::npos) {
                    display_part = label.substr(0, id_pos_double);
                    id_part = label.substr(id_pos_double + 2);
                }
            }

            std::string trimmed_display = display_part;
            while (!trimmed_display.empty() && (trimmed_display.front() == ' ' || trimmed_display.front() == '\t')) trimmed_display.erase(0, 1);
            while (!trimmed_display.empty() && (trimmed_display.back() == ' ' || trimmed_display.back() == '\t')) trimmed_display.pop_back();

            bool is_standalone_pill = (trimmed_display.rfind("<pill", 0) == 0 || trimmed_display.rfind("<badge", 0) == 0);
            bool clicked = false;
            ImGuiID btn_id = 0;
            bool btn_hovered = false;
            bool is_handled = false;

            if (is_standalone_pill) {
                ImGui::AlignTextToFramePadding();
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                const ImVec2 orig_screen_pos      = ImGui::GetCursorScreenPos();
                const ImVec2 saved_curr_line_size = window ? window->DC.CurrLineSize : ImVec2(0.0f, 0.0f);
                const float  saved_curr_base_off  = window ? window->DC.CurrLineTextBaseOffset : 0.0f;
                const ImGui::Detail::LinkClickToggleState saved_toggle_state = ImGui::Detail::GetLinkClickToggleState();

                const MarkdownConfig* base_cfg = GetDefaultMarkdownConfig();
                MarkdownConfig nowrap_cfg = base_cfg ? *base_cfg : MarkdownConfig{};
                nowrap_cfg.wrapWidth = -1.0f;
                Markdown::TextUnformatted(display_part, &nowrap_cfg);
                ImGui::Detail::GetLinkClickToggleState() = saved_toggle_state;

                ImVec2 item_sz = ImGui::GetItemRectSize();
                ImVec2 final_sz = size_arg;
                if (final_sz.x <= 0.0f) final_sz.x = item_sz.x;
                if (final_sz.y <= 0.0f) final_sz.y = ImMax(item_sz.y, ImGui::GetFrameHeight());

                ImGui::SetCursorScreenPos(orig_screen_pos);
                if (window) {
                    window->DC.CurrLineSize           = saved_curr_line_size;
                    window->DC.CurrLineTextBaseOffset = saved_curr_base_off;
                }
                std::string inv_id = "##" + (id_part.empty() ? "btn_md_pill" : id_part);
                clicked = ImGui::InvisibleButton(inv_id.c_str(), final_sz);
                btn_id = ImGui::GetItemID();
                btn_hovered = ImGui::IsItemHovered();
                if (btn_hovered) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
                is_handled = true;
            }

#if defined(CONTRIB_HAS_ICONS)
            if (!is_handled) {
                std::string preprocessed;
                bool had_preprocessing = PreprocessMarkdownIconsAndShortcodes(display_part.c_str(), display_part.length(), preprocessed);
                std::string target_display = had_preprocessing ? preprocessed : display_part;

                bool has_icon = false;
                std::string icon_name = "";
                float custom_icon_size = 0.0f;
                ImVec4 icon_color(0, 0, 0, 0);
                bool has_custom_color = false;
                std::string prefix_text = "";
                std::string suffix_text = "";

                size_t tag_start = target_display.find("<icon");
                if (tag_start != std::string::npos) {
                    size_t tag_end = target_display.find(">", tag_start);
                    if (tag_end != std::string::npos) {
                        prefix_text = target_display.substr(0, tag_start);
                        suffix_text = target_display.substr(tag_end + 1);

                        // Trim whitespace
                        while (!prefix_text.empty() && (prefix_text.front() == ' ' || prefix_text.front() == '\t')) prefix_text.erase(0, 1);
                        while (!prefix_text.empty() && (prefix_text.back() == ' ' || prefix_text.back() == '\t')) prefix_text.pop_back();
                        while (!suffix_text.empty() && (suffix_text.front() == ' ' || suffix_text.front() == '\t')) suffix_text.erase(0, 1);
                        while (!suffix_text.empty() && (suffix_text.back() == ' ' || suffix_text.back() == '\t')) suffix_text.pop_back();

                        // Use dedicated single-icon + label layout only when the icon is strictly leading or trailing
                        // and there is no second <icon> tag; otherwise fall through to full Markdown button layout.
                        if ((prefix_text.empty() || suffix_text.empty()) && suffix_text.find("<icon") == std::string::npos) {
                            has_icon = true;
                            std::string tag_body = target_display.substr(tag_start + 5, tag_end - (tag_start + 5));
                            if (!tag_body.empty() && tag_body.back() == '/') tag_body.pop_back();

                            std::istringstream iss(tag_body);
                            std::string token;
                            while (iss >> token) {
                                if (token.find("color=") == 0 || token.find("col=") == 0) {
                                    size_t eq = token.find('=');
                                    has_custom_color = ParseMarkdownColor(token.substr(eq + 1), icon_color);
                                } else if (token.find("name=") == 0 || token.find("icon=") == 0) {
                                    size_t eq = token.find('=');
                                    icon_name = NormalizeIconName(token.substr(eq + 1).c_str());
                                } else if (token.find("size=") == 0) {
                                    size_t eq = token.find('=');
                                    custom_icon_size = (float)atof(token.substr(eq + 1).c_str());
                                } else {
                                    std::string norm = NormalizeIconName(token.c_str());
                                    if (IsVectorIcon(norm.c_str()) && icon_name.empty()) {
                                        icon_name = norm;
                                    } else {
                                        ImVec4 test_c;
                                        if (ParseMarkdownColor(token, test_c) && !has_custom_color) {
                                            icon_color = test_c;
                                            has_custom_color = true;
                                        } else if (icon_name.empty()) {
                                            icon_name = norm;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    std::string norm = NormalizeIconName(display_part.c_str());
                    if (IsVectorIcon(norm.c_str())) {
                        has_icon = true;
                        icon_name = norm;
                    }
                }

                if (has_icon && !icon_name.empty()) {
                    ImGuiStyle& style = ImGui::GetStyle();
                    float frame_h = ImGui::GetFrameHeight();
                    float font_size = ImGui::GetFontSize();
                    float icon_sz = (custom_icon_size > 0.0f) ? custom_icon_size : std::round(font_size * 0.90f);

                    std::string label_text = prefix_text.empty() ? suffix_text : prefix_text;
                    bool has_text = !label_text.empty();
                    bool label_has_md = has_text && (label_text.find('<') != std::string::npos ||
                                                     label_text.find('*') != std::string::npos ||
                                                     label_text.find('_') != std::string::npos ||
                                                     label_text.find('`') != std::string::npos ||
                                                     label_text.find('~') != std::string::npos ||
                                                     label_text.find('+') != std::string::npos ||
                                                     label_text.find(':') != std::string::npos);
                    ImVec2 text_sz = !has_text ? ImVec2(0.0f, 0.0f)
                                               : (label_has_md ? CalcMarkdownSize(0, 0, label_text)
                                                               : ImGui::CalcTextSize(label_text.c_str()));
                    float spacing = has_text ? style.ItemInnerSpacing.x : 0.0f;
                    float content_w = has_text ? (icon_sz + spacing + text_sz.x) : icon_sz;

                    ImVec2 btn_sz = size_arg;
                    if (btn_sz.x <= 0.0f) {
                        btn_sz.x = has_text ? (content_w + style.FramePadding.x * 2.0f) : ((btn_sz.y > 0.0f) ? btn_sz.y : frame_h);
                    }
                    if (btn_sz.y <= 0.0f) {
                        btn_sz.y = (has_text && text_sz.y > ImGui::GetTextLineHeightWithSpacing() + 1.0f)
                                       ? ImMax(text_sz.y + style.FramePadding.y * 2.0f, frame_h)
                                       : ImMax(text_sz.y, frame_h);
                    }

                    std::string invisible_label = "###" + id_part;
                    clicked = ImGui::Button(invisible_label.c_str(), btn_sz);
                    btn_id = ImGui::GetItemID();
                    btn_hovered = ImGui::IsItemHovered();
                    if (btn_hovered) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    }

                    ImVec2 p_min = ImGui::GetItemRectMin();
                    ImVec2 p_max = ImGui::GetItemRectMax();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
                    ImU32 icol = has_custom_color ? ImGui::GetColorU32(icon_color) : text_col;

                    float start_x = std::round((p_min.x + p_max.x - content_w) * 0.5f);
                    if (!has_text) {
                        ImVec2 icon_pos(start_x, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
                        ImGui::DrawVectorIcon(draw_list, icon_pos, icon_sz, icol, icon_name.c_str());
                    } else if (prefix_text.empty()) {
                        ImVec2 icon_pos(start_x, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
                        ImGui::DrawVectorIcon(draw_list, icon_pos, icon_sz, icol, icon_name.c_str());
                        ImVec2 text_pos(start_x + icon_sz + spacing, std::round((p_min.y + p_max.y - text_sz.y) * 0.5f));
                        if (label_has_md) {
                            Detail::RenderMarkdownOverlayPreservingDC(text_pos, label_text);
                        } else {
                            draw_list->AddText(text_pos, text_col, label_text.c_str());
                        }
                    } else {
                        ImVec2 text_pos(start_x, std::round((p_min.y + p_max.y - text_sz.y) * 0.5f));
                        if (label_has_md) {
                            Detail::RenderMarkdownOverlayPreservingDC(text_pos, label_text);
                        } else {
                            draw_list->AddText(text_pos, text_col, label_text.c_str());
                        }
                        ImVec2 icon_pos(start_x + text_sz.x + spacing, std::round((p_min.y + p_max.y - icon_sz) * 0.5f));
                        ImGui::DrawVectorIcon(draw_list, icon_pos, icon_sz, icol, icon_name.c_str());
                    }
                    is_handled = true;
                }
            }
#endif // CONTRIB_HAS_ICONS

            if (!is_handled) {
                bool has_markdown_formatting = (display_part.find('<') != std::string::npos ||
                                                display_part.find('*') != std::string::npos ||
                                                display_part.find('_') != std::string::npos ||
                                                display_part.find('`') != std::string::npos ||
                                                display_part.find('~') != std::string::npos ||
                                                display_part.find('+') != std::string::npos ||
                                                display_part.find(':') != std::string::npos);

                if (has_markdown_formatting) {
                    ImGuiStyle& style = ImGui::GetStyle();
                    float frame_h = ImGui::GetFrameHeight();
                    ImVec2 measured = CalcMarkdownSize(0, 0, display_part);

                    ImVec2 btn_sz = size_arg;
                    if (btn_sz.x <= 0.0f) {
                        btn_sz.x = measured.x + style.FramePadding.x * 2.0f;
                    }
                    if (btn_sz.y <= 0.0f) {
                        btn_sz.y = (measured.y > ImGui::GetTextLineHeightWithSpacing() + 1.0f)
                                       ? ImMax(measured.y + style.FramePadding.y * 2.0f, frame_h)
                                       : ImMax(measured.y, frame_h);
                    }

                    std::string invisible_label = "###" + (id_part.empty() ? "btn_md" : id_part);
                    clicked = ImGui::Button(invisible_label.c_str(), btn_sz);
                    btn_id = ImGui::GetItemID();
                    btn_hovered = ImGui::IsItemHovered();
                    if (btn_hovered) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    }

                    ImVec2 p_min = ImGui::GetItemRectMin();
                    float offset_x = std::round(ImMax(0.0f, (btn_sz.x - measured.x) * 0.5f));
                    float offset_y = std::round(ImMax(0.0f, (btn_sz.y - measured.y) * 0.5f));

                    Detail::RenderMarkdownOverlayPreservingDC(ImVec2(p_min.x + offset_x, p_min.y + offset_y), display_part);
                    is_handled = true;
                }
            }

            if (!is_handled) {
                clicked = ImGui::Button(label.c_str(), size_arg);
                btn_id = ImGui::GetItemID();
                btn_hovered = ImGui::IsItemHovered();
                if (btn_hovered) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
            }

            if (!tooltip_markdown.empty()) {
                MarkdownDetail::ShowMarkdownTooltip(btn_id, tooltip_allowed && btn_hovered, tooltip_markdown.c_str(), tooltip_wrap_width, -1, clicked);
            }
            
            return clicked;
        }

        inline bool Button(const std::string& label,
                           const std::string& tooltip_markdown,
                           float tooltip_wrap_width = 400.0f,
                           bool tooltip_allowed = true,
                           const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
            return Button(label, size_arg, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
        }

        inline bool Button(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
            return Button(std::string(label ? label : ""), size);
        }

        inline bool Button(const char* label,
                           const char* tooltip_markdown,
                           float tooltip_wrap_width = 400.0f,
                           bool tooltip_allowed = true,
                           const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
            return Button(std::string(label ? label : ""),
                          size_arg,
                          std::string(tooltip_markdown ? tooltip_markdown : ""),
                          tooltip_wrap_width,
                          tooltip_allowed);
        }

        // Backward-compatibility and convenience aliases inside Markdown namespace:
        inline bool ButtonMD(const std::string& label,
                             const ImVec2& size = ImVec2(0.0f, 0.0f),
                             const std::string& tooltip_markdown = "",
                             float tooltip_wrap_width = 400.0f,
                             bool tooltip_allowed = true) {
            return Button(label, size, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
        }

        inline bool ButtonMD(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
            return Button(std::string(label ? label : ""), size);
        }

        inline bool ButtonMD(const char* label,
                             const char* tooltip_markdown,
                             float tooltip_wrap_width = 400.0f,
                             bool tooltip_allowed = true,
                             const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
            return Button(label, tooltip_markdown, tooltip_wrap_width, tooltip_allowed, size_arg);
        }

        inline bool ButtonMD(const std::string& label,
                             const std::string& tooltip_markdown,
                             float tooltip_wrap_width = 400.0f,
                             bool tooltip_allowed = true,
                             const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
            return Button(label, size_arg, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
        }

        inline bool DelayedTooltipMD(const std::string& markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
            if (!is_allowed || markdown_text.empty()) {
                return false;
            }
            ImGuiID raw_id = ImGui::GetItemID();
            ImGuiHoveredFlags hov_flags = ImGuiHoveredFlags_None;
            if (GImGui && GImGui->CurrentWindow && GImGui->ActiveId != 0 && GImGui->ActiveId == GImGui->CurrentWindow->MoveId) {
                hov_flags |= ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
            }
            bool hovered = ImGui::IsItemHovered(hov_flags);
            return MarkdownDetail::ShowMarkdownTooltip(raw_id, hovered, markdown_text.c_str(), wrap_width, delay_frames, ImGui::IsItemClicked());
        }

        inline bool DelayedTooltipMD(const char* markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
            if (!is_allowed || !markdown_text || markdown_text[0] == '\0') {
                return false;
            }
            ImGuiID raw_id = ImGui::GetItemID();
            ImGuiHoveredFlags hov_flags = ImGuiHoveredFlags_None;
            if (GImGui && GImGui->CurrentWindow && GImGui->ActiveId != 0 && GImGui->ActiveId == GImGui->CurrentWindow->MoveId) {
                hov_flags |= ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
            }
            bool hovered = ImGui::IsItemHovered(hov_flags);
            return MarkdownDetail::ShowMarkdownTooltip(raw_id, hovered, markdown_text, wrap_width, delay_frames, ImGui::IsItemClicked());
        }

        inline bool DelayedTooltipMD(ImGuiID id, const char* markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
            if (!is_allowed || !markdown_text || markdown_text[0] == '\0') {
                return false;
            }
            ImGuiHoveredFlags hov_flags = ImGuiHoveredFlags_None;
            if (GImGui && GImGui->CurrentWindow && GImGui->ActiveId != 0 && GImGui->ActiveId == GImGui->CurrentWindow->MoveId) {
                hov_flags |= ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
            }
            bool hovered = ImGui::IsItemHovered(hov_flags);
            return MarkdownDetail::ShowMarkdownTooltip(id, hovered, markdown_text, wrap_width, delay_frames, ImGui::IsItemClicked());
        }

    } // namespace Markdown

    // Canonical namespace alias: ImGui::MD is identical to ImGui::Markdown
    namespace MD = Markdown;

#if !defined(CONTRIB_DISABLE_TEXT_MD) && !defined(IMGUI_DISABLE_TEXT_MD) && !defined(CONTRIB_DISABLE_MARKDOWN_ROOT_ALIASES)
    // Top-level convenience aliases (matching ImGui::Text and ImGui::TextWrapped):
    inline void TextMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::Text(markdown_text, config_override);
    }

    inline void TextMD(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextUnformatted(markdown_text, nullptr, config_override);
    }

#if !defined(CONTRIB_DISABLE_TEXT_WRAPPED_MD) && !defined(IMGUI_DISABLE_TEXT_WRAPPED_MD) && !defined(CONTRIB_DISABLE_TEXTWRAPPEDMD)
    inline void TextWrappedMD(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(markdown_text, config_override);
    }

    inline void TextWrappedMD(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(std::string(markdown_text ? markdown_text : ""), config_override);
    }
#endif

    // Top-level convenience alias for clickable Markdown text links:
    inline LinkClickResult TextMDLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        return Markdown::TextLink(markdown_text, config_override);
    }

    // Full-name root convenience aliases:
    inline void TextMarkdown(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::Text(markdown_text, config_override);
    }

    inline void TextMarkdown(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextUnformatted(markdown_text, nullptr, config_override);
    }

    inline void TextWrappedMarkdown(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(markdown_text, config_override);
    }

    inline void TextWrappedMarkdown(const char* markdown_text, const MarkdownConfig* config_override = nullptr) {
        Markdown::TextWrapped(std::string(markdown_text ? markdown_text : ""), config_override);
    }

    inline LinkClickResult TextMarkdownLink(const std::string& markdown_text, const MarkdownConfig* config_override = nullptr) {
        return Markdown::TextLink(markdown_text, config_override);
    }

    // Top-level convenience aliases for layout calculation:
    inline ImVec2 CalcMarkdownSize(const std::string& markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
        return Markdown::CalcMarkdownSize(markdown_text, wrap_width, font_scale, md_config);
    }

    inline ImVec2 CalcMarkdownSize(const char* markdown_text, float wrap_width = 0.0f, float font_scale = 0.0f, const MarkdownConfig* md_config = nullptr) {
        return Markdown::CalcMarkdownSize(markdown_text, wrap_width, font_scale, md_config);
    }
#endif // !CONTRIB_DISABLE_TEXT_MD

#if !defined(CONTRIB_DISABLE_BUTTON_MD) && !defined(IMGUI_DISABLE_BUTTON_MD) && !defined(CONTRIB_DISABLE_MARKDOWN_ROOT_ALIASES)
    // Top-level convenience aliases for interactive Markdown buttons:
    inline bool ButtonMD(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, size);
    }

    inline bool ButtonMD(const char* label,
                         const char* tooltip_markdown,
                         float tooltip_wrap_width = 400.0f,
                         bool tooltip_allowed = true,
                         const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, tooltip_markdown, tooltip_wrap_width, tooltip_allowed, size_arg);
    }

    inline bool ButtonMD(const std::string& label,
                         const ImVec2& size = ImVec2(0.0f, 0.0f),
                         const std::string& tooltip_markdown = "",
                         float tooltip_wrap_width = 400.0f,
                         bool tooltip_allowed = true) {
        return Markdown::Button(label, size, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
    }

    inline bool ButtonMD(const std::string& label,
                         const std::string& tooltip_markdown,
                         float tooltip_wrap_width = 400.0f,
                         bool tooltip_allowed = true,
                         const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, size_arg, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
    }

    inline bool ButtonMarkdown(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, size);
    }

    inline bool ButtonMarkdown(const char* label,
                               const char* tooltip_markdown,
                               float tooltip_wrap_width = 400.0f,
                               bool tooltip_allowed = true,
                               const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, tooltip_markdown, tooltip_wrap_width, tooltip_allowed, size_arg);
    }

    inline bool ButtonMarkdown(const std::string& label,
                               const ImVec2& size = ImVec2(0.0f, 0.0f),
                               const std::string& tooltip_markdown = "",
                               float tooltip_wrap_width = 400.0f,
                               bool tooltip_allowed = true) {
        return Markdown::Button(label, size, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
    }

    inline bool ButtonMarkdown(const std::string& label,
                               const std::string& tooltip_markdown,
                               float tooltip_wrap_width = 400.0f,
                               bool tooltip_allowed = true,
                               const ImVec2& size_arg = ImVec2(0.0f, 0.0f)) {
        return Markdown::Button(label, size_arg, tooltip_markdown, tooltip_wrap_width, tooltip_allowed);
    }
#endif // !CONTRIB_DISABLE_BUTTON_MD

#if !defined(CONTRIB_DISABLE_DELAYED_TOOLTIP_MD) && !defined(IMGUI_DISABLE_DELAYED_TOOLTIP_MD) && !defined(CONTRIB_DISABLE_MARKDOWN_ROOT_ALIASES)
    inline bool DelayedTooltipMD(const std::string& markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
        return Markdown::DelayedTooltipMD(markdown_text, wrap_width, delay_frames, is_allowed);
    }

    inline bool DelayedTooltipMD(const char* markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
        return Markdown::DelayedTooltipMD(markdown_text, wrap_width, delay_frames, is_allowed);
    }

    inline bool DelayedTooltipMD(ImGuiID id, const char* markdown_text, float wrap_width = 400.0f, int delay_frames = 30, bool is_allowed = true) {
        return Markdown::DelayedTooltipMD(id, markdown_text, wrap_width, delay_frames, is_allowed);
    }
#endif

    namespace MarkdownDetail {
        struct InlineCaptionMarkdownBridgeRunner {
            InlineCaptionMarkdownBridgeRunner() {
                GetMeasureInlineWidthFn() = &MeasureMarkdownLineWidth;
                GetRenderInlineCaptionFn() = [](const char* text, size_t len, const MarkdownConfig* cfg) {
                    MarkdownExt(text, len, cfg, false);
                };
            }
        };
        static InlineCaptionMarkdownBridgeRunner s_InlineCaptionMarkdownBridgeRunner;
    }

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#define IMGUI_MARKDOWN_EXT_READY 1

#endif // IMGUI_MARKDOWN_EXT_H

// Order-independent StyledTextWrapped <-> MarkdownExt bridge:
#if defined(IMGUI_MARKDOWN_EXT_READY) && defined(IMGUI_STYLED_TEXT_READY) && !defined(IMGUI_STYLED_TEXT_MD_BRIDGE_DEFINED)
#define IMGUI_STYLED_TEXT_MD_BRIDGE_DEFINED
namespace ImGui {
namespace Detail {
    struct StyledTextMarkdownBridgeRunner {
        StyledTextMarkdownBridgeRunner() {
            GetStyledTextMarkdownRenderer() = [](const char* text, size_t len) {
                ImGui::MarkdownExt(text, len, nullptr);
            };
        }
    };
    static StyledTextMarkdownBridgeRunner s_StyledTextMarkdownBridgeRunner;
} // namespace Detail
} // namespace ImGui
#endif

// Order-independent TimedTooltip <-> Markdown bridge:
// Activated automatically regardless of whether imgui_tooltip_ext.h or
// imgui_markdown_ext.h was #included first!
#if defined(IMGUI_MARKDOWN_EXT_READY) && defined(IMGUI_TOOLTIP_EXT_H) && !defined(IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED)
#define IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

namespace ImGui {
namespace TimedTooltip {

    inline bool TextMDProxyV(ImGuiID id, bool is_hovered, const TooltipConfig* config_override, const char* fmt, va_list args, bool is_clicked = false, bool toggle_on_click = false) {
        ImGuiMouseCursor previous_cursor = ImGui::GetMouseCursor();
        
        // BRUTE FORCE CURSOR: If the widget is hovered but the cursor is still Arrow, it means the widget's 
        // internal IsItemHovered() check failed or ran out of order. Since this is an interactive tooltippable 
        // widget, we violently force the Hand cursor!
        if (is_hovered && previous_cursor == ImGuiMouseCursor_Arrow) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            previous_cursor = ImGuiMouseCursor_Hand;
        }
        
        if (id == 0) {
            // When id == 0 (e.g. after EndGroup() or custom layout), derive a unique ID
            // so multiple rows in loops calling DelayedTooltip("%s", text) do not collide!
            if (fmt && args) {
                va_list args_id;
                va_copy(args_id, args);
                char id_buf[256];
                vsnprintf(id_buf, sizeof(id_buf), fmt, args_id);
                va_end(args_id);
                if (id_buf[0] != '\0') {
                    id = ImGui::GetID(id_buf);
                }
            }
            if (id == 0) {
                // Secondary fallback: use screen position of current/last item
                ImVec2 item_min = ImGui::GetItemRectMin();
                int coord_key = (int)item_min.x ^ ((int)item_min.y << 16);
                id = ImGui::GetID(coord_key != 0 ? &coord_key : (const void*)(fmt ? fmt : "##timed_tt_anon_md"));
            }
        }

        HandleTooltipClick(id, is_clicked, toggle_on_click);
        
        char buffer[4096];
        vsnprintf(buffer, sizeof(buffer), fmt, args);

        const TooltipConfig* config = config_override ? config_override : GetDefaultTooltipConfig();
        static const TooltipConfig fallback_config;
        if (!config) config = &fallback_config;
        
        TooltipState& state = GetTooltipStateMap()[id];
        
        float scale = TimedTooltip::GetActiveFontScale(config);
        if (scale <= 0.0f) scale = 1.0f;
        
        // 1. Audit and sanitize wrap constraints against screen boundaries
        float max_screen_w = ImMax(100.0f, ImGui::GetIO().DisplaySize.x - 40.0f);
        bool no_wrap = (config->default_wrap_width < 0.0f);
        float wrap_limit = max_screen_w;
        if (!no_wrap) {
            float min_w = config->min_wrap_width * scale;
            float max_w = config->max_wrap_width * scale;
            if (min_w < 50.0f * scale) min_w = 50.0f * scale;
            if (max_w < min_w) max_w = min_w;
            if (max_w > max_screen_w) max_w = max_screen_w;
            if (min_w > max_w) min_w = max_w;

            float target_w = (config->default_wrap_width > 0.0f) ? (config->default_wrap_width * scale) : (360.0f * scale);
            wrap_limit = ImClamp(target_w, min_w, max_w);
            if (wrap_limit > max_screen_w) wrap_limit = max_screen_w;
        }

        // Prepare MarkdownConfig with custom bullet_spacing
        MarkdownConfig local_md;
        const MarkdownConfig* base_md = GetDefaultMarkdownConfig();
        if (base_md) local_md = *base_md;
        if (config->bullet_spacing > 0.0f) local_md.bulletSpacing = config->bullet_spacing * scale;
        if (no_wrap) local_md.wrapWidth = -1.0f;

        const bool scale_changed = (std::abs(scale - state.last_scale) > 0.001f);
        if (scale_changed) {
            state.perfect_size = ImVec2(0.0f, 0.0f);
            state.last_size = ImVec2(0.0f, 0.0f);
            state.last_scale = scale;
        }

        const bool active_or_hovered = (is_hovered || state.is_displayed || state.hover_cycles > 0);
        if (active_or_hovered) {
            ImGuiID cur_text_hash = ImHashStr(buffer);
            if (cur_text_hash != state.last_text_hash || scale_changed) {
                state.last_text_hash = cur_text_hash;
                state.perfect_size = ImVec2(0.0f, 0.0f);
                state.last_size = ImVec2(0.0f, 0.0f);
            }
        }

        if (state.perfect_size.x == 0.0f && active_or_hovered) {
            ImFont* isolated_font = TimedTooltip::Detail::ResolveBaseTooltipFont(config);
            if (isolated_font) {
                ImGui::ImGeeCompat::PushFont(isolated_font);
            }

            // Pass 1: Dry-run unwrapped to measure the natural single-line width
            MarkdownConfig measure_md = local_md;
            measure_md.wrapWidth = -1.0f; // Force unconstrained single-line layout
            ImVec2 raw_size = MD::CalcMarkdownSize(id, 1, std::string(buffer), 0.0f, scale, &measure_md);
            if (raw_size.x <= 0.0f || raw_size.y <= 0.0f) {
                ImVec2 fb = ImGui::CalcTextSize(buffer);
                raw_size.x = fb.x * scale;
                raw_size.y = fb.y * scale;
            }
            
            if (no_wrap || raw_size.x <= wrap_limit) {
                // Short or non-wrapping content: Shrink-wrap perfectly with extra clearance so rightmost glyphs never clip!
                state.perfect_size.x = ceilf(raw_size.x) + 4.0f * scale;
                if (state.perfect_size.x > max_screen_w) state.perfect_size.x = max_screen_w;
                float line_h = raw_size.y;
                if (strstr(buffer, "<pill") || strstr(buffer, "<badge")) {
                    float pill_base_sz = TimedTooltip::Detail::GetFontNativePixelSize(isolated_font);
                    if (pill_base_sz <= 0.0f) pill_base_sz = (ImGui::GetFontSize() > 0.0f ? ImGui::GetFontSize() : 14.0f);
                    line_h = ImMax(line_h, pill_base_sz * scale + 4.0f * scale);
                }
                state.perfect_size.y = line_h + 2.0f * scale; // Symmetric 1px top + 1px bottom headroom
                local_md.wrapWidth = no_wrap ? -1.0f : wrap_limit;
            } else {
                // Content exceeds wrap_limit: Do Pass 2 wrapped at the STRICT wrap_limit!
                local_md.wrapWidth = wrap_limit;
                ImVec2 wrapped_size = MD::CalcMarkdownSize(id, 2, std::string(buffer), wrap_limit, scale, &local_md);
                if (wrapped_size.y <= 0.0f) {
                    ImVec2 fb = ImGui::CalcTextSize(buffer, nullptr, false, wrap_limit);
                    wrapped_size.x = fb.x * scale;
                    wrapped_size.y = fb.y * scale;
                }
                float content_w = ceilf(wrapped_size.x) + 4.0f * scale;
                if (content_w < wrap_limit + 4.0f * scale) {
                    content_w = wrap_limit + 4.0f * scale;
                }
                if (content_w > max_screen_w) content_w = max_screen_w;
                state.perfect_size.x = content_w;
                state.perfect_size.y = wrapped_size.y + 2.0f * scale; // Symmetric 1px top + 1px bottom headroom
            }

            if (isolated_font) {
                ImGui::PopFont();
            }
        } else {
            local_md.wrapWidth = no_wrap ? -1.0f : wrap_limit;
        }

        bool displayed = false;
        if (BeginStationaryTooltipProxy(id, is_hovered, config_override)) {
            displayed = true;

            ImGui::ImGeeCompat::SetWindowFontScale(scale);
            if (no_wrap) {
                MD::Text(std::string(buffer), &local_md);
            } else {
                MD::TextWrapped(std::string(buffer), &local_md);
            }
            EndStationaryTooltip(id);
        }
        
        // Violently restore the underlying widget's cursor if ImGui's Tooltip engine tried to defeat it!
        if (previous_cursor != ImGui::GetMouseCursor()) {
            ImGui::SetMouseCursor(previous_cursor);
        }
        return displayed;
    }

    inline bool TextMDV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool toggle = ImGui::Detail::ShouldToggleTooltipOnClick(raw_id, ImGui::IsItemActive());
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, Detail::IsTooltipTargetItemHovered());
        return TextMDProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), toggle);
    }

    inline bool TextMDLinkV(const TooltipConfig* config_override, const char* fmt, va_list args) {
        ImGuiID raw_id = ImGui::GetItemID();
        bool hovered = ImGui::Detail::ResolveItemTooltipHover(raw_id, Detail::IsTooltipTargetItemHovered());
        return TextMDProxyV(raw_id, hovered, config_override, fmt, args, ImGui::IsItemClicked(), true);
    }

    // Proxy Timed Tooltips with Markdown (accept explicit ID and explicit is_hovered state):
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(4, 5);
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, config, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const char* fmt, ...) IMGUI_PRINTFARGS(3, 4);
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(const char* str_id, bool is_hovered, const char* fmt, ...) IMGUI_PRINTFARGS(3, 4);
    inline bool TextMDProxy(const char* str_id, bool is_hovered, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(ImGui::GetID(str_id), is_hovered, nullptr, fmt, args, ImGui::IsItemClicked());
        va_end(args);
        return res;
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDProxy(id, is_hovered, config, "%s", markdown_text.c_str());
    }

    inline bool TextMDProxy(const char* str_id, bool is_hovered, const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDProxy(ImGui::GetID(str_id), is_hovered, config, "%s", markdown_text.c_str());
    }

    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, bool is_clicked, bool toggle_on_click, const char* fmt, ...) IMGUI_PRINTFARGS(6, 7);
    inline bool TextMDProxy(ImGuiID id, bool is_hovered, const TooltipConfig* config, bool is_clicked, bool toggle_on_click, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDProxyV(id, is_hovered, config, fmt, args, is_clicked, toggle_on_click);
        va_end(args);
        return res;
    }

    // Markdown-enabled Timed Tooltip with per-call config override
    inline bool TextMD(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
    inline bool TextMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(config, fmt, args);
        va_end(args);
        return res;
    }

    // Markdown-enabled Timed Tooltip: mirrors standard ImGui vocabulary (ImGui::TimedTooltip::TextMD)
    // Automatically binds to the last drawn widget using ImGui::GetItemID()
    // Defers string formatting until the tooltip actually appears to save CPU!
    inline bool TextMD(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
    inline bool TextMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMD(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMD(config, "%s", markdown_text.c_str());
    }

    // Non-wrapping Unformatted Markdown Timed Tooltip (never auto-wraps; single line per '\n'):
    inline bool TextMDUnformatted(const char* markdown_text, const TooltipConfig* config = nullptr) {
        if (!markdown_text) return false;
        TooltipConfig local_cfg = config ? *config : GetCurrentTooltipConfig();
        local_cfg.default_wrap_width = -1.0f;
        return TextMD(&local_cfg, "%s", markdown_text);
    }

    inline bool TextMDUnformatted(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDUnformatted(markdown_text.c_str(), config);
    }

    // Explicit click-to-toggle Markdown Timed Tooltip chasers (ImGui::TimedTooltip::TextMDLink):
    inline bool TextMDLink(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
    inline bool TextMDLink(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDLinkV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDLink(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
    inline bool TextMDLink(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDLinkV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TextMDLink(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
        return TextMDLink(config, "%s", markdown_text.c_str());
    }

    // Direct aliases for backward-compatibility
    inline bool TimedTooltipMD(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
    inline bool TimedTooltipMD(const TooltipConfig* config, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(config, fmt, args);
        va_end(args);
        return res;
    }

    inline bool TimedTooltipMD(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
    inline bool TimedTooltipMD(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool res = TextMDV(nullptr, fmt, args);
        va_end(args);
        return res;
    }

    // Bidirectional Channel A: ImGui::TimedTooltip::Markdown:: & ImGui::TimedTooltip::MD::
    namespace Markdown {
        inline bool Text(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool Text(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool Text(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMD(config, "%s", markdown_text.c_str());
        }

        inline bool TextUnformatted(const char* markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDUnformatted(markdown_text, config);
        }

        inline bool TextUnformatted(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDUnformatted(markdown_text, config);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline bool TextLink(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool TextLink(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool TextLink(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDLink(config, "%s", markdown_text.c_str());
        }

        inline bool TextLinkContrib(const TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool TextLinkContrib(const TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool TextLinkContrib(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const std::string& markdown_text, const TooltipConfig* config = nullptr) {
            return TextMDLink(config, "%s", markdown_text.c_str());
        }
#pragma pop_macro("TextLink")
    } // namespace Markdown

    namespace MD = Markdown;

} // namespace TimedTooltip

namespace Markdown {
    // Bidirectional Channel B: ImGui::Markdown::TimedTooltip:: & ImGui::MD::TimedTooltip::
    namespace TimedTooltip {
        inline bool Text(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool Text(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool Text(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool Text(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMD(config, "%s", markdown_text.c_str());
        }

        inline bool TextUnformatted(const char* markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDUnformatted(markdown_text, config);
        }

        inline bool TextUnformatted(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDUnformatted(markdown_text, config);
        }

#pragma push_macro("TextLink")
#undef TextLink
        inline bool TextLink(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool TextLink(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool TextLink(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLink(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDLink(config, "%s", markdown_text.c_str());
        }

        inline bool TextLinkContrib(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) IMGUI_PRINTFARGS(2, 3);
        inline bool TextLinkContrib(const ImGui::TimedTooltip::TooltipConfig* config, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(config, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const char* fmt, ...) IMGUI_PRINTFARGS(1, 2);
        inline bool TextLinkContrib(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            bool res = ImGui::TimedTooltip::TextMDLinkV(nullptr, fmt, args);
            va_end(args);
            return res;
        }

        inline bool TextLinkContrib(const std::string& markdown_text, const ImGui::TimedTooltip::TooltipConfig* config = nullptr) {
            return ImGui::TimedTooltip::TextMDLink(config, "%s", markdown_text.c_str());
        }
#pragma pop_macro("TextLink")
    } // namespace TimedTooltip
} // namespace Markdown

namespace MarkdownDetail {
    inline void InitializeCodeBlockTooltipBridge() {
        GetCodeBlockTooltipFn() = []( ImGuiID id, bool hovered, const char* text, bool toggle_on_click ) {
            if (toggle_on_click) {
                ImGui::Detail::RegisterItemTooltipToggle( true );
            }
            ImGui::TimedTooltip::TextProxyToggle( id, hovered, toggle_on_click, "%s", text );
        };
        GetClearTooltipFn() = []() {
            ImGui::TimedTooltip::ClearActiveTooltip();
        };
        GetDisabledCopyTooltipFn() = []( ImGuiID id, bool hovered ) {
#if (defined(APL) && APL) || defined(__APPLE__)
            ImGui::TimedTooltip::TextProxyToggle( id, hovered, false, "macOS system clipboard integration is disabled.\n\nTo enable:\n* Add #define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS to imconfig.h\n* Link -framework ApplicationServices in your build flags\n* Or provide MarkdownConfig::setClipboardTextFn" );
#elif (defined(LIN) && LIN) || defined(__linux__)
            ImGui::TimedTooltip::TextProxyToggle( id, hovered, false, "Linux system clipboard integration requires a handler.\n\nTo enable:\n* Provide MarkdownConfig::setClipboardTextFn callback\n* Or assign ImGui::GetPlatformIO().Platform_SetClipboardTextFn" );
#else
            ImGui::TimedTooltip::TextProxyToggle( id, hovered, false, "System clipboard integration is not enabled on this platform build." );
#endif
        };
        GetMarkdownTooltipFn() = []( ImGuiID id, bool hovered, const char* text, float wrap_width, int delay_frames, bool is_clicked ) -> bool {
            if (!text || text[0] == '\0') return false;
            TimedTooltip::TooltipConfig cfg = TimedTooltip::GetCurrentTooltipConfig();
            if (wrap_width > 0.0f) {
                cfg.default_wrap_width = wrap_width;
            } else if (wrap_width < 0.0f) {
                cfg.default_wrap_width = -1.0f;
            }
            if (delay_frames >= 0) {
                cfg.hover_delay_cycles = delay_frames;
            }
            bool toggle = ImGui::Detail::ShouldToggleTooltipOnClick(id, ImGui::IsItemActive());
            bool resolved_hovered = ImGui::Detail::ResolveItemTooltipHover(id, hovered);
            return TimedTooltip::TextMDProxy(id, resolved_hovered, &cfg, is_clicked, toggle, "%s", text);
        };
        GetTableTooltipFn() = []( ImGuiID id, bool hovered, const char* markdownText, bool is_clicked ) {
            if (!markdownText || markdownText[0] == '\0') return;
            TimedTooltip::TextMDProxy( id, hovered, nullptr, is_clicked, false, "%s", markdownText );
        };
    }
    struct CodeBlockTooltipBridgeRunner {
        CodeBlockTooltipBridgeRunner() {
            InitializeCodeBlockTooltipBridge();
#if defined(CONTRIB_ICONS_AVAILABLE)
            ImGui::Detail::GetIconTooltipHandler() = [](const char* tooltip) {
                if (tooltip && *tooltip) {
                    ImGui::TimedTooltip::TextMD("%s", tooltip);
                }
            };
#endif
        }
    };
    static CodeBlockTooltipBridgeRunner s_CodeBlockTooltipBridgeRunner;
} // namespace MarkdownDetail

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_MARKDOWN_EXT_READY && IMGUI_TOOLTIP_EXT_H

#endif // !IMGUI_DISABLE_MARKDOWN

