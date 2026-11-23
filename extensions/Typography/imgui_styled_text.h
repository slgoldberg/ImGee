// ============================================================================
// ImGee Typography: Inline Styled Text Engine (extensions/Typography/imgui_styled_text.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg)
// License: BSD 3-Clause (see LICENSE)
//
// Provides lightweight, inline rich text rendering without requiring full
// Markdown document overhead. Supports bold, italic, monospace, headings,
// colors, badges, pills, keyboard keys, and named images with continuous
// baseline alignment.
// ============================================================================

#pragma once

#if !defined(IMGEE_DISABLE_EXTENSIONS) && \
    !defined(CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_CONTRIB_DISABLE_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGUI_DISABLE_EXTENSIONS) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_EXTENSIONS) && \
    !defined(IMGEE_DISABLE_TYPOGRAPHY) && \
    !defined(CONTRIB_DISABLE_TYPOGRAPHY) && \
    !defined(IMGUI_CONTRIB_DISABLE_TYPOGRAPHY) && \
    !defined(IMGUI_DISABLE_CONTRIB_TYPOGRAPHY) && \
    !defined(IMGUI_DISABLE_TYPOGRAPHY) && \
    !defined(IMGWINDOW_DISABLE_CONTRIB_TYPOGRAPHY)
#ifndef IMGUI_STYLED_TEXT_H
#define IMGUI_STYLED_TEXT_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include "imgui.h"
#include "imgui_font_roles.h"
#include <string>
#include <vector>
#include <sstream>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cmath>

#if defined(__has_include)
    #if __has_include("../../imgee_host_backend.h")
        #include "../../imgee_host_backend.h"
    #elif __has_include("imgee_host_backend.h")
        #include "imgee_host_backend.h"
    #endif
#else
    #include "../../imgee_host_backend.h"
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
        #define IMGUI_STYLED_TEXT_HAS_BADGES 1
    #endif
#else
    #include "../../widgets/imgui_badges.h"
    #define IMGUI_STYLED_TEXT_HAS_BADGES 1
#endif
#endif

#if !defined(IMGEE_DISABLE_ICONS) && \
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
        #define IMGUI_STYLED_TEXT_HAS_ICONS 1
    #endif
#else
    #include "../../widgets/imgui_icons.h"
    #define IMGUI_STYLED_TEXT_HAS_ICONS 1
#endif
#endif

namespace ImGui {

#ifndef IMGUI_IMAGE_NAMED_DECLARED
#define IMGUI_IMAGE_NAMED_DECLARED
#if !defined(IMGEE_NO_IMGWINDOW_BRIDGE) && (defined(IMGWINDOW_H) || (defined(__has_include) && __has_include("ImgWindow.h")))
    // Forward declare ImageNamed (implemented in ImgWindow.cpp when bridged to ImgWindow)
    IMGUI_API void ImageNamed(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f), const ImVec2& uv0 = ImVec2(0.0f, 0.0f), const ImVec2& uv1 = ImVec2(1.0f, 1.0f), const ImVec4& tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec4& border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
#else
    // Standalone header-only ImageNamed backed by ImGee::HostBackend and ImGui::SetNamedTextureResolver
    inline void ImageNamed(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f), const ImVec2& uv0 = ImVec2(0.0f, 0.0f), const ImVec2& uv1 = ImVec2(1.0f, 1.0f), const ImVec4& tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec4& border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)) {
        if (!name || !*name) return;
        ImVec2 nativeSize(0.0f, 0.0f);
        ImTextureID tex = (ImTextureID)0;
#if defined(IMGEE_HOST_BACKEND_H)
        const ImGee::HostBackend& backend = ImGee::GetHostBackend();
        if (backend.ResolveNamedTexture) {
            tex = backend.ResolveNamedTexture(name, &nativeSize, backend.UserData);
        }
        if (!tex && Detail::GetNamedTextureResolver()) {
            tex = Detail::GetNamedTextureResolver()(name, &nativeSize);
        }
#endif
        if (!tex) return;

        ImVec2 renderSize = size;
        if (renderSize.x <= 0.0f && renderSize.y <= 0.0f) {
            renderSize = nativeSize;
        } else if (renderSize.x <= 0.0f && nativeSize.y > 0.0f) {
            renderSize.x = std::round(renderSize.y * (nativeSize.x / nativeSize.y));
        } else if (renderSize.y <= 0.0f && nativeSize.x > 0.0f) {
            renderSize.y = std::round(renderSize.x * (nativeSize.y / nativeSize.x));
        }
        if (renderSize.x <= 0.0f || renderSize.y <= 0.0f) return;

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19190)
        if (border_col.w > 0.0f) {
            const float border_size = ImGui::GetStyle().ImageBorderSize;
            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, border_size > 1.0f ? border_size : 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, border_col);
            ImGui::ImageWithBg(tex, renderSize, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0.0f);
            ImGui::ImageWithBg(tex, renderSize, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
            ImGui::PopStyleVar();
        }
#else
        ImGui::Image(tex, renderSize, uv0, uv1, tint_col, border_col);
#endif
    }
#endif
#endif

    namespace Detail {

        // Helper to parse hex colors: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
        inline bool ParseHexColor(const char* s, size_t len, ImU32& outColor) {
            if (len == 0) return false;
            if (s[0] == '#') { s++; len--; }
            if (len != 3 && len != 4 && len != 6 && len != 8) return false;

            char buf[9] = { 0 };
            std::memcpy(buf, s, len);
            char* end = nullptr;
            unsigned long val = std::strtoul(buf, &end, 16);
            if (end != buf + len) return false;

            unsigned int r = 255, g = 255, b = 255, a = 255;
            if (len == 3) {
                r = ((val >> 8) & 0xF) * 17;
                g = ((val >> 4) & 0xF) * 17;
                b = (val & 0xF) * 17;
            } else if (len == 4) {
                r = ((val >> 12) & 0xF) * 17;
                g = ((val >> 8) & 0xF) * 17;
                b = ((val >> 4) & 0xF) * 17;
                a = (val & 0xF) * 17;
            } else if (len == 6) {
                r = (val >> 16) & 0xFF;
                g = (val >> 8) & 0xFF;
                b = val & 0xFF;
            } else if (len == 8) {
                r = (val >> 24) & 0xFF;
                g = (val >> 16) & 0xFF;
                b = (val >> 8) & 0xFF;
                a = val & 0xFF;
            }
            outColor = IM_COL32(r, g, b, a);
            return true;
        }

        // Helper to parse color names or hex strings
        inline bool ParseColor(const char* s, size_t len, ImU32& outColor) {
            if (len == 0) return false;
            if (s[0] == '#') return ParseHexColor(s, len, outColor);

            std::string name(s, len);
            for (auto& c : name) c = (char)std::tolower(c);

            if (name == "red")          outColor = IM_COL32(231, 76, 60, 255);
            else if (name == "green")   outColor = IM_COL32(46, 204, 113, 255);
            else if (name == "blue")    outColor = IM_COL32(52, 152, 219, 255);
            else if (name == "yellow")  outColor = IM_COL32(241, 196, 15, 255);
            else if (name == "orange")  outColor = IM_COL32(230, 126, 34, 255);
            else if (name == "amber")   outColor = IM_COL32(245, 158, 11, 255);
            else if (name == "gold")    outColor = IM_COL32(255, 215, 0, 255);
            else if (name == "cyan")    outColor = IM_COL32(26, 188, 156, 255);
            else if (name == "magenta") outColor = IM_COL32(155, 89, 182, 255);
            else if (name == "purple")  outColor = IM_COL32(142, 68, 173, 255);
            else if (name == "white")   outColor = IM_COL32(255, 255, 255, 255);
            else if (name == "black")   outColor = IM_COL32(0, 0, 0, 255);
            else if (name == "gray" || name == "grey") outColor = IM_COL32(149, 165, 166, 255);
            else if (name == "dim" || name == "dimmed") outColor = IM_COL32(127, 140, 141, 255);
            else if (name == "danger")                  outColor = IM_COL32(231, 76, 60, 255);
            else if (name == "warning")                 outColor = IM_COL32(241, 196, 15, 255);
            else if (name == "success")                 outColor = IM_COL32(46, 204, 113, 255);
            else if (name == "info")                    outColor = IM_COL32(52, 152, 219, 255);
            else if (name == "primary")                 outColor = IM_COL32(52, 152, 219, 255);
            else return ParseHexColor(s, len, outColor);

            return true;
        }

        // Helper to extract attribute values e.g. color=#hex or width=120
        inline bool ExtractAttribute(const std::string& tag, const std::string& attrName, std::string& outVal) {
            size_t pos = tag.find(attrName + "=");
            if (pos == std::string::npos) {
                pos = tag.find(attrName + ":");
                if (pos == std::string::npos) return false;
            }
            pos += attrName.length() + 1;
            if (pos >= tag.length()) return false;

            char quote = 0;
            if (tag[pos] == '"' || tag[pos] == '\'') {
                quote = tag[pos++];
            }
            size_t end = pos;
            if (quote) {
                end = tag.find(quote, pos);
                if (end == std::string::npos) end = tag.length();
            } else {
                while (end < tag.length() && !std::isspace(tag[end]) && tag[end] != '>' && tag[end] != '/') {
                    end++;
                }
            }
            outVal = tag.substr(pos, end - pos);
            return !outVal.empty();
        }

    } // namespace Detail

#ifndef IMGEE_TEXT_SEARCH_HOOK_DEFINED
#define IMGEE_TEXT_SEARCH_HOOK_DEFINED
    namespace Detail {
        typedef void (*TextSearchHighlightHookFn)(const char* text, const char* text_end);
        inline TextSearchHighlightHookFn& GetTextSearchHighlightHook() {
            static TextSearchHighlightHookFn s_Fn = nullptr;
            return s_Fn;
        }
    } // namespace Detail
#endif

    // Renders inline styled text supporting HTML-like tags:
    //   `<b>bold</b>`, `<i>italic</i>`, `<mono>code</mono>`, `<h1>..<h3>headers</h3>`,
    //   `<color="#hex">tinted text</color>`, `<pill color="#hex">capsule</pill>`,
    //   `<badge color="#hex">badge</badge>`, `<kbd>key</kbd>`, `<img name="logo">`
    inline void StyledTextUnformatted(const char* text, const char* text_end = nullptr) {
        if (!text) return;
        if (!text_end) text_end = text + std::strlen(text);
        if (text >= text_end) return;

        auto emitTextRun = [](const char* start, const char* end) {
            if (Detail::TextSearchHighlightHookFn hook = Detail::GetTextSearchHighlightHook()) {
                hook(start, end);
            }
            ImGui::TextUnformatted(start, end);
        };

        const char* cur = text;
        bool needSameLine = false;

        std::vector<bool> fontStack;
        std::vector<bool> colorStack;

        while (cur < text_end) {
            // 1. Literal escape: "\<" or "\>"
            if (*cur == '\\' && cur + 1 < text_end && (cur[1] == '<' || cur[1] == '>')) {
                if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                emitTextRun(cur + 1, cur + 2);
                needSameLine = true;
                cur += 2;
                continue;
            }

            // 2. Newline: "\n"
            if (*cur == '\n') {
                ImGui::NewLine();
                needSameLine = false;
                cur++;
                continue;
            }

            // 3. Normal plain text segment before next tag or escape
            if (*cur != '<') {
#if defined(IMGUI_STYLED_TEXT_HAS_ICONS)
                if (cur + 3 < text_end &&
                    (unsigned char)cur[0] == 0xF0 &&
                    (unsigned char)cur[1] == 0x9F &&
                    (unsigned char)cur[2] == 0x90 &&
                    ((unsigned char)cur[3] == 0x89 || (unsigned char)cur[3] == 0xB2)) {
                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Icon(((unsigned char)cur[3] == 0x89) ? "ouroboros" : "ouroboros-alt");
                    needSameLine = true;
                    cur += 4;
                    continue;
                }
#endif
                const char* seg_start = cur;
                while (cur < text_end && *cur != '<' && *cur != '\n' && *cur != '\\') {
#if defined(IMGUI_STYLED_TEXT_HAS_ICONS)
                    if ((unsigned char)*cur == 0xF0 && cur + 3 < text_end &&
                        (unsigned char)cur[1] == 0x9F && (unsigned char)cur[2] == 0x90 &&
                        ((unsigned char)cur[3] == 0x89 || (unsigned char)cur[3] == 0xB2)) {
                        break;
                    }
#endif
                    cur++;
                }
                if (cur > seg_start) {
                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                    emitTextRun(seg_start, cur);
                    needSameLine = true;
                }
                continue;
            }

            // 4. We found '<', parse tag
            const char* tag_start = cur;
            const char* tag_end = cur;
            while (tag_end < text_end && *tag_end != '>') {
                tag_end++;
            }
            if (tag_end >= text_end) {
                // Unterminated tag, print as bare text
                if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                emitTextRun(cur, text_end);
                break;
            }
            tag_end++; // include '>'

            std::string tagStr(tag_start + 1, tag_end - 1);
            // Trim whitespace
            size_t first = tagStr.find_first_not_of(" \t\r\n");
            size_t last = tagStr.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos) {
                tagStr = tagStr.substr(first, last - first + 1);
            }

            // A. Closing tags
            if (tagStr.rfind("/", 0) == 0) {
                std::string closeName = tagStr.substr(1);
                for (auto& c : closeName) c = (char)std::tolower(c);

                if (closeName == "b" || closeName == "strong" ||
                    closeName == "i" || closeName == "em" ||
                    closeName == "mono" || closeName == "code" ||
                    closeName == "h1" || closeName == "h2" || closeName == "h3" ||
                    closeName == "small" || closeName == "tiny" ||
                    closeName == "big" || closeName == "large" ||
                    closeName == "huge" ||
                    closeName == "regular" || closeName == "prop" || closeName == "sans") {
                    if (!fontStack.empty()) {
                        const bool pushedInlineColor = fontStack.back();
                        ImGui::PopFontRole();
                        fontStack.pop_back();
                        if (pushedInlineColor && !colorStack.empty()) {
                            ImGui::PopStyleColor();
                            colorStack.pop_back();
                        }
                    }
                    cur = tag_end;
                    continue;
                } else if (closeName == "color" || closeName == "col" || closeName == "style") {
                    if (!colorStack.empty()) {
                        ImGui::PopStyleColor();
                        colorStack.pop_back();
                    }
                    cur = tag_end;
                    continue;
                }
                // Unrecognized closing tag: fall through to output literally at line 795
            }

            // B. Opening tags
            std::string tagName;
            size_t spacePos = tagStr.find_first_of(" \t>/=");
            if (spacePos != std::string::npos) tagName = tagStr.substr(0, spacePos);
            else tagName = tagStr;
            for (auto& c : tagName) c = (char)std::tolower(c);

            // <small> / <tiny>
            if (tagName == "small" || tagName == "tiny") {
                ImGui::PushFontRole((tagName == "tiny") ? ImFontRole::Tiny : ImFontRole::Small);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <big> / <large>
            if (tagName == "big" || tagName == "large") {
                ImGui::PushFontRole(ImFontRole::Title);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <huge>
            if (tagName == "huge") {
                ImGui::PushFontRole(ImFontRole::BoldLarge);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <regular> / <prop> / <sans>
            if (tagName == "regular" || tagName == "prop" || tagName == "sans") {
                ImGui::PushFontRole(ImFontRole::Regular);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <b> / <strong>
            if (tagName == "b" || tagName == "strong") {
                ImGui::PushFontRole(ImFontRole::Bold);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <i> / <em>
            if (tagName == "i" || tagName == "em") {
                ImGui::PushFontRole(ImFontRole::Italic);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <mono> / <code>
            if (tagName == "mono" || tagName == "code") {
                ImGui::PushFontRole(ImFontRole::Monospace);
                bool pushedCol = false;
                std::string colVal;
                if (Detail::ExtractAttribute(tagStr, "color", colVal)) {
                    ImU32 ucol = 0;
                    if (Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                        colorStack.push_back(true);
                        pushedCol = true;
                    }
                }
                fontStack.push_back(pushedCol);
                cur = tag_end;
                continue;
            }

            // <h1..h3>
            if (tagName == "h1" || tagName == "h2" || tagName == "h3") {
                ImFontRole role = (tagName == "h1") ? ImFontRole::Heading1 :
                                  ((tagName == "h2") ? ImFontRole::Heading2 : ImFontRole::Heading3);
                ImGui::PushFontRole(role);
                fontStack.push_back(false);
                cur = tag_end;
                continue;
            }

            // <color=#hex>, <color=name>, <col ...>, <style ...>
            if (tagName == "color" || tagName == "col" || tagName == "style") {
                std::string colVal;
                if (!Detail::ExtractAttribute(tagStr, "color", colVal) &&
                    !Detail::ExtractAttribute(tagStr, "col", colVal) &&
                    !Detail::ExtractAttribute(tagStr, "style", colVal)) {
                    // Try parsing content right after "=" or space-separated token
                    size_t eq = tagStr.find('=');
                    if (eq != std::string::npos) {
                        colVal = tagStr.substr(eq + 1);
                    } else {
                        size_t sp = tagStr.find_first_of(" \t");
                        if (sp != std::string::npos) colVal = tagStr.substr(sp + 1);
                    }
                    while (!colVal.empty() && (colVal.back() == ' ' || colVal.back() == '\t' || colVal.back() == '/' || colVal.back() == '>')) colVal.pop_back();
                    while (!colVal.empty() && (colVal.front() == ' ' || colVal.front() == '\t')) colVal.erase(0, 1);
                }
                ImU32 ucol = 0;
                if (!colVal.empty() && Detail::ParseColor(colVal.c_str(), colVal.length(), ucol)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ucol);
                    colorStack.push_back(true);
                }
                cur = tag_end;
                continue;
            }

            // <pill color=#hex>text</pill> or <pill icon=name color=#hex />
            if (tagName == "pill") {
                bool isSelfClosing = false;
                size_t slashPos = tagStr.find_last_not_of(" \t\r\n");
                if (slashPos != std::string::npos && tagStr[slashPos] == '/') {
                    isSelfClosing = true;
                }

                std::string iconName;
                if (!Detail::ExtractAttribute(tagStr, "icon", iconName)) {
                    if (!Detail::ExtractAttribute(tagStr, "img", iconName)) {
                        Detail::ExtractAttribute(tagStr, "image", iconName);
                    }
                }

                std::string colVal;
                ImU32 bgCol = IM_COL32(52, 152, 219, 255); // default primary
                if (Detail::ExtractAttribute(tagStr, "color", colVal) || Detail::ExtractAttribute(tagStr, "bg", colVal)) {
                    Detail::ParseColor(colVal.c_str(), colVal.length(), bgCol);
                } else {
                    std::istringstream iss(tagStr);
                    std::string token;
                    iss >> token; // skip "pill"
                    while (iss >> token) {
                        if (token == "/" || token == "mono" || token == "bold" || token == "italic") continue;
                        if (token.rfind("icon=", 0) == 0 || token.rfind("img=", 0) == 0 || token.rfind("image=", 0) == 0) continue;
                        ImU32 c = 0;
                        if (Detail::ParseColor(token.c_str(), token.length(), c)) {
                            bgCol = c;
                            break;
                        }
                    }
                }

                bool isMono = (tagStr.find("mono") != std::string::npos);
                (void)isMono;

                if (isSelfClosing) {
                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
#if defined(IMGUI_STYLED_TEXT_HAS_BADGES)
                    if (isMono) ImGui::PushFontRole(ImFontRole::Monospace);
                    if (!iconName.empty()) {
                        ImGui::PillNamed(iconName.c_str(), bgCol);
                    } else {
                        ImGui::Pill("", bgCol);
                    }
                    if (isMono) ImGui::PopFontRole();
#endif
                    needSameLine = true;
                    cur = tag_end;
                    continue;
                } else {
                    std::string closeTag = "</pill>";
                    const char* closePos = std::strstr(tag_end, closeTag.c_str());
                    if (closePos && closePos < text_end) {
                        std::string innerText(tag_end, static_cast<size_t>(closePos - tag_end));
                        if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
#if defined(IMGUI_STYLED_TEXT_HAS_BADGES)
                        if (isMono) ImGui::PushFontRole(ImFontRole::Monospace);
                        if (!iconName.empty()) {
                            ImGui::PillNamed(iconName.c_str(), innerText.c_str(), bgCol);
                        } else {
                            ImGui::Pill(innerText.c_str(), bgCol);
                        }
                        if (isMono) ImGui::PopFontRole();
#else
                        ImGui::Text("[%s]", innerText.c_str());
#endif
                        needSameLine = true;
                        cur = closePos + closeTag.length();
                        continue;
                    }
                }
            }

            // <badge color=#hex>text</badge> or <badge icon=name />
            if (tagName == "badge") {
                bool isSelfClosing = false;
                size_t slashPos = tagStr.find_last_not_of(" \t\r\n");
                if (slashPos != std::string::npos && tagStr[slashPos] == '/') {
                    isSelfClosing = true;
                }

                std::string iconName;
                if (!Detail::ExtractAttribute(tagStr, "icon", iconName)) {
                    if (!Detail::ExtractAttribute(tagStr, "img", iconName)) {
                        Detail::ExtractAttribute(tagStr, "image", iconName);
                    }
                }

                std::string colVal;
                ImU32 bgCol = IM_COL32(52, 73, 94, 255); // default neutral badge
                if (Detail::ExtractAttribute(tagStr, "color", colVal) || Detail::ExtractAttribute(tagStr, "bg", colVal)) {
                    Detail::ParseColor(colVal.c_str(), colVal.length(), bgCol);
                } else {
                    std::istringstream iss(tagStr);
                    std::string token;
                    iss >> token; // skip "badge"
                    while (iss >> token) {
                        if (token == "/" || token == "mono" || token == "bold" || token == "italic") continue;
                        if (token.rfind("icon=", 0) == 0 || token.rfind("img=", 0) == 0 || token.rfind("image=", 0) == 0) continue;
                        ImU32 c = 0;
                        if (Detail::ParseColor(token.c_str(), token.length(), c)) {
                            bgCol = c;
                            break;
                        }
                    }
                }

                bool isMono = (tagStr.find("mono") != std::string::npos);
                (void)isMono;

                if (isSelfClosing) {
                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
#if defined(IMGUI_STYLED_TEXT_HAS_BADGES)
                    if (isMono) ImGui::PushFontRole(ImFontRole::Monospace);
                    if (!iconName.empty()) {
                        ImGui::BadgeNamed(iconName.c_str(), bgCol);
                    } else {
                        ImGui::Badge("", bgCol);
                    }
                    if (isMono) ImGui::PopFontRole();
#endif
                    needSameLine = true;
                    cur = tag_end;
                    continue;
                } else {
                    std::string closeTag = "</badge>";
                    const char* closePos = std::strstr(tag_end, closeTag.c_str());
                    if (closePos && closePos < text_end) {
                        std::string innerText(tag_end, static_cast<size_t>(closePos - tag_end));
                        if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
#if defined(IMGUI_STYLED_TEXT_HAS_BADGES)
                        if (isMono) ImGui::PushFontRole(ImFontRole::Monospace);
                        if (!iconName.empty()) {
                            ImGui::BadgeNamed(iconName.c_str(), innerText.c_str(), bgCol);
                        } else {
                            ImGui::Badge(innerText.c_str(), bgCol);
                        }
                        if (isMono) ImGui::PopFontRole();
#else
                        ImGui::Text("[%s]", innerText.c_str());
#endif
                        needSameLine = true;
                        cur = closePos + closeTag.length();
                        continue;
                    }
                }
            }

            // <kbd>key</kbd>
            if (tagName == "kbd" || tagName == "key") {
                std::string closeTag = (tagName == "kbd") ? "</kbd>" : "</key>";
                const char* closePos = std::strstr(tag_end, closeTag.c_str());
                if (closePos && closePos < text_end) {
                    std::string innerText(tag_end, static_cast<size_t>(closePos - tag_end));
                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
#if defined(IMGUI_STYLED_TEXT_HAS_BADGES)
                    ImGui::PushFontRole(ImFontRole::Monospace);
                    ImGui::Kbd(innerText.c_str());
                    ImGui::PopFontRole();
#else
                    ImGui::Text("[%s]", innerText.c_str());
#endif
                    needSameLine = true;
                    cur = closePos + closeTag.length();
                    continue;
                }
            }

            // <img name=...> or <image name=...>
            if (tagName == "img" || tagName == "image") {
                std::string imgName;
                Detail::ExtractAttribute(tagStr, "name", imgName);
                if (imgName.empty()) Detail::ExtractAttribute(tagStr, "src", imgName);

                if (!imgName.empty()) {
                    std::string wStr, hStr;
                    float w = 0.0f, h = 0.0f;
                    if (Detail::ExtractAttribute(tagStr, "width", wStr)) w = (float)std::atof(wStr.c_str());
                    if (Detail::ExtractAttribute(tagStr, "height", hStr)) h = (float)std::atof(hStr.c_str());

                    if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                    ImGui::ImageNamed(imgName.c_str(), ImVec2(w, h));
                    needSameLine = true;
                }
                cur = tag_end;
                continue;
            }

            // <icon name=...> or <icon caret-up /> or <icon orange undo />
#if defined(IMGUI_STYLED_TEXT_HAS_ICONS)
            if (tagName == "icon") {
                bool isSelfClosing = false;
                size_t slashPos = tagStr.find_last_not_of(" \t\r\n");
                if (slashPos != std::string::npos && tagStr[slashPos] == '/') {
                    isSelfClosing = true;
                }
                std::string iconName;
                if (!Detail::ExtractAttribute(tagStr, "name", iconName)) {
                    if (!Detail::ExtractAttribute(tagStr, "icon", iconName)) {
                        std::istringstream iss(tagStr);
                        std::string token;
                        iss >> token; // skip "icon"
                        while (iss >> token) {
                            if (token == "/" || token.rfind("color=", 0) == 0 || token.rfind("col=", 0) == 0 || token.rfind("size=", 0) == 0) continue;
                            std::string norm = NormalizeIconName(token.c_str());
                            if (IsVectorIcon(norm.c_str())) {
                                iconName = norm;
                                break;
                            }
                        }
                    }
                }
                std::string colVal;
                ImU32 iconCol = 0;
                if (Detail::ExtractAttribute(tagStr, "color", colVal) || Detail::ExtractAttribute(tagStr, "col", colVal)) {
                    Detail::ParseColor(colVal.c_str(), colVal.length(), iconCol);
                } else {
                    std::istringstream iss(tagStr);
                    std::string token;
                    iss >> token; // skip "icon"
                    while (iss >> token) {
                        if (token == "/" || token == iconName) continue;
                        ImU32 c = 0;
                        if (Detail::ParseColor(token.c_str(), token.length(), c)) {
                            iconCol = c;
                            break;
                        }
                    }
                }
                std::string szVal;
                float customSz = 0.0f;
                if (Detail::ExtractAttribute(tagStr, "size", szVal)) customSz = (float)std::atof(szVal.c_str());

                if (isSelfClosing) {
                    if (!iconName.empty()) {
                        if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                        ImVec4 c;
                        if (iconCol != 0) c = ColorConvertU32ToFloat4(iconCol);
                        ImGui::Icon(iconName.c_str(), customSz, iconCol != 0 ? &c : nullptr);
                        needSameLine = true;
                    }
                    cur = tag_end;
                    continue;
                } else {
                    std::string closeTag = "</icon>";
                    const char* closePos = std::strstr(tag_end, closeTag.c_str());
                    if (closePos && closePos < text_end) {
                        std::string inner(tag_end, static_cast<size_t>(closePos - tag_end));
                        if (iconName.empty()) iconName = NormalizeIconName(inner.c_str());
                        if (!iconName.empty()) {
                            if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
                            ImVec4 c;
                            if (iconCol != 0) c = ColorConvertU32ToFloat4(iconCol);
                            ImGui::Icon(iconName.c_str(), customSz, iconCol != 0 ? &c : nullptr);
                            needSameLine = true;
                        }
                        cur = closePos + closeTag.length();
                        continue;
                    }
                }
            }
#endif

            // Unrecognized tag: output literally
            if (needSameLine) ImGui::SameLine(0.0f, 0.0f);
            emitTextRun(tag_start, tag_end);
            needSameLine = true;
            cur = tag_end;
        }

        // Clean up unclosed tags on exit
        while (!fontStack.empty()) {
            ImGui::PopFontRole();
            fontStack.pop_back();
        }
        while (!colorStack.empty()) {
            ImGui::PopStyleColor();
            colorStack.pop_back();
        }
    }

    /** Formatted inline styled text */
    inline void StyledText(const char* fmt, ...) IM_FMTARGS(1);
    inline void StyledText(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[2048];
        int len = std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        if (len > 0) {
            StyledTextUnformatted(buf, buf + len);
        }
    }

    namespace Detail {
        typedef void (*StyledTextMarkdownRendererFn)(const char* text, size_t len);
        inline StyledTextMarkdownRendererFn& GetStyledTextMarkdownRenderer() {
            static StyledTextMarkdownRendererFn s_Fn = nullptr;
            return s_Fn;
        }
    } // namespace Detail

    /** Unformatted inline styled text wrapped to current window/column width */
    inline void StyledTextWrappedUnformatted(const char* text, const char* text_end = nullptr) {
        if (!text) return;
        size_t len = text_end ? (size_t)(text_end - text) : strlen(text);
        if (len == 0) return;
        if (Detail::GetStyledTextMarkdownRenderer()) {
            Detail::GetStyledTextMarkdownRenderer()(text, len);
            return;
        }
        ImGui::PushTextWrapPos(0.0f);
        StyledTextUnformatted(text, text_end);
        ImGui::PopTextWrapPos();
    }

    /** Formatted inline styled text wrapped to current window/column width */
    inline void StyledTextWrapped(const char* fmt, ...) IM_FMTARGS(1);
    inline void StyledTextWrapped(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[2048];
        int len = std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        if (len > 0) {
            StyledTextWrappedUnformatted(buf, buf + len);
        }
    }

} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#define IMGUI_STYLED_TEXT_READY 1

#if defined(IMGUI_MARKDOWN_EXT_READY) && !defined(IMGUI_STYLED_TEXT_MD_BRIDGE_DEFINED)
#if defined(__has_include)
#if __has_include("../Markdown/imgui_markdown_ext.h")
#include "../Markdown/imgui_markdown_ext.h"
#elif __has_include("imgui_markdown_ext.h")
#include "imgui_markdown_ext.h"
#endif
#endif
#endif

#endif // IMGUI_STYLED_TEXT_H
#endif // !IMGEE_DISABLE_TYPOGRAPHY