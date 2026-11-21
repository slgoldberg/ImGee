#pragma once
#ifndef IMGUI_MARKDOWN_H
#define IMGUI_MARKDOWN_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

// License: zlib
// Copyright (c) 2019 Juliette Foucaut & Doug Binks

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "imgui_internal.h"

// ============================================================================
// Automatic Mouse-Wheel Defeat on Vertically Scrollable Viewports
// ----------------------------------------------------------------------------
// In windows or child panels with active vertical scrolling (ScrollMax.y > 0),
// spinning the mouse wheel to scroll through content can inadvertently adjust
// controls if the cursor passes over a wheel-enabled widget. To prevent this,
// discrete widgets automatically refuse wheel inputs and suppress custom cursors
// when the viewport is scrollable, giving Dear ImGui's window scrolling priority.
// Opt-out compile-time: #define CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT
// Opt-out runtime:      ImGui::SetMouseWheelAutoDefeat(false);
// ============================================================================
#ifndef IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED
#define IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED

#ifndef IMGUI_CONTRIB_INTERNAL_INCLUDED
  #if defined(__has_include)
    #if __has_include("imgui_internal.h")
      #include "imgui_internal.h"
      #define IMGUI_CONTRIB_INTERNAL_INCLUDED 1
    #endif
  #endif
#endif
#if !defined(IMGUI_CONTRIB_INTERNAL_INCLUDED) && (defined(IMGUI_WINDOW_HARD_MIN_SIZE) || defined(IMGUI_INTERNAL_H))
  #define IMGUI_CONTRIB_INTERNAL_INCLUDED 1
#endif

#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM < 18900)
#ifndef ImGuiButtonFlags_AllowOverlap
  #ifdef ImGuiButtonFlags_AllowItemOverlap
    #define ImGuiButtonFlags_AllowOverlap ImGuiButtonFlags_AllowItemOverlap
  #else
    #define ImGuiButtonFlags_AllowOverlap (1 << 12)
  #endif
#endif
#endif

#ifndef ImTrunc
  #define ImTrunc(v) ((float)(int)(v))
#endif

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
#endif

namespace ImGui {
namespace Contrib {
    inline bool& GetMouseWheelAutoDefeatEnabled() {
        static bool s_Enabled = true;
        return s_Enabled;
    }

    inline bool IsCurrentWindowScrollableY() {
#if defined(CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT) || \
    defined(IMGUI_CONTRIB_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT) || \
    defined(IMGUI_DISABLE_MOUSE_WHEEL_SCROLL_DEFEAT)
        return false;
#else
        if (!GetMouseWheelAutoDefeatEnabled()) {
            return false;
        }
#if defined(IMGUI_CONTRIB_INTERNAL_INCLUDED) || defined(IMGUI_INTERNAL_H)
        ImGuiContext* g = ImGui::GetCurrentContext();
        if (g && g->CurrentWindow) {
            for (ImGuiWindow* w = g->CurrentWindow; w != nullptr; w = w->ParentWindow) {
                if (w->ScrollMax.y > 0.0f && !(w->Flags & ImGuiWindowFlags_NoScrollWithMouse))
                    return true;
                if (!(w->Flags & ImGuiWindowFlags_ChildWindow))
                    break;
            }
            return false;
        }
#endif
        return ImGui::GetScrollMaxY() > 0.0f;
#endif
    }
} // namespace Contrib

    inline void SetMouseWheelAutoDefeat(bool enable = true) {
        Contrib::GetMouseWheelAutoDefeatEnabled() = enable;
    }
    inline bool GetMouseWheelAutoDefeat() {
        return Contrib::GetMouseWheelAutoDefeatEnabled();
    }
} // namespace ImGui
#endif // IMGUI_CONTRIB_WINDOW_SCROLLABLE_Y_DEFINED

#ifndef IMGUI_MARKDOWN_DETAIL_DEFINED
#define IMGUI_MARKDOWN_DETAIL_DEFINED
namespace ImGui {
namespace MarkdownDetail {
    inline void AddRectCompat(ImDrawList* dl, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->AddRect(p_min, p_max, col, rounding, thickness, flags);
#else
        dl->AddRect(p_min, p_max, col, rounding, flags, thickness);
#endif
    }

    inline void StrokeLineCompat(ImDrawList* dl, const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness) {
        dl->PathClear();
        dl->PathLineTo(p1);
        dl->PathLineTo(p2);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->PathStroke(col, thickness, 0);
#else
        dl->PathStroke(col, 0, thickness);
#endif
    }

    inline void StrokePolylineCompat(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col, float thickness, ImDrawFlags flags = 0) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
        dl->AddPolyline(pts, count, col, thickness, flags);
#else
        dl->AddPolyline(pts, count, col, flags, thickness);
#endif
    }

    inline const char* CalcWordWrapPositionCompat(ImFont* font, float scale, const char* text, const char* text_end, float wrap_width) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        float font_size = (font && font->LegacySize > 0.0f) ? font->LegacySize : ImGui::GetFontSize();
        return font->CalcWordWrapPosition(font_size * scale, text, text_end, wrap_width);
#else
        return font->CalcWordWrapPositionA(scale, text, text_end, wrap_width);
#endif
    }

    inline float& GetActiveCaptionFontSizeOverride() {
        static float s_Override = 0.0f;
        return s_Override;
    }

    inline void PushMarkdownFont(ImFont* font) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        const float ovr = GetActiveCaptionFontSizeOverride();
        const float target_sz = (ovr > 0.0f) ? ovr : (font ? font->LegacySize : 0.0f);
        ImGui::PushFont(font, target_sz);
#else
        ImGui::PushFont(font);
#endif
    }
} // namespace MarkdownDetail
} // namespace ImGui
#endif // IMGUI_MARKDOWN_DETAIL_DEFINED

#ifndef ORIGINAL	/* brat's hack */
namespace ImGui
{
/// Get font scale as we prefer to use it:
inline IMGUI_API float GetFontScale (bool useParent = false)
{
#ifdef NEW_ZOOM_METHOD
	return GetIO().FontGlobalScale;
#else
	ImGuiWindow *window = GetCurrentContext()->CurrentWindow;
	if ((useParent || (window && window->FontWindowScale == 1.f &&
					   window->ParentWindow &&
					   window->ParentWindow->FontWindowScale != 1.f)) &&
		window->ParentWindow)
		return window->ParentWindow->FontWindowScale;
	else if (window)
		return window->FontWindowScale;
	else
		return 1.f;				  // fall-back in case something's really wrong
#endif
}

/// Helper to get effective visual scale considering both window zoom and font size
inline IMGUI_API float GetVisualScale()
{
	float font_scale = GetFontScale(false);
	float font_size = GetFontSize();
	if (font_scale > 0.0f && fabsf(font_scale - 1.0f) > 0.001f)
	{
		float base_sz = font_size / font_scale;
		if (base_sz > 0.0f && fabsf(base_sz - 14.0f) > 1.0f)
		{
			return font_scale * (base_sz / 14.0f);
		}
		return font_scale;
	}
	return (font_size > 0.0f) ? (font_size / 14.0f) : 1.0f;
}
}
#endif

// Supported link delimiters:
// ImGee natively supports BOTH standard CommonMark [...] and legacy/alternative {...} brackets simultaneously.
// Define IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY (or CONTRIB_MARKDOWN_CURLY_ONLY) to strictly require {...} brackets,
// preserving unescaped square brackets [...] as literal text without needing backslash escapes.
#define MD_SQUARE_BRACKET_OPEN '['
#define MD_SQUARE_BRACKET_CLOSE ']'
#define MD_CURLY_BRACKET_OPEN '{'
#define MD_CURLY_BRACKET_CLOSE '}'

namespace ImGui
{
    //-----------------------------------------------------------------------------
    // Basic types
    //-----------------------------------------------------------------------------

    struct Link;
    struct MarkdownConfig;

    struct MarkdownLinkCallbackData                                 // for both links and images
    {
        const char*             text;                               // text between square brackets []
        int                     textLength;
        const char*             link;                               // text between brackets ()
        int                     linkLength;
        void*                   userData;
        bool                    isImage;                            // true if '!' is detected in front of the link syntax
    };

    struct MarkdownTooltipCallbackData                              // for tooltips
    {
        MarkdownLinkCallbackData linkData;
        const char*              linkIcon;
    };
    
    enum class MarkdownImageAlign : unsigned char
    {
        None = 0,
        Left,
        Center,
        Right
    };

    enum class MarkdownImageFloat : unsigned char
    {
        None = 0,
        Left,
        Right
    };

    struct MarkdownImageData
    {
        bool                    isValid = false;                    // if true, will draw the image
        bool                    useLinkCallback = false;            // if true, linkCallback will be called when image is clicked
        bool                    isLoading = false;                  // if true, image is downloading in background
        ImTextureID             user_texture_id = 0;                // see ImGui::Image
        ImVec2                  size = ImVec2( 100.0f, 100.0f );    // see ImGui::Image
        ImVec2                  uv0 = ImVec2( 0, 0 );               // see ImGui::Image
        ImVec2                  uv1 = ImVec2( 1, 1 );               // see ImGui::Image
        ImVec4                  tint_col = ImVec4( 1, 1, 1, 1 );    // see ImGui::Image
        ImVec4                  border_col = ImVec4( 0, 0, 0, 0 );  // see ImGui::Image
        ImVec2                  native_size = ImVec2( 0.0f, 0.0f ); // Unscaled pixel dimensions (used by lightbox zoom)
        MarkdownImageAlign      align = MarkdownImageAlign::None;   // Optional horizontal alignment (left, center, right)
        int                     captionMode = 0;                    // 0 = follow MarkdownConfig::showImageCaptions, 1 = force show, -1 = force hide
        std::string             customCaption;                      // Optional custom caption override (uses [alt] when empty)
        std::string             altOverride;                        // Optional alt text extracted from HTML <img alt="...">
        int                     lightboxMode = 0;                   // 0 = follow MarkdownConfig::enableImageLightbox, 1 = force enable, -1 = force disable
        MarkdownImageFloat      floatMode = MarkdownImageFloat::None; // Optional text-wrapping float (float=left, float=right)
        float                   floatGapX = -1.0f;                  // Optional horizontal gap between floated figure and wrapped text (-1 = auto ~12px * scale)
        float                   floatGapY = -1.0f;                  // Optional vertical margin below floated figure (-1 = auto ~6px * scale)
    };

    enum class MarkdownFormatType
    {
         NORMAL_TEXT,
         HEADING,
         UNORDERED_LIST,
         ORDERED_LIST,
         LINK,
         EMPHASIS,
         CODE_BLOCK,
    };

    struct MarkdownFormatInfo
    {
        MarkdownFormatType      type    = MarkdownFormatType::NORMAL_TEXT;
        int32_t                 level   = 0;                               // Set for headings: 1 for H1, 2 for H2 etc.
		char                    emphasisChar  = '\0';                      // The character used for emphasis (e.g. *, _, ^, >)
        bool                    itemHovered = false;                       // Currently only set for links when mouse hovered, only valid when start_ == false
        const MarkdownConfig*   config  = NULL;
        bool                    isStartOfDoc = false;                      // True if this block is at start of markdown document/text (no preceding non-whitespace content)
        bool                    isPrecededBySeparator = false;             // True if preceded by a heading separator or thematic break
        bool                    isPrecededByBlankLine = false;             // True if preceded by an empty line
        const char*             blockLang = nullptr;                       // Language identifier for CODE_BLOCK (e.g. "cpp", "lua")
        int32_t                 blockLangLength = 0;
    };

    typedef void                MarkdownLinkCallback( MarkdownLinkCallbackData data );    
    typedef void                MarkdownTooltipCallback( MarkdownTooltipCallbackData data );

    inline void defaultMarkdownTooltipCallback( MarkdownTooltipCallbackData data_ )
    {
        if( data_.linkData.isImage )
        {
            ImGui::SetTooltip( "%.*s", data_.linkData.linkLength, data_.linkData.link );
        }
        else
        {
            std::string link(data_.linkData.link, (size_t)data_.linkData.linkLength);
            if (!link.empty() && link[0] == '#')
            {
                ImGui::SetTooltip( "Jump to section\n%.*s", data_.linkData.linkLength, data_.linkData.link );
            }
            else if (link.rfind("file:", 0) == 0 || link.rfind("local:", 0) == 0 || link.find(".pdf") != std::string::npos)
            {
                ImGui::SetTooltip( "Open document\n%.*s", data_.linkData.linkLength, data_.linkData.link );
            }
            else
            {
                ImGui::SetTooltip( "%s Open in browser\n%.*s", data_.linkIcon ? data_.linkIcon : "", data_.linkData.linkLength, data_.linkData.link );
            }
        }
    }

    typedef MarkdownImageData   MarkdownImageCallback( MarkdownLinkCallbackData data );

    inline MarkdownImageCallback*& GetDefaultMarkdownImageCallback() {
        static MarkdownImageCallback* s_DefaultImageCallback = nullptr;
        return s_DefaultImageCallback;
    }

    inline void SetDefaultMarkdownImageCallback(MarkdownImageCallback* callback) {
        GetDefaultMarkdownImageCallback() = callback;
    }

    inline MarkdownLinkCallback*& GetDefaultMarkdownLinkCallback() {
        static MarkdownLinkCallback* s_DefaultLinkCallback = nullptr;
        return s_DefaultLinkCallback;
    }

    inline void SetDefaultMarkdownLinkCallback(MarkdownLinkCallback* callback) {
        GetDefaultMarkdownLinkCallback() = callback;
    }
    typedef void                MarkdownFormalCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    struct MarkdownHeadingFormat
    {   
        ImFont*                 font;                               // ImGui font
        bool                    separator;                          // if true, an underlined separator is drawn after the header
    };

    struct MarkdownTagData
    {
        const char*             tagName;
        size_t                  tagNameLength;
        const char*             tagParam;
        size_t                  tagParamLength;
        const char*             innerText;
        size_t                  innerTextLength;
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownTagCallback( const MarkdownTagData& data );

    struct MarkdownCodeBlockData
    {
        const char*             code = nullptr;
        size_t                  codeLength = 0;
        const char*             language = nullptr;
        size_t                  languageLength = 0;
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownCodeBlockCallback( const MarkdownCodeBlockData& data );

#ifndef IMGUI_HAS_TABLE
    typedef int ImGuiTableFlags;
    typedef int ImGuiTableRowFlags;
    typedef int ImGuiTableColumnFlags;
#endif

    struct MarkdownTableState
    {
        size_t                  tableIndex = 0;                 // 0-based index of table in the markdown document
        int                     textStart = 0;                  // Start byte offset of table in markdown text
        int                     textEnd = 0;                    // End byte offset of table (including trailing comments)
        int                     rowsEnd = 0;                    // End byte offset of last data row
        float                   totalWidth = 0.0f;              // Total table width (normalized to 1.0x scale; 0.0f = auto content width)
        float                   presetWidth = 0.0f;             // Author preset width (0.0f = auto content width)
        int                     sortColumn = -1;                // Sorted column index (-1 = none)
        int                     sortDirection = 0;              // 0 = None, 1 = Ascending, 2 = Descending
        std::string             sortColumnName;                 // Plaintext header of the sorted column
        std::vector<float>      columnWidths;                   // Per-column widths (normalized)
        bool                    hasCustomColumnWidths = false;  // True if individual column widths were customized
        bool                    hasPreset = false;              // True if <!-- table-preset: ... --> exists
        bool                    hasState = false;               // True if <!-- table-state: ... --> exists
        bool                    isCommitted = true;             // True if change was committed (clicked sort, double-click toggled, drag released)
        const MarkdownConfig*   config = nullptr;
        void*                   userData = nullptr;
    };
    typedef void                MarkdownTableStateCallback( const MarkdownTableState& state, void* userData );
    typedef void                MarkdownTableTooltipCallback( ImGuiID id, bool hovered, const char* markdownText, void* userData );

    // -------------------------------------------------------------------------
    // Markdown Color Parser (Named UI Colors, Hex #RRGGBB, #RRGGBBAA, 0xRRGGBB)
    // -------------------------------------------------------------------------
    inline bool ParseMarkdownColor( const std::string& raw_str, ImVec4& out_col )
    {
        if( raw_str.empty() ) return false;

        std::string str = raw_str;
        for( char& c : str ) c = (char)tolower( (unsigned char)c );

        // Strip optional key= prefix
        if( str.rfind( "bgcolor=", 0 ) == 0 )       str = str.substr( 8 );
        else if( str.rfind( "bgcol=", 0 ) == 0 )    str = str.substr( 6 );
        else if( str.rfind( "bg=", 0 ) == 0 )       str = str.substr( 3 );
        else if( str.rfind( "fgcolor=", 0 ) == 0 )  str = str.substr( 8 );
        else if( str.rfind( "fgcol=", 0 ) == 0 )    str = str.substr( 6 );
        else if( str.rfind( "fg=", 0 ) == 0 )       str = str.substr( 3 );
        else if( str.rfind( "color=", 0 ) == 0 )    str = str.substr( 6 );
        else if( str.rfind( "col=", 0 ) == 0 )      str = str.substr( 4 );
        else if( str.rfind( "text=", 0 ) == 0 )     str = str.substr( 5 );
        else if( str.rfind( "border=", 0 ) == 0 )   str = str.substr( 7 );
        else if( str.rfind( "bcolor=", 0 ) == 0 )   str = str.substr( 7 );
        else if( str.rfind( "bcol=", 0 ) == 0 )     str = str.substr( 5 );

        if( str.size() >= 2 && ( ( str.front() == '"' && str.back() == '"' ) || ( str.front() == '\'' && str.back() == '\'' ) ) )
        {
            str = str.substr( 1, str.size() - 2 );
        }
        if( str.empty() ) return false;

        // Named UI colors
        if( str == "red" )     { out_col = ImVec4( 0.95f, 0.25f, 0.25f, 1.0f ); return true; }
        if( str == "green" )   { out_col = ImVec4( 0.25f, 0.85f, 0.35f, 1.0f ); return true; }
        if( str == "blue" )    { out_col = ImVec4( 0.30f, 0.65f, 1.0f,  1.0f ); return true; }
        if( str == "yellow" )  { out_col = ImVec4( 1.0f,  0.85f, 0.20f, 1.0f ); return true; }
        if( str == "orange" )  { out_col = ImVec4( 1.0f,  0.647f, 0.0f, 1.0f ); return true; }
        if( str == "purple" || str == "violet" ) { out_col = ImVec4( 0.60f, 0.25f, 0.90f, 1.0f ); return true; }
        if( str == "cyan" )    { out_col = ImVec4( 0.20f, 0.85f, 0.95f, 1.0f ); return true; }
        if( str == "magenta" ) { out_col = ImVec4( 0.90f, 0.30f, 0.90f, 1.0f ); return true; }
        if( str == "white" )   { out_col = ImVec4( 1.0f,  1.0f,  1.0f,  1.0f ); return true; }
        if( str == "black" )   { out_col = ImVec4( 0.0f,  0.0f,  0.0f,  1.0f ); return true; }
        if( str == "gray" || str == "grey" ) { out_col = ImVec4( 0.60f, 0.60f, 0.60f, 1.0f ); return true; }
        if( str == "gold" )    { out_col = ImVec4( 1.0f,  0.84f, 0.0f,  1.0f ); return true; }
        if( str == "dark" )    { out_col = ImVec4( 0.18f, 0.18f, 0.22f, 1.0f ); return true; }
        if( str == "light" )   { out_col = ImVec4( 0.90f, 0.90f, 0.92f, 1.0f ); return true; }
        if( str == "danger" )  { out_col = ImVec4( 0.85f, 0.15f, 0.15f, 1.0f ); return true; }
        if( str == "warning" ) { out_col = ImVec4( 0.95f, 0.75f, 0.10f, 1.0f ); return true; }
        if( str == "success" ) { out_col = ImVec4( 0.15f, 0.70f, 0.25f, 1.0f ); return true; }
        if( str == "info" )    { out_col = ImVec4( 0.20f, 0.55f, 0.90f, 1.0f ); return true; }

        // Hex formats: #RRGGBB, #RRGGBBAA, 0xRRGGBB, 0xRRGGBBAA
        size_t start = 0;
        if( str[0] == '#' )
        {
            start = 1;
        }
        else if( str.size() >= 2 && str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) )
        {
            start = 2;
        }
        else if( isxdigit( (unsigned char)str[0] ) )
        {
            start = 0;
        }
        else
        {
            return false;
        }

        std::string hex = str.substr( start );
        auto parseHexU64 = []( const std::string& h, uint64_t& outVal ) -> bool {
            if( h.empty() ) return false;
            uint64_t acc = 0;
            for( char ch : h )
            {
                unsigned char uc = static_cast<unsigned char>( ch );
                uint64_t digit = 0;
                if( uc >= '0' && uc <= '9' ) digit = static_cast<uint64_t>( uc - '0' );
                else if( uc >= 'a' && uc <= 'f' ) digit = static_cast<uint64_t>( 10 + ( uc - 'a' ) );
                else if( uc >= 'A' && uc <= 'F' ) digit = static_cast<uint64_t>( 10 + ( uc - 'A' ) );
                else return false;
                acc = ( acc << 4 ) | digit;
            }
            outVal = acc;
            return true;
        };

        uint64_t val = 0;
        if( hex.length() == 3 && parseHexU64( hex, val ) )
        {
            unsigned int r = static_cast<unsigned int>( ( val >> 8 ) & 0xF );
            unsigned int g = static_cast<unsigned int>( ( val >> 4 ) & 0xF );
            unsigned int b = static_cast<unsigned int>( val & 0xF );
            out_col.x = ( ( r << 4 ) | r ) / 255.0f;
            out_col.y = ( ( g << 4 ) | g ) / 255.0f;
            out_col.z = ( ( b << 4 ) | b ) / 255.0f;
            out_col.w = 1.0f;
            return true;
        }
        else if( hex.length() == 4 && parseHexU64( hex, val ) )
        {
            unsigned int r = static_cast<unsigned int>( ( val >> 12 ) & 0xF );
            unsigned int g = static_cast<unsigned int>( ( val >> 8 ) & 0xF );
            unsigned int b = static_cast<unsigned int>( ( val >> 4 ) & 0xF );
            unsigned int a = static_cast<unsigned int>( val & 0xF );
            out_col.x = ( ( r << 4 ) | r ) / 255.0f;
            out_col.y = ( ( g << 4 ) | g ) / 255.0f;
            out_col.z = ( ( b << 4 ) | b ) / 255.0f;
            out_col.w = ( ( a << 4 ) | a ) / 255.0f;
            return true;
        }
        else if( hex.length() == 6 && parseHexU64( hex, val ) )
        {
            out_col.x = static_cast<float>( ( val >> 16 ) & 0xFF ) / 255.0f;
            out_col.y = static_cast<float>( ( val >> 8 ) & 0xFF ) / 255.0f;
            out_col.z = static_cast<float>( val & 0xFF ) / 255.0f;
            out_col.w = 1.0f;
            return true;
        }
        else if( hex.length() == 8 && parseHexU64( hex, val ) )
        {
            out_col.x = static_cast<float>( ( val >> 24 ) & 0xFF ) / 255.0f;
            out_col.y = static_cast<float>( ( val >> 16 ) & 0xFF ) / 255.0f;
            out_col.z = static_cast<float>( ( val >> 8 ) & 0xFF ) / 255.0f;
            out_col.w = static_cast<float>( val & 0xFF ) / 255.0f;
            return true;
        }
        return false;
    }

    struct MarkdownConfig
    {
        static const int        NUMHEADINGS = 4;

        MarkdownLinkCallback*   linkCallback = NULL;
        MarkdownTooltipCallback* tooltipCallback = NULL;
        MarkdownImageCallback*  imageCallback = NULL;
        MarkdownTagCallback*    tagCallback = NULL;
        MarkdownCodeBlockCallback* codeBlockCallback = NULL;        // Optional host custom syntax/layout handler
        MarkdownTableStateCallback* tableStateCallback = NULL;      // Optional notification callback when table sort or size changes
        MarkdownTableTooltipCallback* tableTooltipCallback = NULL;  // Optional delayed-tooltip sink for interactive table controls (silent if NULL without TimedTooltip)
        const char*             (*codeBlockDescriptionCallback)( const char* lang, size_t langLength, void* userData ) = NULL; // Optional host language tooltip description callback
        void                    (*setClipboardTextFn)( const char* text, void* userData ) = NULL; // Optional host OS clipboard provider override
        bool                    showCodeCopyButton = true;          // If true, shows one-click [Copy] button in code header
        bool                    enableTableTooltips = true;         // If true, emits delayed tooltips for sortable table headers and resize grips
        int                     tabSize = 4;                        // Tab stop width for \t expansion (default: 4)
        const char*             linkIcon = "";                      // icon displayd in link tooltip
        const char*             emphasisChars = "*_`~$+";           // string of characters recognized as emphasis markers (*, _, `, ~, $, +)
        bool                    underlineWithUnderscores = false;   // If true, __text__ renders as underline instead of bold
        MarkdownHeadingFormat   headingFormats[ NUMHEADINGS ] = { { NULL, true }, { NULL, true }, { NULL, true }, { NULL, true } };
        void*                   userData = NULL;
        MarkdownFormalCallback* formatCallback = defaultMarkdownFormatCallback;
        float                   bulletSpacing = 0.0f;               // 0.0f = default (4.0f); > 0.0f = custom gap between bullet and text
        float                   wrapWidth = 0.0f;                   // 0.0f = wrap at GetContentRegionAvail().x; > 0.0f = explicit wrap width; < 0.0f = no word-wrap (single line per \n)
        ImGuiTableFlags         tableFlags = 0;                     // Optional table flags override (0 = default GFM styling: Borders | RowBg | SizingStretchProp | NoSavedSettings)

        // Semantic font aliases (if set, takes priority over headingFormats):
        ImFont*                 h1Font = nullptr;                   // If nullptr, falls back to headingFormats[0].font
        ImFont*                 h2Font = nullptr;                   // If nullptr, falls back to headingFormats[1].font
        ImFont*                 h3Font = nullptr;                   // If nullptr, falls back to headingFormats[2].font
        ImFont*                 h1ItalicFont = nullptr;             // Optional italic font for # H1
        ImFont*                 h2ItalicFont = nullptr;             // Optional italic font for ## H2
        ImFont*                 h3ItalicFont = nullptr;             // Optional italic font for ### H3
        ImFont*                 headingItalicFont = nullptr;        // Fallback italic font for all headings
        ImFont*                 boldFont = nullptr;                 // If nullptr, falls back to headingFormats[3].font
        ImFont*                 italicFont = nullptr;               // If nullptr, falls back to ImGuiCol_TextDisabled
        ImFont*                 boldItalicFont = nullptr;           // If nullptr, falls back to boldFont or italicFont
        ImFont*                 monoFont = nullptr;                 // If nullptr, falls back to ambient window font
        ImFont*                 regularFont = nullptr;              // If nullptr, falls back to ambient window font
        ImFont*                 captionFont = nullptr;              // Optional font override for image/lightbox captions (defaults to ImFontRole::Small or 0.875x scale)
        const char*             baseDir = nullptr;                  // Optional base directory for resolving relative Markdown image paths
        bool                    showImageCaptions = false;          // If true, renders [alt] text as a centered figure caption beneath images by default
        bool                    enableImageLightbox = false;        // If true, clicking rendered images opens an in-context zoom lightbox overlay by default
    };

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

    //-----------------------------------------------------------------------------
    // External interface (Low-Level Rendering Engine)
    //-----------------------------------------------------------------------------

    namespace Markdown
    {
        inline void Render( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ );
    }

    //-----------------------------------------------------------------------------
    // Internals
    //-----------------------------------------------------------------------------

    struct TextRegion;
    struct Line;
    inline void UnderLine( ImColor col_ );
    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ );

    inline bool IsCharInsideWord( char c_ )
    {
        return c_ != ' ' && c_ != '\t' && c_ != '\n' && c_ != '\r' && 
               c_ != '.' && c_ != ',' && c_ != ';' && c_ != '!' && c_ != '?' && 
               c_ != '\"' && c_ != '(' && c_ != ')' && c_ != '[' && c_ != ']' && 
               c_ != '{' && c_ != '}';
    }

    // Typographic wrap helper: Prevents orphaned opening quotes/brackets and severed contractions/hyphens
    inline const char* AdjustWrapForTypography( const char* text_, const char* endLine, const char* text_end_ )
    {
        if( endLine <= text_ || endLine >= text_end_ )
            return endLine;

        char c_ = *endLine;
        char prev_c = *(endLine - 1);

        // 1. Orphaned Opening Delimiter: e.g. " word -> break before the opening delimiter so it travels with the word!
        if( ( prev_c == '\"' || prev_c == '\'' || prev_c == '(' || prev_c == '[' || prev_c == '{' ) &&
            ( endLine - 1 > text_ && *(endLine - 2) == ' ' ) )
        {
            return endLine - 1;
        }

        // 2. Severed Contraction or Hyphen: e.g. "man's" or "semi-final" -> break before the whole compound word!
        if( ( prev_c == '\'' || prev_c == '-' ) &&
            ( endLine - 1 > text_ && IsCharInsideWord( *(endLine - 2) ) ) &&
            ( IsCharInsideWord( c_ ) ) )
        {
            const char* rewind = endLine - 2;
            while( rewind > text_ && *rewind != ' ' )
                --rewind;

            if( rewind > text_ && *rewind == ' ' )
                return rewind;
        }

        return endLine;
    }

    struct MarkdownRenderedLineSpan
    {
        ImVec2 min = ImVec2( 0.0f, 0.0f );
        ImVec2 max = ImVec2( 0.0f, 0.0f );
    };

    namespace MarkdownDetail
    {
        struct FloatBox
        {
            MarkdownImageFloat side = MarkdownImageFloat::None;
            float              minY = 0.0f;
            float              maxY = 0.0f;
            float              xMin = 0.0f;
            float              xMax = 0.0f;
            float              hGap = 0.0f;
        };

        struct FloatLayoutScope
        {
            ImGuiWindow*        window = nullptr;
            float               containerLeftX = 0.0f;
            float               containerRightX = 0.0f;
            float               deepestMaxY = -FLT_MAX;
            bool                justPlacedFloat = false;
            std::vector<FloatBox> boxes;
        };

        inline std::vector<FloatLayoutScope*>& GetFloatScopeStack()
        {
            static std::vector<FloatLayoutScope*> s_Stack;
            return s_Stack;
        }

        inline FloatLayoutScope* GetActiveFloatScope()
        {
            std::vector<FloatLayoutScope*>& st = GetFloatScopeStack();
            if( st.empty() ) return nullptr;
            FloatLayoutScope* top = st.back();
            if( !top || top->window != ImGui::GetCurrentWindow() ) return nullptr;
            return top;
        }

        struct ScopedSuspendFloatScope
        {
            ScopedSuspendFloatScope()
            {
                GetFloatScopeStack().push_back( nullptr );
            }
            ~ScopedSuspendFloatScope()
            {
                std::vector<FloatLayoutScope*>& st = GetFloatScopeStack();
                if( !st.empty() && st.back() == nullptr ) {
                    st.pop_back();
                }
            }
        };

        inline std::vector<MarkdownRenderedLineSpan>& GetRenderedLineSpans()
        {
            static std::vector<MarkdownRenderedLineSpan> s_Lines;
            return s_Lines;
        }

        inline void RecordRenderedTextLineSpan( const ImVec2& r_min, const ImVec2& r_max )
        {
            std::vector<MarkdownRenderedLineSpan>& lines = GetRenderedLineSpans();
            if( lines.size() < 128 ) {
                MarkdownRenderedLineSpan sp;
                sp.min = r_min;
                sp.max = r_max;
                lines.push_back( sp );
            }
        }

        inline float GetWindowNaturalLineStartX( ImGuiWindow* win )
        {
            if( !win ) return ImGui::GetCursorScreenPos().x;
            return ImTrunc( win->Pos.x + win->DC.Indent.x + win->DC.ColumnsOffset.x );
        }

        // Queries active float insets (leftInset, rightInset) over [lineY, lineY + lineH).
        // Returns true if at least one active float overlaps that vertical span.
        inline bool QueryFloatBandAtY( float lineY, float lineH, float& out_left_inset, float& out_right_inset )
        {
            out_left_inset = 0.0f;
            out_right_inset = 0.0f;
            FloatLayoutScope* scope = GetActiveFloatScope();
            if( !scope || scope->boxes.empty() ) return false;

            float eff_left = scope->containerLeftX;
            float eff_right = scope->containerRightX;
            bool any_overlap = false;
            const float span_h = ( lineH > 1.0f ) ? lineH : ImMax( 1.0f, ImGui::GetTextLineHeight() );

            for( size_t i = 0; i < scope->boxes.size(); ++i ) {
                const FloatBox& b = scope->boxes[i];
                if( lineY < ( b.maxY - 0.5f ) && ( lineY + span_h ) > ( b.minY + 0.5f ) ) {
                    any_overlap = true;
                    if( b.side == MarkdownImageFloat::Left ) {
                        eff_left = ImMax( eff_left, b.xMax + b.hGap );
                    } else if( b.side == MarkdownImageFloat::Right ) {
                        eff_right = ImMin( eff_right, b.xMin - b.hGap );
                    }
                }
            }

            if( any_overlap ) {
                out_left_inset = ImMax( 0.0f, eff_left - scope->containerLeftX );
                out_right_inset = ImMax( 0.0f, scope->containerRightX - eff_right );
            }
            return any_overlap;
        }

        inline void PrepareLineStartForFloats()
        {
            FloatLayoutScope* scope = GetActiveFloatScope();
            if( !scope || scope->boxes.empty() ) return;
            scope->justPlacedFloat = false;

            ImGuiWindow* win = scope->window;
            const ImVec2 cur_pos = ImGui::GetCursorScreenPos();
            float left_inset = 0.0f, right_inset = 0.0f;
            if( !QueryFloatBandAtY( cur_pos.y, ImGui::GetTextLineHeight(), left_inset, right_inset ) ) {
                return;
            }
            if( left_inset > 0.0f ) {
                const float natural_start_x = GetWindowNaturalLineStartX( win );
                const float semantic_indent = ImMax( 0.0f, natural_start_x - scope->containerLeftX );
                const float target_start_x = scope->containerLeftX + left_inset + semantic_indent;
                if( cur_pos.x < target_start_x - 0.5f ) {
                    ImGui::SetCursorScreenPos( ImVec2( target_start_x, cur_pos.y ) );
                }
            }
        }

        inline void ResolveLineFloatGeometry( float explicit_wrap_width, float& out_line_start_x, float& out_width_left )
        {
            ImGuiWindow* win = ImGui::GetCurrentWindow();
            ImVec2 cur_pos = ImGui::GetCursorScreenPos();
            float natural_start_x = GetWindowNaturalLineStartX( win );
#if !defined(IMGUI_VERSION_NUM) || IMGUI_VERSION_NUM >= 18000
            if( ImGuiTable* cur_table = ImGui::GetCurrentTable() ) {
                if( cur_table->CurrentColumn != -1 && cur_table->CurrentColumn < cur_table->ColumnsCount ) {
                    natural_start_x = ImMax( natural_start_x, cur_table->Columns[cur_table->CurrentColumn].WorkMinX );
                }
            }
#endif
            FloatLayoutScope* scope = GetActiveFloatScope();
            if( scope ) {
                scope->justPlacedFloat = false;
            }
            if( !scope || scope->boxes.empty() ) {
                out_line_start_x = natural_start_x;
                const float used_on_line = ImMax( 0.0f, cur_pos.x - out_line_start_x );
                out_width_left = ( explicit_wrap_width > 0.0f )
                                     ? ImMax( 10.0f, explicit_wrap_width - used_on_line )
                                     : ImGui::GetContentRegionAvail().x;
                return;
            }

            float left_inset = 0.0f, right_inset = 0.0f;
            QueryFloatBandAtY( cur_pos.y, ImGui::GetTextLineHeight(), left_inset, right_inset );

            const float semantic_indent = ImMax( 0.0f, natural_start_x - scope->containerLeftX );
            const float target_start_x = scope->containerLeftX + left_inset + semantic_indent;
            if( left_inset > 0.0f && cur_pos.x < target_start_x - 0.5f ) {
                ImGui::SetCursorScreenPos( ImVec2( target_start_x, cur_pos.y ) );
                cur_pos.x = target_start_x;
            }
            out_line_start_x = target_start_x;

            const float base_right_x = ( explicit_wrap_width > 0.0f )
                                           ? ( scope->containerLeftX + explicit_wrap_width )
                                           : scope->containerRightX;
            const float right_limit_x = base_right_x - right_inset;
            out_width_left = ImMax( 10.0f, right_limit_x - cur_pos.x );
        }

        // Advances cursor Y past active floats matching 'which' (None = clear both Left and Right).
        inline bool ClearActiveFloats( MarkdownImageFloat which = MarkdownImageFloat::None )
        {
            FloatLayoutScope* scope = GetActiveFloatScope();
            if( !scope ) return false;
            scope->justPlacedFloat = false;
            if( scope->boxes.empty() ) return false;

            ImGuiWindow* win = scope->window;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
            if( win && win->DC.IsSameLine ) {
                ImGui::NewLine();
            }
#endif
            const float cur_y = ImGui::GetCursorScreenPos().y;
            float target_y = cur_y;
            for( size_t i = 0; i < scope->boxes.size(); ++i ) {
                const FloatBox& b = scope->boxes[i];
                if( which == MarkdownImageFloat::None || b.side == which ) {
                    if( b.maxY > target_y ) target_y = b.maxY;
                }
            }
            if( target_y > cur_y + 0.25f ) {
                const float natural_start_x = GetWindowNaturalLineStartX( win );
                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );
                ImGui::SetCursorScreenPos( ImVec2( natural_start_x, target_y ) );
                ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
                ImGui::PopStyleVar();
                return true;
            }
            return false;
        }

        inline MarkdownImageFloat ParseClearFloatMode( const char* param, size_t param_len, bool require_clear_keyword )
        {
            std::string s = ( param && param_len > 0 ) ? std::string( param, param_len ) : std::string();
            for( size_t i = 0; i < s.size(); ++i ) {
                s[i] = static_cast<char>( tolower( static_cast<unsigned char>( s[i] ) ) );
            }
            size_t st = 0;
            while( st < s.size() && ( s[st] == ' ' || s[st] == '\t' || s[st] == '=' || s[st] == ':' || s[st] == '"' || s[st] == '\'' ) ) ++st;
            size_t en = s.size();
            while( en > st && ( s[en - 1] == ' ' || s[en - 1] == '\t' || s[en - 1] == '/' || s[en - 1] == '"' || s[en - 1] == '\'' ) ) --en;
            s = s.substr( st, en - st );

            if( require_clear_keyword ) {
                size_t cpos = s.find( "clear" );
                if( cpos == std::string::npos ) return static_cast<MarkdownImageFloat>( 255 ); // Not a clear directive
                s = s.substr( cpos + 5 );
                size_t cst = 0;
                while( cst < s.size() && ( s[cst] == ' ' || s[cst] == '\t' || s[cst] == '=' || s[cst] == ':' || s[cst] == '"' || s[cst] == '\'' ) ) ++cst;
                s = s.substr( cst );
            }

            if( s.rfind( "left", 0 ) == 0 ) return MarkdownImageFloat::Left;
            if( s.rfind( "right", 0 ) == 0 ) return MarkdownImageFloat::Right;
            return MarkdownImageFloat::None; // None == clear all/both
        }
    } // namespace MarkdownDetail

    inline const std::vector<MarkdownRenderedLineSpan>& GetLastMarkdownRenderedLines()
    {
        return MarkdownDetail::GetRenderedLineSpans();
    }

    struct TextRegion
    {
        TextRegion() : wrapWidth( 0.0f ), indentX( 0.0f ), leadIndents( 0 )
        {
        }
        ~TextRegion()
        {
            ResetIndent();
        }

        void RenderTextWrapped( const char* text_, const char* text_end_, bool bIndentToHere_ = false )
        {
            (void)bIndentToHere_;
            if( wrapWidth < 0.0f )
            {
                if( text_end_ > text_ )
                {
                    MarkdownDetail::PrepareLineStartForFloats();
                }
                ImGui::TextUnformatted( text_, text_end_ );
                if( text_end_ > text_ )
                {
                    MarkdownDetail::RecordRenderedTextLineSpan( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                }
                return;
            }
#ifdef ORIGINAL
            float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
			float		scale = ImGui::GetFontScale(false);// use parent's scale
#endif
            float       lineStartX = 0.0f;
            float       widthLeft = 0.0f;
            MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
            float       usedOnLine = ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
            const char* endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // If we are continuing on an existing line with prior content (usedOnLine > 0),
            // and the first word cannot cleanly fit in widthLeft, push the word to the next line.
            // When wrapping to a new line, strip any leading spaces that served only as inter-item separation on the previous line.
            if( usedOnLine > 0.0f )
            {
                const char* first_non_space = text_;
                while( first_non_space < text_end_ && *first_non_space == ' ' ) ++first_non_space;

                bool breaks_first_word = false;
                if( endLine <= first_non_space && first_non_space < text_end_ )
                {
                    breaks_first_word = true;
                }
                else if( endLine > first_non_space && endLine < text_end_ )
                {
                    const char* word_end = first_non_space;
                    while( word_end < text_end_ && IsCharInsideWord( *word_end ) ) ++word_end;
                    if( endLine < word_end )
                    {
                        breaks_first_word = true;
                    }
                }

                if( breaks_first_word )
                {
                    // Push to next line and suppress inter-item leading space!
                    ImGui::NewLine();
                    text_ = first_non_space;
                    MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
                    endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
                    endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                }
            }
            
            ImGui::TextUnformatted( text_, endLine );
            if( endLine > text_ )
            {
                MarkdownDetail::RecordRenderedTextLineSpan( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
            }
            while( endLine < text_end_ )
            {
                text_ = endLine;
                while( text_ < text_end_ && *text_ == ' ' ) { ++text_; }    // skip spaces at start of line
                MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
                endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine && text_ < text_end_ ) 
                {
                    endLine++;
                }
                ImGui::TextUnformatted( text_, endLine );
                if( endLine > text_ )
                {
                    MarkdownDetail::RecordRenderedTextLineSpan( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                }
            }
        }

        static float GetBulletSpacing( float custom_spacing = -1.0f )
        {
            if( custom_spacing >= 0.0f )
                return custom_spacing;
            return 4.0f; // Clean, elegant 4px spacing instead of bloated gap
        }

        void ApplyBullet( float custom_spacing = -1.0f )
        {
            MarkdownDetail::PrepareLineStartForFloats();
            float startX = ImGui::GetCursorScreenPos().x;
            ImGui::Bullet();
            ImGui::SameLine( 0.0f, GetBulletSpacing( custom_spacing ) );
            float textStartX = ImGui::GetCursorScreenPos().x;
            float bulletIndent = textStartX - startX;
            if( bulletIndent > 0.0f )
            {
                ImGui::Indent( bulletIndent );
                indentX += bulletIndent;
                MarkdownDetail::PrepareLineStartForFloats();
            }
        }

        void RenderListTextWrapped( const char* text_, const char* text_end_, float bullet_spacing = -1.0f )
        {
            ApplyBullet( bullet_spacing );
            RenderTextWrapped( text_, text_end_ );
        }

        void ApplyOrderedNumber( const char* num_str, const char* num_str_end, float custom_spacing = -1.0f )
        {
            MarkdownDetail::PrepareLineStartForFloats();
            float startX = ImGui::GetCursorScreenPos().x;
            ImGui::TextUnformatted( num_str, num_str_end );
            float spacing = ( custom_spacing >= 0.0f ) ? custom_spacing : ( ImGui::GetStyle().ItemSpacing.x * 0.5f );
            ImGui::SameLine( 0.0f, spacing );
            float textStartX = ImGui::GetCursorScreenPos().x;
            float numIndent = textStartX - startX;
            if( numIndent > 0.0f )
            {
                ImGui::Indent( numIndent );
                indentX += numIndent;
                MarkdownDetail::PrepareLineStartForFloats();
            }
        }

        void RenderOrderedListTextWrapped( const char* num_str, const char* num_str_end, const char* text_, const char* text_end_, float custom_spacing = -1.0f )
        {
            ApplyOrderedNumber( num_str, num_str_end, custom_spacing );
            RenderTextWrapped( text_, text_end_ );
        }

        bool RenderLinkText( const char* text_, const char* text_end_, const Link& link_, 
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ );

        void RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ = false );

        float GetIndent() const
        {
            return indentX;
        }

        int GetLeadIndents() const
        {
            return leadIndents;
        }

        void ApplyLeadIndent( int count )
        {
            while( leadIndents < count )
            {
                ImGui::Indent();
                ++leadIndents;
            }
            while( leadIndents > count )
            {
                ImGui::Unindent();
                --leadIndents;
            }
        }

        void ResetIndent()
        {
            if( indentX > 0.0f )
            {
                ImGui::Unindent( indentX );
            }
            indentX = 0.0f;
            while( leadIndents > 0 )
            {
                ImGui::Unindent();
                --leadIndents;
            }
        }

        float       wrapWidth;

    private:
        float       indentX;
        int         leadIndents;
    };

    struct Line {
        bool isHeading = false;
        bool isEmphasis = false;
        bool isUnorderedListStart = false;
        bool isOrderedListStart = false;
        int  orderedListNumStart = 0;
        int  orderedListNumEnd = 0;
        bool isLeadingSpace = true;     // spaces at start of line
        int  leadSpaceCount = 0;
        int  headingCount = 0;
        int  emphasisCount = 0;
		char emphasisChar  = '\0';
        int  lineStart = 0;
        int  lineEnd   = 0;
        int  lastRenderPosition = 0;     // lines may get rendered in multiple pieces
        bool isPrecededBySeparator = false;
        bool isPrecededByBlankLine = false;
        bool hadSeparator = false;
        bool wasBlank = false;
    };

    struct TextBlock {                  // subset of line
        int start = 0;
        int stop  = 0;
        int size() const
        {
            return stop - start;
        }
    };

    struct Link {
        enum LinkState {
            NO_LINK,
            HAS_SQUARE_BRACKET_OPEN,
            HAS_SQUARE_BRACKETS,
            HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
        };
        LinkState state = NO_LINK;
        TextBlock text;
        TextBlock url;
        bool isImage = false;
        int num_brackets_open = 0;
        char openBracketChar = 0;
    };

	struct Emphasis {
		enum EmphasisState {
			NONE,
			LEFT,
			MIDDLE,
			RIGHT,
		};
        EmphasisState state = NONE;
        TextBlock text;
        char sym = 0;
	};

    inline void DrawTextDecorationLine( ImColor col_, bool is_strikethrough_ )
    {
        ImVec2 r_min = ImGui::GetItemRectMin();
        ImVec2 r_max = ImGui::GetItemRectMax();
        float y = is_strikethrough_ ? std::floor( ( r_min.y + r_max.y ) * 0.5f ) : std::floor( r_max.y - 1.0f );
        ImGui::GetWindowDrawList()->AddLine( ImVec2( r_min.x, y ), ImVec2( r_max.x, y ), col_, 1.0f );
    }

    inline void UnderLine( ImColor col_ )
    {
        DrawTextDecorationLine( col_, false );
    }

    inline bool IsMarkdownEscapable( char c_ )
    {
        return ( c_ == '!' || c_ == '\"' || c_ == '#' || c_ == '$' || c_ == '%' || c_ == '&' ||
                 c_ == '\'' || c_ == '(' || c_ == ')' || c_ == '*' || c_ == '+' || c_ == ',' ||
                 c_ == '-' || c_ == '.' || c_ == '/' || c_ == ':' || c_ == ';' || c_ == '<' ||
                 c_ == '=' || c_ == '>' || c_ == '?' || c_ == '@' || c_ == '[' || c_ == '\\' ||
                 c_ == ']' || c_ == '^' || c_ == '_' || c_ == '`' || c_ == '{' || c_ == '|' ||
                 c_ == '}' || c_ == '~' );
    }

    // CommonMark 4.2: Strip optional closing sequence of '#' characters
    inline void TrimTrailingHeadingHashes( const char* text, const char*& textEnd )
    {
        while( textEnd > text && ( textEnd[-1] == ' ' || textEnd[-1] == '\t' || textEnd[-1] == '\r' ) )
        {
            --textEnd;
        }
        const char* hashScan = textEnd;
        while( hashScan > text && hashScan[-1] == '#' )
        {
            --hashScan;
        }
        if( hashScan < textEnd && ( hashScan == text || hashScan[-1] == ' ' || hashScan[-1] == '\t' ) )
        {
            textEnd = hashScan;
            while( textEnd > text && ( textEnd[-1] == ' ' || textEnd[-1] == '\t' ) )
            {
                --textEnd;
            }
        }
    }

    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ )
    {
        // Apply line-level leading space indentation (nested list depth)
        int numLeadIndents = line_.leadSpaceCount / 2;
        textRegion_.ApplyLeadIndent( numLeadIndents );

        // render
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.isPrecededBySeparator = line_.isPrecededBySeparator;
        formatInfo.isPrecededByBlankLine = line_.isPrecededByBlankLine;

        // Determine if this line/block is at the start of the markdown document (no preceding non-whitespace content)
        bool isAtStart = true;
        int checkLimit = line_.lineStart + line_.leadSpaceCount;
        for( int k = 0; k < checkLimit; ++k )
        {
            char ch = markdown_[k];
            if( ch == '\0' ) { break; }
            if( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
            {
                isAtStart = false;
                break;
            }
        }
        formatInfo.isStartOfDoc = isAtStart;

        int textStart = line_.lastRenderPosition + 1;
        int textSize = line_.lineEnd - textStart;
        if( line_.isUnorderedListStart )    // render unordered list
        {
            formatInfo.type = MarkdownFormatType::UNORDERED_LIST;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            const char* textEnd = text + textSize - 1;
            while( text < textEnd && *text == ' ' ) { ++text; }
            textRegion_.RenderListTextWrapped( text, textEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
        }
        else if( line_.isOrderedListStart ) // render ordered list (e.g. 1. , 2. )
        {
            formatInfo.type = MarkdownFormatType::ORDERED_LIST;
            mdConfig_.formatCallback( formatInfo, true );
            const char* numStart = markdown_ + line_.orderedListNumStart;
            const char* numEnd = markdown_ + line_.orderedListNumEnd;
            const char* text = markdown_ + textStart;
            const char* textEnd = text + textSize;
            while( text < textEnd && *text == ' ' ) { ++text; }
            textRegion_.RenderOrderedListTextWrapped( numStart, numEnd, text, textEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
        }
        else if( line_.isHeading )          // render heading
        {
            formatInfo.level = line_.headingCount;
            formatInfo.type = MarkdownFormatType::HEADING;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            const char* textEnd = markdown_ + line_.lineEnd;
            TrimTrailingHeadingHashes( text, textEnd );
            if( textEnd > text )
            {
                textRegion_.RenderTextWrapped( text, textEnd );
            }

            MarkdownHeadingFormat fmt;
            if( line_.headingCount > MarkdownConfig::NUMHEADINGS )
            {
                fmt = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
            }
            else
            {
                fmt = mdConfig_.headingFormats[ line_.headingCount - 1 ];
            }
            if( fmt.separator )
            {
                line_.hadSeparator = true;
            }
        }
		else if( line_.isEmphasis )         // render emphasis
		{
			formatInfo.level = line_.emphasisCount;
			formatInfo.emphasisChar = line_.emphasisChar;
			formatInfo.type = MarkdownFormatType::EMPHASIS;
			mdConfig_.formatCallback(formatInfo, true);
			const char* text = markdown_ + textStart;
			const char* textEnd = text + textSize;

			bool hasNestedCode = false;
			if( line_.emphasisChar != '`' && textSize > 2 )
			{
				for( const char* s = text; s < textEnd; ++s )
				{
					if( *s == '`' ) { hasNestedCode = true; break; }
				}
			}

			if( hasNestedCode )
			{
				const char* cur = text;
				bool renderedAny = false;
				while( cur < textEnd )
				{
					const char* bOpen = nullptr;
					for( const char* s = cur; s < textEnd; ++s )
					{
						if( *s == '`' && ( s == text || *(s - 1) != '\\' ) )
						{
							bOpen = s;
							break;
						}
					}
					if( !bOpen )
					{
						if( cur < textEnd )
						{
							if( renderedAny ) ImGui::SameLine( 0.0f, 0.0f );
							textRegion_.RenderTextWrapped( cur, textEnd );
						}
						break;
					}

					const char* bClose = nullptr;
					for( const char* s = bOpen + 1; s < textEnd; ++s )
					{
						if( *s == '`' && *(s - 1) != '\\' )
						{
							bClose = s;
							break;
						}
					}
					if( !bClose )
					{
						// No matching close backtick; render remainder as normal emphasized text
						if( cur < textEnd )
						{
							if( renderedAny ) ImGui::SameLine( 0.0f, 0.0f );
							textRegion_.RenderTextWrapped( cur, textEnd );
						}
						break;
					}

					// 1. Render preceding text segment if non-empty
					if( bOpen > cur )
					{
						if( renderedAny ) ImGui::SameLine( 0.0f, 0.0f );
						textRegion_.RenderTextWrapped( cur, bOpen );
						renderedAny = true;
					}

					// 2. Render code span segment without enclosing backtick delimiters
					if( renderedAny ) ImGui::SameLine( 0.0f, 0.0f );
					MarkdownFormatInfo codeInfo = formatInfo;
					codeInfo.type = MarkdownFormatType::EMPHASIS;
					codeInfo.emphasisChar = '`';
					codeInfo.level = 1;
					mdConfig_.formatCallback( codeInfo, true );
					textRegion_.RenderTextWrapped( bOpen + 1, bClose );
					mdConfig_.formatCallback( codeInfo, false );
					renderedAny = true;

					cur = bClose + 1;
				}
			}
			else
			{
				textRegion_.RenderTextWrapped( text, textEnd );
			}
		}
        else                                // render a normal paragraph chunk
        {
            bool isLineEmpty = true;
            for( int k = textStart; k < line_.lineEnd; ++k )
            {
                if( markdown_[k] != ' ' && markdown_[k] != '\t' && markdown_[k] != '\r' )
                {
                    isLineEmpty = false;
                    break;
                }
            }
            if( isLineEmpty )
            {
                line_.wasBlank = true;
                MarkdownDetail::FloatLayoutScope* fl_scope = MarkdownDetail::GetActiveFloatScope();
                if( fl_scope && fl_scope->justPlacedFloat )
                {
                    // Suppress the single blank separator line immediately following a floated image
                    // so the adjacent paragraph/heading aligns pixel-flush with the top of the float!
                    fl_scope->justPlacedFloat = false;
                    return;
                }
            }

            formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped( text, text + textSize );
        }
        mdConfig_.formatCallback( formatInfo, false );
    }

    inline void ExpandTabs( const char* line_start, const char* line_end, std::string& out, int tab_size = 4 )
    {
        out.clear();
        int col = 0;
        for( const char* p = line_start; p < line_end; ++p )
        {
            if( *p == '\t' )
            {
                int spaces = tab_size - ( col % tab_size );
                if( spaces == 0 ) spaces = tab_size;
                out.append( spaces, ' ' );
                col += spaces;
            }
            else
            {
                out.push_back( *p );
                ++col;
            }
        }
    }

    inline void DefaultRenderCodeLine( const std::string& line_str, bool is_lua, bool is_cpp, float wrap_screen_x = -FLT_MAX, float scale = 1.0f )
    {
        if( line_str.empty() )
        {
            ImGui::NewLine();
            return;
        }

        float line_start_x = ImGui::GetCursorScreenPos().x;
        const bool has_wrap = ( wrap_screen_x > line_start_x );

        if( !is_lua && !is_cpp )
        {
            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 220, 225, 235, 255 ) );
            if( has_wrap )
            {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                ImGui::PushTextWrapPos( wrap_local_x );
                ImGui::TextUnformatted( line_str.c_str() );
                ImGui::PopTextWrapPos();
            }
            else
            {
                ImGui::TextUnformatted( line_str.c_str() );
            }
            ImGui::PopStyleColor();
            return;
        }

        // Check leading whitespace
        size_t first_non_space = line_str.find_first_not_of( " \t" );
        float indent_w = 0.0f;
        if( first_non_space != std::string::npos && first_non_space > 0 )
        {
            indent_w = ImGui::CalcTextSize( line_str.c_str(), line_str.c_str() + first_non_space ).x;
        }
        float max_continuation = has_wrap ? ( ( wrap_screen_x - line_start_x ) * 0.5f ) : ( 32.0f * scale );
        float continuation_indent = ImMin( indent_w + 16.0f * scale, max_continuation );

        if( first_non_space != std::string::npos )
        {
            if( ( is_lua && first_non_space + 1 < line_str.length() && line_str[first_non_space] == '-' && line_str[first_non_space + 1] == '-' ) ||
                ( is_cpp && first_non_space + 1 < line_str.length() && line_str[first_non_space] == '/' && line_str[first_non_space + 1] == '/' ) )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 106, 153, 85, 255 ) ); // Comment green
                if( has_wrap )
                {
                    ImGuiWindow* win = ImGui::GetCurrentWindow();
                    float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                    ImGui::PushTextWrapPos( wrap_local_x );
                    ImGui::TextUnformatted( line_str.c_str() );
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::TextUnformatted( line_str.c_str() );
                }
                ImGui::PopStyleColor();
                return;
            }
            if( is_cpp && line_str[first_non_space] == '#' )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 198, 120, 221, 255 ) ); // Preprocessor purple
                if( has_wrap )
                {
                    ImGuiWindow* win = ImGui::GetCurrentWindow();
                    float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                    ImGui::PushTextWrapPos( wrap_local_x );
                    ImGui::TextUnformatted( line_str.c_str() );
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::TextUnformatted( line_str.c_str() );
                }
                ImGui::PopStyleColor();
                return;
            }
        }

        // Scan tokens across the line
        size_t pos = 0;
        size_t len = line_str.length();
        float min_x_for_wrap = line_start_x;

        auto renderToken = [&]( const char* text_start, const char* text_end, ImU32 col, bool is_last ) {
            float tok_w = ImGui::CalcTextSize( text_start, text_end ).x;
            float cur_x = ImGui::GetCursorScreenPos().x;

            if( has_wrap && ( cur_x + tok_w > wrap_screen_x ) && ( cur_x > min_x_for_wrap + 1.0f ) )
            {
                ImGui::NewLine();
                ImGui::SetCursorScreenPos( ImVec2( line_start_x + continuation_indent, ImGui::GetCursorScreenPos().y ) );
                min_x_for_wrap = line_start_x + continuation_indent;
            }

            ImGui::PushStyleColor( ImGuiCol_Text, col );
            if( has_wrap )
            {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                float wrap_local_x = wrap_screen_x - ( win->Pos.x - win->Scroll.x );
                ImGui::PushTextWrapPos( wrap_local_x );
                ImGui::TextUnformatted( text_start, text_end );
                ImGui::PopTextWrapPos();
            }
            else
            {
                ImGui::TextUnformatted( text_start, text_end );
            }
            ImGui::PopStyleColor();

            if( !is_last )
            {
                ImGui::SameLine( 0.0f, 0.0f );
            }
        };

        while( pos < len )
        {
            // 1. Comments:
            if( ( is_lua && pos + 1 < len && line_str[pos] == '-' && line_str[pos + 1] == '-' ) ||
                ( is_cpp && pos + 1 < len && line_str[pos] == '/' && line_str[pos + 1] == '/' ) )
            {
                renderToken( line_str.c_str() + pos, line_str.c_str() + len, IM_COL32( 106, 153, 85, 255 ), true );
                break;
            }

            // 2. Strings:
            if( line_str[pos] == '\"' || line_str[pos] == '\'' )
            {
                char q = line_str[pos];
                size_t q_end = pos + 1;
                while( q_end < len && line_str[q_end] != q )
                {
                    if( line_str[q_end] == '\\' && q_end + 1 < len ) ++q_end;
                    ++q_end;
                }
                if( q_end < len ) ++q_end;
                renderToken( line_str.c_str() + pos, line_str.c_str() + q_end, IM_COL32( 206, 145, 120, 255 ), q_end >= len );
                pos = q_end;
                continue;
            }

            // 3. Identifiers / words:
            if( isalpha( (unsigned char)line_str[pos] ) || line_str[pos] == '_' )
            {
                size_t w_end = pos;
                while( w_end < len && ( isalnum( (unsigned char)line_str[w_end] ) || line_str[w_end] == '_' ) )
                {
                    ++w_end;
                }
                std::string word = line_str.substr( pos, w_end - pos );
                bool is_kw = false;
                if( is_lua )
                {
                    static const char* kws[] = { "local", "function", "if", "then", "else", "elseif", "end", "return", "for", "while", "do", "break", "nil", "true", "false", nullptr };
                    for( int k = 0; kws[k]; ++k ) { if( word == kws[k] ) { is_kw = true; break; } }
                }
                else if( is_cpp )
                {
                    static const char* kws[] = { "int", "float", "double", "bool", "void", "char", "auto", "const", "class", "struct", "if", "else", "return", "for", "while", "true", "false", "nullptr", nullptr };
                    for( int k = 0; kws[k]; ++k ) { if( word == kws[k] ) { is_kw = true; break; } }
                }

                ImU32 col = is_kw ? IM_COL32( 198, 120, 221, 255 ) : IM_COL32( 220, 225, 235, 255 );
                renderToken( word.c_str(), word.c_str() + word.length(), col, w_end >= len );
                pos = w_end;
                continue;
            }

            // 4. Other characters (whitespace, punctuation, numbers):
            size_t other_end = pos;
            while( other_end < len && !isalpha( (unsigned char)line_str[other_end] ) && line_str[other_end] != '_' &&
                   line_str[other_end] != '\"' && line_str[other_end] != '\'' &&
                   !( is_lua && other_end + 1 < len && line_str[other_end] == '-' && line_str[other_end + 1] == '-' ) &&
                   !( is_cpp && other_end + 1 < len && line_str[other_end] == '/' && line_str[other_end + 1] == '/' ) )
            {
                ++other_end;
            }
            renderToken( line_str.c_str() + pos, line_str.c_str() + other_end, IM_COL32( 220, 225, 235, 255 ), other_end >= len );
            pos = other_end;
        }
    }

    namespace MarkdownDetail {
        typedef void (*CodeBlockTooltipFn)( ImGuiID id, bool hovered, const char* text, bool toggle_on_click );
        typedef void (*DisabledCopyTooltipFn)( ImGuiID id, bool hovered );
        typedef void (*ClearTooltipFn)();
        typedef bool (*MarkdownTooltipFn)( ImGuiID id, bool hovered, const char* text, float wrap_width, int delay_frames, bool is_clicked );
        typedef void (*TableTooltipFn)( ImGuiID id, bool hovered, const char* markdownText, bool is_clicked );
        typedef float (*MeasureInlineWidthFn)( const char* text, size_t len, const MarkdownConfig* config );
        typedef void (*RenderInlineCaptionFn)( const char* text, size_t len, const MarkdownConfig* config );
        typedef ImFont* (*CaptionFontResolverFn)();
        typedef bool (*DrawVectorIconFn)( ImDrawList* dl, ImVec2 pos, float size, ImU32 col, const char* name );

        inline CodeBlockTooltipFn& GetCodeBlockTooltipFn() {
            static CodeBlockTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline DisabledCopyTooltipFn& GetDisabledCopyTooltipFn() {
            static DisabledCopyTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline ClearTooltipFn& GetClearTooltipFn() {
            static ClearTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline MarkdownTooltipFn& GetMarkdownTooltipFn() {
            static MarkdownTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline TableTooltipFn& GetTableTooltipFn() {
            static TableTooltipFn s_Fn = nullptr;
            return s_Fn;
        }

        inline MeasureInlineWidthFn& GetMeasureInlineWidthFn() {
            static MeasureInlineWidthFn s_Fn = nullptr;
            return s_Fn;
        }

        inline RenderInlineCaptionFn& GetRenderInlineCaptionFn() {
            static RenderInlineCaptionFn s_Fn = nullptr;
            return s_Fn;
        }

        inline CaptionFontResolverFn& GetCaptionFontResolver() {
            static CaptionFontResolverFn s_Fn = nullptr;
            return s_Fn;
        }

        inline DrawVectorIconFn& GetDrawVectorIconFn() {
            static DrawVectorIconFn s_Fn = nullptr;
            return s_Fn;
        }

        inline float GetNativeFontPixelSize( ImFont* font ) {
            if( !font ) return 0.0f;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
            return ( font->LegacySize > 0.0f ) ? font->LegacySize : 0.0f;
#else
            return ( font->FontSize > 0.0f ) ? font->FontSize : 0.0f;
#endif
        }

        inline float GetActiveContentZoom() {
            ImFont* f = ImGui::GetFont();
            float cur_sz = ImGui::GetFontSize();
            float nat_sz = GetNativeFontPixelSize( f );
            if( cur_sz > 0.0f && nat_sz > 0.0f ) {
                return cur_sz / nat_sz;
            }
            float win_scale = ImGui::GetFontScale( false );
            return ( win_scale > 0.0f ) ? win_scale : 1.0f;
        }

        struct FullLineWidthContext {
            float width = 0.0f;
            bool  parentAligned = false;
        };

        inline FullLineWidthContext& GetActiveFullLineWidthContext() {
            static FullLineWidthContext s_Ctx;
            return s_Ctx;
        }

        struct ScopedFullLineWidth {
            FullLineWidthContext prev;
            ScopedFullLineWidth( float w, bool aligned ) : prev( GetActiveFullLineWidthContext() ) {
                FullLineWidthContext next;
                next.width = w;
                next.parentAligned = aligned;
                GetActiveFullLineWidthContext() = next;
            }
            ~ScopedFullLineWidth() {
                GetActiveFullLineWidthContext() = prev;
            }
        };

        inline float ResolveFullLineAvailWidth( const MarkdownConfig* cfg ) {
            const FullLineWidthContext& ctx = GetActiveFullLineWidthContext();
            if( ctx.width > 0.0f ) return ctx.width;
            if( cfg && cfg->wrapWidth > 0.0f ) return cfg->wrapWidth;
            if( FloatLayoutScope* fl = GetActiveFloatScope() ) {
                const float scope_w = fl->containerRightX - fl->containerLeftX;
                if( scope_w > 1.0f ) return scope_w;
            }
            return ImGui::GetContentRegionAvail().x;
        }

        // Nesting-safe background underlay helper: records draw command & index offsets before
        // rendering foreground content, then draws background primitives and rotates those
        // commands behind the foreground without opening an ImDrawListSplitter.
        struct ImDrawListUnderlayScope {
            ImDrawList* dl;
            bool        enabled;
            int         fg_cmd_start;
            int         fg_idx_start;
            int         bg_cmd_start;
            int         bg_idx_start;

            explicit ImDrawListUnderlayScope( ImDrawList* draw_list, bool enable = true )
                : dl( draw_list ), enabled( enable ), fg_cmd_start( 0 ), fg_idx_start( 0 ), bg_cmd_start( 0 ), bg_idx_start( 0 ) {
                if( !dl || !enabled ) return;
                if( dl->CmdBuffer.Size == 0 || dl->CmdBuffer.back().ElemCount > 0 || dl->CmdBuffer.back().UserCallback != nullptr ) {
                    dl->AddDrawCmd();
                }
                fg_cmd_start = dl->CmdBuffer.Size - 1;
                fg_idx_start = dl->IdxBuffer.Size;
            }

            void BeginBackground() {
                if( !dl || !enabled ) return;
                while( dl->CmdBuffer.Size > fg_cmd_start &&
                       dl->CmdBuffer.back().ElemCount == 0 &&
                       dl->CmdBuffer.back().UserCallback == nullptr ) {
                    dl->CmdBuffer.pop_back();
                }
                dl->AddDrawCmd();
                bg_cmd_start = dl->CmdBuffer.Size - 1;
                bg_idx_start = dl->IdxBuffer.Size;
            }

            void EndUnderlay() {
                if( !dl || !enabled ) return;
                while( dl->CmdBuffer.Size > bg_cmd_start &&
                       dl->CmdBuffer.back().ElemCount == 0 &&
                       dl->CmdBuffer.back().UserCallback == nullptr ) {
                    dl->CmdBuffer.pop_back();
                }
                const int total_cmds = dl->CmdBuffer.Size;
                const int total_idxs = dl->IdxBuffer.Size;
                if( bg_cmd_start > fg_cmd_start && total_cmds > bg_cmd_start &&
                    bg_idx_start > fg_idx_start && total_idxs > bg_idx_start ) {
                    unsigned int running_idx_offset = dl->CmdBuffer.Data[fg_cmd_start].IdxOffset;
                    std::rotate( dl->IdxBuffer.Data + fg_idx_start,
                                 dl->IdxBuffer.Data + bg_idx_start,
                                 dl->IdxBuffer.Data + total_idxs );
                    std::rotate( dl->CmdBuffer.Data + fg_cmd_start,
                                 dl->CmdBuffer.Data + bg_cmd_start,
                                 dl->CmdBuffer.Data + total_cmds );
                    for( int c = fg_cmd_start; c < total_cmds; ++c ) {
                        dl->CmdBuffer.Data[c].IdxOffset = running_idx_offset;
                        running_idx_offset += dl->CmdBuffer.Data[c].ElemCount;
                    }
                    dl->_IdxWritePtr = dl->IdxBuffer.Data + dl->IdxBuffer.Size;
                }
                if( dl->CmdBuffer.Size == 0 || dl->CmdBuffer.back().ElemCount > 0 || dl->CmdBuffer.back().UserCallback != nullptr ) {
                    dl->AddDrawCmd();
                }
            }
        };

        struct LightboxCaptionCacheEntry {
            ImGuiID popupId = 0;
            ImGuiID textHash = 0;
            int     maxWidthPx = 0;
            ImVec2  finalTextPos = ImVec2( 0.0f, 0.0f );
            bool    valid = false;
        };

        struct LightboxCaptionCache {
            LightboxCaptionCacheEntry entries[8];
            int                       nextSlot = 0;
        };

        inline LightboxCaptionCache& GetLightboxCaptionCache() {
            static LightboxCaptionCache s_Cache;
            return s_Cache;
        }

        struct LightboxViewState {
            ImGuiID popupId     = 0;
            float   zoom        = 1.0f;
            float   lastZoom    = 1.0f;
            ImVec2  pan         = ImVec2( 0.0f, 0.0f );
            bool    wasDragging = false;

            void ResetForPopup( ImGuiID id ) {
                popupId     = id;
                zoom        = 1.0f;
                lastZoom    = 1.0f;
                pan         = ImVec2( 0.0f, 0.0f );
                wasDragging = false;
            }

            static float ClampZoom( float z ) {
                return ( z < 0.50f ) ? 0.50f : ( ( z > 4.00f ) ? 4.00f : z );
            }

            bool SetZoom( float target ) {
                const float c = std::round( ClampZoom( target ) * 100.0f ) / 100.0f;
                const bool changed = ( std::fabs( c - zoom ) > 0.0005f );
                zoom = c;
                if( zoom <= 1.001f ) {
                    pan = ImVec2( 0.0f, 0.0f );
                }
                return changed;
            }

            bool ToggleResetZoom() {
                if( std::fabs( zoom - 1.0f ) > 0.005f ) {
                    lastZoom = zoom;
                    zoom     = 1.0f;
                    pan      = ImVec2( 0.0f, 0.0f );
                    return true;
                }
                const float prev = ClampZoom( lastZoom );
                if( std::fabs( prev - 1.0f ) > 0.005f ) {
                    zoom = prev;
                    if( zoom <= 1.001f ) {
                        pan = ImVec2( 0.0f, 0.0f );
                    }
                    return true;
                }
                return false;
            }
        };

        inline LightboxViewState& GetLightboxViewState() {
            static LightboxViewState s_View;
            return s_View;
        }

        struct LightboxTelemetryState {
            int     openFrame = -1;
            ImGuiID activePopupId = 0;
            bool    hasLightboxCaption = false;
            float   lightboxZoom = 1.0f;
            ImVec2  lightboxPan = ImVec2( 0.0f, 0.0f );
            ImVec2  zoomStripMin = ImVec2( 0.0f, 0.0f );
            ImVec2  zoomStripMax = ImVec2( 0.0f, 0.0f );
            ImVec2  closeButtonMin = ImVec2( 0.0f, 0.0f );
            ImVec2  closeButtonMax = ImVec2( 0.0f, 0.0f );
            ImVec2  lightboxImageMin = ImVec2( 0.0f, 0.0f );
            ImVec2  lightboxImageMax = ImVec2( 0.0f, 0.0f );
            ImVec2  lightboxCaptionPlateMin = ImVec2( 0.0f, 0.0f );
            ImVec2  lightboxCaptionPlateMax = ImVec2( 0.0f, 0.0f );
            ImVec2  lastInlineImageMin = ImVec2( 0.0f, 0.0f );
            ImVec2  lastInlineImageMax = ImVec2( 0.0f, 0.0f );
            ImVec2  lastCaptionMin = ImVec2( 0.0f, 0.0f );
            ImVec2  lastCaptionMax = ImVec2( 0.0f, 0.0f );
        };

        inline LightboxTelemetryState& GetLightboxTelemetryState() {
            static LightboxTelemetryState s_State;
            return s_State;
        }

        inline unsigned int NextWindowImageOrdinal( ImGuiWindow* cur_win ) {
            if( !cur_win ) return 0u;
            ImGuiWindow* root = cur_win->RootWindow ? cur_win->RootWindow : cur_win;
            ImGuiStorage* st = &root->StateStorage;
            const ImGuiID frame_key = root->GetID( "##imgee_md_img_ord_frame" );
            const ImGuiID count_key = root->GetID( "##imgee_md_img_ord_count" );
            const int cur_frame = ImGui::GetFrameCount();
            if( st->GetInt( frame_key, -1 ) != cur_frame ) {
                st->SetInt( frame_key, cur_frame );
                st->SetInt( count_key, 0 );
            }
            const int idx = st->GetInt( count_key, 0 );
            st->SetInt( count_key, idx + 1 );
            return static_cast<unsigned int>( idx );
        }

        inline bool IsEscapePressedCompat() {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            return ImGui::IsKeyPressed( ImGuiKey_Escape, false );
#else
            const int esc_idx = ImGui::GetIO().KeyMap[ ImGuiKey_Escape ];
            return ( esc_idx >= 0 ) ? ImGui::IsKeyPressed( esc_idx, false ) : false;
#endif
        }

        inline void ShowCodeBlockTooltip( ImGuiID id, bool hovered, const char* text, bool toggle_on_click = false ) {
            if( GetCodeBlockTooltipFn() ) {
                GetCodeBlockTooltipFn()( id, hovered, text, toggle_on_click );
            }
        }

        inline void ClearActiveTooltip() {
            if( GetClearTooltipFn() ) {
                GetClearTooltipFn()();
            }
        }

        inline void ShowTableTooltip( const MarkdownConfig& mdConfig, ImGuiID id, bool hovered, const char* markdownText, bool is_clicked = false ) {
            if( !mdConfig.enableTableTooltips || !markdownText || markdownText[0] == '\0' ) {
                return;
            }
            if( mdConfig.tableTooltipCallback ) {
                mdConfig.tableTooltipCallback( id, hovered && !is_clicked, markdownText, mdConfig.userData );
                return;
            }
            if( GetTableTooltipFn() ) {
                GetTableTooltipFn()( id, hovered, markdownText, is_clicked );
            }
            // Intentional silent fallback when TimedTooltip is omitted and no custom callback is supplied (UX Rule #6).
        }

        inline void ShowImageLightboxTooltip( const MarkdownConfig& mdConfig, ImGuiID id, bool hovered, const char* markdownText, bool is_clicked = false ) {
            if( !markdownText || markdownText[0] == '\0' ) {
                return;
            }
            if( mdConfig.tableTooltipCallback ) {
                mdConfig.tableTooltipCallback( id, hovered && !is_clicked, markdownText, mdConfig.userData );
                return;
            }
            if( GetTableTooltipFn() ) {
                GetTableTooltipFn()( id, hovered, markdownText, is_clicked );
            }
            // Intentional silent fallback when TimedTooltip is omitted and no custom callback is supplied (UX Rule #6).
        }

        inline bool ShowMarkdownTooltip( ImGuiID id, bool hovered, const char* text, float wrap_width = 400.0f, int delay_frames = -1, bool is_clicked = false ) {
            if( GetMarkdownTooltipFn() ) {
                return GetMarkdownTooltipFn()( id, hovered, text, wrap_width, delay_frames, is_clicked );
            }
            if( hovered && text && text[0] != '\0' ) {
                ImGui::BeginTooltip();
                ImGui::TextWrapped( "%s", text );
                ImGui::EndTooltip();
                return true;
            }
            return false;
        }
    }

    inline bool IsMarkdownLightboxOpen()
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        return ( st.openFrame == ImGui::GetFrameCount() ) && ( st.activePopupId != 0 );
    }

    inline ImGuiID GetActiveMarkdownLightboxID()
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        return ( st.openFrame == ImGui::GetFrameCount() ) ? st.activePopupId : 0;
    }

    inline void GetLastMarkdownImageRect( ImVec2* out_min, ImVec2* out_max )
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        if( out_min ) *out_min = st.lastInlineImageMin;
        if( out_max ) *out_max = st.lastInlineImageMax;
    }

    inline void GetLastMarkdownCaptionRect( ImVec2* out_min, ImVec2* out_max )
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        if( out_min ) *out_min = st.lastCaptionMin;
        if( out_max ) *out_max = st.lastCaptionMax;
    }

    inline bool IsMarkdownClipboardSupported( const MarkdownConfig& config )
    {
        // 1. Explicit host callback always satisfies clipboard support:
        if( config.setClipboardTextFn != nullptr )
        {
            return true;
        }

        // 2. Windows (IBM desktop plugin or _WIN32): Dear ImGui defaults to Win32 OpenClipboard/SetClipboardData
#if (defined(IBM) && IBM) || defined(_WIN32)
#if defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)
        return false;
#else
        return true;
#endif

        // 3. macOS (APL desktop plugin or __APPLE__): Requires IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS
#elif (defined(APL) && APL) || defined(__APPLE__)
#if defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)
        return true;
#else
        return false;
#endif

        // 4. Linux (LIN desktop plugin or __linux__): Vanilla Dear ImGui has no default X11 clipboard
#elif (defined(LIN) && LIN) || defined(__linux__)
        return false;
#else
        return false;
#endif
    }

    inline void RenderCodeBlock( const char* code_, size_t codeLength_, const char* lang_, size_t langLength_, const MarkdownConfig& mdConfig_, size_t block_index = 0, int fence_indent = 0 )
    {
        if( !code_ ) return;

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        if( !window ) return;

        float scale = ImGui::GetVisualScale();
        float pad_x = 12.0f * scale;
        float pad_y = 8.0f * scale;
        float rounding = 4.0f * scale;

        ImFont* ambient_font = ImGui::GetFont();
        float ambient_font_sz = ImGui::GetFontSize();
        ImFont* regular_font = mdConfig_.regularFont ? mdConfig_.regularFont : ( ( ambient_font && ambient_font != mdConfig_.monoFont ) ? ambient_font : ( ( ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->Fonts.Size > 0 ) ? ImGui::GetIO().Fonts->Fonts[0] : nullptr ) );
        float regular_font_sz = ambient_font_sz;
        ImFont* mono_font = mdConfig_.monoFont;
        if( mono_font )
        {
            ImGui::ImGeeCompat::PushFont( mono_font );
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );

        ImVec2 start_pos = ImGui::GetCursorScreenPos();
        float avail_width = ( mdConfig_.wrapWidth > 0.0f ) ? mdConfig_.wrapWidth : ImGui::GetContentRegionAvail().x;
        if( avail_width <= 20.0f ) avail_width = 200.0f;

        std::string lang_tag;
        if( langLength_ > 0 && lang_ )
        {
            lang_tag.assign( lang_, langLength_ );
        }
        bool is_markdown = ( lang_tag == "markdown" || lang_tag == "md" );
        bool is_lua = ( lang_tag == "lua" || lang_tag == "lua51" );
        bool is_cpp = ( lang_tag == "cpp" || lang_tag == "c" || lang_tag == "h" || lang_tag == "hpp" );
        bool is_plaintext = ( lang_tag == "text" || lang_tag == "plaintext" || lang_tag == "txt" );
        bool is_custom = ( mdConfig_.codeBlockCallback != nullptr );
        bool is_recognized = is_markdown || is_lua || is_cpp || is_plaintext || is_custom;

        // Generate unique, scroll-stable ID combining content hash and block index:
        ImU32 content_hash = ImHashData( code_, codeLength_ );
        char id_buf[48];
        snprintf( id_buf, sizeof(id_buf), "##cb_%08X_%zu", content_hash, block_index );
        ImGuiID code_id = ImGui::GetID( id_buf );
        ImGui::PushID( (int)code_id );

        ImGuiStorage* storage = ImGui::GetStateStorage();
        char prev_id_buf[32];
        snprintf( prev_id_buf, sizeof(prev_id_buf), "##cb_prev_%zu", block_index );
        ImGuiID preview_id = ImGui::GetID( prev_id_buf );

        bool is_preview = false;
        if( is_markdown && storage )
        {
            is_preview = storage->GetBool( code_id, false ) || storage->GetBool( preview_id, false );
        }

        float font_sz = ImGui::GetFontSize();
        float control_h = font_sz + 4.0f * scale;
        float header_h = 0.0f;
        if( lang_tag.length() > 0 || mdConfig_.showCodeCopyButton || is_markdown )
        {
            header_h = control_h + 8.0f * scale;
        }
        float control_y = start_pos.y + ( header_h - control_h ) * 0.5f + 0.5f * scale;

        ImDrawList* dl = window->DrawList;
        ImDrawListSplitter splitter;
        splitter.Split( dl, 2 );
        splitter.SetCurrentChannel( dl, 1 ); // Foreground text channel

        ImVec2 content_clip_min = ImVec2( start_pos.x, start_pos.y + header_h );
        ImVec2 content_clip_max = ImVec2( start_pos.x + avail_width, window->ClipRect.Max.y );
        dl->PushClipRect( content_clip_min, content_clip_max, true );

        ImGui::SetCursorScreenPos( ImVec2( start_pos.x + pad_x, start_pos.y + header_h + pad_y ) );
        ImGui::BeginGroup();

        if( is_markdown && is_preview )
        {
            if( mono_font ) ImGui::PopFont();
            MarkdownConfig preview_config = mdConfig_;
            preview_config.wrapWidth = avail_width - 2.0f * pad_x;
            if( fence_indent > 0 )
            {
                std::string stripped_preview;
                const char* cp = code_;
                const char* cend = code_ + codeLength_;
                while( cp < cend )
                {
                    const char* ls = cp;
                    while( cp < cend && *cp != '\n' && *cp != '\r' ) ++cp;
                    const char* le = cp;
                    int stripped = 0;
                    while( ls < le && stripped < fence_indent && ( *ls == ' ' || *ls == '\t' ) )
                    {
                        stripped += ( *ls == '\t' ) ? 4 : 1;
                        ++ls;
                    }
                    stripped_preview.append( ls, le - ls );
                    if( cp < cend && *cp == '\r' ) { stripped_preview.push_back( *cp ); ++cp; }
                    if( cp < cend && *cp == '\n' ) { stripped_preview.push_back( *cp ); ++cp; }
                }
                ImGui::Markdown::Render( stripped_preview.c_str(), stripped_preview.length(), preview_config );
            }
            else
            {
                ImGui::Markdown::Render( code_, codeLength_, preview_config );
            }
            if( mono_font ) ImGui::ImGeeCompat::PushFont( mono_font );
        }
        else if( mdConfig_.codeBlockCallback )
        {
            MarkdownCodeBlockData cbData;
            cbData.code = code_;
            cbData.codeLength = codeLength_;
            cbData.language = lang_;
            cbData.languageLength = langLength_;
            cbData.config = &mdConfig_;
            cbData.userData = mdConfig_.userData;
            mdConfig_.codeBlockCallback( cbData );
        }
        else
        {
            float wrap_screen_x = start_pos.x + avail_width - pad_x;
            const char* p = code_;
            const char* end = code_ + codeLength_;
            std::string line_buf;
            while( p < end )
            {
                const char* line_start = p;
                while( p < end && *p != '\n' && *p != '\r' ) ++p;
                const char* line_end = p;

                // CommonMark Sec 4.5: strip up to fence_indent leading spaces/tabs
                int stripped = 0;
                while( line_start < line_end && stripped < fence_indent && ( *line_start == ' ' || *line_start == '\t' ) )
                {
                    stripped += ( *line_start == '\t' ) ? 4 : 1;
                    ++line_start;
                }

                ExpandTabs( line_start, line_end, line_buf, mdConfig_.tabSize > 0 ? mdConfig_.tabSize : 4 );
                DefaultRenderCodeLine( line_buf, is_lua, is_cpp, wrap_screen_x, scale );

                if( p < end && *p == '\r' ) ++p;
                if( p < end && *p == '\n' ) ++p;
            }
        }

        // Add bottom container padding inside the group so CursorMaxPos accounts for it
        ImGui::Dummy( ImVec2( 0.0f, pad_y ) );
        ImGui::EndGroup();
        dl->PopClipRect();

        ImVec2 content_max = ImGui::GetItemRectMax();
        float content_w = ( content_max.x - start_pos.x ) + pad_x;

        ImVec2 raw_sz = ( regular_font && regular_font != mono_font )
            ? regular_font->CalcTextSizeA( regular_font_sz, FLT_MAX, -1.0f, "Raw" )
            : ImGui::CalcTextSize( "Raw" );
        ImVec2 prev_sz = ( regular_font && regular_font != mono_font )
            ? regular_font->CalcTextSizeA( regular_font_sz, FLT_MAX, -1.0f, "Preview" )
            : ImGui::CalcTextSize( "Preview" );
        float toggle_gap = 5.0f * scale;
        float toggle_track_h = ImMax( 11.0f * scale, font_sz * 0.72f );
        float toggle_track_w = toggle_track_h * 1.9f;
        float toggle_total_w = raw_sz.x + toggle_gap + toggle_track_w + toggle_gap + prev_sz.x + 4.0f * scale;

        float chip_w = ( lang_tag.length() > 0 ) ? ( ImGui::CalcTextSize( lang_tag.c_str() ).x + 12.0f * scale ) : 0.0f;
        float toggle_w = is_markdown ? toggle_total_w : 0.0f;

        float icon_w = 10.0f * scale;
        float icon_gap = 4.0f * scale;
        ImVec2 copy_max_text_sz = ImGui::CalcTextSize( "Copied!" );
        float fixed_copy_w = icon_w + icon_gap + copy_max_text_sz.x + 14.0f * scale;
        float copy_w = mdConfig_.showCodeCopyButton ? fixed_copy_w : 0.0f;

        float min_header_w = pad_x * 2.0f + chip_w + ( toggle_w > 0.0f ? ( toggle_w + 16.0f * scale ) : 0.0f ) + ( copy_w > 0.0f ? ( copy_w + 12.0f * scale ) : 0.0f );

        float needed_width = ImMax( min_header_w, content_w );

        // Persistent maximum width cache: prevents jarring container shrink when toggling between Raw and Preview!
        ImGuiID max_width_id = code_id + 0x5744; // 'WD' (width cache)
        float* p_max_w = storage ? storage->GetFloatRef( max_width_id, 0.0f ) : nullptr;
        if( p_max_w )
        {
            if( *p_max_w > avail_width )
            {
                *p_max_w = avail_width;
            }
            if( needed_width > *p_max_w )
            {
                *p_max_w = ImMin( needed_width, avail_width );
            }
            needed_width = ImMax( needed_width, *p_max_w );
        }

        float min_block_w = ImMin( 100.0f * scale, avail_width );
        float block_width = ImClamp( needed_width, min_block_w, avail_width );
        float block_height = content_max.y - start_pos.y;

        ImVec2 box_min = start_pos;
        ImVec2 box_max = ImVec2( start_pos.x + block_width, start_pos.y + block_height );

        splitter.SetCurrentChannel( dl, 0 ); // Background channel
        ImU32 bg_col = IM_COL32( 22, 22, 26, 240 );
        ImU32 border_col = IM_COL32( 60, 60, 70, 180 );
        dl->AddRectFilled( box_min, box_max, bg_col, rounding );

        if( header_h > 0.0f )
        {
            ImU32 header_bg = IM_COL32( 32, 33, 40, 240 );
            ImU32 divider_col = IM_COL32( 50, 52, 62, 200 );
            ImVec2 header_max( box_max.x, start_pos.y + header_h );
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM < 18200)
#ifndef ImDrawFlags_RoundCornersTop
#define ImDrawFlags_RoundCornersTop ( 1 | 2 ) // ImDrawCornerFlags_Top
#endif
#endif
            dl->AddRectFilled( box_min, header_max, header_bg, rounding, ImDrawFlags_RoundCornersTop );
            dl->AddLine( ImVec2( box_min.x, header_max.y ), header_max, divider_col, 1.0f * scale );
        }

        ImGui::MarkdownDetail::AddRectCompat( dl, box_min, box_max, border_col, rounding, 0, 1.0f * scale );

        splitter.SetCurrentChannel( dl, 1 ); // Foreground channel

        if( header_h > 0.0f )
        {
            // 1. Language pill on the LEFT (Steve TextLink with tooltip & hand cursor):
            if( lang_tag.length() > 0 )
            {
                ImVec2 chip_min( start_pos.x + pad_x, control_y );
                ImVec2 chip_max( chip_min.x + chip_w, control_y + control_h );

                ImGui::SetCursorScreenPos( chip_min );
                ImGui::PushID( (int)code_id + 3 );
                ImGui::InvisibleButton( "##lang_chip", ImVec2( chip_w, control_h ) );
                bool chip_hovered = ImGui::IsItemHovered();
                if( chip_hovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                }

                const char* tip_text = nullptr;
                if( mdConfig_.codeBlockDescriptionCallback )
                {
                    tip_text = mdConfig_.codeBlockDescriptionCallback( lang_, langLength_, mdConfig_.userData );
                }
                if( !tip_text )
                {
                    if( is_cpp ) tip_text = "C++ format (Syntax highlighted)";
                    else if( is_lua ) tip_text = "Lua format (Syntax highlighted)";
                    else if( is_markdown ) tip_text = "Markdown format (Supports live Preview)";
                    else if( is_plaintext ) tip_text = "Plain text format (Unformatted)";
                    else if( is_custom ) tip_text = "Custom host language syntax";
                    else tip_text = "Plain text (Unrecognized language)";
                }

                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 3, chip_hovered, tip_text, true );
                ImGui::PopID();

                // Style colors:
                ImU32 chip_bg = IM_COL32( 40, 42, 48, 180 );
                ImU32 chip_bg_hover = IM_COL32( 50, 52, 60, 210 );
                ImU32 chip_fg = IM_COL32( 150, 155, 165, 220 );

                if( is_lua )
                {
                    chip_bg = IM_COL32( 24, 75, 135, 230 );
                    chip_bg_hover = IM_COL32( 32, 95, 170, 245 );
                    chip_fg = IM_COL32( 215, 235, 255, 255 );
                }
                else if( is_cpp )
                {
                    chip_bg = IM_COL32( 40, 60, 140, 230 );
                    chip_bg_hover = IM_COL32( 52, 78, 175, 245 );
                    chip_fg = IM_COL32( 220, 230, 255, 255 );
                }
                else if( is_markdown )
                {
                    chip_bg = IM_COL32( 28, 95, 75, 230 );
                    chip_bg_hover = IM_COL32( 36, 120, 95, 245 );
                    chip_fg = IM_COL32( 200, 245, 225, 255 );
                }
                else if( is_plaintext )
                {
                    chip_bg = IM_COL32( 45, 60, 80, 220 );
                    chip_bg_hover = IM_COL32( 58, 76, 102, 245 );
                    chip_fg = IM_COL32( 210, 225, 235, 255 );
                }
                else if( is_custom )
                {
                    chip_bg = IM_COL32( 75, 45, 120, 230 );
                    chip_bg_hover = IM_COL32( 95, 58, 150, 245 );
                    chip_fg = IM_COL32( 235, 215, 255, 255 );
                }

                ImU32 cur_bg = chip_hovered ? chip_bg_hover : chip_bg;
                dl->AddRectFilled( chip_min, chip_max, cur_bg, 3.0f * scale );
                if( !is_recognized )
                {
                    ImGui::MarkdownDetail::AddRectCompat( dl, chip_min, chip_max, IM_COL32( 65, 68, 75, 180 ), 3.0f * scale, 0, 1.0f * scale );
                }

                ImVec2 tag_sz = ImGui::CalcTextSize( lang_tag.c_str() );
                float text_x = chip_min.x + ( chip_w - tag_sz.x ) * 0.5f;
                float text_y = control_y + ( control_h - font_sz ) * 0.5f;
                dl->AddText( ImVec2( text_x, text_y ), chip_fg, lang_tag.c_str() );
            }

            // 2. Copy button on far RIGHT (Actionable push-button with vector icon & tooltip):
            float now = (float)ImGui::GetTime();
            float last_time = storage ? storage->GetFloat( code_id, -1000.0f ) : -1000.0f;
            bool is_recently_copied = ( now >= last_time ) && ( ( now - last_time ) < 1.5f );
            const char* copy_label = is_recently_copied ? "Copied!" : "Copy";

            float cur_copy_x = box_max.x - fixed_copy_w - pad_x;

            bool is_clipboard_supported = IsMarkdownClipboardSupported( mdConfig_ );

            if( mdConfig_.showCodeCopyButton )
            {
                ImVec2 btn_min( cur_copy_x, control_y );
                ImVec2 btn_max( cur_copy_x + fixed_copy_w, control_y + control_h );

                ImGui::SetCursorScreenPos( btn_min );
                ImGui::PushID( (int)code_id + 2 );

                bool btn_clicked = false;
                bool btn_hovered = false;
                bool btn_active = false;

                if( is_clipboard_supported )
                {
                    btn_clicked = ImGui::InvisibleButton( "##copy_btn", ImVec2( fixed_copy_w, control_h ) );
                    btn_hovered = ImGui::IsItemHovered();
                    btn_active = ImGui::IsItemActive();

                    if( btn_hovered )
                    {
                        ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    }

                    if( btn_clicked )
                    {
                        std::string clip_text;
                        if( fence_indent > 0 )
                        {
                            const char* cp = code_;
                            const char* cend = code_ + codeLength_;
                            while( cp < cend )
                            {
                                const char* ls = cp;
                                while( cp < cend && *cp != '\n' && *cp != '\r' ) ++cp;
                                const char* le = cp;
                                int stripped = 0;
                                while( ls < le && stripped < fence_indent && ( *ls == ' ' || *ls == '\t' ) )
                                {
                                    stripped += ( *ls == '\t' ) ? 4 : 1;
                                    ++ls;
                                }
                                clip_text.append( ls, le - ls );
                                if( cp < cend && *cp == '\r' ) { clip_text.push_back( *cp ); ++cp; }
                                if( cp < cend && *cp == '\n' ) { clip_text.push_back( *cp ); ++cp; }
                            }
                        }
                        else
                        {
                            clip_text.assign( code_, codeLength_ );
                        }

                        if( mdConfig_.setClipboardTextFn )
                        {
                            mdConfig_.setClipboardTextFn( clip_text.c_str(), mdConfig_.userData );
                        }
                        ImGui::SetClipboardText( clip_text.c_str() );
                        if( storage )
                        {
                            storage->SetFloat( code_id, now );
                        }
                        is_recently_copied = true;
                        MarkdownDetail::ClearActiveTooltip();
                    }

                    // Rich TimedTooltip when clipboard is active
                    if( !is_recently_copied )
                    {
                        const char* copy_tip = is_markdown
                            ? "Copies raw markdown plain text to your clipboard (unformatted text)."
                            : "Copies the contents of this code block to your clipboard.";
                        MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 2, btn_hovered, copy_tip, false );
                    }
                }
                else
                {
                    ImGui::InvisibleButton( "##copy_btn_disabled", ImVec2( fixed_copy_w, control_h ) );
                    btn_hovered = ImGui::IsItemHovered();

                    if( MarkdownDetail::GetDisabledCopyTooltipFn() )
                    {
                        MarkdownDetail::GetDisabledCopyTooltipFn()( ImGui::GetItemID(), btn_hovered );
                    }
                    else if( btn_hovered )
                    {
                        ImGui::SetTooltip( "System clipboard integration is not enabled on this platform build." );
                    }
                }

                // Render Actionable Button Visuals:
                ImU32 btn_bg, btn_border, btn_text_col, icon_col;
                if( !is_clipboard_supported )
                {
                    btn_bg = IM_COL32( 36, 38, 44, 140 );
                    btn_border = IM_COL32( 55, 58, 66, 160 );
                    btn_text_col = IM_COL32( 115, 120, 130, 160 );
                    icon_col = IM_COL32( 105, 110, 120, 150 );
                }
                else if( is_recently_copied )
                {
                    btn_bg = IM_COL32( 28, 92, 45, 230 );
                    btn_border = IM_COL32( 45, 140, 70, 240 );
                    btn_text_col = IM_COL32( 180, 250, 195, 255 );
                    icon_col = IM_COL32( 180, 250, 195, 255 );
                }
                else if( btn_active )
                {
                    btn_bg = IM_COL32( 65, 70, 84, 255 );
                    btn_border = IM_COL32( 95, 105, 125, 255 );
                    btn_text_col = IM_COL32( 240, 243, 250, 255 );
                    icon_col = IM_COL32( 220, 230, 245, 255 );
                }
                else if( btn_hovered )
                {
                    btn_bg = IM_COL32( 52, 56, 68, 240 );
                    btn_border = IM_COL32( 85, 95, 115, 240 );
                    btn_text_col = IM_COL32( 235, 240, 248, 255 );
                    icon_col = IM_COL32( 210, 220, 235, 240 );
                }
                else
                {
                    btn_bg = IM_COL32( 40, 42, 50, 210 );
                    btn_border = IM_COL32( 65, 70, 82, 180 );
                    btn_text_col = IM_COL32( 195, 202, 215, 230 );
                    icon_col = IM_COL32( 165, 175, 192, 210 );
                }

                float btn_rounding = 3.5f * scale;
                dl->AddRectFilled( btn_min, btn_max, btn_bg, btn_rounding );
                ImGui::MarkdownDetail::AddRectCompat( dl, btn_min, btn_max, btn_border, btn_rounding, 0, 1.0f * scale );

                // Layout icon & text centered inside button:
                ImVec2 cur_text_sz = ImGui::CalcTextSize( copy_label );
                float total_inner_w = icon_w + icon_gap + cur_text_sz.x;
                float start_inner_x = btn_min.x + ( fixed_copy_w - total_inner_w ) * 0.5f;
                float icon_x = start_inner_x;
                float icon_y = btn_min.y + ( control_h - 10.0f * scale ) * 0.5f;

                // Draw Icon:
                if( is_recently_copied )
                {
                    // Checkmark vector icon (zero-bias mitered 3-point polyline)
                    const ImVec2 chk_pts[3] = {
                        ImVec2( icon_x, icon_y + 5.5f * scale ),
                        ImVec2( icon_x + 3.2f * scale, icon_y + 8.5f * scale ),
                        ImVec2( icon_x + 9.0f * scale, icon_y + 1.5f * scale )
                    };
                    ImGui::MarkdownDetail::StrokePolylineCompat( dl, chk_pts, 3, icon_col, 1.5f * scale, 0 );
                }
                else
                {
                    // Classic overlapping double-sheet copy icon
                    // Back page:
                    ImVec2 b_min( icon_x + 2.5f * scale, icon_y );
                    ImVec2 b_max( icon_x + 9.5f * scale, icon_y + 7.5f * scale );
                    ImGui::MarkdownDetail::AddRectCompat( dl, b_min, b_max, icon_col, 1.0f * scale, 0, 1.0f * scale );

                    // Front page:
                    ImVec2 f_min( icon_x, icon_y + 2.5f * scale );
                    ImVec2 f_max( icon_x + 7.0f * scale, icon_y + 10.0f * scale );
                    dl->AddRectFilled( f_min, f_max, btn_bg, 1.0f * scale ); // mask out back sheet
                    ImGui::MarkdownDetail::AddRectCompat( dl, f_min, f_max, icon_col, 1.0f * scale, 0, 1.0f * scale );
                }

                // Draw Label:
                float text_x = start_inner_x + icon_w + icon_gap;
                float text_y = btn_min.y + ( control_h - font_sz ) * 0.5f;
                dl->AddText( ImVec2( text_x, text_y ), btn_text_col, copy_label );

                ImGui::PopID();
            }

            // 3. Markdown Mode Toggle: Raw [===O] Preview
            if( is_markdown )
            {
                float toggle_x = ( mdConfig_.showCodeCopyButton ? cur_copy_x : ( box_max.x - pad_x ) ) - toggle_total_w - 14.0f * scale;

                ImGui::PushID( (int)code_id + 1 );
                ImVec2 toggle_start( toggle_x, control_y );

                // Zone 1: Raw text button
                ImGui::SetCursorScreenPos( toggle_start );
                if( ImGui::InvisibleButton( "##raw_zone", ImVec2( raw_sz.x + 2.0f * scale, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool raw_hovered = ImGui::IsItemHovered();
                if( raw_hovered ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 4, raw_hovered, "Switch to raw markdown text format (Unrendered plain text with markdown syntax).", false );

                // Zone 2: Switch track (middle)
                ImVec2 track_pos( toggle_start.x + raw_sz.x + 2.0f * scale + toggle_gap, control_y + ( control_h - toggle_track_h ) * 0.5f );
                ImGui::SetCursorScreenPos( ImVec2( track_pos.x, control_y ) );
                if( ImGui::InvisibleButton( "##track_zone", ImVec2( toggle_track_w, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool track_hovered = ImGui::IsItemHovered();
                if( track_hovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    if( !ImGui::Contrib::IsCurrentWindowScrollableY() )
                    {
                        ImGuiIO& io = ImGui::GetIO();
                        float wheel = ( io.MouseWheel != 0.0f ) ? io.MouseWheel : io.MouseWheelH;
                        if( wheel > 0.0f && !is_preview )
                        {
                            is_preview = true;
                            if( storage )
                            {
                                storage->SetBool( code_id, true );
                                storage->SetBool( preview_id, true );
                            }
                            io.MouseWheel = 0.0f;
                            io.MouseWheelH = 0.0f;
                            MarkdownDetail::ClearActiveTooltip();
                        }
                        else if( wheel < 0.0f && is_preview )
                        {
                            is_preview = false;
                            if( storage )
                            {
                                storage->SetBool( code_id, false );
                                storage->SetBool( preview_id, false );
                            }
                            io.MouseWheel = 0.0f;
                            io.MouseWheelH = 0.0f;
                            MarkdownDetail::ClearActiveTooltip();
                        }
                    }
                }
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 1, track_hovered, "Toggle between raw source markdown and rendered preview (Click or mouse wheel).", false );

                // Zone 3: Preview text button
                ImVec2 prev_pos( track_pos.x + toggle_track_w + toggle_gap, control_y );
                ImGui::SetCursorScreenPos( prev_pos );
                if( ImGui::InvisibleButton( "##prev_zone", ImVec2( prev_sz.x + 2.0f * scale, control_h ) ) )
                {
                    is_preview = !is_preview;
                    if( storage )
                    {
                        storage->SetBool( code_id, is_preview );
                        storage->SetBool( preview_id, is_preview );
                    }
                    MarkdownDetail::ClearActiveTooltip();
                }
                bool prev_hovered = ImGui::IsItemHovered();
                if( prev_hovered ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                MarkdownDetail::ShowCodeBlockTooltip( (int)code_id + 5, prev_hovered, "Switch to live rendered markdown preview (Formatted headings, lists, pills, and styles).", false );

                // Animation smoothing
                ImGuiID anim_id = ImGui::GetID( "##raw_prev_anim" );
                float* p_t = storage ? storage->GetFloatRef( anim_id, is_preview ? 1.0f : 0.0f ) : nullptr;
                float target_t = is_preview ? 1.0f : 0.0f;
                if( p_t )
                {
                    float speed = 12.0f;
                    float dt = ImGui::GetIO().DeltaTime;
                    if( *p_t < target_t ) *p_t = ImMin( target_t, *p_t + speed * dt );
                    else if( *p_t > target_t ) *p_t = ImMax( target_t, *p_t - speed * dt );
                }
                float t = p_t ? *p_t : target_t;

                // Visual Rendering:
                // Text "Raw"
                ImU32 raw_text_col = !is_preview ? IM_COL32( 240, 243, 250, 255 ) : IM_COL32( 135, 140, 150, 180 );
                float r_font_sz = ( regular_font && regular_font != mono_font ) ? regular_font_sz : font_sz;
                float raw_y = control_y + ( control_h - r_font_sz ) * 0.5f;
                if( regular_font && regular_font != mono_font )
                {
                    dl->AddText( regular_font, r_font_sz, ImVec2( toggle_start.x, raw_y ), raw_text_col, "Raw" );
                }
                else
                {
                    dl->AddText( ImVec2( toggle_start.x, raw_y ), raw_text_col, "Raw" );
                }

                // Track pill
                float track_r = toggle_track_h * 0.5f;
                ImVec2 pill_min = track_pos;
                ImVec2 pill_max( track_pos.x + toggle_track_w, track_pos.y + toggle_track_h );

                ImVec4 c_off( 0.16f, 0.17f, 0.20f, 0.90f );
                ImVec4 c_on( 0.15f, 0.35f, 0.65f, 0.95f );
                ImVec4 track_c(
                    c_off.x + ( c_on.x - c_off.x ) * t,
                    c_off.y + ( c_on.y - c_off.y ) * t,
                    c_off.z + ( c_on.z - c_off.z ) * t,
                    c_off.w + ( c_on.w - c_off.w ) * t
                );
                ImU32 track_col = ImGui::ColorConvertFloat4ToU32( track_c );
                dl->AddRectFilled( pill_min, pill_max, track_col, track_r );
                ImGui::MarkdownDetail::AddRectCompat( dl, pill_min, pill_max, IM_COL32( 70, 74, 85, 180 ), track_r, 0, 1.0f * scale );

                // Sliding thumb
                float thumb_r = track_r - 1.25f * scale;
                if( thumb_r < 2.0f ) thumb_r = 2.0f;
                float thumb_x_min = pill_min.x + track_r;
                float thumb_x_max = pill_max.x - track_r;
                float thumb_x = thumb_x_min + ( thumb_x_max - thumb_x_min ) * t;
                float thumb_y = pill_min.y + track_r;
                dl->AddCircleFilled( ImVec2( thumb_x, thumb_y ), thumb_r, IM_COL32( 250, 250, 252, 255 ) );
                dl->AddCircle( ImVec2( thumb_x, thumb_y ), thumb_r, IM_COL32( 0, 0, 0, 60 ), 0, 1.0f * scale );

                // Text "Preview"
                ImU32 prev_text_col = is_preview ? IM_COL32( 240, 243, 250, 255 ) : IM_COL32( 135, 140, 150, 180 );
                float prev_y = control_y + ( control_h - r_font_sz ) * 0.5f;
                if( regular_font && regular_font != mono_font )
                {
                    dl->AddText( regular_font, r_font_sz, ImVec2( prev_pos.x, prev_y ), prev_text_col, "Preview" );
                }
                else
                {
                    dl->AddText( ImVec2( prev_pos.x, prev_y ), prev_text_col, "Preview" );
                }

                ImGui::PopID();
            }
        }

        splitter.Merge( dl );

        if( mono_font )
        {
            ImGui::PopFont();
        }

        ImGui::PopID();

        // Align cursor to box bottom and submit spacing item cleanly with block_width
        ImGui::SetCursorScreenPos( ImVec2( start_pos.x, box_max.y ) );
        ImGui::Dummy( ImVec2( block_width, 4.0f * scale ) );
    }

    //-----------------------------------------------------------------------------
    // GitHub Flavored Markdown (GFM) Tables
    //-----------------------------------------------------------------------------

    enum class TableColumnAlign
    {
        LEFT,
        CENTER,
        RIGHT
    };

    struct TableCell
    {
        const char* start = nullptr;
        const char* end = nullptr;

        size_t size() const { return ( end >= start ) ? (size_t)( end - start ) : 0; }
        bool empty() const { return size() == 0; }
    };

    namespace TableDetail
    {
        inline bool SplitRow( const char* line_start, const char* line_end, std::vector<TableCell>& out_cells )
        {
            out_cells.clear();
            if( !line_start || line_start >= line_end ) return false;

            // 1. Trim leading spaces/tabs
            while( line_start < line_end && ( *line_start == ' ' || *line_start == '\t' ) ) ++line_start;
            // 2. Trim trailing spaces/tabs/newlines
            while( line_end > line_start && ( line_end[-1] == ' ' || line_end[-1] == '\t' || line_end[-1] == '\r' || line_end[-1] == '\n' ) ) --line_end;

            if( line_start >= line_end ) return false;

            // 3. Skip optional leading '|'
            if( *line_start == '|' )
            {
                ++line_start;
            }

            // 4. Strip optional trailing '|' (must not be escaped with '\')
            if( line_end > line_start && line_end[-1] == '|' )
            {
                bool is_escaped = false;
                const char* check = line_end - 2;
                while( check >= line_start && *check == '\\' )
                {
                    is_escaped = !is_escaped;
                    --check;
                }
                if( !is_escaped )
                {
                    --line_end;
                }
            }

            // 5. Scan cells split by unescaped '|' (respecting backticks)
            const char* cell_start = line_start;
            bool in_code = false;
            bool is_escaped = false;

            for( const char* p = line_start; p < line_end; ++p )
            {
                char c = *p;
                if( is_escaped )
                {
                    is_escaped = false;
                    continue;
                }
                if( c == '\\' )
                {
                    is_escaped = true;
                    continue;
                }
                if( c == '`' )
                {
                    in_code = !in_code;
                    continue;
                }
                if( c == '|' && !in_code )
                {
                    const char* cs = cell_start;
                    const char* ce = p;
                    while( cs < ce && ( *cs == ' ' || *cs == '\t' ) ) ++cs;
                    while( ce > cs && ( ce[-1] == ' ' || ce[-1] == '\t' ) ) --ce;
                    TableCell cell;
                    cell.start = cs;
                    cell.end = ce;
                    out_cells.push_back( cell );
                    cell_start = p + 1;
                }
            }

            // Final cell
            const char* cs = cell_start;
            const char* ce = line_end;
            while( cs < ce && ( *cs == ' ' || *cs == '\t' ) ) ++cs;
            while( ce > cs && ( ce[-1] == ' ' || ce[-1] == '\t' ) ) --ce;
            TableCell last_cell;
            last_cell.start = cs;
            last_cell.end = ce;
            out_cells.push_back( last_cell );

            return !out_cells.empty();
        }

        inline bool ParseDelimiterCell( const TableCell& cell, TableColumnAlign& out_align )
        {
            if( cell.empty() ) return false;
            const char* start = cell.start;
            const char* end = cell.end;

            while( start < end && ( *start == ' ' || *start == '\t' ) ) ++start;
            while( end > start && ( end[-1] == ' ' || end[-1] == '\t' ) ) --end;
            if( start >= end ) return false;

            bool left_colon = ( *start == ':' );
            if( left_colon ) ++start;

            bool right_colon = ( end > start && end[-1] == ':' );
            if( right_colon ) --end;

            if( start >= end ) return false; // Must have at least one hyphen

            for( const char* k = start; k < end; ++k )
            {
                if( *k != '-' ) return false;
            }

            if( left_colon && right_colon )
            {
                out_align = TableColumnAlign::CENTER;
            }
            else if( right_colon )
            {
                out_align = TableColumnAlign::RIGHT;
            }
            else
            {
                out_align = TableColumnAlign::LEFT;
            }
            return true;
        }

        inline bool ParseDelimiterRow( const char* line_start, const char* line_end, std::vector<TableColumnAlign>& out_alignments )
        {
            out_alignments.clear();
            std::vector<TableCell> cells;
            if( !SplitRow( line_start, line_end, cells ) || cells.empty() ) return false;

            for( size_t i = 0; i < cells.size(); ++i )
            {
                TableColumnAlign align;
                if( !ParseDelimiterCell( cells[i], align ) )
                {
                    out_alignments.clear();
                    return false;
                }
                out_alignments.push_back( align );
            }
            return true;
        }

        inline bool CheckLineHasPipe( const char* line_start, const char* line_end )
        {
            bool in_code = false;
            bool is_escaped = false;
            for( const char* p = line_start; p < line_end; ++p )
            {
                char c = *p;
                if( is_escaped ) { is_escaped = false; continue; }
                if( c == '\\' ) { is_escaped = true; continue; }
                if( c == '`' ) { in_code = !in_code; continue; }
                if( c == '|' && !in_code ) return true;
            }
            return false;
        }

        inline int FindLineEnd( const char* md, int start, int len )
        {
            int pos = start;
            while( pos < len && md[pos] != '\r' && md[pos] != '\n' )
            {
                ++pos;
            }
            return pos;
        }

        inline int SkipNewline( const char* md, int line_end, int len )
        {
            int pos = line_end;
            if( pos < len && md[pos] == '\r' && pos + 1 < len && md[pos + 1] == '\n' )
            {
                return pos + 2;
            }
            else if( pos < len && ( md[pos] == '\r' || md[pos] == '\n' ) )
            {
                return pos + 1;
            }
            return pos;
        }

        inline bool StrCaseEqualN( const char* a, const char* b, size_t n )
        {
            for( size_t i = 0; i < n; ++i )
            {
                if( tolower( (unsigned char)a[i] ) != tolower( (unsigned char)b[i] ) )
                    return false;
            }
            return true;
        }

        inline float CalcCellVisualWidth( const TableCell& cell, bool is_header, const MarkdownConfig& mdConfig_ )
        {
            if( cell.empty() ) return 0.0f;

            const char* p = cell.start;
            const char* end = cell.end;
            while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
            while( end > p && ( end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n' ) ) --end;
            if( p >= end ) return 0.0f;

            float scale = ImGui::GetVisualScale();

            // If cell begins with an HTML-style tag: <tag_name ...>inner_text</tag_name>
            if( *p == '<' )
            {
                const char* tag_close = (const char*)memchr( p, '>', end - p );
                if( tag_close && tag_close < end )
                {
                    // Find tag name
                    const char* tn_start = p + 1;
                    while( tn_start < tag_close && ( *tn_start == ' ' || *tn_start == '\t' ) ) ++tn_start;
                    const char* tn_end = tn_start;
                    while( tn_end < tag_close && *tn_end != ' ' && *tn_end != '=' && *tn_end != '\t' ) ++tn_end;

                    size_t tn_len = (size_t)( tn_end - tn_start );
                    std::string tag_name( tn_start, tn_len );
                    for( size_t k = 0; k < tag_name.size(); ++k )
                        tag_name[k] = (char)tolower( (unsigned char)tag_name[k] );

                    // Look for closing tag </tag_name>
                    std::string close_tag = "</" + tag_name + ">";
                    const char* content_start = tag_close + 1;
                    const char* content_end = nullptr;
                    const char* found_close = nullptr;

                    for( const char* s = content_start; s + close_tag.length() <= end; ++s )
                    {
                        if( StrCaseEqualN( s, close_tag.c_str(), close_tag.length() ) )
                        {
                            content_end = s;
                            found_close = s;
                            break;
                        }
                    }

                    if( found_close )
                    {
                        // Check font for inner text
                        ImFont* chosen_font = nullptr;
                        if( is_header )
                        {
                            if( mdConfig_.boldFont ) chosen_font = mdConfig_.boldFont;
                            else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
                                chosen_font = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
                        }
                        else if( ( tag_name == "mono" || tag_name == "code" ) && mdConfig_.monoFont )
                        {
                            chosen_font = mdConfig_.monoFont;
                        }
                        else if( mdConfig_.regularFont )
                        {
                            chosen_font = mdConfig_.regularFont;
                        }

                        ImVec2 inner_sz;
                        if( chosen_font )
                        {
                            ImGui::ImGeeCompat::PushFont( chosen_font );
                            inner_sz = ImGui::CalcTextSize( content_start, content_end );
                            ImGui::PopFont();
                        }
                        else
                        {
                            inner_sz = ImGui::CalcTextSize( content_start, content_end );
                        }

                        float visual_w = inner_sz.x;
                        if( tag_name == "pill" )
                        {
                            float pad_x = 7.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "badge" || tag_name == "tag" )
                        {
                            float pad_x = 5.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "kbd" || tag_name == "key" || tag_name == "kdb" )
                        {
                            float pad_x = 5.5f * scale;
                            visual_w += pad_x * 2.0f;
                        }
                        else if( tag_name == "btn" )
                        {
                            visual_w += ImGui::GetStyle().FramePadding.x * 2.0f * scale;
                        }
                        else if( tag_name == "mono" || tag_name == "code" || tag_name == "backdrop" || tag_name == "bg" || tag_name == "highlight" )
                        {
                            float pad_x = 3.0f * scale;
                            visual_w += pad_x * 2.0f;
                        }

                        // Any trailing content after closing tag? (e.g. "<pill>OK</pill> extra")
                        const char* after = found_close + close_tag.length();
                        while( after < end && ( *after == ' ' || *after == '\t' ) ) ++after;
                        if( after < end )
                        {
                            visual_w += ImGui::GetStyle().ItemSpacing.x * scale + ImGui::CalcTextSize( after, end ).x;
                        }

                        return visual_w;
                    }
                }
            }

            // Check if markdown link [text](url) or {text}(url)
            if( ( *p == '[' || *p == '{' ) )
            {
                char close_b = ( *p == '[' ) ? ']' : '}';
                const char* b_close = (const char*)memchr( p, close_b, end - p );
                if( b_close && b_close + 1 < end && *(b_close + 1) == '(' )
                {
                    const char* link_text_start = p + 1;
                    const char* link_text_end = b_close;
                    if( is_header && mdConfig_.boldFont )
                    {
                        ImGui::ImGeeCompat::PushFont( mdConfig_.boldFont );
                        ImVec2 sz = ImGui::CalcTextSize( link_text_start, link_text_end );
                        ImGui::PopFont();
                        return sz.x;
                    }
                    return ImGui::CalcTextSize( link_text_start, link_text_end ).x;
                }
            }

            // Markdown formatting symbols: strip leading/trailing **, *, __, _, ~~, ++
            const char* s = p;
            const char* e = end;
            while( s < e && ( *s == ' ' || *s == '\t' ) ) ++s;
            while( e > s && ( e[-1] == ' ' || e[-1] == '\t' ) ) --e;

            bool is_bold = ( ( e - s >= 4 && s[0] == '*' && s[1] == '*' && e[-1] == '*' && e[-2] == '*' ) ||
                             ( e - s >= 4 && s[0] == '_' && s[1] == '_' && e[-1] == '_' && e[-2] == '_' ) );
            bool is_mono = ( e - s >= 2 && s[0] == '`' && e[-1] == '`' );

            while( e > s && ( e[-1] == '*' || e[-1] == '_' || e[-1] == '~' || e[-1] == '+' || e[-1] == '`' ) ) --e;
            while( s < e && ( *s == '*' || *s == '_' || *s == '~' || *s == '+' || *s == '`' ) ) ++s;
            while( s < e && ( *s == ' ' || *s == '\t' ) ) ++s;
            while( e > s && ( e[-1] == ' ' || e[-1] == '\t' ) ) --e;

            ImFont* chosen_font = nullptr;
            if( is_header )
            {
                if( mdConfig_.boldFont ) chosen_font = mdConfig_.boldFont;
                else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
                    chosen_font = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
            }
            else if( is_bold && mdConfig_.boldFont )
            {
                chosen_font = mdConfig_.boldFont;
            }
            else if( is_mono && mdConfig_.monoFont )
            {
                chosen_font = mdConfig_.monoFont;
            }
            else if( mdConfig_.regularFont )
            {
                chosen_font = mdConfig_.regularFont;
            }

            ImVec2 sz;
            if( chosen_font )
            {
                MarkdownDetail::PushMarkdownFont( chosen_font );
                sz = ImGui::CalcTextSize( s, e );
                ImGui::PopFont();
            }
            else
            {
                sz = ImGui::CalcTextSize( s, e );
            }

            if( is_mono )
            {
                sz.x += 6.0f * scale; // monospace code padding
            }

            return sz.x;
        }

        inline std::string GetCellPlainText( const TableCell& cell )
        {
            if( cell.empty() ) return "";
            const char* p = cell.start;
            const char* end = cell.end;
            while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
            while( end > p && ( end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n' ) ) --end;

            std::string out;
            out.reserve( (size_t)( end - p ) );
            while( p < end )
            {
                if( *p == '<' )
                {
                    const char* close = (const char*)memchr( p, '>', end - p );
                    if( close && close < end )
                    {
                        p = close + 1;
                        continue;
                    }
                }
                if( *p != '*' && *p != '_' && *p != '`' && *p != '~' )
                {
                    out.push_back( *p );
                }
                ++p;
            }
            size_t s = 0;
            while( s < out.size() && ( out[s] == ' ' || out[s] == '\t' ) ) ++s;
            size_t e = out.size();
            while( e > s && ( out[e - 1] == ' ' || out[e - 1] == '\t' ) ) --e;
            return out.substr( s, e - s );
        }

        inline int NaturalCompare( const std::string& a, const std::string& b )
        {
            size_t i = 0, j = 0;
            while( i < a.size() && j < b.size() )
            {
                // Skip thousand-separator commas in numbers (e.g. 5,500 vs 12,000)
                if( a[i] == ',' && i + 1 < a.size() && isdigit((unsigned char)a[i+1]) && i > 0 && isdigit((unsigned char)a[i-1]) )
                {
                    i++;
                    continue;
                }
                if( b[j] == ',' && j + 1 < b.size() && isdigit((unsigned char)b[j+1]) && j > 0 && isdigit((unsigned char)b[j-1]) )
                {
                    j++;
                    continue;
                }

                bool a_is_num = isdigit( (unsigned char)a[i] ) || ( ( a[i] == '+' || a[i] == '-' ) && i + 1 < a.size() && isdigit( (unsigned char)a[i+1] ) );
                bool b_is_num = isdigit( (unsigned char)b[j] ) || ( ( b[j] == '+' || b[j] == '-' ) && j + 1 < b.size() && isdigit( (unsigned char)b[j+1] ) );

                if( a_is_num && b_is_num )
                {
                    std::string subA, subB;
                    size_t k = i;
                    while( k < a.size() && ( isdigit((unsigned char)a[k]) || a[k] == '.' || a[k] == '+' || a[k] == '-' || a[k] == ',' ) )
                    {
                        if( a[k] != ',' ) subA.push_back( a[k] );
                        k++;
                    }
                    size_t advA = k - i;
                    k = j;
                    while( k < b.size() && ( isdigit((unsigned char)b[k]) || b[k] == '.' || b[k] == '+' || b[k] == '-' || b[k] == ',' ) )
                    {
                        if( b[k] != ',' ) subB.push_back( b[k] );
                        k++;
                    }
                    size_t advB = k - j;
                    char* endA = nullptr;
                    char* endB = nullptr;
                    double vA = strtod( subA.c_str(), &endA );
                    double vB = strtod( subB.c_str(), &endB );
                    if( vA < vB ) return -1;
                    if( vA > vB ) return 1;
                    i += advA;
                    j += advB;
                    continue;
                }

                char ca = (char)tolower( (unsigned char)a[i] );
                char cb = (char)tolower( (unsigned char)b[j] );
                if( ca < cb ) return -1;
                if( ca > cb ) return 1;
                i++;
                j++;
            }
            if( i < a.size() ) return 1;
            if( j < b.size() ) return -1;
            return 0;
        }

        inline int CompareCells( const TableCell& a, const TableCell& b )
        {
            if( a.empty() && b.empty() ) return 0;
            if( a.empty() ) return 1;
            if( b.empty() ) return -1;

            std::string strA = GetCellPlainText( a );
            std::string strB = GetCellPlainText( b );
            return NaturalCompare( strA, strB );
        }

        inline bool StrCaseEqual( const std::string& a, const std::string& b )
        {
            if( a.size() != b.size() ) return false;
            for( size_t i = 0; i < a.size(); ++i )
            {
                if( tolower( (unsigned char)a[i] ) != tolower( (unsigned char)b[i] ) )
                    return false;
            }
            return true;
        }

        inline bool IsSuppressedTooltipValue( const std::string& val )
        {
            return StrCaseEqual( val, "off" ) ||
                   StrCaseEqual( val, "false" ) ||
                   StrCaseEqual( val, "none" ) ||
                   StrCaseEqual( val, "0" );
        }

        inline bool IsEnabledTooltipValue( const std::string& val )
        {
            return StrCaseEqual( val, "on" ) ||
                   StrCaseEqual( val, "true" ) ||
                   StrCaseEqual( val, "auto" ) ||
                   StrCaseEqual( val, "default" ) ||
                   StrCaseEqual( val, "1" );
        }

        inline bool TryParseIndexedTooltipKey( const std::string& key, int& out_col_idx )
        {
            // Supports 0-based column tooltip keys:
            // tip0, tooltip0, tip-0, tooltip-0, col0-tooltip, col0_tooltip, header0-tooltip, header0_tooltip
            std::string num_str;
            if( key.rfind( "tooltip-", 0 ) == 0 || key.rfind( "tooltip_", 0 ) == 0 )
                num_str = key.substr( 8 );
            else if( key.rfind( "tooltip", 0 ) == 0 && key.size() > 7 )
                num_str = key.substr( 7 );
            else if( key.rfind( "tip-", 0 ) == 0 || key.rfind( "tip_", 0 ) == 0 )
                num_str = key.substr( 4 );
            else if( key.rfind( "tip", 0 ) == 0 && key.size() > 3 )
                num_str = key.substr( 3 );
            else if( key.rfind( "col", 0 ) == 0 || key.rfind( "header", 0 ) == 0 )
            {
                size_t prefix_len = ( key.rfind( "header", 0 ) == 0 ) ? 6 : 3;
                size_t dash = key.find_first_of( "-_", prefix_len );
                if( dash != std::string::npos && dash > prefix_len )
                {
                    std::string suffix = key.substr( dash + 1 );
                    if( suffix == "tooltip" || suffix == "tip" )
                    {
                        num_str = key.substr( prefix_len, dash - prefix_len );
                    }
                }
            }

            if( num_str.empty() ) return false;
            for( size_t i = 0; i < num_str.size(); ++i )
            {
                if( !isdigit( (unsigned char)num_str[i] ) ) return false;
            }
            int idx = atoi( num_str.c_str() );
            if( idx < 0 || idx >= 64 ) return false;
            out_col_idx = idx;
            return true;
        }

        struct MarkdownTableMetadata
        {
            // Author baseline preset (<!-- table-preset: ... -->)
            bool                hasPreset = false;
            float               presetWidth = 0.0f;
            int                 presetSortCol = -1;
            int                 presetSortDir = 0;
            std::string         presetSortColName;
            std::vector<float>  presetColWidths;
            bool                enableTooltips = true;
            std::vector<std::string> colHeaderTooltips;

            // User active / saved state (<!-- table-state: ... --> or <!-- table: ... -->)
            bool                hasState = false;
            float               stateWidth = 0.0f;
            int                 stateSortCol = -1;
            int                 stateSortDir = 0;
            std::string         stateSortColName;
            std::vector<float>  stateColWidths;

            // Byte offsets within markdown_
            int                 tableStart = 0;
            int                 rowsEnd = 0;
            int                 presetCommentStart = -1;
            int                 presetCommentEnd = -1;
            int                 stateCommentStart = -1;
            int                 stateCommentEnd = -1;
            int                 fullTableEnd = 0;
        };

        inline void ParseTableDirective(
            const char* p_start, size_t len,
            bool is_preset,
            MarkdownTableMetadata& meta,
            const std::vector<TableCell>& headers )
        {
            const char* p = p_start;
            const char* end = p_start + len;

            float& target_w = is_preset ? meta.presetWidth : meta.stateWidth;
            int& target_sort_col = is_preset ? meta.presetSortCol : meta.stateSortCol;
            int& target_sort_dir = is_preset ? meta.presetSortDir : meta.stateSortDir;
            std::string& target_col_name = is_preset ? meta.presetSortColName : meta.stateSortColName;
            std::vector<float>& target_cols = is_preset ? meta.presetColWidths : meta.stateColWidths;

            if( is_preset ) meta.hasPreset = true;
            else meta.hasState = true;

            while( p < end )
            {
                while( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',' || *p == ';' ) ) ++p;
                if( p >= end ) break;

                const char* tok_start = p;
                while( p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '=' && *p != ',' && *p != ';' ) ++p;
                std::string key( tok_start, (size_t)( p - tok_start ) );
                for( char& c : key ) c = (char)tolower( (unsigned char)c );

                std::string val;
                if( p < end && *p == '=' )
                {
                    ++p; // Skip '='
                    while( p < end && ( *p == ' ' || *p == '\t' ) ) ++p;
                    if( p < end && ( *p == '"' || *p == '\'' ) )
                    {
                        char q = *p++;
                        const char* v_start = p;
                        while( p < end && *p != q ) ++p;
                        val.assign( v_start, (size_t)( p - v_start ) );
                        if( p < end && *p == q ) ++p;
                    }
                    else
                    {
                        const char* v_start = p;
                        while( p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != ',' && *p != ';' ) ++p;
                        val.assign( v_start, (size_t)( p - v_start ) );
                    }
                }

                if( key == "width" || key == "w" )
                {
                    if( !val.empty() )
                    {
                        if( val == "auto" || val == "default" )
                        {
                            target_w = 0.0f;
                        }
                        else
                        {
                            target_w = (float)atof( val.c_str() );
                        }
                    }
                }
                else if( key == "cols" || key == "columns" )
                {
                    target_cols.clear();
                    const char* cp = val.c_str();
                    while( *cp )
                    {
                        while( *cp == ' ' || *cp == ',' || *cp == ';' ) ++cp;
                        if( !*cp ) break;
                        float cw = (float)atof( cp );
                        target_cols.push_back( cw );
                        while( *cp && *cp != ',' && *cp != ';' && *cp != ' ' ) ++cp;
                    }
                }
                else if( key == "sort" )
                {
                    if( !val.empty() )
                    {
                        if( val == "none" || val == "default" || val == "-1" )
                        {
                            target_sort_col = -1;
                            target_sort_dir = 0;
                            target_col_name.clear();
                        }
                        else
                        {
                            bool is_num = true;
                            for( size_t k = 0; k < val.size(); ++k )
                            {
                                if( k == 0 && ( val[k] == '-' || val[k] == '+' ) ) continue;
                                if( !isdigit( (unsigned char)val[k] ) ) { is_num = false; break; }
                            }

                            if( is_num )
                            {
                                int idx = atoi( val.c_str() );
                                target_sort_col = idx;
                                if( idx >= 0 && (size_t)idx < headers.size() )
                                {
                                    target_col_name = GetCellPlainText( headers[idx] );
                                }
                            }
                            else
                            {
                                target_col_name = val;
                                target_sort_col = -1;
                                for( size_t c = 0; c < headers.size(); ++c )
                                {
                                    std::string h_name = GetCellPlainText( headers[c] );
                                    if( StrCaseEqual( h_name, val ) )
                                    {
                                        target_sort_col = (int)c;
                                        break;
                                    }
                                }
                            }

                            if( target_sort_dir == 0 )
                            {
                                target_sort_dir = 1;
                            }
                        }
                    }
                }
                else if( key == "dir" || key == "direction" )
                {
                    std::string d = val;
                    for( char& c : d ) c = (char)tolower( (unsigned char)c );
                    if( d == "asc" || d == "ascending" || d == "up" )
                        target_sort_dir = 1;
                    else if( d == "desc" || d == "descending" || d == "down" )
                        target_sort_dir = 2;
                    else if( d == "none" || d == "default" )
                        target_sort_dir = 0;
                }
                else if( key == "asc" || key == "ascending" )
                {
                    target_sort_dir = 1;
                }
                else if( key == "desc" || key == "descending" )
                {
                    target_sort_dir = 2;
                }
                else if( key == "none" )
                {
                    target_sort_dir = 0;
                }
                else if( key == "default" || key == "preset" )
                {
                    meta.hasPreset = true;
                }
                else if( key == "user" || key == "state" )
                {
                    meta.hasState = true;
                }
                else if( key == "tooltips" || key == "tooltip" )
                {
                    if( val.empty() || IsEnabledTooltipValue( val ) )
                        meta.enableTooltips = true;
                    else if( IsSuppressedTooltipValue( val ) )
                        meta.enableTooltips = false;
                }
                else if( key == "notooltips" || key == "no-tooltips" || key == "no_tooltips" )
                {
                    meta.enableTooltips = false;
                }
                else if( key == "col-tooltips" || key == "col_tooltips" || key == "header-tooltips" || key == "header_tooltips" )
                {
                    size_t col_i = 0;
                    std::string cur_seg;
                    for( size_t i = 0; i < val.size(); ++i )
                    {
                        if( val[i] == '\\' && i + 1 < val.size() && val[i + 1] == '|' )
                        {
                            cur_seg.push_back( '|' );
                            ++i;
                        }
                        else if( val[i] == '|' )
                        {
                            if( meta.colHeaderTooltips.size() <= col_i )
                                meta.colHeaderTooltips.resize( col_i + 1 );
                            meta.colHeaderTooltips[col_i] = cur_seg;
                            cur_seg.clear();
                            ++col_i;
                        }
                        else
                        {
                            cur_seg.push_back( val[i] );
                        }
                    }
                    if( !val.empty() || col_i > 0 )
                    {
                        if( meta.colHeaderTooltips.size() <= col_i )
                            meta.colHeaderTooltips.resize( col_i + 1 );
                        meta.colHeaderTooltips[col_i] = cur_seg;
                    }
                }
                else
                {
                    int col_tip_idx = -1;
                    if( TryParseIndexedTooltipKey( key, col_tip_idx ) && col_tip_idx >= 0 )
                    {
                        if( meta.colHeaderTooltips.size() <= (size_t)col_tip_idx )
                            meta.colHeaderTooltips.resize( (size_t)col_tip_idx + 1 );
                        meta.colHeaderTooltips[(size_t)col_tip_idx] = val;
                    }
                }
            }
        }

        // Checks if current line is the start of a GFM Markdown Table.
        // Returns the end byte offset of the table (including attached metadata directives) if valid, or -1 if not a table.
        inline int TryParseTable(
            const char* md, int md_len, int line_start,
            std::vector<TableCell>& out_headers,
            std::vector<TableColumnAlign>& out_alignments,
            std::vector<std::vector<TableCell>>& out_rows,
            MarkdownTableMetadata& out_meta )
        {
            out_headers.clear();
            out_alignments.clear();
            out_rows.clear();
            out_meta = MarkdownTableMetadata();
            out_meta.tableStart = line_start;

            // 1. Current line (Header row candidate)
            int line1_end = FindLineEnd( md, line_start, md_len );
            if( !CheckLineHasPipe( md + line_start, md + line1_end ) )
            {
                return -1;
            }

            // 2. Next line (Delimiter row candidate)
            int line2_start = SkipNewline( md, line1_end, md_len );
            if( line2_start >= md_len ) return -1;

            int line2_lead_spaces = 0;
            while( line2_start + line2_lead_spaces < md_len && 
                   ( md[line2_start + line2_lead_spaces] == ' ' || md[line2_start + line2_lead_spaces] == '\t' ) )
            {
                line2_lead_spaces += ( md[line2_start + line2_lead_spaces] == '\t' ) ? 4 : 1;
            }
            if( line2_lead_spaces > 3 ) return -1;

            int line2_end = FindLineEnd( md, line2_start, md_len );
            if( !ParseDelimiterRow( md + line2_start, md + line2_end, out_alignments ) || out_alignments.empty() )
            {
                return -1;
            }

            if( !SplitRow( md + line_start, md + line1_end, out_headers ) || out_headers.empty() )
            {
                return -1;
            }

            size_t num_cols = out_alignments.size();
            while( out_headers.size() < num_cols )
            {
                TableCell empty_cell;
                empty_cell.start = md + line1_end;
                empty_cell.end = md + line1_end;
                out_headers.push_back( empty_cell );
            }
            if( out_headers.size() > num_cols )
            {
                out_headers.resize( num_cols );
            }

            // 3. Subsequent Data Rows
            int cur_line_start = SkipNewline( md, line2_end, md_len );
            int table_end = line2_end;

            while( cur_line_start < md_len )
            {
                int cur_line_end = FindLineEnd( md, cur_line_start, md_len );

                bool is_blank = true;
                for( int k = cur_line_start; k < cur_line_end; ++k )
                {
                    if( md[k] != ' ' && md[k] != '\t' && md[k] != '\r' )
                    {
                        is_blank = false;
                        break;
                    }
                }
                if( is_blank )
                {
                    break;
                }

                int lead_sp = 0;
                while( cur_line_start + lead_sp < cur_line_end && ( md[cur_line_start + lead_sp] == ' ' || md[cur_line_start + lead_sp] == '\t' ) )
                {
                    ++lead_sp;
                }
                char first_c = ( cur_line_start + lead_sp < cur_line_end ) ? md[cur_line_start + lead_sp] : '\0';

                // Heading, code fence, or HTML comment block interrupts table rows
                if( first_c == '#' || first_c == '`' || first_c == '~' )
                {
                    break;
                }
                if( cur_line_start + lead_sp + 4 <= cur_line_end &&
                    strncmp( md + cur_line_start + lead_sp, "<!--", 4 ) == 0 )
                {
                    break;
                }

                if( !CheckLineHasPipe( md + cur_line_start, md + cur_line_end ) )
                {
                    break;
                }

                std::vector<TableCell> row_cells;
                if( SplitRow( md + cur_line_start, md + cur_line_end, row_cells ) )
                {
                    while( row_cells.size() < num_cols )
                    {
                        TableCell empty_cell;
                        empty_cell.start = md + cur_line_end;
                        empty_cell.end = md + cur_line_end;
                        row_cells.push_back( empty_cell );
                    }
                    if( row_cells.size() > num_cols )
                    {
                        row_cells.resize( num_cols );
                    }
                    out_rows.push_back( row_cells );
                }

                table_end = cur_line_end;
                cur_line_start = SkipNewline( md, cur_line_end, md_len );
            }

            out_meta.rowsEnd = table_end;
            out_meta.fullTableEnd = table_end;

            // 4. Scan for attached table metadata comment directives (<!-- table-preset: ... --> and/or <!-- table-state: ... -->)
            while( cur_line_start < md_len )
            {
                int check_line_end = FindLineEnd( md, cur_line_start, md_len );
                int p = cur_line_start;
                while( p < check_line_end && ( md[p] == ' ' || md[p] == '\t' ) ) ++p;

                if( p + 4 <= check_line_end && strncmp( md + p, "<!--", 4 ) == 0 )
                {
                    int content_p = p + 4;
                    while( content_p < check_line_end && ( md[content_p] == ' ' || md[content_p] == '\t' ) ) ++content_p;

                    bool is_table_directive = false;
                    bool is_preset = false;

                    if( StrCaseEqualN( md + content_p, "table-preset:", 13 ) )
                    {
                        is_table_directive = true;
                        is_preset = true;
                        content_p += 13;
                    }
                    else if( StrCaseEqualN( md + content_p, "table-state:", 12 ) )
                    {
                        is_table_directive = true;
                        is_preset = false;
                        content_p += 12;
                    }
                    else if( StrCaseEqualN( md + content_p, "table:", 6 ) )
                    {
                        is_table_directive = true;
                        content_p += 6;
                        while( content_p < check_line_end && ( md[content_p] == ' ' || md[content_p] == '\t' ) ) ++content_p;
                        if( StrCaseEqualN( md + content_p, "default", 7 ) )
                        {
                            is_preset = true;
                            content_p += 7;
                        }
                        else if( StrCaseEqualN( md + content_p, "preset", 6 ) )
                        {
                            is_preset = true;
                            content_p += 6;
                        }
                        else if( StrCaseEqualN( md + content_p, "user", 4 ) )
                        {
                            is_preset = false;
                            content_p += 4;
                        }
                        else if( StrCaseEqualN( md + content_p, "state", 5 ) )
                        {
                            is_preset = false;
                            content_p += 5;
                        }
                        else
                        {
                            is_preset = false;
                        }
                    }

                    if( is_table_directive )
                    {
                        int close_pos = -1;
                        for( int k = content_p; k + 3 <= md_len; ++k )
                        {
                            if( md[k] == '-' && md[k+1] == '-' && md[k+2] == '>' )
                            {
                                close_pos = k;
                                break;
                            }
                        }
                        if( close_pos != -1 )
                        {
                            int comment_end = close_pos + 3;
                            int comment_line_end = FindLineEnd( md, comment_end, md_len );

                            ParseTableDirective( md + content_p, (size_t)( close_pos - content_p ), is_preset, out_meta, out_headers );

                            if( is_preset )
                            {
                                out_meta.presetCommentStart = cur_line_start;
                                out_meta.presetCommentEnd = comment_line_end;
                            }
                            else
                            {
                                out_meta.stateCommentStart = cur_line_start;
                                out_meta.stateCommentEnd = comment_line_end;
                            }

                            table_end = comment_line_end;
                            out_meta.fullTableEnd = comment_line_end;
                            cur_line_start = SkipNewline( md, comment_line_end, md_len );
                            continue;
                        }
                    }
                }
                break;
            }

            return table_end;
        }

        inline int TryParseTable(
            const char* md, int md_len, int line_start,
            std::vector<TableCell>& out_headers,
            std::vector<TableColumnAlign>& out_alignments,
            std::vector<std::vector<TableCell>>& out_rows )
        {
            MarkdownTableMetadata dummy_meta;
            return TryParseTable( md, md_len, line_start, out_headers, out_alignments, out_rows, dummy_meta );
        }
    } // namespace TableDetail

    inline void RenderTableCell( const TableCell& cell, TableColumnAlign align, bool is_header, const MarkdownConfig& mdConfig_ )
    {
        if( cell.empty() )
        {
            return;
        }

        float avail = ImGui::GetContentRegionAvail().x;
        float text_w = TableDetail::CalcCellVisualWidth( cell, is_header, mdConfig_ );
        float scale = ImGui::GetVisualScale();

        // In Dear ImGui table cells, GetContentRegionAvail().x already accounts for style.CellPadding.x.
        // For header cells, reserve margin for the sort arrow (14px).
        // For body cells, reserve a 6px breathing gutter so right-aligned glyphs don't clip against the border.
        float right_margin = ( is_header ? 14.0f : 6.0f ) * scale;

        // Apply alignment offset if CENTER or RIGHT
        if( align == TableColumnAlign::RIGHT )
        {
            if( text_w + right_margin < avail )
            {
                float offset = avail - text_w - right_margin;
                if( offset > 0.0f )
                {
                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );
                }
            }
        }
        else if( align == TableColumnAlign::CENTER )
        {
            if( text_w < avail )
            {
                float offset = ( avail - text_w ) * 0.5f;
                if( offset > 0.0f )
                {
                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );
                }
            }
        }

        bool pushed_font = false;
        if( is_header )
        {
            if( mdConfig_.boldFont )
            {
                ImGui::ImGeeCompat::PushFont( mdConfig_.boldFont );
                pushed_font = true;
            }
            else if( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font )
            {
                ImGui::ImGeeCompat::PushFont( mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font );
                pushed_font = true;
            }
        }

        MarkdownConfig cell_config = mdConfig_;
        // Prevent mid-word syllable chopping: only enable wrapping if the cell contains whitespace
        bool has_space = false;
        for( const char* sp = cell.start; sp < cell.end; ++sp )
        {
            if( *sp == ' ' || *sp == '\t' || *sp == '\n' ) { has_space = true; break; }
        }

        // If text fits comfortably in the column, do NOT wrap (prevents right-aligned sub-pixel wrap anomalies)
        // If multi-word text genuinely exceeds available column width, enable wrapping!
        if( has_space && ( text_w + right_margin > avail + 0.5f ) )
        {
            cell_config.wrapWidth = avail;
        }
        else
        {
            cell_config.wrapWidth = -1.0f;
        }
        Markdown::Render( cell.start, cell.size(), cell_config );

        if( pushed_font )
        {
            ImGui::PopFont();
        }
    }

    inline void RenderTableBlock(
        const std::vector<TableCell>& headers,
        const std::vector<TableColumnAlign>& alignments,
        const std::vector<std::vector<TableCell>>& rows,
        const TableDetail::MarkdownTableMetadata& metadata,
        const MarkdownConfig& mdConfig_,
        size_t table_idx )
    {
        size_t num_cols = alignments.size();
        if( num_cols == 0 ) return;

        // 1. Stable table ID: hash header text + index so ID is 100% deterministic across frames
        ImGuiID header_hash = 0;
        for( size_t c = 0; c < headers.size(); ++c )
        {
            if( headers[c].start && headers[c].end > headers[c].start )
            {
                header_hash = ImHashData( headers[c].start, (size_t)( headers[c].end - headers[c].start ), header_hash );
            }
        }
        header_hash = ImHashData( &table_idx, sizeof(table_idx), header_hash );

        char table_id[64];
        snprintf( table_id, sizeof(table_id), "##md_table_%08x_%d", (unsigned int)header_hash, (int)table_idx );

#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18000
        // Pre-1.80 fallback: Dear ImGui did not introduce official BeginTable() until v1.80
        float fallback_scale = ImGui::GetFontScale( false );
        for( size_t col = 0; col < headers.size(); ++col )
        {
            if( col > 0 ) ImGui::SameLine( 0.0f, 16.0f * fallback_scale );
            RenderTableCell( headers[col], TableColumnAlign::LEFT, true, mdConfig_ );
        }
        ImGui::Separator();
        for( size_t row = 0; row < rows.size(); ++row )
        {
            for( size_t col = 0; col < rows[row].size(); ++col )
            {
                if( col > 0 ) ImGui::SameLine( 0.0f, 16.0f * fallback_scale );
                RenderTableCell( rows[row][col], ( col < alignments.size() ) ? alignments[col] : TableColumnAlign::LEFT, false, mdConfig_ );
            }
        }
        return;
#endif

#if !defined(IMGUI_VERSION_NUM) || IMGUI_VERSION_NUM >= 18000

        // Persistent sort & width state stored in ImGuiStorage for this deterministic table hash
        ImGuiID sort_col_key   = header_hash;
        ImGuiID sort_dir_key   = header_hash + 1;
        ImGuiID width_key      = header_hash + 2;
        ImGuiID reset_key      = header_hash + 3;
        ImGuiID last_width_key = header_hash + 4;
        ImGuiID init_key       = header_hash + 5;
        ImGuiID preset_w_key        = header_hash + 6;
        ImGuiID meta_hash_key       = header_hash + 7;
        ImGuiID dragging_key        = header_hash + 8;
        ImGuiID col_resizing_key    = header_hash + 9;
        ImGuiID col_custom_key      = header_hash + 10;
        ImGuiID last_col_custom_key = header_hash + 11;

        // Detect if metadata was loaded or edited in raw markdown
        ImGuiID current_meta_hash = 1;
        if( metadata.hasPreset )
        {
            current_meta_hash = ImHashData( &metadata.presetWidth, sizeof(float), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.presetSortCol, sizeof(int), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.presetSortDir, sizeof(int), current_meta_hash );
            for( size_t c = 0; c < metadata.presetColWidths.size(); ++c )
            {
                current_meta_hash = ImHashData( &metadata.presetColWidths[c], sizeof(float), current_meta_hash );
            }
        }
        if( metadata.hasState )
        {
            current_meta_hash = ImHashData( &metadata.stateWidth, sizeof(float), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.stateSortCol, sizeof(int), current_meta_hash );
            current_meta_hash = ImHashData( &metadata.stateSortDir, sizeof(int), current_meta_hash );
            for( size_t c = 0; c < metadata.stateColWidths.size(); ++c )
            {
                current_meta_hash = ImHashData( &metadata.stateColWidths[c], sizeof(float), current_meta_hash );
            }
        }

        bool need_init = !ImGui::GetStateStorage()->GetBool( init_key, false ) ||
                         ( ImGui::GetStateStorage()->GetInt( meta_hash_key, 0 ) != (int)current_meta_hash );

        if( need_init )
        {
            int initial_sort_col = -1;
            int initial_sort_dir = (int)ImGuiSortDirection_None;
            float initial_norm_w = 0.0f;
            float preset_norm_w  = 0.0f;

            if( metadata.hasPreset )
            {
                preset_norm_w    = metadata.presetWidth;
                initial_norm_w   = preset_norm_w;
                initial_sort_col = metadata.presetSortCol;
                initial_sort_dir = metadata.presetSortDir;

                for( size_t c = 0; c < metadata.presetColWidths.size(); ++c )
                {
                    ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)c, metadata.presetColWidths[c] );
                }
            }
            if( metadata.hasState )
            {
                if( metadata.stateWidth > 0.0f )
                {
                    initial_norm_w = metadata.stateWidth;
                    // Seed last_width_key with saved custom width so A/B toggle remembers it!
                    ImGui::GetStateStorage()->SetFloat( last_width_key, metadata.stateWidth );
                }
                else if( metadata.hasPreset )
                {
                    initial_norm_w = metadata.presetWidth;
                }

                if( !metadata.stateColWidths.empty() )
                {
                    ImGui::GetStateStorage()->SetBool( col_custom_key, true );
                    ImGui::GetStateStorage()->SetBool( last_col_custom_key, true );
                    for( size_t c = 0; c < metadata.stateColWidths.size(); ++c )
                    {
                        ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)c, metadata.stateColWidths[c] );
                        ImGui::GetStateStorage()->SetFloat( header_hash + 200 + (ImGuiID)c, metadata.stateColWidths[c] );
                    }
                }

                if( metadata.stateSortCol >= -1 ) initial_sort_col = metadata.stateSortCol;
                if( metadata.stateSortDir >= 0 )  initial_sort_dir = metadata.stateSortDir;
            }

            ImGui::GetStateStorage()->SetInt( sort_col_key, initial_sort_col );
            ImGui::GetStateStorage()->SetInt( sort_dir_key, initial_sort_dir );
            ImGui::GetStateStorage()->SetFloat( width_key, initial_norm_w );
            ImGui::GetStateStorage()->SetFloat( preset_w_key, preset_norm_w );
            ImGui::GetStateStorage()->SetInt( meta_hash_key, (int)current_meta_hash );
            ImGui::GetStateStorage()->SetBool( init_key, true );
        }

        int active_sort_col = ImGui::GetStateStorage()->GetInt( sort_col_key, -1 );
        int active_sort_dir = ImGui::GetStateStorage()->GetInt( sort_dir_key, (int)ImGuiSortDirection_None );
        float user_norm_w   = ImGui::GetStateStorage()->GetFloat( width_key, 0.0f );
        float preset_norm_w = ImGui::GetStateStorage()->GetFloat( preset_w_key, 0.0f );

        ImGuiTableFlags flags = mdConfig_.tableFlags;
        if( flags == 0 )
        {
            flags = ImGuiTableFlags_Borders | 
                    ImGuiTableFlags_RowBg | 
                    ImGuiTableFlags_SizingStretchProp | 
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Sortable |
                    ImGuiTableFlags_SortTristate |
                    ImGuiTableFlags_NoSavedSettings;
        }
        else if( flags & ImGuiTableFlags_Sortable )
        {
            flags |= ImGuiTableFlags_SortTristate;
        }

        float scale = ImGui::GetVisualScale();
        float user_table_w = ( user_norm_w > 0.0f ) ? ( user_norm_w * scale ) : 0.0f;

        // 2. Measure natural content width for each column to prevent 100% window width blowout
        float total_content_w = 0.0f;
        std::vector<float> col_widths( num_cols, 0.0f );
        for( size_t col = 0; col < num_cols; ++col )
        {
            float custom_cw = ImGui::GetStateStorage()->GetFloat( header_hash + 100 + (ImGuiID)col, 0.0f );
            if( custom_cw > 0.0f )
            {
                col_widths[col] = custom_cw * scale;
                total_content_w += col_widths[col];
            }
            else
            {
                TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
                float max_w = TableDetail::CalcCellVisualWidth( h_cell, true, mdConfig_ );
                if( flags & ImGuiTableFlags_Sortable )
                {
                    max_w += 14.0f * scale; // Extra margin for header sort indicator arrow
                }
                for( size_t r = 0; r < rows.size(); ++r )
                {
                    if( col < rows[r].size() )
                    {
                        float cw = TableDetail::CalcCellVisualWidth( rows[r][col], false, mdConfig_ );
                        if( cw > max_w ) max_w = cw;
                    }
                }
                // Add padding: left + right cell padding + extra breathing room (scaled by zoom/font scale)
                max_w += ( ImGui::GetStyle().CellPadding.x * 2.0f + 16.0f ) * scale;
                if( max_w < 30.0f * scale ) max_w = 30.0f * scale;
                col_widths[col] = max_w;
                total_content_w += max_w;
            }
        }

        bool has_h_scroll = ( ImGui::GetCurrentWindow() && ( ImGui::GetCurrentWindow()->Flags & ImGuiWindowFlags_HorizontalScrollbar ) != 0 );
        float min_table_w = (float)num_cols * 28.0f * scale;
        float avail_w = ( mdConfig_.wrapWidth > 0.0f ) ? mdConfig_.wrapWidth : ImGui::GetContentRegionAvail().x;
        float table_w = total_content_w;
        if( avail_w > 0.0f && !has_h_scroll )
        {
            table_w = ImMin( total_content_w, avail_w );
            if( user_table_w > 0.0f )
            {
                table_w = ImClamp( user_table_w, ImMin( min_table_w, avail_w ), avail_w );
            }
        }
        else if( user_table_w > 0.0f )
        {
            table_w = ImMax( user_table_w, min_table_w );
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );

        ImGuiTable* table = nullptr;
        bool sort_changed = false;

        auto fire_state_callback = [&]( bool is_committed )
        {
            if( !mdConfig_.tableStateCallback ) return;
            MarkdownTableState st;
            st.tableIndex = table_idx;
            st.textStart = metadata.tableStart;
            st.textEnd = metadata.fullTableEnd;
            st.rowsEnd = metadata.rowsEnd;
            st.totalWidth = ImGui::GetStateStorage()->GetFloat( width_key, 0.0f );
            st.presetWidth = preset_norm_w;
            st.sortColumn = active_sort_col;
            st.sortDirection = active_sort_dir;
            if( active_sort_col >= 0 && (size_t)active_sort_col < headers.size() )
            {
                st.sortColumnName = TableDetail::GetCellPlainText( headers[active_sort_col] );
            }
            st.hasPreset = metadata.hasPreset;
            bool custom_cols = ImGui::GetStateStorage()->GetBool( col_custom_key, false );
            st.hasState = ( metadata.hasState || st.totalWidth > 0.0f || st.sortColumn >= 0 || custom_cols );
            st.hasCustomColumnWidths = custom_cols;
            st.isCommitted = is_committed;
            st.config = &mdConfig_;
            st.userData = mdConfig_.userData;

            st.columnWidths.resize( num_cols, 0.0f );
            for( size_t c = 0; c < num_cols; ++c )
            {
                float cw = ImGui::GetStateStorage()->GetFloat( header_hash + 100 + (ImGuiID)c, 0.0f );
                if( cw <= 0.0f && table && c < (size_t)table->ColumnsCount )
                {
                    float w = table->Columns[(int)c].WidthGiven;
                    cw = ( scale > 0.0f ) ? ( w / scale ) : w;
                }
                st.columnWidths[c] = cw;
            }
            mdConfig_.tableStateCallback( st, mdConfig_.userData );
        };

        if( ImGui::BeginTable( table_id, (int)num_cols, flags, ImVec2( table_w, 0.0f ) ) )
        {
            // 1. Setup column headers with proportional stretch weights based on content
            for( size_t col = 0; col < num_cols; ++col )
            {
                TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
                std::string col_name = TableDetail::GetCellPlainText( h_cell );
                if( col_name.empty() ) col_name = "##col";
                ImGui::TableSetupColumn( col_name.c_str(), ImGuiTableColumnFlags_WidthStretch, col_widths[col] );
            }

            table = ImGui::GetCurrentTable();
            if( need_init && table )
            {
                for( size_t c = 0; c < num_cols; ++c )
                {
                    if( c < (size_t)table->ColumnsCount && col_widths[c] > 0.0f )
                    {
                        table->Columns[(int)c].WidthRequest = col_widths[c];
                        table->Columns[(int)c].StretchWeight = col_widths[c];
                        table->Columns[(int)c].AutoFitQueue = 0x00;
                    }
                }
                ImGui::TableUpdateColumnsWeightFromWidth( table );
            }

            const bool allow_table_tooltips = mdConfig_.enableTableTooltips && metadata.enableTooltips;

            // 2. Submit Header Row (with interactive sort click, hand cursor, delayed tooltips, and visual sort indicators)
            float header_h = ImMax( ImGui::GetFrameHeight(), ImGui::GetFontSize() + ImGui::GetStyle().CellPadding.y * 2.0f );
            ImGui::TableNextRow( ImGuiTableRowFlags_Headers, header_h );

            for( size_t col = 0; col < num_cols; ++col )
            {
                if( !ImGui::TableSetColumnIndex( (int)col ) ) continue;

                TableCell h_cell = ( col < headers.size() ) ? headers[col] : TableCell();
                ImGuiTableColumn* column = ( table && (int)col < table->ColumnsCount ) ? &table->Columns[(int)col] : nullptr;
                bool is_sortable = ( ( flags & ImGuiTableFlags_Sortable ) && column && !( column->Flags & ImGuiTableColumnFlags_NoSort ) );

                if( is_sortable && table )
                {
                    ImRect cell_r = ImGui::TableGetCellBgRect( table, (int)col );
                    if( cell_r.Max.y <= cell_r.Min.y )
                    {
                        cell_r.Max.y = cell_r.Min.y + header_h;
                    }

                    ImGuiID header_id = ImGui::GetID( (const void*)(intptr_t)col );
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18300
                    ImGui::ItemAdd( cell_r, header_id, NULL );
#else
                    ImGui::ItemAdd( cell_r, header_id, NULL, ImGuiItemFlags_NoNav );
#endif
                    bool hovered = false, held = false;
                    bool pressed = ImGui::ButtonBehavior( cell_r, header_id, &hovered, &held, ImGuiButtonFlags_AllowOverlap );
                    const bool border_active = ( table->HoveredColumnBorder != -1 || table->ResizedColumn != -1 );

                    if( hovered && !border_active )
                    {
                        ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    }

                    if( pressed && table->ReorderColumn != (int)col && !border_active )
                    {
                        if( active_sort_col != (int)col )
                        {
                            active_sort_col = (int)col;
                            active_sort_dir = (int)ImGuiSortDirection_Ascending;
                        }
                        else
                        {
                            // Cycle: Ascending -> Descending -> None (Tristate)
                            if( active_sort_dir == (int)ImGuiSortDirection_Ascending )
                            {
                                active_sort_dir = (int)ImGuiSortDirection_Descending;
                            }
                            else if( active_sort_dir == (int)ImGuiSortDirection_Descending )
                            {
                                active_sort_col = -1;
                                active_sort_dir = (int)ImGuiSortDirection_None;
                            }
                            else
                            {
                                active_sort_dir = (int)ImGuiSortDirection_Ascending;
                            }
                        }
                        ImGui::GetStateStorage()->SetInt( sort_col_key, active_sort_col );
                        ImGui::GetStateStorage()->SetInt( sort_dir_key, active_sort_dir );
                        sort_changed = true;
                    }

                    if( allow_table_tooltips )
                    {
                        const std::string& custom_tip = ( col < metadata.colHeaderTooltips.size() )
                            ? metadata.colHeaderTooltips[col]
                            : std::string();
                        if( !TableDetail::IsSuppressedTooltipValue( custom_tip ) )
                        {
                            std::string tip;
                            if( !custom_tip.empty() )
                            {
                                tip = custom_tip;
                            }
                            else
                            {
                                std::string col_label = TableDetail::GetCellPlainText( h_cell );
                                if( col_label.empty() ) col_label = "column";
                                if( active_sort_col == (int)col && active_sort_dir == (int)ImGuiSortDirection_Ascending )
                                {
                                    tip = "Sort **" + col_label + "** in descending order (Z to A)";
                                }
                                else if( active_sort_col == (int)col && active_sort_dir == (int)ImGuiSortDirection_Descending )
                                {
                                    tip = "Clear sort on **" + col_label + "** (restore default order)";
                                }
                                else
                                {
                                    tip = "Sort table by **" + col_label + "** (A to Z)";
                                }
                            }
                            bool header_tt_hovered = hovered && !held && !pressed && !border_active && table->ReorderColumn == -1;
                            MarkdownDetail::ShowTableTooltip( mdConfig_, header_id, header_tt_hovered, tip.c_str(), pressed );
                        }
                    }

                    if( ( held || hovered ) && !border_active )
                    {
                        ImU32 col_bg = ImGui::GetColorU32( held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, col_bg, (int)col );
                    }

                    // Sync internal ImGui column sort state
                    column->SortOrder = ( (int)col == active_sort_col ) ? 0 : -1;
                    column->SortDirection = ( (int)col == active_sort_col ) ? (ImU8)active_sort_dir : (ImU8)ImGuiSortDirection_None;
                }

                RenderTableCell( h_cell, alignments[col], true, mdConfig_ );

                // Draw sort indicator arrow if this column is sorted
                if( is_sortable && active_sort_col == (int)col && active_sort_dir != (int)ImGuiSortDirection_None )
                {
                    ImRect cell_r = ImGui::TableGetCellBgRect( table, (int)col );
                    if( cell_r.Max.y <= cell_r.Min.y ) cell_r.Max.y = cell_r.Min.y + header_h;

                    float arrow_scale = 0.65f;
                    float w_arrow = ImTrunc( ImGui::GetFontSize() * arrow_scale + ImGui::GetStyle().FramePadding.x );
                    float x = ImMax( cell_r.Min.x, cell_r.Max.x - w_arrow );
                    float y = cell_r.Min.y + ImMax( 0.0f, ( ( cell_r.Max.y - cell_r.Min.y ) - ImGui::GetFontSize() ) * 0.5f );
                    ImGui::RenderArrow( ImGui::GetWindowDrawList(), ImVec2( x, y ), ImGui::GetColorU32( ImGuiCol_Text ),
                                        ( active_sort_dir == (int)ImGuiSortDirection_Ascending ) ? ImGuiDir_Up : ImGuiDir_Down,
                                        arrow_scale );
                }
            }

            if( sort_changed )
            {
                fire_state_callback( true );
            }

            // 3. Prepare Row Indices & Handle Sorting
            std::vector<size_t> row_indices( rows.size() );
            for( size_t i = 0; i < rows.size(); ++i ) row_indices[i] = i;

            if( ( flags & ImGuiTableFlags_Sortable ) && !rows.empty() && active_sort_col >= 0 && (size_t)active_sort_col < num_cols && active_sort_dir != (int)ImGuiSortDirection_None )
            {
                std::stable_sort( row_indices.begin(), row_indices.end(),
                    [&]( size_t a_idx, size_t b_idx ) -> bool {
                        const std::vector<TableCell>& rA = rows[a_idx];
                        const std::vector<TableCell>& rB = rows[b_idx];
                        TableCell cA = ( (size_t)active_sort_col < rA.size() ) ? rA[active_sort_col] : TableCell();
                        TableCell cB = ( (size_t)active_sort_col < rB.size() ) ? rB[active_sort_col] : TableCell();

                        int cmp = TableDetail::CompareCells( cA, cB );
                        if( active_sort_dir == (int)ImGuiSortDirection_Descending )
                            return cmp > 0;
                        else
                            return cmp < 0;
                    } );
            }

            // 4. Submit Data Rows in sorted order
            for( size_t r = 0; r < row_indices.size(); ++r )
            {
                ImGui::TableNextRow();
                const std::vector<TableCell>& row = rows[row_indices[r]];
                for( size_t col = 0; col < num_cols; ++col )
                {
                    if( !ImGui::TableSetColumnIndex( (int)col ) ) continue;
                    TableCell c_cell = ( col < row.size() ) ? row[col] : TableCell();
                    RenderTableCell( c_cell, alignments[col], false, mdConfig_ );
                }
            }

            // Check if user is resizing any internal column separator
            bool is_col_resizing  = ( table && table->ResizedColumn != -1 );
            bool was_col_resizing = ImGui::GetStateStorage()->GetBool( col_resizing_key, false );

            if( is_col_resizing )
            {
                ImGui::GetStateStorage()->SetBool( col_resizing_key, true );
                ImGui::GetStateStorage()->SetBool( col_custom_key, true );
                ImGui::GetStateStorage()->SetBool( last_col_custom_key, true );
            }
            else if( was_col_resizing && !ImGui::IsMouseDown( 0 ) )
            {
                ImGui::GetStateStorage()->SetBool( col_resizing_key, false );
                if( table )
                {
                    for( size_t c = 0; c < num_cols; ++c )
                    {
                        if( c < (size_t)table->ColumnsCount )
                        {
                            float cw = table->Columns[(int)c].WidthGiven;
                            float norm_c = ( scale > 0.0f ) ? ( cw / scale ) : cw;
                            ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)c, norm_c );
                            ImGui::GetStateStorage()->SetFloat( header_hash + 200 + (ImGuiID)c, norm_c );
                        }
                    }
                }
                ImGui::GetStateStorage()->SetBool( col_custom_key, true );
                ImGui::GetStateStorage()->SetBool( last_col_custom_key, true );
                fire_state_callback( true );
            }

            // Internal column resize separator tooltips & double-click auto-fit sync
            if( table && ( flags & ImGuiTableFlags_Resizable ) && num_cols > 1 )
            {
                for( int col_n = 0; col_n + 1 < (int)num_cols && col_n < table->ColumnsCount; ++col_n )
                {
                    ImGuiTableColumn* sep_col = &table->Columns[col_n];
                    if( sep_col->Flags & ImGuiTableColumnFlags_NoResize ) continue;

                    if( table->HoveredColumnBorder == col_n && ImGui::IsMouseDoubleClicked( 0 ) )
                    {
                        ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)col_n, 0.0f );
                    }

                    if( allow_table_tooltips )
                    {
                        ImGuiID sep_id = ImGui::TableGetColumnResizeID( table, col_n );
                        bool sep_hovered = ( table->HoveredColumnBorder == col_n ) && ( table->ResizedColumn == -1 ) && !ImGui::IsMouseDown( 0 );
                        bool sep_clicked = ( table->ResizedColumn == col_n ) || ( table->HoveredColumnBorder == col_n && ImGui::IsMouseClicked( 0 ) );
                        std::string col_label = ( (size_t)col_n < headers.size() ) ? TableDetail::GetCellPlainText( headers[(size_t)col_n] ) : std::string();
                        if( col_label.empty() ) col_label = "column";
                        std::string sep_tip = "Drag to resize **" + col_label + "** column\nDouble-click to auto-fit";
                        MarkdownDetail::ShowTableTooltip( mdConfig_, sep_id, sep_hovered, sep_tip.c_str(), sep_clicked );
                    }
                }
            }

            ImGui::EndTable();

            // 5. Rightmost Border Resize Grip: allows expanding/contracting total table width
            if( table && ( flags & ImGuiTableFlags_Resizable ) && avail_w > 0.0f && table->OuterRect.Max.x > table->OuterRect.Min.x )
            {
                float right_x = table->OuterRect.Max.x;
                float top_y   = table->OuterRect.Min.y;
                float bot_y   = table->OuterRect.Max.y;
                const float hit_half_w = 4.0f * scale;

                ImRect grip_r( right_x - hit_half_w, top_y, right_x + hit_half_w, bot_y );
                char grip_id_str[80];
                snprintf( grip_id_str, sizeof(grip_id_str), "##md_table_right_resize_grip_%08x_%d", (unsigned int)header_hash, (int)table_idx );
                ImGuiID grip_id = ImGui::GetID( grip_id_str );

#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18300
                ImGui::ItemAdd( grip_r, grip_id, NULL );
#else
                ImGui::ItemAdd( grip_r, grip_id, NULL, ImGuiItemFlags_NoNav );
#endif
                bool grip_hovered = false, grip_held = false;
                bool grip_pressed = ImGui::ButtonBehavior( grip_r, grip_id, &grip_hovered, &grip_held,
                    ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDoubleClick );

                bool is_resetting = ImGui::GetStateStorage()->GetBool( reset_key, false );
                if( !ImGui::IsMouseDown( 0 ) )
                {
                    is_resetting = false;
                    ImGui::GetStateStorage()->SetBool( reset_key, false );
                }

                float max_grip_w = has_h_scroll ? ImMax( avail_w * 5.0f, total_content_w * 3.0f ) : avail_w;

                if( ( grip_pressed || grip_hovered ) && ImGui::IsMouseDoubleClicked( 0 ) )
                {
                    // Double click: A/B toggle between baseline (preset_norm_w or 0.0f auto) and last manual width & column widths!
                    float cur_w = ImGui::GetStateStorage()->GetFloat( width_key, preset_norm_w );
                    float last_w = ImGui::GetStateStorage()->GetFloat( last_width_key, 0.0f );

                    if( std::fabs( cur_w - preset_norm_w ) > 0.1f )
                    {
                        // Transition: Custom Layout -> Baseline (Preset / Auto)
                        ImGui::GetStateStorage()->SetFloat( last_width_key, cur_w );
                        ImGui::GetStateStorage()->SetFloat( width_key, preset_norm_w );

                        // Remember current custom column widths in header_hash + 200 + c
                        bool had_custom_cols = ImGui::GetStateStorage()->GetBool( col_custom_key, false );
                        ImGui::GetStateStorage()->SetBool( last_col_custom_key, had_custom_cols );

                        for( size_t c = 0; c < num_cols; ++c )
                        {
                            float cur_cw = ImGui::GetStateStorage()->GetFloat( header_hash + 100 + (ImGuiID)c, 0.0f );
                            if( cur_cw <= 0.0f && table && c < (size_t)table->ColumnsCount )
                            {
                                float cw = table->Columns[(int)c].WidthGiven;
                                cur_cw = ( scale > 0.0f ) ? ( cw / scale ) : cw;
                            }
                            ImGui::GetStateStorage()->SetFloat( header_hash + 200 + (ImGuiID)c, cur_cw );

                            // Reset to baseline preset column width (or 0.0f auto)
                            float pw = ( metadata.hasPreset && c < metadata.presetColWidths.size() ) ? metadata.presetColWidths[c] : 0.0f;
                            ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)c, pw );

                            if( table && c < (size_t)table->ColumnsCount )
                            {
                                if( pw > 0.0f )
                                {
                                    table->Columns[(int)c].WidthRequest = pw * scale;
                                    table->Columns[(int)c].StretchWeight = pw * scale;
                                    table->Columns[(int)c].AutoFitQueue = 0x00;
                                }
                            }
                        }

                        if( table )
                        {
                            if( metadata.hasPreset && !metadata.presetColWidths.empty() )
                                ImGui::TableUpdateColumnsWeightFromWidth( table );
                            else
                                ImGui::TableSetColumnWidthAutoAll( table );
                        }

                        ImGui::GetStateStorage()->SetBool( col_custom_key, ( metadata.hasPreset && !metadata.presetColWidths.empty() ) );
                    }
                    else if( last_w > 0.0f && std::fabs( last_w - preset_norm_w ) > 0.1f )
                    {
                        // Transition: Baseline -> Custom Layout (Restore prior custom width & column widths!)
                        ImGui::GetStateStorage()->SetFloat( width_key, last_w );

                        bool restore_custom = ImGui::GetStateStorage()->GetBool( last_col_custom_key, false );
                        ImGui::GetStateStorage()->SetBool( col_custom_key, restore_custom );

                        for( size_t c = 0; c < num_cols; ++c )
                        {
                            float saved_cw = ImGui::GetStateStorage()->GetFloat( header_hash + 200 + (ImGuiID)c, 0.0f );
                            ImGui::GetStateStorage()->SetFloat( header_hash + 100 + (ImGuiID)c, saved_cw );

                            if( table && c < (size_t)table->ColumnsCount && saved_cw > 0.0f )
                            {
                                table->Columns[(int)c].WidthRequest = saved_cw * scale;
                                table->Columns[(int)c].StretchWeight = saved_cw * scale;
                                table->Columns[(int)c].AutoFitQueue = 0x00;
                            }
                        }

                        if( table && restore_custom )
                        {
                            ImGui::TableUpdateColumnsWeightFromWidth( table );
                        }
                    }

                    ImGui::GetStateStorage()->SetBool( reset_key, true );
                    ImGui::ClearActiveID();
                    fire_state_callback( true );
                }
                else if( grip_held && !is_resetting && ImGui::IsMouseDragging( 0, 1.0f ) )
                {
                    float mouse_x = ImGui::GetIO().MousePos.x;
                    float new_w = mouse_x - table->OuterRect.Min.x;
                    float clamped_w = ImClamp( new_w, min_table_w, max_grip_w );
                    float norm_w = ( scale > 0.0f ) ? ( clamped_w / scale ) : clamped_w;
                    ImGui::GetStateStorage()->SetFloat( width_key, norm_w );
                    ImGui::GetStateStorage()->SetBool( dragging_key, true );
                }

                // Check if drag resize just completed on mouse release
                if( ImGui::GetStateStorage()->GetBool( dragging_key, false ) && !ImGui::IsMouseDown( 0 ) )
                {
                    ImGui::GetStateStorage()->SetBool( dragging_key, false );
                    fire_state_callback( true );
                }

                if( grip_hovered || ( grip_held && !is_resetting ) )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeEW );
                    ImU32 line_col = ImGui::GetColorU32( ( grip_held && !is_resetting ) ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered );
                    ImGui::GetWindowDrawList()->AddLine( ImVec2( right_x, top_y ), ImVec2( right_x, bot_y ), line_col, 2.0f * scale );
                }

                if( allow_table_tooltips )
                {
                    const bool has_layout_preset = metadata.hasPreset && ( metadata.presetWidth > 0.0f || !metadata.presetColWidths.empty() );
                    const float cur_w_now  = ImGui::GetStateStorage()->GetFloat( width_key, preset_norm_w );
                    const float last_w_now = ImGui::GetStateStorage()->GetFloat( last_width_key, 0.0f );
                    const char* grip_tip = nullptr;
                    if( std::fabs( cur_w_now - preset_norm_w ) > 0.1f )
                    {
                        grip_tip = has_layout_preset
                            ? "Drag to resize table width\nDouble-click to reset to **preset layout**"
                            : "Drag to resize table width\nDouble-click to reset to **auto-fit layout**";
                    }
                    else if( last_w_now > 0.0f && std::fabs( last_w_now - preset_norm_w ) > 0.1f )
                    {
                        grip_tip = "Drag to resize table width\nDouble-click to restore **custom layout**";
                    }
                    else
                    {
                        grip_tip = has_layout_preset
                            ? "Drag to resize table width\nDouble-click after resizing to toggle **preset layout**"
                            : "Drag to resize table width\nDouble-click after resizing to toggle **auto-fit layout**";
                    }
                    bool grip_tt_hovered = grip_hovered && !grip_held && !is_resetting && !ImGui::IsMouseDown( 0 );
                    MarkdownDetail::ShowTableTooltip( mdConfig_, grip_id, grip_tt_hovered, grip_tip, grip_pressed || grip_held );
                }
            }
        }

        ImGui::Dummy( ImVec2( 0.0f, 2.0f * scale ) );
#endif // IMGUI_VERSION_NUM >= 18000
    }

    inline void RenderTableBlock(
        const std::vector<TableCell>& headers,
        const std::vector<TableColumnAlign>& alignments,
        const std::vector<std::vector<TableCell>>& rows,
        const MarkdownConfig& mdConfig_,
        size_t table_idx )
    {
        TableDetail::MarkdownTableMetadata dummy_meta;
        RenderTableBlock( headers, alignments, rows, dummy_meta, mdConfig_, table_idx );
    }

    inline void GetActiveMarkdownLightboxCloseRect( ImVec2* out_min, ImVec2* out_max )
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        if( st.openFrame == ImGui::GetFrameCount() ) {
            if( out_min ) *out_min = st.closeButtonMin;
            if( out_max ) *out_max = st.closeButtonMax;
        } else {
            if( out_min ) *out_min = ImVec2( 0.0f, 0.0f );
            if( out_max ) *out_max = ImVec2( 0.0f, 0.0f );
        }
    }

    inline void GetActiveMarkdownLightboxImageRect( ImVec2* out_min, ImVec2* out_max )
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        if( st.openFrame == ImGui::GetFrameCount() ) {
            if( out_min ) *out_min = st.lightboxImageMin;
            if( out_max ) *out_max = st.lightboxImageMax;
        } else {
            if( out_min ) *out_min = ImVec2( 0.0f, 0.0f );
            if( out_max ) *out_max = ImVec2( 0.0f, 0.0f );
        }
    }

    inline bool GetActiveMarkdownLightboxCaptionPlateRect( ImVec2* out_min, ImVec2* out_max )
    {
        const MarkdownDetail::LightboxTelemetryState& st = MarkdownDetail::GetLightboxTelemetryState();
        if( st.openFrame == ImGui::GetFrameCount() && st.hasLightboxCaption ) {
            if( out_min ) *out_min = st.lightboxCaptionPlateMin;
            if( out_max ) *out_max = st.lightboxCaptionPlateMax;
            return true;
        }
        if( out_min ) *out_min = ImVec2( 0.0f, 0.0f );
        if( out_max ) *out_max = ImVec2( 0.0f, 0.0f );
        return false;
    }

    namespace MarkdownDetail
    {
        struct ScopedCaptionTypography
        {
            ImGuiWindow* win = nullptr;
            float        prevRawWinScale = 1.0f;
            float        prevClampOverride = 0.0f;
            bool         pushedCapFont = false;
            bool         changedWinScale = false;

            ScopedCaptionTypography( const MarkdownConfig& srcCfg, MarkdownConfig& outCapCfg )
            {
                outCapCfg = srcCfg;
                outCapCfg.showImageCaptions = false;
                outCapCfg.enableImageLightbox = false;
                outCapCfg.h1Font = nullptr;
                outCapCfg.h2Font = nullptr;
                outCapCfg.h3Font = nullptr;
                for( int hi = 0; hi < MarkdownConfig::NUMHEADINGS; ++hi ) {
                    outCapCfg.headingFormats[hi].font = nullptr;
                    outCapCfg.headingFormats[hi].separator = false;
                }

                win = ImGui::GetCurrentWindow();
                if( win ) {
                    prevRawWinScale = win->FontWindowScale;
                }
                prevClampOverride = GetActiveCaptionFontSizeOverride();

                ImFont* reg_font = srcCfg.regularFont ? srcCfg.regularFont : ImGui::GetFont();
                ImFont* cap_font = srcCfg.captionFont
                                       ? srcCfg.captionFont
                                       : ( GetCaptionFontResolver() ? GetCaptionFontResolver()() : nullptr );

                if( !srcCfg.captionFont && cap_font && reg_font ) {
                    const float cap_nat = GetNativeFontPixelSize( cap_font );
                    const float reg_nat = GetNativeFontPixelSize( reg_font );
                    if( cap_font == reg_font || ( cap_nat > 0.0f && reg_nat > 0.0f && cap_nat >= reg_nat - 0.1f ) ) {
                        cap_font = nullptr;
                    }
                }

                if( cap_font ) {
                    const float cap_nat = GetNativeFontPixelSize( cap_font );
                    if( cap_nat > 0.0f ) {
                        GetActiveCaptionFontSizeOverride() = cap_nat;
                    }
                    PushMarkdownFont( cap_font );
                    pushedCapFont = true;
                    outCapCfg.regularFont = cap_font;
                    outCapCfg.captionFont = cap_font;
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM < 19200)
                    if( cap_nat > 0.0f ) {
                        auto fitOrClear = [&]( ImFont*& slot ) {
                            if( slot && GetNativeFontPixelSize( slot ) > cap_nat + 0.5f ) {
                                slot = nullptr;
                            }
                        };
                        fitOrClear( outCapCfg.boldFont );
                        fitOrClear( outCapCfg.italicFont );
                        fitOrClear( outCapCfg.boldItalicFont );
                        fitOrClear( outCapCfg.monoFont );
                    }
#endif
                } else {
                    const float reg_nat = GetNativeFontPixelSize( reg_font );
                    if( reg_nat > 0.0f ) {
                        GetActiveCaptionFontSizeOverride() = reg_nat;
                    }
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM < 19200)
                    if( reg_nat > 0.0f ) {
                        auto fitOrClear = [&]( ImFont*& slot ) {
                            if( slot && GetNativeFontPixelSize( slot ) > reg_nat + 0.5f ) {
                                slot = nullptr;
                            }
                        };
                        fitOrClear( outCapCfg.boldFont );
                        fitOrClear( outCapCfg.italicFont );
                        fitOrClear( outCapCfg.boldItalicFont );
                        fitOrClear( outCapCfg.monoFont );
                    }
#endif
                    if( win ) {
                        const float eff_scale = ImGui::GetFontScale( false );
                        ImGui::ImGeeCompat::SetWindowFontScale( eff_scale * 0.875f );
                        changedWinScale = true;
                    }
                }
            }

            ~ScopedCaptionTypography()
            {
                if( pushedCapFont ) {
                    ImGui::PopFont();
                }
                if( changedWinScale && win ) {
                    ImGui::ImGeeCompat::SetWindowFontScale( prevRawWinScale );
                }
                GetActiveCaptionFontSizeOverride() = prevClampOverride;
            }
        };

        inline float MeasureCaptionWidthFallback( const char* text, size_t len, const MarkdownConfig& cfg )
        {
            if( !text || len == 0 ) return 0.0f;
            if( GetMeasureInlineWidthFn() ) {
                return GetMeasureInlineWidthFn()( text, len, &cfg );
            }
            TableCell cell;
            cell.start = text;
            cell.end = text + len;
            return TableDetail::CalcCellVisualWidth( cell, false, cfg );
        }

        inline void RenderCaptionInlineMarkdown( const char* text, size_t len, const MarkdownConfig& cfg )
        {
            if( !text || len == 0 ) return;
            ScopedSuspendFloatScope suspend_floats;
            if( GetRenderInlineCaptionFn() ) {
                GetRenderInlineCaptionFn()( text, len, &cfg );
                return;
            }
            Markdown::Render( text, len, cfg );
        }

        inline std::string CleanDisplayImageUrl( const char* raw_url, int raw_len )
        {
            if( !raw_url || raw_len <= 0 ) return std::string();
            std::string s( raw_url, (size_t)raw_len );
            size_t src_pos = s.find( "src=" );
            if( src_pos != std::string::npos && ( src_pos == 0 || s[src_pos - 1] == ' ' || s[src_pos - 1] == '\t' ) ) {
                std::string sub = s.substr( src_pos + 4 );
                if( !sub.empty() && ( sub.front() == '"' || sub.front() == '\'' ) ) {
                    char q = sub.front();
                    sub.erase( 0, 1 );
                    size_t qe = sub.find( q );
                    if( qe != std::string::npos ) sub.erase( qe );
                } else {
                    size_t sp = sub.find_first_of( " \t/>" );
                    if( sp != std::string::npos ) sub.erase( sp );
                }
                return sub;
            }
            size_t st = 0;
            while( st < s.size() && ( s[st] == ' ' || s[st] == '\t' ) ) ++st;
            if( st < s.size() && s[st] == '<' ) ++st;
            size_t en = st;
            while( en < s.size() && s[en] != ' ' && s[en] != '\t' && s[en] != '>' && s[en] != '{' ) ++en;
            return s.substr( st, en - st );
        }

        struct MarkdownImageRenderResult
        {
            bool   drawnImage = false;
            bool   isBlockFigure = false;
            ImVec2 imageRectMin = ImVec2( 0.0f, 0.0f );
            ImVec2 imageRectMax = ImVec2( 0.0f, 0.0f );
        };

        inline MarkdownImageRenderResult RenderMarkdownImageElement(
            const MarkdownConfig& mdConfig_,
            const MarkdownImageData& imageData,
            const char* alt_ptr,
            int alt_len,
            const char* raw_url_ptr,
            int raw_url_len,
            bool allow_link_callback )
        {
            MarkdownImageRenderResult res;

            std::string effective_alt = ( alt_ptr && alt_len > 0 )
                                            ? std::string( alt_ptr, (size_t)alt_len )
                                            : imageData.altOverride;
            std::string caption_text = !imageData.customCaption.empty()
                                           ? imageData.customCaption
                                           : effective_alt;

            bool show_caption = false;
            if( imageData.captionMode < 0 ) {
                show_caption = false;
            } else if( imageData.captionMode > 0 ) {
                show_caption = !caption_text.empty();
            } else if( mdConfig_.showImageCaptions ) {
                show_caption = !caption_text.empty();
            }

            const bool explicit_lightbox = ( imageData.lightboxMode > 0 );
            const bool suppressed_lightbox = ( imageData.lightboxMode < 0 );
            MarkdownLinkCallback* linkCb = mdConfig_.linkCallback ? mdConfig_.linkCallback : GetDefaultMarkdownLinkCallback();
            const bool has_link_cb = ( allow_link_callback && imageData.useLinkCallback && linkCb != nullptr );

            bool use_lightbox = false;
            if( !suppressed_lightbox ) {
                if( explicit_lightbox ) {
                    use_lightbox = true;
                } else if( mdConfig_.enableImageLightbox && !imageData.useLinkCallback ) {
                    use_lightbox = true;
                }
            }

            ImGuiWindow* cur_win = ImGui::GetCurrentWindow();
            const unsigned int img_ordinal = NextWindowImageOrdinal( cur_win );
            ImGuiID content_hash = 0x811C9DC5u;
            if( raw_url_ptr && raw_url_len > 0 ) {
                content_hash = ImHashData( raw_url_ptr, (size_t)raw_url_len, content_hash );
            }
            if( !effective_alt.empty() ) {
                content_hash = ImHashData( effective_alt.c_str(), effective_alt.size(), content_hash );
            }

            char popup_str_id[80];
            std::snprintf( popup_str_id, sizeof(popup_str_id), "##imgee_md_lb_%u_%08x", img_ordinal, (unsigned int)content_hash );
            char tip_str_id[80];
            std::snprintf( tip_str_id, sizeof(tip_str_id), "##imgee_md_imgtip_%u_%08x", img_ordinal, (unsigned int)content_hash );
            char thumb_btn_str_id[80];
            std::snprintf( thumb_btn_str_id, sizeof(thumb_btn_str_id), "##imgee_md_imgbtn_%u_%08x", img_ordinal, (unsigned int)content_hash );
            const ImGuiID popup_id = ImGui::GetID( popup_str_id );
            const ImGuiID tip_id = ImGui::GetID( tip_str_id );
            const ImGuiID thumb_btn_id = ImGui::GetID( thumb_btn_str_id );

            if( !imageData.isValid ) {
                if( use_lightbox ) {
                    ShowImageLightboxTooltip( mdConfig_, tip_id, false, "Click to enlarge", false );
                }
                std::string disp = CleanDisplayImageUrl( raw_url_ptr, raw_url_len );
                if( imageData.isLoading ) {
                    ImGui::TextDisabled( "( Loading %s... )", disp.c_str() );
                } else {
                    ImGui::Text( "( Image %s not loaded )", disp.c_str() );
                }
                return res;
            }

            const bool parent_aligned = GetActiveFullLineWidthContext().parentAligned;
            FloatLayoutScope* float_scope = GetActiveFloatScope();
            MarkdownImageFloat effective_float = ( !parent_aligned && float_scope != nullptr )
                                                     ? imageData.floatMode
                                                     : MarkdownImageFloat::None;

            const float full_avail_w = ResolveFullLineAvailWidth( &mdConfig_ );
            ImVec2 render_size = imageData.size;
            if( full_avail_w > 1.0f && render_size.x > full_avail_w ) {
                const float clamp_s = full_avail_w / render_size.x;
                render_size.x = full_avail_w;
                render_size.y = std::round( render_size.y * clamp_s );
            }
            if( render_size.x < 1.0f ) render_size.x = 1.0f;
            if( render_size.y < 1.0f ) render_size.y = 1.0f;

            float vis_scale = ImGui::GetVisualScale();
            if( vis_scale <= 0.0f ) vis_scale = 1.0f;
            const float float_h_gap = ( imageData.floatGapX >= 0.0f ) ? imageData.floatGapX : std::round( 12.0f * vis_scale );
            const float float_v_gap = ( imageData.floatGapY >= 0.0f ) ? imageData.floatGapY : std::round( 6.0f * vis_scale );
            const float min_readable_text_w = std::round( 90.0f * vis_scale );

            if( effective_float != MarkdownImageFloat::None ) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                if( cur_win && cur_win->DC.IsSameLine ) {
                    ImGui::NewLine();
                }
#endif
                // Responsive narrow-viewport fallback: if container is too narrow to leave a readable
                // text column beside this image, clear active floats and render as a standard block figure!
                if( full_avail_w > 1.0f && ( full_avail_w - ( render_size.x + float_h_gap ) ) < min_readable_text_w ) {
                    ClearActiveFloats();
                    effective_float = MarkdownImageFloat::None;
                }
            }

            float float_top_y = 0.0f;
            float margin_left_screen_x = ImGui::GetCursorScreenPos().x;

            if( effective_float != MarkdownImageFloat::None && float_scope != nullptr ) {
                res.isBlockFigure = true;
                float_top_y = ImGui::GetCursorScreenPos().y;
                float left_in = 0.0f, right_in = 0.0f;
                if( QueryFloatBandAtY( float_top_y, render_size.y, left_in, right_in ) ) {
                    const float band_w = ( float_scope->containerRightX - float_scope->containerLeftX ) - left_in - right_in;
                    if( ( band_w - ( render_size.x + float_h_gap ) ) < min_readable_text_w ) {
                        // Stack below conflicting active floats when horizontal room is exhausted
                        ClearActiveFloats();
                        float_top_y = ImGui::GetCursorScreenPos().y;
                        left_in = 0.0f;
                        right_in = 0.0f;
                        QueryFloatBandAtY( float_top_y, render_size.y, left_in, right_in );
                    }
                }

                margin_left_screen_x = float_scope->containerLeftX;
                const float band_left_x = float_scope->containerLeftX + left_in;
                const float band_right_x = float_scope->containerRightX - right_in;
                const float fig_x = ( effective_float == MarkdownImageFloat::Right )
                                        ? std::round( band_right_x - render_size.x )
                                        : std::round( band_left_x );
                ImGui::SetCursorScreenPos( ImVec2( fig_x, float_top_y ) );
            } else {
                const bool has_explicit_align = ( imageData.align != MarkdownImageAlign::None );
                res.isBlockFigure = has_explicit_align || show_caption || ( imageData.floatMode != MarkdownImageFloat::None );

                if( !parent_aligned && ( imageData.align == MarkdownImageAlign::Center || imageData.align == MarkdownImageAlign::Right ) ) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                    if( cur_win && cur_win->DC.IsSameLine ) {
                        ImGui::NewLine();
                    }
#endif
                }
                margin_left_screen_x = ImGui::GetCursorScreenPos().x;

                if( !parent_aligned ) {
                    float offset_x = 0.0f;
                    if( imageData.align == MarkdownImageAlign::Center && full_avail_w > render_size.x ) {
                        offset_x = std::round( ( full_avail_w - render_size.x ) * 0.5f );
                    } else if( imageData.align == MarkdownImageAlign::Right && full_avail_w > render_size.x ) {
                        offset_x = std::round( full_avail_w - render_size.x );
                    }
                    if( offset_x > 0.0f ) {
                        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset_x );
                    }
                }
            }

            if( show_caption ) {
                ImGui::BeginGroup();
            }

            ImGui::ImGeeCompat::Image( imageData.user_texture_id, render_size, imageData.uv0, imageData.uv1, imageData.tint_col, imageData.border_col );
            res.drawnImage = true;
            res.imageRectMin = ImGui::GetItemRectMin();
            res.imageRectMax = ImGui::GetItemRectMax();

            LightboxTelemetryState& telem = GetLightboxTelemetryState();
            telem.lastInlineImageMin = res.imageRectMin;
            telem.lastInlineImageMax = res.imageRectMax;

            bool img_hovered = false;
            bool img_released = false;
            bool img_clicked_down = false;
            if( use_lightbox || has_link_cb ) {
                const ImRect thumb_r( res.imageRectMin, res.imageRectMax );
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18300
                ImGui::ItemAdd( thumb_r, thumb_btn_id, NULL );
#else
                ImGui::ItemAdd( thumb_r, thumb_btn_id, NULL, ImGuiItemFlags_NoNav );
#endif
                bool thumb_held = false;
                img_released = ImGui::ButtonBehavior( thumb_r, thumb_btn_id, &img_hovered, &thumb_held );
                img_clicked_down = thumb_held;
            } else {
                img_hovered = ImGui::IsItemHovered();
            }

            if( show_caption ) {
                float scale = vis_scale;
                const float cap_gap_y = std::round( 4.0f * scale );

                {
                    MarkdownConfig cap_cfg;
                    ScopedCaptionTypography cap_typo( mdConfig_, cap_cfg );
                    ScopedSuspendFloatScope cap_no_floats;

                    const float cap_single_w = MeasureCaptionWidthFallback( caption_text.c_str(), caption_text.size(), cap_cfg );
                    const float img_center_x = ( res.imageRectMin.x + res.imageRectMax.x ) * 0.5f;
                    const float min_readable_w = ( effective_float != MarkdownImageFloat::None )
                                                     ? render_size.x
                                                     : ImMin( cap_single_w, 180.0f * scale );
                    const float max_cap_w = ( full_avail_w > 10.0f )
                                                ? ImMin( full_avail_w, ImMax( render_size.x, min_readable_w ) )
                                                : ImMax( render_size.x, cap_single_w );

                    const ImVec4 base_txt = ImGui::GetStyle().Colors[ImGuiCol_Text];
                    const ImVec4 cap_col( base_txt.x, base_txt.y, base_txt.z, base_txt.w * 0.75f );

                    if( cap_single_w <= max_cap_w + 0.5f ) {
                        float cap_x = std::round( img_center_x - cap_single_w * 0.5f );
                        if( full_avail_w > 10.0f && !parent_aligned ) {
                            if( cap_x + cap_single_w > margin_left_screen_x + full_avail_w ) {
                                cap_x = margin_left_screen_x + full_avail_w - cap_single_w;
                            }
                            if( cap_x < margin_left_screen_x ) {
                                cap_x = margin_left_screen_x;
                            }
                        }
                        const ImVec2 cap_pos( cap_x, res.imageRectMax.y + cap_gap_y );
                        ImGui::SetCursorScreenPos( cap_pos );
                        cap_cfg.wrapWidth = -1.0f;
                        ImGui::PushStyleColor( ImGuiCol_Text, cap_col );
                        ScopedFullLineWidth cap_w_scope( cap_single_w, true );
                        RenderCaptionInlineMarkdown( caption_text.c_str(), caption_text.size(), cap_cfg );
                        ImGui::PopStyleColor();
                        telem.lastCaptionMin = cap_pos;
                        telem.lastCaptionMax = ImVec2( cap_pos.x + cap_single_w, ImMax( cap_pos.y + ImGui::GetTextLineHeight(), ImGui::GetItemRectMax().y ) );
                    } else {
                        float cap_x = std::round( img_center_x - max_cap_w * 0.5f );
                        if( full_avail_w > 10.0f && !parent_aligned ) {
                            if( cap_x + max_cap_w > margin_left_screen_x + full_avail_w ) {
                                cap_x = margin_left_screen_x + full_avail_w - max_cap_w;
                            }
                            if( cap_x < margin_left_screen_x ) {
                                cap_x = margin_left_screen_x;
                            }
                        }
                        const ImVec2 cap_pos( cap_x, res.imageRectMax.y + cap_gap_y );
                        ImGui::SetCursorScreenPos( cap_pos );
                        cap_cfg.wrapWidth = max_cap_w;
                        ImGui::PushStyleColor( ImGuiCol_Text, cap_col );
                        ImGui::BeginGroup();
                        ScopedFullLineWidth cap_w_scope( max_cap_w, true );
                        RenderCaptionInlineMarkdown( caption_text.c_str(), caption_text.size(), cap_cfg );
                        ImGui::EndGroup();
                        ImGui::PopStyleColor();
                        telem.lastCaptionMin = ImGui::GetItemRectMin();
                        telem.lastCaptionMax = ImGui::GetItemRectMax();
                    }
                }

                ImGui::EndGroup();
            }

            if( effective_float != MarkdownImageFloat::None && float_scope != nullptr ) {
                const ImVec2 fig_min = ImGui::GetItemRectMin();
                const ImVec2 fig_max = ImGui::GetItemRectMax();
                FloatBox fbox;
                fbox.side = effective_float;
                fbox.minY = float_top_y;
                fbox.maxY = fig_max.y + float_v_gap;
                fbox.xMin = ImMin( res.imageRectMin.x, fig_min.x );
                fbox.xMax = ImMax( res.imageRectMax.x, fig_max.x );
                fbox.hGap = float_h_gap;
                float_scope->boxes.push_back( fbox );
                if( fbox.maxY > float_scope->deepestMaxY ) {
                    float_scope->deepestMaxY = fbox.maxY;
                }
                float_scope->justPlacedFloat = true;

                // Rewind cursor back to float_top_y so subsequent prose/headings/lists wrap alongside the figure!
                const float natural_start_x = GetWindowNaturalLineStartX( cur_win );
                ImGui::SetCursorScreenPos( ImVec2( natural_start_x, float_top_y ) );
                if( cur_win ) {
                    cur_win->DC.CurrLineSize = ImVec2( 0.0f, 0.0f );
                    cur_win->DC.CurrLineTextBaseOffset = 0.0f;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                    cur_win->DC.IsSameLine = false;
                    cur_win->DC.IsSetPos = false;
#endif
                }
            }

            if( use_lightbox || has_link_cb ) {
                if( img_hovered ) {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                }
            }

            if( use_lightbox ) {
                const bool popup_was_open = ImGui::IsPopupOpen( popup_id, ImGuiPopupFlags_None );
                const std::string tip_md = ( !show_caption && !effective_alt.empty() )
                                               ? ( "**" + effective_alt + "**\nClick to enlarge" )
                                               : std::string( "Click to enlarge" );
                ShowImageLightboxTooltip( mdConfig_, tip_id, img_hovered && !popup_was_open, tip_md.c_str(), img_clicked_down || img_released );

                if( img_released && !popup_was_open ) {
                    ClearActiveTooltip();
                    ImGui::OpenPopup( popup_str_id );
                }

                if( ImGui::IsPopupOpen( popup_id, ImGuiPopupFlags_None ) ) {
                    ImGuiWindow* root_win = cur_win ? ( cur_win->RootWindow ? cur_win->RootWindow : cur_win ) : nullptr;
                    ImVec2 host_min = root_win ? root_win->InnerRect.Min : ImVec2( 0.0f, 0.0f );
                    ImVec2 host_max = root_win ? root_win->InnerRect.Max : ImGui::GetIO().DisplaySize;
                    if( host_max.x - host_min.x < 32.0f || host_max.y - host_min.y < 32.0f ) {
                        if( root_win && root_win->Size.x >= 32.0f && root_win->Size.y >= 32.0f ) {
                            host_min = root_win->Pos;
                            host_max = ImVec2( root_win->Pos.x + root_win->Size.x, root_win->Pos.y + root_win->Size.y );
                        } else {
                            host_min = ImVec2( 0.0f, 0.0f );
                            host_max = ImGui::GetIO().DisplaySize;
                        }
                    }
                    const ImVec2 host_size( ImMax( 32.0f, host_max.x - host_min.x ), ImMax( 32.0f, host_max.y - host_min.y ) );

                    ImGui::SetNextWindowPos( host_min, ImGuiCond_Always );
                    ImGui::SetNextWindowSize( host_size, ImGuiCond_Always );
                    ImGui::PushStyleColor( ImGuiCol_PopupBg, ImVec4( 0.02f, 0.03f, 0.05f, 0.94f ) );
                    ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.35f, 0.42f, 0.52f, 0.55f ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );

                    const ImGuiWindowFlags lb_flags =
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse |
                        ImGuiWindowFlags_NoCollapse;

                    if( ImGui::BeginPopupEx( popup_id, lb_flags ) ) {
                        if( ImGuiWindow* lb_win = ImGui::GetCurrentWindow() ) {
                            // Size and position are explicitly supplied upfront via SetNextWindowPos/Size,
                            // so prevent Dear ImGui's auto-resize 1-frame popup hide flash on first open:
                            lb_win->HiddenFramesCanSkipItems = 0;
                            lb_win->Hidden = false;
                        }
                        ImGui::ImGeeCompat::SetWindowFontScale( 1.0f );
                        const bool is_appearing = ImGui::IsWindowAppearing();
                        bool should_close = false;

                        LightboxViewState& lb_view = GetLightboxViewState();
                        if( is_appearing || lb_view.popupId != popup_id ) {
                            lb_view.ResetForPopup( popup_id );
                        }

                        const float pad = 10.0f;
                        const float close_sz = 28.0f;
                        const ImVec2 close_min( host_min.x + host_size.x - pad - close_sz, host_min.y + pad );
                        const ImVec2 close_max( close_min.x + close_sz, close_min.y + close_sz );

                        // 1. Register [X] close button first so it always has top hit-test priority:
                        ImGui::SetCursorScreenPos( close_min );
                        const bool close_pressed = ImGui::InvisibleButton( "##imgee_lb_close", ImVec2( close_sz, close_sz ) );
                        const bool close_hovered = ImGui::IsItemHovered();
                        const bool close_active = ImGui::IsItemActive();
                        if( close_hovered ) {
                            ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                        }
                        if( close_pressed ) {
                            should_close = true;
                        }
                        MarkdownDetail::ShowImageLightboxTooltip( mdConfig_, ImGui::GetID( "##imgee_lb_close_tip" ), close_hovered, "Close Lightbox (or click outside)" );

                        // 1b. Top-right hover zoom control strip docked immediately to the LEFT of [X]:
                        //     [-]  [100%]  [+]  [Reset/Restore]
                        const bool show_zoom_strip =
                            ( host_size.x >= 220.0f ) &&
                            ( ImGui::IsMouseHoveringRect( host_min, host_max, false ) || std::fabs( lb_view.zoom - 1.0f ) > 0.005f );
                        const float strip_pad_x = 3.0f;
                        const float strip_gap_x = 3.0f;
                        const float z_dec_w = 26.0f;
                        const float z_pct_w = 50.0f;
                        const float z_inc_w = 26.0f;
                        const float z_rst_w = 28.0f;
                        const float strip_total_w = strip_pad_x * 2.0f + z_dec_w + strip_gap_x + z_pct_w + strip_gap_x + z_inc_w + strip_gap_x + z_rst_w;
                        const ImVec2 strip_max( close_min.x - 6.0f, close_max.y );
                        const ImVec2 strip_min( strip_max.x - strip_total_w, close_min.y );

                        bool z_dec_hov = false, z_dec_act = false;
                        bool z_pct_hov = false, z_pct_act = false;
                        bool z_inc_hov = false, z_inc_act = false;
                        bool z_rst_hov = false, z_rst_act = false;
                        bool any_strip_hovered = false;

                        const float bx_dec = strip_min.x + strip_pad_x;
                        const float bx_pct = bx_dec + z_dec_w + strip_gap_x;
                        const float bx_inc = bx_pct + z_pct_w + strip_gap_x;
                        const float bx_rst = bx_inc + z_inc_w + strip_gap_x;
                        const float btn_in_h = close_sz - 4.0f;
                        const float btn_in_y = strip_min.y + 2.0f;

                        if( show_zoom_strip ) {
                            const bool at_min_z = ( lb_view.zoom <= 0.501f );
                            const bool at_max_z = ( lb_view.zoom >= 3.999f );
                            const bool is_z_mod = ( std::fabs( lb_view.zoom - 1.0f ) > 0.005f );
                            const bool can_z_restore = !is_z_mod && ( std::fabs( lb_view.lastZoom - 1.0f ) > 0.005f );

                            ImGui::SetCursorScreenPos( ImVec2( bx_dec, btn_in_y ) );
                            const bool dec_clicked = ImGui::InvisibleButton( "##imgee_lb_zdec", ImVec2( z_dec_w, btn_in_h ) );
                            z_dec_hov = ImGui::IsItemHovered();
                            z_dec_act = ImGui::IsItemActive();
                            if( z_dec_hov && !at_min_z ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            if( dec_clicked && !at_min_z ) lb_view.SetZoom( lb_view.zoom - 0.25f );
                            MarkdownDetail::ShowImageLightboxTooltip( mdConfig_, ImGui::GetID( "##imgee_lb_zdec_tip" ), z_dec_hov, "Zoom Out (Wheel Down)" );

                            ImGui::SetCursorScreenPos( ImVec2( bx_pct, btn_in_y ) );
                            const bool pct_clicked = ImGui::InvisibleButton( "##imgee_lb_zpct", ImVec2( z_pct_w, btn_in_h ) );
                            z_pct_hov = ImGui::IsItemHovered();
                            z_pct_act = ImGui::IsItemActive();
                            if( z_pct_hov && ( is_z_mod || can_z_restore ) ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            if( pct_clicked ) lb_view.ToggleResetZoom();
                            MarkdownDetail::ShowImageLightboxTooltip( mdConfig_, ImGui::GetID( "##imgee_lb_zpct_tip" ), z_pct_hov, is_z_mod ? "Reset to Fitted View (Click or 1-click Reset)" : "Restore Previous Zoom" );

                            ImGui::SetCursorScreenPos( ImVec2( bx_inc, btn_in_y ) );
                            const bool inc_clicked = ImGui::InvisibleButton( "##imgee_lb_zinc", ImVec2( z_inc_w, btn_in_h ) );
                            z_inc_hov = ImGui::IsItemHovered();
                            z_inc_act = ImGui::IsItemActive();
                            if( z_inc_hov && !at_max_z ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            if( inc_clicked && !at_max_z ) lb_view.SetZoom( lb_view.zoom + 0.25f );
                            MarkdownDetail::ShowImageLightboxTooltip( mdConfig_, ImGui::GetID( "##imgee_lb_zinc_tip" ), z_inc_hov, "Zoom In (Wheel Up)" );

                            ImGui::SetCursorScreenPos( ImVec2( bx_rst, btn_in_y ) );
                            const bool rst_clicked = ImGui::InvisibleButton( "##imgee_lb_zrst", ImVec2( z_rst_w, btn_in_h ) );
                            z_rst_hov = ImGui::IsItemHovered();
                            z_rst_act = ImGui::IsItemActive();
                            if( z_rst_hov && ( is_z_mod || can_z_restore ) ) ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            if( rst_clicked ) lb_view.ToggleResetZoom();
                            MarkdownDetail::ShowImageLightboxTooltip( mdConfig_, ImGui::GetID( "##imgee_lb_zrst_tip" ), z_rst_hov, can_z_restore ? "Restore Previous Zoom" : "Reset Zoom & Pan" );

                            any_strip_hovered = z_dec_hov || z_pct_hov || z_inc_hov || z_rst_hov ||
                                                z_dec_act || z_pct_act || z_inc_act || z_rst_act;
                        }

                        ImDrawList* lb_dl = ImGui::GetWindowDrawList();

                        const bool has_lb_caption = !caption_text.empty();
                        const float side_margin = 20.0f;
                        const float top_margin = pad + close_sz + 8.0f;
                        const float cap_pad_x = 12.0f;
                        const float cap_pad_y = 6.0f;
                        const float cap_rounding = 6.0f;
                        const float cap_img_gap = has_lb_caption ? 8.0f : 0.0f;
                        const float bot_outer_margin = has_lb_caption ? 10.0f : 14.0f;
                        const float avail_lb_w = ImMax( 24.0f, host_size.x - side_margin * 2.0f );
                        const float max_cap_text_w = ImMax( 16.0f, avail_lb_w - cap_pad_x * 2.0f );

                        ImDrawListUnderlayScope img_underlay( lb_dl, has_lb_caption );
                        ImDrawListUnderlayScope cap_underlay( lb_dl, has_lb_caption );

                        ImVec2 raw_cap_min( 0.0f, 0.0f );
                        ImVec2 raw_cap_max( 0.0f, 0.0f );
                        float cap_actual_w = 0.0f;
                        float cap_actual_h = 0.0f;
                        float plate_w = 0.0f;
                        float plate_h = 0.0f;
                        int cap_vtx_start = 0;
                        int cap_vtx_end = 0;
                        ImGuiID cap_hash = 0;
                        int max_cap_w_px = 0;

                        if( has_lb_caption ) {
                            MarkdownConfig lb_cap_cfg;
                            ScopedCaptionTypography lb_cap_typo( mdConfig_, lb_cap_cfg );
                            ScopedSuspendFloatScope lb_cap_no_floats;

                            const float target_lb_cap_w = ImMin( max_cap_text_w, ImMax( 520.0f, avail_lb_w * 0.68f ) );
                            const float lb_cap_single_w = MeasureCaptionWidthFallback( caption_text.c_str(), caption_text.size(), lb_cap_cfg );
                            const float clamped_cap_w = ImMin( target_lb_cap_w, lb_cap_single_w );
                            lb_cap_cfg.wrapWidth = ( lb_cap_single_w <= target_lb_cap_w + 0.5f ) ? -1.0f : target_lb_cap_w;

                            cap_hash = ImHashData( caption_text.c_str(), caption_text.size(), 0x9E3779B9u );
                            max_cap_w_px = static_cast<int>( std::round( target_lb_cap_w ) );

                            ImVec2 initial_cap_pos(
                                std::round( host_min.x + ( host_size.x - clamped_cap_w ) * 0.5f ),
                                host_min.y + top_margin );
                            LightboxCaptionCache& cap_cache = GetLightboxCaptionCache();
                            for( int ci = 0; ci < 8; ++ci ) {
                                const LightboxCaptionCacheEntry& ce = cap_cache.entries[ci];
                                if( ce.valid && ce.popupId == popup_id && ce.textHash == cap_hash && ce.maxWidthPx == max_cap_w_px ) {
                                    initial_cap_pos = ce.finalTextPos;
                                    break;
                                }
                            }

                            cap_vtx_start = lb_dl->VtxBuffer.Size;
                            ImGui::SetCursorScreenPos( initial_cap_pos );
                            ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.92f, 0.95f, 0.99f, 0.96f ) );
                            ImGui::BeginGroup();
                            {
                                ScopedFullLineWidth lb_cap_scope( clamped_cap_w, true );
                                RenderCaptionInlineMarkdown( caption_text.c_str(), caption_text.size(), lb_cap_cfg );
                            }
                            ImGui::EndGroup();
                            ImGui::PopStyleColor();
                            cap_vtx_end = lb_dl->VtxBuffer.Size;

                            raw_cap_min = ImGui::GetItemRectMin();
                            raw_cap_max = ImGui::GetItemRectMax();
                            cap_actual_w = ImMax( 1.0f, raw_cap_max.x - raw_cap_min.x );
                            cap_actual_h = ImMax( ImGui::GetTextLineHeight(), raw_cap_max.y - raw_cap_min.y );
                            plate_w = ImMin( avail_lb_w, cap_actual_w + cap_pad_x * 2.0f );
                            plate_h = cap_actual_h + cap_pad_y * 2.0f;
                        }

                        const float total_avail_h = ImMax( 24.0f, host_size.y - top_margin - bot_outer_margin );
                        const float avail_lb_h = has_lb_caption
                                                     ? ImMax( 24.0f, total_avail_h - ( cap_img_gap + plate_h ) )
                                                     : total_avail_h;

                        const ImVec2 nat = ( imageData.native_size.x > 0.0f && imageData.native_size.y > 0.0f )
                                               ? imageData.native_size
                                               : render_size;
                        float fit_scale = ( std::min )( avail_lb_w / ImMax( 1.0f, nat.x ), avail_lb_h / ImMax( 1.0f, nat.y ) );
                        const float thumb_ratio = ( std::max )( render_size.x / ImMax( 1.0f, nat.x ), render_size.y / ImMax( 1.0f, nat.y ) );
                        const float max_upscale = ( std::max )( 3.0f, thumb_ratio * 1.5f );
                        if( fit_scale > max_upscale ) {
                            fit_scale = max_upscale;
                        }
                        ImVec2 base_img_sz( std::round( nat.x * fit_scale ), std::round( nat.y * fit_scale ) );
                        if( base_img_sz.x < 1.0f ) base_img_sz.x = 1.0f;
                        if( base_img_sz.y < 1.0f ) base_img_sz.y = 1.0f;

                        //Compute zoomed dimensions & clamped viewport bounds (never overlapping top strip or bottom caption):
                        ImVec2 scaled_img_sz(
                            ImMax( 1.0f, std::round( base_img_sz.x * lb_view.zoom ) ),
                            ImMax( 1.0f, std::round( base_img_sz.y * lb_view.zoom ) ) );
                        const float vis_w = ImMin( avail_lb_w, scaled_img_sz.x );
                        const float vis_h = ImMin( avail_lb_h, scaled_img_sz.y );
                        const float combo_h = vis_h + ( has_lb_caption ? ( cap_img_gap + plate_h ) : 0.0f );
                        const ImVec2 vis_img_min(
                            std::round( host_min.x + ( host_size.x - vis_w ) * 0.5f ),
                            std::round( host_min.y + top_margin + ImMax( 0.0f, ( total_avail_h - combo_h ) * 0.5f ) ) );
                        const ImVec2 vis_img_max( vis_img_min.x + vis_w, vis_img_min.y + vis_h );

                        const ImVec2 lb_img_min = vis_img_min;
                        const ImVec2 lb_img_max = vis_img_max;

                        ImVec2 plate_min( 0.0f, 0.0f );
                        ImVec2 plate_max( 0.0f, 0.0f );
                        if( has_lb_caption ) {
                            plate_min.x = std::round( host_min.x + ( host_size.x - plate_w ) * 0.5f );
                            plate_max.x = plate_min.x + std::round( plate_w );
                            plate_min.y = std::round( vis_img_max.y + cap_img_gap );
                            plate_max.y = plate_min.y + std::round( plate_h );

                            if( plate_max.y > host_max.y - 2.0f ) {
                                const float shift_up = plate_max.y - ( host_max.y - 2.0f );
                                plate_min.y -= shift_up;
                                plate_max.y -= shift_up;
                            }
                            if( plate_min.y < host_min.y + 2.0f ) {
                                plate_min.y = host_min.y + 2.0f;
                            }
                            if( plate_min.x < host_min.x + 2.0f ) plate_min.x = host_min.x + 2.0f;
                            if( plate_max.x > host_max.x - 2.0f ) plate_max.x = host_max.x - 2.0f;

                            const ImVec2 final_cap_pos(
                                std::round( plate_min.x + ( ( plate_max.x - plate_min.x ) - cap_actual_w ) * 0.5f ),
                                std::round( plate_min.y + cap_pad_y ) );
                            const float dx = final_cap_pos.x - raw_cap_min.x;
                            const float dy = final_cap_pos.y - raw_cap_min.y;
                            if( std::fabs( dx ) > 0.01f || std::fabs( dy ) > 0.01f ) {
                                for( int vi = cap_vtx_start; vi < cap_vtx_end && vi < lb_dl->VtxBuffer.Size; ++vi ) {
                                    lb_dl->VtxBuffer.Data[vi].pos.x += dx;
                                    lb_dl->VtxBuffer.Data[vi].pos.y += dy;
                                }
                            }

                            LightboxCaptionCache& cap_cache = GetLightboxCaptionCache();
                            int slot = -1;
                            for( int ci = 0; ci < 8; ++ci ) {
                                const LightboxCaptionCacheEntry& ce = cap_cache.entries[ci];
                                if( ce.valid && ce.popupId == popup_id && ce.textHash == cap_hash && ce.maxWidthPx == max_cap_w_px ) {
                                    slot = ci;
                                    break;
                                }
                            }
                            if( slot < 0 ) {
                                slot = cap_cache.nextSlot & 7;
                                cap_cache.nextSlot = ( cap_cache.nextSlot + 1 ) & 7;
                            }
                            cap_cache.entries[slot].popupId = popup_id;
                            cap_cache.entries[slot].textHash = cap_hash;
                            cap_cache.entries[slot].maxWidthPx = max_cap_w_px;
                            cap_cache.entries[slot].finalTextPos = final_cap_pos;
                            cap_cache.entries[slot].valid = true;

                            cap_underlay.BeginBackground();
                            lb_dl->AddRectFilled( plate_min, plate_max, IM_COL32( 10, 14, 22, 245 ), cap_rounding );
                            AddRectCompat( lb_dl, plate_min, plate_max, IM_COL32( 255, 255, 255, 34 ), cap_rounding, 0, 1.0f );
                            cap_underlay.EndUnderlay();
                        }

                        // 2. Interactive zoom & pan surface over the enlarged image viewport:
                        {
                            const ImRect lb_img_r( vis_img_min, vis_img_max );
                            const ImGuiID lb_img_id = ImGui::GetID( "##imgee_lb_image" );
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18300
                            ImGui::ItemAdd( lb_img_r, lb_img_id, NULL );
#else
                            ImGui::ItemAdd( lb_img_r, lb_img_id, NULL, ImGuiItemFlags_NoNav );
#endif
                            bool lb_img_hov = false, lb_img_held = false;
                            const bool lb_img_clicked = ImGui::ButtonBehavior( lb_img_r, lb_img_id, &lb_img_hov, &lb_img_held );

                            ImGuiIO& lb_io = ImGui::GetIO();
                            if( ( lb_img_hov || any_strip_hovered ) && std::fabs( lb_io.MouseWheel ) > 0.0001f ) {
                                lb_view.SetZoom( lb_view.zoom + 0.10f * lb_io.MouseWheel );
                                lb_io.MouseWheel = 0.0f;
                                scaled_img_sz = ImVec2(
                                    ImMax( 1.0f, std::round( base_img_sz.x * lb_view.zoom ) ),
                                    ImMax( 1.0f, std::round( base_img_sz.y * lb_view.zoom ) ) );
                            }

                            const bool can_pan = ( lb_view.zoom > 1.001f ) &&
                                                 ( scaled_img_sz.x > vis_w + 0.5f || scaled_img_sz.y > vis_h + 0.5f );
                            if( lb_img_held ) {
                                if( can_pan && ImGui::IsMouseDragging( 0, 2.0f ) ) {
                                    lb_view.wasDragging = true;
                                    lb_view.pan.x += lb_io.MouseDelta.x;
                                    lb_view.pan.y += lb_io.MouseDelta.y;
                                    ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeAll );
                                } else if( lb_img_hov ) {
                                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                                }
                            } else if( lb_img_hov ) {
                                ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            }

                            if( lb_img_clicked ) {
                                if( !lb_view.wasDragging ) {
                                    if( std::fabs( lb_view.zoom - 1.0f ) > 0.05f ) {
                                        lb_view.lastZoom = lb_view.zoom;
                                        lb_view.SetZoom( 1.0f );
                                    } else {
                                        lb_view.lastZoom = 1.0f;
                                        lb_view.SetZoom( 2.0f );
                                    }
                                    scaled_img_sz = ImVec2(
                                        ImMax( 1.0f, std::round( base_img_sz.x * lb_view.zoom ) ),
                                        ImMax( 1.0f, std::round( base_img_sz.y * lb_view.zoom ) ) );
                                }
                                lb_view.wasDragging = false;
                            } else if( !lb_img_held ) {
                                lb_view.wasDragging = false;
                            }
                        }

                        // Clamp pan so scaled artwork edges never pull inward past the viewport frame:
                        const float max_pan_x = ImMax( 0.0f, ( scaled_img_sz.x - vis_w ) * 0.5f );
                        const float max_pan_y = ImMax( 0.0f, ( scaled_img_sz.y - vis_h ) * 0.5f );
                        lb_view.pan.x = ImClamp( lb_view.pan.x, -max_pan_x, max_pan_x );
                        lb_view.pan.y = ImClamp( lb_view.pan.y, -max_pan_y, max_pan_y );

                        const ImVec2 draw_img_min(
                            std::round( ( vis_img_min.x + vis_img_max.x - scaled_img_sz.x ) * 0.5f + lb_view.pan.x ),
                            std::round( ( vis_img_min.y + vis_img_max.y - scaled_img_sz.y ) * 0.5f + lb_view.pan.y ) );

                        if( has_lb_caption ) {
                            img_underlay.BeginBackground();
                        }
                        lb_dl->PushClipRect( ImVec2( vis_img_min.x - 1.0f, vis_img_min.y - 1.0f ),
                                             ImVec2( vis_img_max.x + 1.0f, vis_img_max.y + 1.0f ), true );
                        ImGui::SetCursorScreenPos( draw_img_min );
                        ImGui::ImGeeCompat::Image(
                            imageData.user_texture_id,
                            scaled_img_sz,
                            imageData.uv0,
                            imageData.uv1,
                            imageData.tint_col,
                            ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
                        lb_dl->PopClipRect();
                        AddRectCompat( lb_dl, vis_img_min, vis_img_max, IM_COL32( 184, 199, 224, 128 ), 0.0f, 0, 1.0f );
                        if( has_lb_caption ) {
                            img_underlay.EndUnderlay();
                        }

                        // Draw top-right zoom control strip foreground chrome (immediately left of [X]):
                        if( show_zoom_strip ) {
                            lb_dl->AddRectFilled( strip_min, strip_max, IM_COL32( 22, 27, 36, 222 ), 5.0f );
                            AddRectCompat( lb_dl, strip_min, strip_max, IM_COL32( 140, 155, 180, 105 ), 5.0f, 0, 1.0f );

                            const bool at_min_z = ( lb_view.zoom <= 0.501f );
                            const bool at_max_z = ( lb_view.zoom >= 3.999f );
                            const bool is_z_mod = ( std::fabs( lb_view.zoom - 1.0f ) > 0.005f );
                            const bool can_z_restore = !is_z_mod && ( std::fabs( lb_view.lastZoom - 1.0f ) > 0.005f );

                            auto drawStripPill = [&]( float x, float w, bool hov, bool act, bool enabled ) {
                                const ImVec2 bmin( x, btn_in_y );
                                const ImVec2 bmax( x + w, btn_in_y + btn_in_h );
                                ImU32 bg = IM_COL32( 36, 42, 54, 215 );
                                if( enabled && act ) bg = IM_COL32( 74, 112, 168, 240 );
                                else if( enabled && hov ) bg = IM_COL32( 54, 64, 82, 232 );
                                lb_dl->AddRectFilled( bmin, bmax, bg, 4.0f );
                            };

                            const ImU32 fg_on  = IM_COL32( 242, 245, 250, 255 );
                            const ImU32 fg_dim = IM_COL32( 148, 158, 174, 125 );
                            const float cy = btn_in_y + btn_in_h * 0.5f;

                            // [-]
                            drawStripPill( bx_dec, z_dec_w, z_dec_hov, z_dec_act, !at_min_z );
                            const float cx_dec = bx_dec + z_dec_w * 0.5f;
                            StrokeLineCompat( lb_dl, ImVec2( cx_dec - 4.5f, cy ), ImVec2( cx_dec + 4.5f, cy ), at_min_z ? fg_dim : fg_on, 1.7f );

                            // [100%]
                            drawStripPill( bx_pct, z_pct_w, z_pct_hov, z_pct_act, is_z_mod || can_z_restore );
                            char pct_buf[24];
                            std::snprintf( pct_buf, sizeof( pct_buf ), "%d%%", static_cast<int>( std::round( lb_view.zoom * 100.0f ) ) );
                            const ImVec2 pct_sz = ImGui::CalcTextSize( pct_buf );
                            lb_dl->AddText(
                                ImVec2( std::round( bx_pct + ( z_pct_w - pct_sz.x ) * 0.5f ), std::round( cy - pct_sz.y * 0.5f ) ),
                                fg_on, pct_buf );

                            // [+]
                            drawStripPill( bx_inc, z_inc_w, z_inc_hov, z_inc_act, !at_max_z );
                            const float cx_inc = bx_inc + z_inc_w * 0.5f;
                            StrokeLineCompat( lb_dl, ImVec2( cx_inc - 4.5f, cy ), ImVec2( cx_inc + 4.5f, cy ), at_max_z ? fg_dim : fg_on, 1.7f );
                            StrokeLineCompat( lb_dl, ImVec2( cx_inc, cy - 4.5f ), ImVec2( cx_inc, cy + 4.5f ), at_max_z ? fg_dim : fg_on, 1.7f );

                            // [Reset / Restore] (counter-clockwise undo when modified, clockwise restore when at 100% with lastZoom)
                            const bool rst_enabled = is_z_mod || can_z_restore;
                            drawStripPill( bx_rst, z_rst_w, z_rst_hov, z_rst_act, rst_enabled );
                            const ImVec2 c_rst( bx_rst + z_rst_w * 0.5f, cy );
                            const ImU32 rst_col = rst_enabled ? fg_on : fg_dim;
                            const float icon_sz = 13.0f;
                            bool drew_vec_icon = false;
#if defined(IMGUI_ICONS_H) && !defined(IMGEE_DISABLE_WIDGETS) && !defined(CONTRIB_DISABLE_WIDGETS) && !defined(IMGEE_DISABLE_ICONS) && !defined(CONTRIB_DISABLE_ICONS)
                            drew_vec_icon = ImGui::DrawVectorIcon(
                                lb_dl,
                                ImVec2( c_rst.x - icon_sz * 0.5f, c_rst.y - icon_sz * 0.5f ),
                                icon_sz,
                                rst_col,
                                can_z_restore ? "redo" : "undo" );
#else
                            if( GetDrawVectorIconFn() ) {
                                drew_vec_icon = GetDrawVectorIconFn()(
                                    lb_dl,
                                    ImVec2( c_rst.x - icon_sz * 0.5f, c_rst.y - icon_sz * 0.5f ),
                                    icon_sz,
                                    rst_col,
                                    can_z_restore ? "redo" : "undo" );
                            }
#endif
                            if( !drew_vec_icon ) {
                                const ImVec2 arc_c(
                                    c_rst.x + ( can_z_restore ? -icon_sz * 0.02f : icon_sz * 0.02f ),
                                    c_rst.y + icon_sz * 0.02f );
                                const float ar = icon_sz * 0.25f;
                                const float th = ImMax( 1.2f, icon_sz * 0.095f );
                                const float a_min = can_z_restore ? -5.49f : -2.30f;
                                const float a_max = can_z_restore ? -0.84f :  2.35f;
                                lb_dl->PathClear();
                                lb_dl->PathArcTo( arc_c, ar, a_min, a_max, 16 );
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
                                lb_dl->PathStroke( rst_col, th, 0 );
#else
                                lb_dl->PathStroke( rst_col, 0, th );
#endif
                                const float a_head = can_z_restore ? a_max : a_min;
                                const float cos_a = std::cos( a_head );
                                const float sin_a = std::sin( a_head );
                                const float tx = can_z_restore ? -sin_a :  sin_a;
                                const float ty = can_z_restore ?  cos_a : -cos_a;
                                const float nx = cos_a;
                                const float ny = sin_a;
                                const ImVec2 base( arc_c.x + ar * cos_a, arc_c.y + ar * sin_a );
                                const float arrow_len = ar * 0.95f;
                                const float arrow_w   = ar * 0.68f;
                                const ImVec2 tip( base.x + tx * ( arrow_len * 0.65f ), base.y + ty * ( arrow_len * 0.65f ) );
                                const ImVec2 back( base.x - tx * ( arrow_len * 0.35f ), base.y - ty * ( arrow_len * 0.35f ) );
                                const ImVec2 p1( back.x + nx * arrow_w, back.y + ny * arrow_w );
                                const ImVec2 p2( back.x - nx * arrow_w, back.y - ny * arrow_w );
                                lb_dl->AddTriangleFilled( tip, p1, p2, rst_col );
                            }
                        }

                        // Draw [X] close button foreground glyph:
                        const ImU32 close_bg = close_active
                                                   ? IM_COL32( 220, 62, 62, 235 )
                                                   : ( close_hovered ? IM_COL32( 195, 58, 58, 220 ) : IM_COL32( 36, 42, 54, 210 ) );
                        const ImU32 close_fg = IM_COL32( 242, 245, 250, 255 );
                        lb_dl->AddRectFilled( close_min, close_max, close_bg, 5.0f );
                        AddRectCompat( lb_dl, close_min, close_max, IM_COL32( 140, 155, 180, 110 ), 5.0f, 0, 1.0f );
                        const float x_ins = 8.5f;
                        StrokeLineCompat( lb_dl, ImVec2( close_min.x + x_ins, close_min.y + x_ins ), ImVec2( close_max.x - x_ins, close_max.y - x_ins ), close_fg, 1.8f );
                        StrokeLineCompat( lb_dl, ImVec2( close_max.x - x_ins, close_min.y + x_ins ), ImVec2( close_min.x + x_ins, close_max.y - x_ins ), close_fg, 1.8f );

                        // 3. Release-triggered backdrop button covering the popup outside [X], the zoom strip, caption, and image.
                        //    Keeps the popup alive and owning ActiveId across multi-frame press-and-release gestures,
                        //    preventing orphan mouse-releases from bleeding into underlying thumbnails, links, or scrollbars.
                        {
                            const ImRect lb_backdrop_r( host_min, host_max );
                            const ImGuiID lb_backdrop_id = ImGui::GetID( "##imgee_lb_backdrop" );
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 18300
                            ImGui::ItemAdd( lb_backdrop_r, lb_backdrop_id, NULL );
#else
                            ImGui::ItemAdd( lb_backdrop_r, lb_backdrop_id, NULL, ImGuiItemFlags_NoNav );
#endif
                            bool backdrop_hovered = false, backdrop_held = false;
                            const bool backdrop_pressed = ImGui::ButtonBehavior( lb_backdrop_r, lb_backdrop_id, &backdrop_hovered, &backdrop_held );
                            const ImVec2 mp = ImGui::GetIO().MousePos;
                            const bool inside_img = ( mp.x >= lb_img_min.x && mp.x <= lb_img_max.x && mp.y >= lb_img_min.y && mp.y <= lb_img_max.y );
                            const bool inside_plate = has_lb_caption && ( mp.x >= plate_min.x && mp.x <= plate_max.x && mp.y >= plate_min.y && mp.y <= plate_max.y );
                            const bool inside_strip = show_zoom_strip && ( mp.x >= strip_min.x && mp.x <= strip_max.x && mp.y >= strip_min.y && mp.y <= strip_max.y );
                            if( !is_appearing && backdrop_pressed && !inside_img && !inside_plate && !inside_strip && !close_hovered && !any_strip_hovered ) {
                                should_close = true;
                            }
                        }

                        if( IsEscapePressedCompat() ) {
                            should_close = true;
                        }

                        if( should_close ) {
                            ImGui::CloseCurrentPopup();
                        } else {
                            telem.openFrame = ImGui::GetFrameCount();
                            telem.activePopupId = popup_id;
                            telem.hasLightboxCaption = has_lb_caption;
                            telem.lightboxZoom = lb_view.zoom;
                            telem.lightboxPan = lb_view.pan;
                            telem.zoomStripMin = show_zoom_strip ? strip_min : ImVec2( 0.0f, 0.0f );
                            telem.zoomStripMax = show_zoom_strip ? strip_max : ImVec2( 0.0f, 0.0f );
                            telem.closeButtonMin = close_min;
                            telem.closeButtonMax = close_max;
                            telem.lightboxImageMin = lb_img_min;
                            telem.lightboxImageMax = lb_img_max;
                            telem.lightboxCaptionPlateMin = plate_min;
                            telem.lightboxCaptionPlateMax = plate_max;
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::PopStyleVar( 3 );
                    ImGui::PopStyleColor( 2 );
                }
            } else if( img_hovered ) {
                if( img_released && has_link_cb ) {
                    linkCb( { alt_ptr ? alt_ptr : "", alt_len, raw_url_ptr ? raw_url_ptr : "", raw_url_len, mdConfig_.userData, true } );
                }
                if( alt_len > 0 && mdConfig_.tooltipCallback ) {
                    mdConfig_.tooltipCallback( { { alt_ptr, alt_len, raw_url_ptr ? raw_url_ptr : "", raw_url_len, mdConfig_.userData, true }, mdConfig_.linkIcon } );
                }
            }

            return res;
        }
        struct ScopedFloatLayout
        {
            FloatLayoutScope scope;
            bool             active = false;

            ScopedFloatLayout( bool enable, const MarkdownConfig& cfg )
            {
                if( !enable ) return;
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                if( !win ) return;
                active = true;
                if( GetFloatScopeStack().empty() ) {
                    GetRenderedLineSpans().clear();
                }
                scope.window = win;
                scope.containerLeftX = GetWindowNaturalLineStartX( win );
                const float full_w = ResolveFullLineAvailWidth( &cfg );
                scope.containerRightX = scope.containerLeftX + ImMax( 0.0f, full_w );
                GetFloatScopeStack().push_back( &scope );
            }

            ~ScopedFloatLayout()
            {
                if( !active ) return;
                // End-of-scope clearance: ensure cursor Y and parent group/card/CalcMarkdownSize bounds
                // advance past the deepest floated figure registered in this scope!
                if( scope.window == ImGui::GetCurrentWindow() && scope.deepestMaxY > -FLT_MAX * 0.5f ) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                    if( scope.window->DC.IsSameLine ) {
                        ImGui::NewLine();
                    }
#endif
                    const float cur_y = ImGui::GetCursorScreenPos().y;
                    if( scope.deepestMaxY > cur_y + 0.25f ) {
                        const float natural_start_x = GetWindowNaturalLineStartX( scope.window );
                        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );
                        ImGui::SetCursorScreenPos( ImVec2( natural_start_x, scope.deepestMaxY ) );
                        ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
                        ImGui::PopStyleVar();
                    }
                }
                std::vector<FloatLayoutScope*>& st = GetFloatScopeStack();
                if( !st.empty() && st.back() == &scope ) {
                    st.pop_back();
                } else {
                    for( size_t idx = st.size(); idx > 0; --idx ) {
                        if( st[idx - 1] == &scope ) {
                            st.erase( st.begin() + static_cast<std::ptrdiff_t>( idx - 1 ) );
                            break;
                        }
                    }
                }
            }
        };

        inline void RenderInternal( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_, bool create_float_scope );
    } // namespace MarkdownDetail
    
    // render markdown (low-level rendering pipeline)
    namespace Markdown
    {
        inline void Render( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ )
        {
            MarkdownDetail::RenderInternal( markdown_, markdownLength_, mdConfig_, true );
        }
    } // namespace Markdown

    inline void MarkdownDetail::RenderInternal( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_, bool create_float_scope )
    {
        struct ScopedActiveMarkdownConfig {
            const MarkdownConfig* prev;
            explicit ScopedActiveMarkdownConfig( const MarkdownConfig* cur ) : prev( GetActiveMarkdownConfig() ) {
                GetActiveMarkdownConfig() = cur;
            }
            ~ScopedActiveMarkdownConfig() {
                GetActiveMarkdownConfig() = prev;
            }
        } activeConfigScope( &mdConfig_ );

        ScopedFloatLayout floatLayoutScope( create_float_scope, mdConfig_ );

        static const char* linkHoverStart = NULL; // we need to preserve status of link hovering between frames
        Line        line;
        Link        link;
        Emphasis    em;
        TextRegion  textRegion;
        textRegion.wrapWidth = mdConfig_.wrapWidth;
        size_t      code_block_idx = 0;
        size_t      table_idx = 0;
        bool        lastWasSeparator = false;
        bool        lastWasBlankLine = false;

        char c = 0;
        for( int i=0; i < (int)markdownLength_; ++i )
        {
            c = markdown_[i];               // get the character at index
            if( c == 0 ) { break; }         // shouldn't happen but don't go beyond 0.

            // If we're at the beginning of the line, count any spaces
            if( line.isLeadingSpace )
            {
                if( c == ' ' )
                {
                    ++line.leadSpaceCount;
                    continue;
                }
                else if( c == '\t' )
                {
                    line.leadSpaceCount += ( mdConfig_.tabSize > 0 ? mdConfig_.tabSize : 4 );
                    continue;
                }
                else
                {
                    line.isLeadingSpace = false;
                    line.lastRenderPosition = i - 1;

                    // GitHub Flavored Markdown (GFM) Section 4.10: Tables
                    if( line.leadSpaceCount <= 3 )
                    {
                        std::vector<TableCell> headers;
                        std::vector<TableColumnAlign> alignments;
                        std::vector<std::vector<TableCell>> rows;
                        TableDetail::MarkdownTableMetadata metadata;
                        int table_end = TableDetail::TryParseTable(
                            markdown_, (int)markdownLength_, i,
                            headers, alignments, rows, metadata );
                        if( table_end > i )
                        {
                            ClearActiveFloats();
                            RenderTableBlock( headers, alignments, rows, metadata, mdConfig_, table_idx++ );

                            textRegion.ResetIndent();
                            lastWasSeparator = false;
                            lastWasBlankLine = false;

                            int next_pos = TableDetail::SkipNewline( markdown_, table_end, (int)markdownLength_ );

                            line = Line();
                            line.isLeadingSpace = true;
                            line.lineStart = next_pos;
                            line.lastRenderPosition = next_pos - 1;
                            em = Emphasis();
                            link = Link();
                            i = next_pos - 1;
                            continue;
                        }
                    }

                    // CommonMark Section 4.1: Thematic breaks
                    // A line with 0-3 leading spaces, consisting solely of 3 or more matching '-', '*', or '_'
                    // characters (with optional spaces/tabs between them), followed only by whitespace until '\n' or EOF.
                    if( line.leadSpaceCount <= 3 && ( c == '-' || c == '*' || c == '_' ) )
                    {
                        char tb_char = c;
                        int tb_count = 0;
                        bool is_thematic_break = true;
                        int scan = i;
                        while( scan < (int)markdownLength_ && markdown_[scan] != '\n' && markdown_[scan] != '\r' )
                        {
                            char sc = markdown_[scan];
                            if( sc == tb_char )
                            {
                                ++tb_count;
                            }
                            else if( sc == ' ' || sc == '\t' )
                            {
                                // spaces and tabs are allowed between and after characters
                            }
                            else
                            {
                                is_thematic_break = false;
                                break;
                            }
                            ++scan;
                        }
                        if( is_thematic_break && tb_count >= 3 )
                        {
                            ClearActiveFloats();
                            ImGui::Separator();
                            textRegion.ResetIndent();
                            lastWasSeparator = true;
                            lastWasBlankLine = false;

                            // Advance past newline (\r\n, \r, or \n)
                            if( scan < (int)markdownLength_ && markdown_[scan] == '\r' && scan + 1 < (int)markdownLength_ && markdown_[scan + 1] == '\n' )
                            {
                                scan += 2;
                            }
                            else if( scan < (int)markdownLength_ && ( markdown_[scan] == '\n' || markdown_[scan] == '\r' ) )
                            {
                                scan += 1;
                            }

                            line = Line();
                            line.isLeadingSpace = true;
                            line.lineStart = scan;
                            line.lastRenderPosition = scan - 1;
                            em = Emphasis();
                            link = Link();
                            i = scan - 1;
                            continue;
                        }
                    }

                    if(( c == '*' || c == '-' || c == '+' ) && 
                       ( (int)markdownLength_ > i + 1 ) && ( markdown_[ i + 1 ] == ' ' ))
                    {
                        line.isUnorderedListStart = true;
                        ++i;
                        ++line.lastRenderPosition;
                        line.lineStart = i + 1;
                        continue;
                    }
                    else if( c >= '0' && c <= '9' )
                    {
                        int d = i;
                        while( d < (int)markdownLength_ && markdown_[d] >= '0' && markdown_[d] <= '9' && (d - i) < 9 )
                        {
                            ++d;
                        }
                        if( d > i && d + 1 < (int)markdownLength_ && ( markdown_[d] == '.' || markdown_[d] == ')' ) && markdown_[d + 1] == ' ' )
                        {
                            line.isOrderedListStart = true;
                            line.orderedListNumStart = i;
                            line.orderedListNumEnd = d + 1; // includes '.' or ')'
                            i = d + 1; // advances past the delimiter
                            line.lastRenderPosition = i;
                            line.lineStart = i + 1;
                            continue;
                        }
                    }
                    else if( c == '#' )
                    {
                        int h_count = 1;
                        bool bContinueChecking = true;
                        int j = i;
                        while( ++j < (int)markdownLength_ && bContinueChecking )
                        {
                            c = markdown_[j];
                            if( c == '#' )
                            {
                                ++h_count;
                                if( h_count > 6 ) bContinueChecking = false;
                            }
                            else if( c == ' ' || c == '\t' )
                            {
                                bContinueChecking = false;
                            }
                            else
                            {
                                h_count = 0;
                                bContinueChecking = false;
                            }
                        }
                        if( h_count > 0 && h_count <= 6 && ( c == ' ' || c == '\t' ) )
                        {
                            // CommonMark 4.2: ATX Heading block processing (inline parsed, trailing '#' stripped)
                            int line_end = j;
                            while( line_end < (int)markdownLength_ && markdown_[line_end] != '\n' && markdown_[line_end] != '\r' )
                            {
                                ++line_end;
                            }

                            int text_start = j;
                            while( text_start < line_end && ( markdown_[text_start] == ' ' || markdown_[text_start] == '\t' ) )
                            {
                                ++text_start;
                            }

                            const char* h_start = markdown_ + text_start;
                            const char* h_end = markdown_ + line_end;
                            TrimTrailingHeadingHashes( h_start, h_end );
                            size_t heading_text_len = (size_t)( h_end - h_start );

                            MarkdownFormatInfo formatInfo;
                            formatInfo.config = &mdConfig_;
                            formatInfo.level = h_count;
                            formatInfo.type = MarkdownFormatType::HEADING;
                            formatInfo.isPrecededBySeparator = lastWasSeparator;
                            formatInfo.isPrecededByBlankLine = lastWasBlankLine;

                            bool isAtStart = true;
                            for( int k = 0; k < i; ++k )
                            {
                                char ch = markdown_[k];
                                if( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
                                {
                                    isAtStart = false;
                                    break;
                                }
                            }
                            formatInfo.isStartOfDoc = isAtStart;

                            mdConfig_.formatCallback( formatInfo, true );

                            if( heading_text_len > 0 )
                            {
                                MarkdownConfig headingConfig = mdConfig_;
                                headingConfig.regularFont = nullptr; // preserve active heading font for normal text

                                // Resolve active heading font
                                ImFont* h_font = nullptr;
                                if( h_count == 1 && mdConfig_.h1Font ) h_font = mdConfig_.h1Font;
                                else if( h_count == 2 && mdConfig_.h2Font ) h_font = mdConfig_.h2Font;
                                else if( h_count == 3 && mdConfig_.h3Font ) h_font = mdConfig_.h3Font;
                                else if( h_count <= MarkdownConfig::NUMHEADINGS ) h_font = mdConfig_.headingFormats[ h_count - 1 ].font;
                                else h_font = mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;

                                // Resolve heading-level italic font if provided
                                ImFont* h_italic = nullptr;
                                if( h_count == 1 && mdConfig_.h1ItalicFont ) h_italic = mdConfig_.h1ItalicFont;
                                else if( h_count == 2 && mdConfig_.h2ItalicFont ) h_italic = mdConfig_.h2ItalicFont;
                                else if( h_count >= 3 && mdConfig_.h3ItalicFont ) h_italic = mdConfig_.h3ItalicFont;
                                if( !h_italic ) h_italic = mdConfig_.headingItalicFont;

                                float h_size = 0.0f;
                                if( h_font )
                                {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
                                    h_size = ( h_font->LegacySize > 0.0f ) ? h_font->LegacySize : ImGui::GetFontSize();
#else
                                    h_size = ( h_font->FontSize > 0.0f ) ? h_font->FontSize : ImGui::GetFontSize();
#endif
                                }

                                float body_italic_size = 0.0f;
                                if( mdConfig_.italicFont )
                                {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
                                    body_italic_size = ( mdConfig_.italicFont->LegacySize > 0.0f ) ? mdConfig_.italicFont->LegacySize : 0.0f;
#else
                                    body_italic_size = ( mdConfig_.italicFont->FontSize > 0.0f ) ? mdConfig_.italicFont->FontSize : 0.0f;
#endif
                                }

                                if( h_italic )
                                {
                                    headingConfig.italicFont = h_italic;
                                }
                                else if( body_italic_size > 0.0f && h_size > 0.0f && body_italic_size < ( h_size - 0.5f ) )
                                {
                                    // Body italic font is smaller than heading; inherit heading font to avoid jarring drop
                                    headingConfig.italicFont = h_font;
                                }

                                // Bold inside heading stays at full heading size:
                                if( h_font )
                                {
                                    headingConfig.boldFont = h_font;
                                }

                                RenderInternal( h_start, heading_text_len, headingConfig, false );
                            }

                            mdConfig_.formatCallback( formatInfo, false );

                            lastWasSeparator = ( h_count <= MarkdownConfig::NUMHEADINGS ) ?
                                               mdConfig_.headingFormats[ h_count - 1 ].separator :
                                               mdConfig_.headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].separator;
                            lastWasBlankLine = false;

                            int peek = line_end;
                            if( peek < (int)markdownLength_ && markdown_[peek] == '\r' ) ++peek;
                            if( peek < (int)markdownLength_ && markdown_[peek] == '\n' ) ++peek;

                            textRegion.ResetIndent();
                            line = Line();
                            line.isLeadingSpace = true;
                            line.lineStart = peek;
                            line.lastRenderPosition = peek - 1;
                            em = Emphasis();
                            link = Link();
                            i = peek - 1;
                            continue;
                        }
                    }
                    else if( ( c == '`' || c == '~' ) && i + 2 < (int)markdownLength_ &&
                             markdown_[ i + 1 ] == c && markdown_[ i + 2 ] == c )
                    {
                        char fence_char = c;
                        int fence_len = 0;
                        while( i + fence_len < (int)markdownLength_ && markdown_[ i + fence_len ] == fence_char )
                        {
                            ++fence_len;
                        }

                        // Info string (language identifier, e.g. "cpp", "lua")
                        int info_start = i + fence_len;
                        while( info_start < (int)markdownLength_ && ( markdown_[ info_start ] == ' ' || markdown_[ info_start ] == '\t' ) )
                        {
                            ++info_start;
                        }
                        int info_end = info_start;
                        while( info_end < (int)markdownLength_ && markdown_[ info_end ] != '\n' && markdown_[ info_end ] != '\r' )
                        {
                            ++info_end;
                        }
                        int trimmed_info_end = info_end;
                        while( trimmed_info_end > info_start && ( markdown_[ trimmed_info_end - 1 ] == ' ' || markdown_[ trimmed_info_end - 1 ] == '\t' ) )
                        {
                            --trimmed_info_end;
                        }

                        // Start of code block content
                        int code_start = info_end;
                        if( code_start < (int)markdownLength_ && markdown_[ code_start ] == '\r' ) ++code_start;
                        if( code_start < (int)markdownLength_ && markdown_[ code_start ] == '\n' ) ++code_start;

                        int code_end = (int)markdownLength_;
                        int scan = code_start;
                        while( scan < (int)markdownLength_ )
                        {
                            int line_start = scan;
                            while( line_start < (int)markdownLength_ && ( markdown_[ line_start ] == ' ' || markdown_[ line_start ] == '\t' ) )
                            {
                                ++line_start;
                            }
                            if( line_start + fence_len <= (int)markdownLength_ )
                            {
                                bool is_match = true;
                                for( int f = 0; f < fence_len; ++f )
                                {
                                    if( markdown_[ line_start + f ] != fence_char ) { is_match = false; break; }
                                }
                                if( is_match )
                                {
                                    int f_scan = line_start + fence_len;
                                    while( f_scan < (int)markdownLength_ && markdown_[ f_scan ] == fence_char ) ++f_scan;
                                    while( f_scan < (int)markdownLength_ && ( markdown_[ f_scan ] == ' ' || markdown_[ f_scan ] == '\t' ) ) ++f_scan;
                                    if( f_scan >= (int)markdownLength_ || markdown_[ f_scan ] == '\n' || markdown_[ f_scan ] == '\r' )
                                    {
                                        code_end = scan;
                                        if( code_end > code_start && markdown_[ code_end - 1 ] == '\n' ) --code_end;
                                        if( code_end > code_start && markdown_[ code_end - 1 ] == '\r' ) --code_end;

                                        while( f_scan < (int)markdownLength_ && ( markdown_[ f_scan ] == '\r' || markdown_[ f_scan ] == '\n' ) )
                                        {
                                            ++f_scan;
                                        }
                                        i = f_scan - 1;
                                        break;
                                    }
                                }
                            }
                            while( scan < (int)markdownLength_ && markdown_[ scan ] != '\n' ) ++scan;
                            if( scan < (int)markdownLength_ && markdown_[ scan ] == '\n' ) ++scan;
                        }
                        if( scan >= (int)markdownLength_ )
                        {
                            code_end = (int)markdownLength_;
                            i = (int)markdownLength_ - 1;
                        }

                        ClearActiveFloats();
                        RenderCodeBlock( markdown_ + code_start, (size_t)(code_end - code_start),
                                         markdown_ + info_start, (size_t)(trimmed_info_end - info_start),
                                         mdConfig_, code_block_idx++, line.leadSpaceCount );

                        textRegion.ResetIndent();
                        lastWasSeparator = false;
                        lastWasBlankLine = false;

                        line = Line();
                        line.isLeadingSpace = true;
                        line.lineStart = i + 1;
                        line.lastRenderPosition = i;
                        em = Emphasis();
                        continue;
                    }
                }
            }

            // Test to see if we have an escaped character via backslash (\* or \# or \<)
            // CommonMark Section 6.1: Backslash escapes do not apply inside inline code spans, links, or emphasis
            if( c == '\\' && em.state == Emphasis::NONE && link.state == Link::NO_LINK &&
                i + 1 < (int)markdownLength_ && IsMarkdownEscapable( markdown_[ i + 1 ] ) )
            {
                // Render any preceding plain text before the backslash
                line.lineEnd = i;
                if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                {
                    RenderLine( markdown_, line, textRegion, mdConfig_ );
                    ImGui::SameLine( 0.0f, 0.0f );
                }
                // Skip the backslash character so it is never displayed
                ++i;
                line.lineStart = i;
                line.lastRenderPosition = i - 1;
                // markdown_[i] is now an escaped literal character; skip normal markup checks for this character
                continue;
            }

            // HTML comments (e.g. <!-- clear -->, <!-- clear: left -->, or general inline/block comments):
            if( c == '<' && link.state == Link::NO_LINK && !( em.state != Emphasis::NONE && em.sym == '`' ) &&
                i + 3 < (int)markdownLength_ && markdown_[i + 1] == '!' && markdown_[i + 2] == '-' && markdown_[i + 3] == '-' )
            {
                int commentEnd = -1;
                for( int k = i + 4; k + 2 < (int)markdownLength_; ++k )
                {
                    if( markdown_[k] == '-' && markdown_[k + 1] == '-' && markdown_[k + 2] == '>' )
                    {
                        commentEnd = k + 3;
                        break;
                    }
                }
                if( commentEnd != -1 )
                {
                    line.lineEnd = i;
                    const bool hadPrecedingText = ( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 );
                    if( hadPrecedingText )
                    {
                        RenderLine( markdown_, line, textRegion, mdConfig_ );
                        line.isUnorderedListStart = false;
                        line.isOrderedListStart = false;
                    }

                    const char* c_body = markdown_ + i + 4;
                    const size_t c_len = (size_t)( ( commentEnd - 3 ) - ( i + 4 ) );
                    size_t cs = 0;
                    while( cs < c_len && ( c_body[cs] == ' ' || c_body[cs] == '\t' || c_body[cs] == '\r' || c_body[cs] == '\n' ) ) ++cs;
                    size_t ce = c_len;
                    while( ce > cs && ( c_body[ce - 1] == ' ' || c_body[ce - 1] == '\t' || c_body[ce - 1] == '\r' || c_body[ce - 1] == '\n' ) ) --ce;
                    const size_t trimmed_len = ce - cs;
                    bool wasClearComment = false;
                    if( trimmed_len >= 5 && TableDetail::StrCaseEqualN( c_body + cs, "clear", 5 ) )
                    {
                        const char after_c = ( trimmed_len > 5 ) ? c_body[cs + 5] : '\0';
                        if( after_c == '\0' || after_c == ':' || after_c == '=' || after_c == ' ' || after_c == '\t' || after_c == '-' )
                        {
                            const MarkdownImageFloat mode = ParseClearFloatMode( c_body + cs + 5, trimmed_len - 5, false );
                            ClearActiveFloats( mode );
                            wasClearComment = true;
                        }
                    }

                    int peek = commentEnd;
                    while( peek < (int)markdownLength_ && ( markdown_[peek] == ' ' || markdown_[peek] == '\t' ) ) ++peek;
                    if( peek >= (int)markdownLength_ || markdown_[peek] == '\n' || markdown_[peek] == '\r' )
                    {
                        if( peek < (int)markdownLength_ && markdown_[peek] == '\r' ) ++peek;
                        if( peek < (int)markdownLength_ && markdown_[peek] == '\n' ) ++peek;
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18900
                        if( hadPrecedingText && ImGui::GetCurrentWindow()->DC.IsSameLine )
                        {
                            ImGui::NewLine();
                        }
#endif
                        textRegion.ResetIndent();
                        line = Line();
                        em = Emphasis();
                        link = Link();
                        line.lineStart = peek;
                        line.lastRenderPosition = peek - 1;
                        i = peek - 1;
                        continue;
                    }
                    else
                    {
                        if( hadPrecedingText && !wasClearComment )
                        {
                            ImGui::SameLine( 0.0f, 0.0f );
                        }
                        i = commentEnd - 1;
                        line.lastRenderPosition = commentEnd - 1;
                        line.lineStart = commentEnd;
                        continue;
                    }
                }
            }

            // Transition from Emphasis::LEFT to Emphasis::MIDDLE if we hit a tag immediately after opening delimiter (e.g. **<icon...>)
            if( em.state == Emphasis::LEFT && c == '<' )
            {
                em.text.start = i;
                em.state = Emphasis::MIDDLE;
            }

            // Test to see if we have a custom styling tag <tag=param>...</tag> or self-closing/void tag <img ... />, <clear/>, <br .../>
            if( c == '<' && link.state == Link::NO_LINK && !( em.state != Emphasis::NONE && em.sym == '`' ) )
            {
                int openClose = -1;
                for( int k = i + 1; k < (int)markdownLength_; ++k )
                {
                    if( markdown_[k] == '>' ) { openClose = k; break; }
                    if( markdown_[k] == '\n' || markdown_[k] == '<' ) break;
                }
                if( openClose > i + 1 )
                {
                    int tagHeaderStart = i + 1;
                    int tagHeaderEnd = openClose;
                    int sep = -1;
                    for( int k = tagHeaderStart; k < tagHeaderEnd; ++k )
                    {
                        if( markdown_[k] == '=' || markdown_[k] == ':' || markdown_[k] == ' ' || markdown_[k] == '\t' || markdown_[k] == '/' )
                        {
                            sep = k;
                            break;
                        }
                    }
                    int nameEnd = ( sep != -1 ) ? sep : tagHeaderEnd;
                    int nameLen = nameEnd - tagHeaderStart;

                    // Check for self-closing tag (<tag ... />) or HTML void tag (<img ...>, <clear>, <br>)
                    bool isSelfClosing = false;
                    int actualHeaderEnd = openClose;
                    int p_slash = openClose - 1;
                    while( p_slash >= tagHeaderStart && ( markdown_[p_slash] == ' ' || markdown_[p_slash] == '\t' ) )
                    {
                        --p_slash;
                    }
                    if( p_slash >= tagHeaderStart && markdown_[p_slash] == '/' )
                    {
                        isSelfClosing = true;
                        actualHeaderEnd = p_slash;
                        if( sep == p_slash ) sep = -1;
                    }

                    const bool isImgTag = ( nameLen == 3 && TableDetail::StrCaseEqualN( markdown_ + tagHeaderStart, "img", 3 ) );
                    const bool isClearTag = ( nameLen == 5 && TableDetail::StrCaseEqualN( markdown_ + tagHeaderStart, "clear", 5 ) );
                    const bool isBrTag = ( nameLen == 2 && TableDetail::StrCaseEqualN( markdown_ + tagHeaderStart, "br", 2 ) );
                    const bool isVoidTag = isImgTag || isClearTag || isBrTag;

                    if( mdConfig_.tagCallback != NULL || isVoidTag || mdConfig_.imageCallback != NULL || GetDefaultMarkdownImageCallback() != NULL )
                    {
                    int closeStart = -1;
                    int closeEnd = -1;

                    if( isSelfClosing || isVoidTag )
                    {
                        closeStart = openClose + 1;
                        closeEnd = openClose + 1;
                        tagHeaderEnd = actualHeaderEnd;
                        // Consume optional immediate </clear> or </br> if written as paired tag:
                        const int optCloseLen = nameLen + 3;
                        if( ( isClearTag || isBrTag ) && !isSelfClosing && closeEnd + optCloseLen <= (int)markdownLength_ &&
                            markdown_[closeEnd] == '<' && markdown_[closeEnd + 1] == '/' && markdown_[closeEnd + 2 + nameLen] == '>' &&
                            TableDetail::StrCaseEqualN( markdown_ + closeEnd + 2, markdown_ + tagHeaderStart, (size_t)nameLen ) )
                        {
                            closeEnd += optCloseLen;
                        }
                    }
                    else
                    {
                        // Case-insensitive, fence-aware, inline-backtick-aware, and nesting-depth-aware
                        // closing tag search (supports nested <blockquote> / <callout> and code examples):
                        int closeTagLen = nameLen + 3; // "</" + name + ">"
                        if( nameLen > 0 && nameLen < 60 && openClose + 1 + closeTagLen <= (int)markdownLength_ )
                        {
                            int depth = 1;
                            bool scanInFence = false;
                            char scanFenceChar = '\0';
                            int scanFenceLen = 0;
                            int scanBackticks = 0;

                            int k = openClose + 1;
                            int maxPos = (int)markdownLength_;
                            while( k < maxPos )
                            {
                                // Check for fenced code block boundaries at start of line
                                if( scanBackticks == 0 && ( markdown_[k - 1] == '\n' || markdown_[k - 1] == '\r' ) )
                                {
                                    int ls = k;
                                    while( ls < maxPos && ( markdown_[ls] == ' ' || markdown_[ls] == '\t' || markdown_[ls] == '>' ) ) ++ls;
                                    if( ls < maxPos && ( markdown_[ls] == '`' || markdown_[ls] == '~' ) )
                                    {
                                        char fc = markdown_[ls];
                                        int fl = 0;
                                        while( ls + fl < maxPos && markdown_[ls + fl] == fc ) ++fl;
                                        if( fl >= 3 )
                                        {
                                            if( !scanInFence )
                                            {
                                                scanInFence = true;
                                                scanFenceChar = fc;
                                                scanFenceLen = fl;
                                            }
                                            else if( fc == scanFenceChar && fl >= scanFenceLen )
                                            {
                                                scanInFence = false;
                                                scanFenceChar = '\0';
                                                scanFenceLen = 0;
                                            }
                                            int eol = ls + fl;
                                            while( eol < maxPos && markdown_[eol] != '\n' && markdown_[eol] != '\r' ) ++eol;
                                            k = eol;
                                            continue;
                                        }
                                    }
                                }

                                if( scanInFence )
                                {
                                    ++k;
                                    continue;
                                }

                                // Track inline code backticks on the current line
                                if( markdown_[k] == '\n' || markdown_[k] == '\r' )
                                {
                                    scanBackticks = 0;
                                    ++k;
                                    continue;
                                }
                                if( markdown_[k] == '`' )
                                {
                                    int bc = 0;
                                    while( k + bc < maxPos && markdown_[k + bc] == '`' ) ++bc;
                                    if( scanBackticks == 0 ) scanBackticks = bc;
                                    else if( bc == scanBackticks ) scanBackticks = 0;
                                    k += bc;
                                    continue;
                                }
                                if( scanBackticks > 0 )
                                {
                                    ++k;
                                    continue;
                                }

                                if( markdown_[k] == '<' )
                                {
                                    // 1. Check matching closing tag </name>
                                    if( k + closeTagLen <= maxPos &&
                                        markdown_[k + 1] == '/' &&
                                        markdown_[k + 2 + nameLen] == '>' &&
                                        TableDetail::StrCaseEqualN( markdown_ + k + 2, markdown_ + tagHeaderStart, (size_t)nameLen ) )
                                    {
                                        --depth;
                                        if( depth == 0 )
                                        {
                                            closeStart = k;
                                            closeEnd = k + closeTagLen;
                                            break;
                                        }
                                        k += closeTagLen;
                                        continue;
                                    }

                                    // 2. Check nested non-self-closing opening tag <name ...>
                                    if( k + 1 + nameLen < maxPos &&
                                        TableDetail::StrCaseEqualN( markdown_ + k + 1, markdown_ + tagHeaderStart, (size_t)nameLen ) )
                                    {
                                        char afterName = markdown_[k + 1 + nameLen];
                                        if( afterName == '>' || afterName == ' ' || afterName == '\t' || afterName == '=' || afterName == ':' || afterName == '/' )
                                        {
                                            int nestedClose = -1;
                                            for( int nk = k + 1 + nameLen; nk < maxPos; ++nk )
                                            {
                                                if( markdown_[nk] == '>' ) { nestedClose = nk; break; }
                                                if( markdown_[nk] == '\n' || markdown_[nk] == '<' ) break;
                                            }
                                            if( nestedClose != -1 )
                                            {
                                                int ns = nestedClose - 1;
                                                while( ns > k && ( markdown_[ns] == ' ' || markdown_[ns] == '\t' ) ) --ns;
                                                if( markdown_[ns] != '/' )
                                                {
                                                    ++depth;
                                                }
                                                k = nestedClose + 1;
                                                continue;
                                            }
                                        }
                                    }
                                }
                                ++k;
                            }
                        }
                    }

                    if( closeStart != -1 )
                    {
                        if( em.state == Emphasis::MIDDLE )
                        {
                            // 1. Render any plain text that preceded the opening emphasis delimiter:
                            int preEmphasisEnd = em.text.start - line.emphasisCount;
                            if( preEmphasisEnd > line.lineStart && preEmphasisEnd > line.lastRenderPosition + 1 )
                            {
                                line.lineEnd = preEmphasisEnd;
                                RenderLine( markdown_, line, textRegion, mdConfig_ );
                                line.isUnorderedListStart = false;
                                line.isOrderedListStart = false;
                                ImGui::SameLine( 0.0f, 0.0f );
                                line.lastRenderPosition = preEmphasisEnd - 1;
                                line.lineStart = preEmphasisEnd;
                            }
                            else if( line.isUnorderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isUnorderedListStart = false;
                            }
                            else if( line.isOrderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isOrderedListStart = false;
                            }

                            // 2. Render any emphasized text that preceded this tag (from em.text.start to i):
                            if( i > em.text.start && i > line.lastRenderPosition + 1 )
                            {
                                line.isEmphasis = true;
                                line.lastRenderPosition = em.text.start - 1;
                                line.lineStart = em.text.start;
                                line.lineEnd = i;
                                RenderLine( markdown_, line, textRegion, mdConfig_ );
                                line.isEmphasis = false;
                                ImGui::SameLine( 0.0f, 0.0f );
                                line.lastRenderPosition = i - 1;
                                line.lineStart = i;
                            }
                        }
                        else
                        {
                            line.lineEnd = i;
                            if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                            {
                                RenderLine( markdown_, line, textRegion, mdConfig_ );
                                line.isUnorderedListStart = false;
                                line.isOrderedListStart = false;
                                ImGui::SameLine( 0.0f, 0.0f );
                            }
                            else if( line.isUnorderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isUnorderedListStart = false;
                            }
                            else if( line.isOrderedListStart )
                            {
                                int numLeadIndents = line.leadSpaceCount / 2;
                                textRegion.ApplyLeadIndent( numLeadIndents );
                                textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                                line.isOrderedListStart = false;
                            }
                        }

                        MarkdownTagData tagData;
                        tagData.tagName = markdown_ + tagHeaderStart;
                        tagData.tagNameLength = nameLen;
                        tagData.tagParam = ( sep != -1 && tagHeaderEnd > sep + 1 ) ? ( markdown_ + sep + 1 ) : nullptr;
                        tagData.tagParamLength = ( sep != -1 && tagHeaderEnd > sep + 1 ) ? ( tagHeaderEnd - sep - 1 ) : 0;
                        while( tagData.tagParamLength > 0 && ( *tagData.tagParam == ' ' || *tagData.tagParam == '\t' ) )
                        {
                            ++tagData.tagParam;
                            --tagData.tagParamLength;
                        }
                        while( tagData.tagParamLength > 0 && ( tagData.tagParam[tagData.tagParamLength - 1] == ' ' || tagData.tagParam[tagData.tagParamLength - 1] == '\t' ) )
                        {
                            --tagData.tagParamLength;
                        }
                        if( tagData.tagParamLength >= 2 &&
                            ( ( *tagData.tagParam == '\"' && tagData.tagParam[tagData.tagParamLength - 1] == '\"' ) ||
                              ( *tagData.tagParam == '\'' && tagData.tagParam[tagData.tagParamLength - 1] == '\'' ) ) &&
                            memchr( tagData.tagParam + 1, *tagData.tagParam, tagData.tagParamLength - 2 ) == NULL )
                        {
                            ++tagData.tagParam;
                            tagData.tagParamLength -= 2;
                        }
                        if( isSelfClosing || isVoidTag )
                        {
                            tagData.innerText = "";
                            tagData.innerTextLength = 0;
                        }
                        else
                        {
                            tagData.innerText = markdown_ + openClose + 1;
                            tagData.innerTextLength = closeStart - ( openClose + 1 );
                        }
                        tagData.config = &mdConfig_;
                        tagData.userData = mdConfig_.userData;

                        if( isClearTag )
                        {
                            const MarkdownImageFloat mode = ParseClearFloatMode( tagData.tagParam, tagData.tagParamLength, false );
                            ClearActiveFloats( mode );
                        }
                        else if( isBrTag )
                        {
                            const MarkdownImageFloat mode = ParseClearFloatMode( tagData.tagParam, tagData.tagParamLength, true );
                            if( static_cast<unsigned char>( mode ) != 255 )
                            {
                                if( !ClearActiveFloats( mode ) )
                                {
                                    ImGui::NewLine();
                                }
                            }
                            else
                            {
                                ImGui::NewLine();
                            }
                        }
                        else if( mdConfig_.tagCallback != NULL )
                        {
                            mdConfig_.tagCallback( tagData );
                        }
                        else if( isImgTag )
                        {
                            MarkdownImageCallback* imageCb = mdConfig_.imageCallback ? mdConfig_.imageCallback : GetDefaultMarkdownImageCallback();
                            MarkdownImageData imgData;
                            if( imageCb )
                            {
                                imgData = imageCb( { "", 0, tagData.tagParam, (int)tagData.tagParamLength, mdConfig_.userData, true } );
                            }
                            MarkdownDetail::RenderMarkdownImageElement( mdConfig_, imgData, "", 0, tagData.tagParam, (int)tagData.tagParamLength, false );
                        }

                        int peek = closeEnd;
                        while( peek < (int)markdownLength_ && ( markdown_[peek] == ' ' || markdown_[peek] == '\t' ) )
                        {
                            ++peek;
                        }

                        if( peek >= (int)markdownLength_ || markdown_[peek] == '\n' || markdown_[peek] == '\r' )
                        {
                            // Tag was at end of line. The tag widget normally advances ImGui to the next line.
                            // Consume optional \r and single trailing \n so it doesn't double-advance!
                            if( peek < (int)markdownLength_ && markdown_[peek] == '\r' )
                            {
                                ++peek;
                            }
                            if( peek < (int)markdownLength_ && markdown_[peek] == '\n' )
                            {
                                ++peek;
                            }
                            // Safety guard: if the tag did not advance or left IsSameLine set,
                            // explicitly break to a new line so the next line never renders on top of this line!
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18900
                            if( ImGui::GetCurrentWindow()->DC.IsSameLine )
                            {
                                ImGui::NewLine();
                            }
#endif
                            textRegion.ResetIndent();
                            line = Line();
                            em = Emphasis();
                            link = Link();
                            line.lineStart = peek;
                            line.lastRenderPosition = peek - 1;
                            i = peek - 1;
                            continue;
                        }
                        else
                        {
                            // Tag was mid-line; flow inline with following text without eating spaces!
                            i = closeEnd - 1;
                            line.lastRenderPosition = closeEnd - 1;
                            line.lineStart = closeEnd;
                            if( em.state == Emphasis::MIDDLE )
                            {
                                em.text.start = closeEnd;
                            }
                            if( !isClearTag && !isBrTag && !( isImgTag && GetActiveFloatScope() && GetActiveFloatScope()->justPlacedFloat ) )
                            {
                                ImGui::SameLine( 0.0f, 0.0f );
                            }
                            continue;
                        }
                    }
                    }
                }
            }

            // Test to see if we have a link
            switch( link.state )
            {
            case Link::NO_LINK:
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                if( c == '{' && !line.isHeading && !( em.state != Emphasis::NONE && em.sym == '`' ) )
#else
                if( ( c == '[' || c == '{' ) && !line.isHeading && !( em.state != Emphasis::NONE && em.sym == '`' ) )
#endif
                {
                    link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                    link.openBracketChar = c;
                    link.text.start = i + 1;
                    if( i > 0 && markdown_[i - 1] == '!' )
                    {
                        link.isImage = true;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKET_OPEN:
                {
                    char expectedClose = ( link.openBracketChar == '{' ) ? '}' : ']';
                    if( c == expectedClose )
                    {
                        link.state = Link::HAS_SQUARE_BRACKETS;
                        link.text.stop = i;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKETS:
                if( c == '(' )
                {
                    link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                    link.url.start = i + 1;
                    link.num_brackets_open = 1;
                }
                else
                {
                    // If not immediately followed by '(', reset link state:
                    link = Link();
#if defined(IMGUI_MARKDOWN_DELIMITER_CURLY_ONLY) || defined(CONTRIB_MARKDOWN_CURLY_ONLY)
                    if( c == '{' && !line.isHeading )
#else
                    if( ( c == '[' || c == '{' ) && !line.isHeading )
#endif
                    {
                        link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                        link.openBracketChar = c;
                        link.text.start = i + 1;
                        if( i > 0 && markdown_[i - 1] == '!' )
                        {
                            link.isImage = true;
                        }
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
                if( c == '(' )
                {
                    ++link.num_brackets_open;
                }
                else if( c == ')' )
                {
                    --link.num_brackets_open;
                }
                if( link.num_brackets_open == 0 )
                {
                    const bool isEnclosedInEmphasis = ( em.state == Emphasis::MIDDLE );
                    const int linkOpenPos = link.text.start - ( link.isImage ? 2 : 1 );

                    if( isEnclosedInEmphasis )
                    {
                        // 1. Render any plain text that preceded the opening emphasis delimiter:
                        int preEmphasisEnd = em.text.start - line.emphasisCount;
                        if( preEmphasisEnd > line.lineStart && preEmphasisEnd > line.lastRenderPosition + 1 )
                        {
                            line.lineEnd = preEmphasisEnd;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
                            line.isUnorderedListStart = false;
                            line.isOrderedListStart = false;
                            ImGui::SameLine( 0.0f, 0.0f );
                            line.lastRenderPosition = preEmphasisEnd - 1;
                            line.lineStart = preEmphasisEnd;
                        }
                        else if( line.isUnorderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isUnorderedListStart = false;
                        }
                        else if( line.isOrderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isOrderedListStart = false;
                        }

                        // 2. Render any emphasized text that preceded this link (from em.text.start to linkOpenPos):
                        if( linkOpenPos > em.text.start && linkOpenPos > line.lastRenderPosition + 1 )
                        {
                            line.isEmphasis = true;
                            line.lastRenderPosition = em.text.start - 1;
                            line.lineStart = em.text.start;
                            line.lineEnd = linkOpenPos;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
                            line.isEmphasis = false;
                            ImGui::SameLine( 0.0f, 0.0f );
                            line.lastRenderPosition = linkOpenPos - 1;
                            line.lineStart = linkOpenPos;
                        }
                    }
                    else
                    {
                        em = Emphasis();
                        line.lineEnd = linkOpenPos;
                        if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                        {
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
                            line.isUnorderedListStart = false;
                            line.isOrderedListStart = false;
                            ImGui::SameLine( 0.0f, 0.0f );
                        }
                        else if( line.isUnorderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isUnorderedListStart = false;
                        }
                        else if( line.isOrderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isOrderedListStart = false;
                        }
                    }
                    link.url.stop = i;
                    if( link.isImage )
                    {
                        // Check for trailing Pandoc attribute block: ![alt](url){ width=180px ... }
                        int attrStart = -1;
                        int attrEnd = -1;
                        int p_attr = i + 1;
                        while( p_attr < (int)markdownLength_ && ( markdown_[p_attr] == ' ' || markdown_[p_attr] == '\t' ) )
                        {
                            ++p_attr;
                        }
                        if( p_attr < (int)markdownLength_ && markdown_[p_attr] == '{' )
                        {
                            int closeBrace = -1;
                            char attrQuote = '\0';
                            for( int k = p_attr + 1; k < (int)markdownLength_; ++k )
                            {
                                char kc = markdown_[k];
                                if( kc == '\n' || kc == '\r' ) break;
                                if( attrQuote != '\0' )
                                {
                                    if( kc == attrQuote && markdown_[k - 1] != '\\' ) attrQuote = '\0';
                                    continue;
                                }
                                if( kc == '"' || kc == '\'' ) { attrQuote = kc; continue; }
                                if( kc == '}' ) { closeBrace = k; break; }
                            }
                            if( closeBrace != -1 )
                            {
                                attrStart = p_attr + 1;
                                attrEnd = closeBrace;
                                i = closeBrace; // Advance loop index to consume the attribute block
                            }
                        }

                        MarkdownImageData imageData;
                        MarkdownImageCallback* imageCb = mdConfig_.imageCallback ? mdConfig_.imageCallback : GetDefaultMarkdownImageCallback();
                        std::string combinedUrl;
                        if( attrStart != -1 && attrEnd > attrStart )
                        {
                            combinedUrl.assign( markdown_ + link.url.start, (size_t)link.url.size() );
                            combinedUrl += ' ';
                            combinedUrl.append( markdown_ + attrStart, (size_t)( attrEnd - attrStart ) );
                        }
                        if( imageCb )
                        {
                            if( !combinedUrl.empty() )
                            {
                                imageData = imageCb( { markdown_ + link.text.start, link.text.size(), combinedUrl.c_str(), (int)combinedUrl.length(), mdConfig_.userData, true } );
                            }
                            else
                            {
                                imageData = imageCb( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );
                            }
                        }

                        const MarkdownDetail::MarkdownImageRenderResult imgRes = MarkdownDetail::RenderMarkdownImageElement(
                            mdConfig_,
                            imageData,
                            markdown_ + link.text.start,
                            link.text.size(),
                            markdown_ + link.url.start,
                            link.url.size(),
                            true );

                        if( imgRes.isBlockFigure )
                        {
                            int peek = i + 1;
                            while( peek < (int)markdownLength_ && ( markdown_[peek] == ' ' || markdown_[peek] == '\t' ) )
                            {
                                ++peek;
                            }
                            if( peek >= (int)markdownLength_ || markdown_[peek] == '\n' || markdown_[peek] == '\r' )
                            {
                                if( peek < (int)markdownLength_ && markdown_[peek] == '\r' ) ++peek;
                                if( peek < (int)markdownLength_ && markdown_[peek] == '\n' ) ++peek;
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18900
                                if( ImGui::GetCurrentWindow()->DC.IsSameLine )
                                {
                                    ImGui::NewLine();
                                }
#endif
                                textRegion.ResetIndent();
                                line = Line();
                                em = Emphasis();
                                link = Link();
                                line.lineStart = peek;
                                line.lastRenderPosition = peek - 1;
                                i = peek - 1;
                                continue;
                            }
                            else
                            {
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18900
                                if( ImGui::GetCurrentWindow()->DC.IsSameLine )
                                {
                                    ImGui::NewLine();
                                }
#endif
                                textRegion.ResetIndent();
                                line = Line();
                                em = Emphasis();
                                link = Link();
                                line.lineStart = i + 1;
                                line.lastRenderPosition = i;
                                continue;
                            }
                        }
                    }
                    else
                    {
                        if( isEnclosedInEmphasis )
                        {
                            MarkdownFormatInfo emFormatInfo;
                            emFormatInfo.config = &mdConfig_;
                            emFormatInfo.type = MarkdownFormatType::EMPHASIS;
                            emFormatInfo.emphasisChar = line.emphasisChar;
                            emFormatInfo.level = line.emphasisCount;
                            mdConfig_.formatCallback( emFormatInfo, true );
                            textRegion.RenderLinkTextWrapped( markdown_ + link.text.start, markdown_ + link.text.start + link.text.size(), link, markdown_, mdConfig_, &linkHoverStart, false );
                            mdConfig_.formatCallback( emFormatInfo, false );
                        }
                        else
                        {
                            textRegion.RenderLinkTextWrapped( markdown_ + link.text.start, markdown_ + link.text.start + link.text.size(), link, markdown_, mdConfig_, &linkHoverStart, false );
                        }
                    }
                    ImGui::SameLine( 0.0f, 0.0f );
                    link = Link();
                    line.lastRenderPosition = i;
                    if( isEnclosedInEmphasis )
                    {
                        em.text.start = i + 1;
                    }
                    break;
                }
            }

            // Test to see if we have emphasis styling
			switch( em.state )
			{
			case Emphasis::NONE:
				if( link.state == Link::NO_LINK && !line.isHeading )
                {
                    int next = i + 1;
                    int prev = i - 1;
					// Check if the character is in the user's registered emphasisChars string
					if( mdConfig_.emphasisChars && strchr(mdConfig_.emphasisChars, c) != NULL
                        // For markers that strictly require at least 2 characters (CriticMarkup '++' underline,
                        // GFM '~~' strikethrough, and ABC legacy '$$' strikethrough), do not trigger on single chars
                        // unless a custom formatCallback is registered:
                        && ( mdConfig_.formatCallback != defaultMarkdownFormatCallback
                             || ( c != '+' && c != '~' && c != '$' )
                             || ( (int)markdownLength_ > next && markdown_[ next ] == c ) )
                        && ( i == line.lineStart
                            || isspace((unsigned char)markdown_[ prev ])
                            || ( ispunct((unsigned char)markdown_[ prev ]) && markdown_[ prev ] != c ) )
                        && (int)markdownLength_ > next 
                        && markdown_[ next ] != ' '
                        && markdown_[ next ] != '\n'
					    && markdown_[ next ] != ')'
                        && markdown_[ next ] != '\t' )
                    {
						em.state = Emphasis::LEFT;
						em.sym = c;
                        em.text.start = i;
						line.emphasisCount = 1;
						line.emphasisChar = c;
						continue;
					}
				}
				break;
			case Emphasis::LEFT:
				if( em.sym == c )
                {
					++line.emphasisCount;
					continue;
				}
                else
                {
					em.text.start = i;
					em.state = Emphasis::MIDDLE;
				}
				break;
			case Emphasis::MIDDLE:
				if( em.sym == c )
                {
					em.state = Emphasis::RIGHT;
					em.text.stop = i;
				}
                else
                {
                    break;
                }
			case Emphasis::RIGHT:
				if( em.sym == c )
                {
					if( line.emphasisCount <= 3 && ( i - em.text.stop + 1 == line.emphasisCount ) )
                    {
                        int lineEnd = em.text.start - line.emphasisCount;
                        if( lineEnd > line.lineStart && lineEnd > line.lastRenderPosition + 1 )
                        {
                            line.lineEnd = lineEnd;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
						    ImGui::SameLine( 0.0f, 0.0f );
                            line.isUnorderedListStart = false;
                            line.isOrderedListStart = false;
                        }
                        else if( line.isUnorderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyBullet( mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isUnorderedListStart = false;
                        }
                        else if( line.isOrderedListStart )
                        {
                            int numLeadIndents = line.leadSpaceCount / 2;
                            textRegion.ApplyLeadIndent( numLeadIndents );
                            textRegion.ApplyOrderedNumber( markdown_ + line.orderedListNumStart, markdown_ + line.orderedListNumEnd, mdConfig_.bulletSpacing > 0.0f ? mdConfig_.bulletSpacing : -1.0f );
                            line.isOrderedListStart = false;
                        }
						line.isEmphasis = true;
						line.lastRenderPosition = em.text.start - 1;
                        line.lineStart = em.text.start;
					    line.lineEnd = em.text.stop;
                        if( line.lineEnd > line.lineStart && line.lineEnd > line.lastRenderPosition + 1 )
                        {
					        RenderLine( markdown_, line, textRegion, mdConfig_ );
					        ImGui::SameLine( 0.0f, 0.0f );
                        }
					    line.isEmphasis = false;
					    line.lastRenderPosition = i;
					    em = Emphasis();
                    }
                    continue;
				} 
                else
                {
                    em.state = Emphasis::NONE;
                    int start = em.text.start - line.emphasisCount;
                    if( start < line.lineStart )
                    {
                        line.lineEnd = line.lineStart;
                        line.lineStart = start;
                        line.lastRenderPosition = start - 1;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        line.lineStart          = line.lineEnd;
                        line.lastRenderPosition = line.lineStart - 1;
                    }
                }
				break;
			}

            if( c == '\n' )
            {
                line.lineEnd = i;
                line.isPrecededBySeparator = lastWasSeparator;
                line.isPrecededByBlankLine = lastWasBlankLine;
                RenderLine( markdown_, line, textRegion, mdConfig_ );

                lastWasSeparator = line.hadSeparator;
                lastWasBlankLine = line.wasBlank;

				line = Line();
                em = Emphasis();

                line.lineStart = i + 1;
                line.lastRenderPosition = i;
                // Check whether the next line is a continuation line of the current list item
                bool isNextContinuation = false;
                if( ( textRegion.GetIndent() > 0.0f || textRegion.GetLeadIndents() > 0 ) && i + 1 < (int)markdownLength_ )
                {
                    int next = i + 1;
                    int spaceCount = 0;
                    while( next < (int)markdownLength_ && ( markdown_[next] == ' ' || markdown_[next] == '\t' ) )
                    {
                        spaceCount += ( markdown_[next] == '\t' ) ? 4 : 1;
                        ++next;
                    }
                    if( spaceCount >= 2 && next < (int)markdownLength_ )
                    {
                        char nextC = markdown_[next];
                        if( nextC != '*' && nextC != '-' && nextC != '+' && nextC != '#' && !( nextC >= '0' && nextC <= '9' ) && nextC != '\n' && nextC != '\r' )
                        {
                            isNextContinuation = true;
                        }
                    }
                }

                if( !isNextContinuation )
                {
                    textRegion.ResetIndent();
                }
                link = Link();
            }
        }

        if( markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[ line.lineStart ] != 0 )
        {
            line.lineEnd = (int)markdownLength_;
            if( 0 == markdown_[ line.lineEnd - 1 ] )
            {
                --line.lineEnd;
            }
            line.isPrecededBySeparator = lastWasSeparator;
            line.isPrecededByBlankLine = lastWasBlankLine;
            RenderLine( markdown_, line, textRegion, mdConfig_ );
        }
    }

    inline bool TextRegion::RenderLinkText( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ )
    {
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.type = MarkdownFormatType::LINK;
        mdConfig_.formatCallback( formatInfo, true );
        ImGui::PushTextWrapPos( -1.0f );
        ImGui::TextUnformatted( text_, text_end_ );
        ImGui::PopTextWrapPos();

        bool bThisItemHovered = ImGui::IsItemHovered();
        if(bThisItemHovered)
        {
            *linkHoverStart_ = markdown_ + link_.text.start;
        }
        bool bHovered = bThisItemHovered || ( *linkHoverStart_ == ( markdown_ + link_.text.start ) );

        formatInfo.itemHovered = bHovered;
        mdConfig_.formatCallback( formatInfo, false );

        if(bHovered)
        {
            MarkdownLinkCallback* linkCb = mdConfig_.linkCallback ? mdConfig_.linkCallback : GetDefaultMarkdownLinkCallback();
            if( ImGui::IsMouseReleased( 0 ) && linkCb )
            {
                linkCb( { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false } );
            }
            if( mdConfig_.tooltipCallback )
            {
                mdConfig_.tooltipCallback( { { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false }, mdConfig_.linkIcon } );
            }
        }
        return bThisItemHovered;
    }

    inline void TextRegion::RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ )
        {
            if( wrapWidth < 0.0f )
            {
                if( text_end_ > text_ )
                {
                    MarkdownDetail::PrepareLineStartForFloats();
                }
                bool bHovered = RenderLinkText( text_, text_end_, link_, markdown_, mdConfig_, linkHoverStart_ );
                if( !bHovered && *linkHoverStart_ == markdown_ + link_.text.start )
                {
                    *linkHoverStart_ = NULL;
                }
                return;
            }
#ifdef ORIGINAL
		float       scale = ImGui::GetIO().FontGlobalScale;
#else /* brat's version: */
		float		scale = ImGui::GetFontScale(false);	  // use parent's scale
#endif
            float       lineStartX = 0.0f;
            float       widthLeft = 0.0f;
            MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
            float       usedOnLine = ImMax( 0.0f, ImGui::GetCursorScreenPos().x - lineStartX );
            const char* endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
            endLine = AdjustWrapForTypography( text_, endLine, text_end_ );

            // If we are continuing on an existing line with prior content (usedOnLine > 0),
            // and the first word cannot cleanly fit in widthLeft, push the word to the next line.
            // When wrapping to a new line, strip any leading spaces that served only as inter-item separation on the previous line.
            if( usedOnLine > 0.0f )
            {
                const char* first_non_space = text_;
                while( first_non_space < text_end_ && *first_non_space == ' ' ) ++first_non_space;

                bool breaks_first_word = false;
                if( endLine <= first_non_space && first_non_space < text_end_ )
                {
                    breaks_first_word = true;
                }
                else if( endLine > first_non_space && endLine < text_end_ )
                {
                    const char* word_end = first_non_space;
                    while( word_end < text_end_ && IsCharInsideWord( *word_end ) ) ++word_end;
                    if( endLine < word_end )
                    {
                        breaks_first_word = true;
                    }
                }

                if( breaks_first_word )
                {
                    // Push to next line and suppress inter-item leading space!
                    ImGui::NewLine();
                    text_ = first_non_space;
                    MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
                    endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
                    endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                }
            }
            
            bool bHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
            if( bIndentToHere_ )
            {
                float indentNeeded = GetContentRegionAvail().x - widthLeft;
                if( indentNeeded )
                {
                    ImGui::Indent( indentNeeded );
                    indentX += indentNeeded;
                }
            }
            while( endLine < text_end_ )
            {
                text_ = endLine;
                while( text_ < text_end_ && *text_ == ' ' ) { ++text_; }    // skip spaces at start of line
                MarkdownDetail::ResolveLineFloatGeometry( wrapWidth, lineStartX, widthLeft );
                endLine = ImGui::MarkdownDetail::CalcWordWrapPositionCompat( ImGui::GetFont(), scale, text_, text_end_, widthLeft );
                endLine = AdjustWrapForTypography( text_, endLine, text_end_ );
                if( text_ == endLine && text_ < text_end_ ) 
                {
                    endLine++;
                }
                bool bThisLineHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
                bHovered = bHovered || bThisLineHovered;
            }
            if( !bHovered && *linkHoverStart_ == markdown_ + link_.text.start )
            {
                *linkHoverStart_ = NULL;
            }
        }

    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ )
    {
        switch( markdownFormatInfo_.type )
        {
        case MarkdownFormatType::NORMAL_TEXT:
        case MarkdownFormatType::UNORDERED_LIST:
        case MarkdownFormatType::ORDERED_LIST:
            if( markdownFormatInfo_.config && markdownFormatInfo_.config->regularFont )
            {
                if( start_ ) MarkdownDetail::PushMarkdownFont( markdownFormatInfo_.config->regularFont );
                else         ImGui::PopFont();
            }
            break;
		case MarkdownFormatType::EMPHASIS:
        {
            if( markdownFormatInfo_.emphasisChar == '`' )
            {
                // Monospace code span (`code`):
                ImFont* mono_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->monoFont : nullptr;
                if( mono_font )
                {
                    if( start_ ) MarkdownDetail::PushMarkdownFont( mono_font );
                    else         ImGui::PopFont();
                }
                break;
            }

            if( ( markdownFormatInfo_.emphasisChar == '~' || markdownFormatInfo_.emphasisChar == '$' ) && markdownFormatInfo_.level >= 2 )
            {
                // GFM Strikethrough (~~text~~) or ABC legacy strikethrough ($$text$$):
                if( !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), true );
                }
                break;
            }

            if( markdownFormatInfo_.emphasisChar == '_' && markdownFormatInfo_.config && markdownFormatInfo_.config->underlineWithUnderscores && markdownFormatInfo_.level == 2 )
            {
                // Underline via __text__ when underlineWithUnderscores is enabled:
                if( !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), false );
                }
                break;
            }

            if( markdownFormatInfo_.emphasisChar == '+' )
            {
                // Underline via ++text++ (CriticMarkup / Markdown Extended):
                if( markdownFormatInfo_.level >= 2 && !start_ )
                {
                    DrawTextDecorationLine( ImGui::GetColorU32( ImGuiCol_Text ), false );
                }
                break;
            }

            if( markdownFormatInfo_.level == 1 )
            {
                // Level 1: Italic (*italic* or _italic_)
                ImFont* italic_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->italicFont : nullptr;
                if( italic_font )
                {
                    if( start_ ) MarkdownDetail::PushMarkdownFont( italic_font );
                    else         ImGui::PopFont();
                }
                else
                {
                    if( start_ ) ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ ImGuiCol_TextDisabled ] );
                    else         ImGui::PopStyleColor();
                }
            }
            else if( markdownFormatInfo_.level >= 3 )
            {
                // Level 3: Bold-Italic (***bold-italic*** or ___bold-italic___)
                ImFont* bi_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->boldItalicFont : nullptr;
                if( !bi_font && markdownFormatInfo_.config )
                {
                    bi_font = markdownFormatInfo_.config->boldFont;
                    if( !bi_font )
                    {
                        bi_font = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
                    }
                }
                if( start_ )
                {
                    if( bi_font )
                    {
                        MarkdownDetail::PushMarkdownFont( bi_font );
                    }
                }
                else
                {
                    if( bi_font )
                    {
                        ImGui::PopFont();
                    }
                }
            }
            else
            {
                // Level 2: Bold (**bold** or __bold__)
                ImFont* bold_font = markdownFormatInfo_.config ? markdownFormatInfo_.config->boldFont : nullptr;
                if( !bold_font && markdownFormatInfo_.config )
                {
                    bold_font = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ].font;
                }
			    if( start_ )
			    {
				    if( bold_font )
				    {
					    MarkdownDetail::PushMarkdownFont( bold_font );
				    }
			    }
                else
			    {
				    if( bold_font )
				    {
					    ImGui::PopFont();
				    }
			    }
            }
            break;
        }
        case MarkdownFormatType::HEADING:
        {
            MarkdownHeadingFormat fmt;
            if( markdownFormatInfo_.level > MarkdownConfig::NUMHEADINGS )
            {
                fmt = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
            }
            else
            {
                fmt = markdownFormatInfo_.config->headingFormats[ markdownFormatInfo_.level - 1 ];
            }

            // Semantic font overrides:
            if( markdownFormatInfo_.config )
            {
                if( markdownFormatInfo_.level == 1 && markdownFormatInfo_.config->h1Font )
                    fmt.font = markdownFormatInfo_.config->h1Font;
                else if( markdownFormatInfo_.level == 2 && markdownFormatInfo_.config->h2Font )
                    fmt.font = markdownFormatInfo_.config->h2Font;
                else if( markdownFormatInfo_.level == 3 && markdownFormatInfo_.config->h3Font )
                    fmt.font = markdownFormatInfo_.config->h3Font;
            }

            if( start_ )
            {
                if( fmt.font  )
                {
                    ImGui::ImGeeCompat::PushFont( fmt.font );
                }
                if( !markdownFormatInfo_.isStartOfDoc &&
                    !markdownFormatInfo_.isPrecededBySeparator &&
                    !markdownFormatInfo_.isPrecededByBlankLine )
                {
                    float font_sz = ImGui::GetFontSize();
                    float top_margin = ( markdownFormatInfo_.level <= 2 ) ? ( font_sz * 0.45f ) : ( font_sz * 0.30f );
                    ImGui::Dummy( ImVec2( 0.0f, top_margin ) );
                }
            }
            else
            {
                if( fmt.separator )
                {
                    MarkdownDetail::FloatLayoutScope* fl_scope = MarkdownDetail::GetActiveFloatScope();
                    float left_in = 0.0f, right_in = 0.0f;
                    const float sep_y = ImGui::GetCursorScreenPos().y;
                    if( fl_scope && MarkdownDetail::QueryFloatBandAtY( sep_y, 2.0f, left_in, right_in ) )
                    {
                        const float x1 = fl_scope->containerLeftX + left_in;
                        const float x2 = fl_scope->containerRightX - right_in;
                        if( x2 > x1 + 4.0f )
                        {
                            ImGui::GetWindowDrawList()->AddLine( ImVec2( x1, sep_y ), ImVec2( x2, sep_y ), ImGui::GetColorU32( ImGuiCol_Separator ), 1.0f );
                            ImGui::Dummy( ImVec2( 0.0f, ImMax( 2.0f, ImGui::GetStyle().ItemSpacing.y * 0.5f ) ) );
                        }
                    }
                    else
                    {
                        ImGui::Separator();
                    }
                }
                else
                {
                    float font_sz = ImGui::GetFontSize();
                    float bottom_margin = font_sz * 0.20f;
                    ImGui::Dummy( ImVec2( 0.0f, bottom_margin ) );
                }
                if( fmt.font )
                {
                    ImGui::PopFont();
                }
            }
            break;
        }
        case MarkdownFormatType::LINK:
        {
            // Resolve link color: prefer ImGuiCol_TextLink if available and non-zero,
            // otherwise use a crisp, high-contrast sky blue accent (0.45f, 0.75f, 1.0f).
            ImVec4 linkCol = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19100)
            if (ImGui::GetStyle().Colors[ImGuiCol_TextLink].w > 0.0f) {
                linkCol = ImGui::GetStyle().Colors[ImGuiCol_TextLink];
            }
#endif
            ImVec4 linkHoverCol = ImVec4((std::min)(1.0f, linkCol.x * 1.25f), (std::min)(1.0f, linkCol.y * 1.25f), (std::min)(1.0f, linkCol.z * 1.25f), 1.0f);

            if( start_ )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, markdownFormatInfo_.itemHovered ? linkHoverCol : linkCol );
            }
            else
            {
                ImGui::PopStyleColor();
                if( markdownFormatInfo_.itemHovered )
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                    ImGui::UnderLine( linkHoverCol );
                }
                else
                {
                    ImGui::UnderLine( ImVec4(linkCol.x, linkCol.y, linkCol.z, 0.60f) );
                }
            }
            break;
        }
        case MarkdownFormatType::CODE_BLOCK:
            break;
        }
    }

    // -------------------------------------------------------------------------
    // Markdown Table Layout & Sort State Helper
    // -------------------------------------------------------------------------
    // Updates or inserts an HTML comment directive (<!-- table-state: ... -->)
    // immediately following the table rows in the provided markdown text.
    // If the table is reset to its baseline/default state, any existing
    // <!-- table-state: ... --> comment is cleanly removed.
    // Author baseline presets (<!-- table-preset: ... -->) are always preserved intact.
    inline std::string ApplyTableStateToMarkdown( const std::string& markdownText, const MarkdownTableState& state )
    {
        // 1. Determine target directive contents
        bool is_default_width = ( state.totalWidth <= 0.0f ) ||
                                ( state.hasPreset && std::fabs( state.totalWidth - state.presetWidth ) < 0.5f );
        bool is_default_cols  = !state.hasCustomColumnWidths || state.columnWidths.empty();
        bool is_default_sort  = ( state.sortColumn < 0 ) || ( state.sortDirection == 0 );

        std::string new_directive;
        if( !is_default_width || !is_default_cols || !is_default_sort )
        {
            new_directive = "<!-- table-state:";
            if( !is_default_width )
            {
                char buf[32];
                if( std::fabs( state.totalWidth - std::round( state.totalWidth ) ) < 0.05f )
                    snprintf( buf, sizeof(buf), " width=%.0f", state.totalWidth );
                else
                    snprintf( buf, sizeof(buf), " width=%.1f", state.totalWidth );
                new_directive += buf;
            }
            if( !is_default_cols )
            {
                new_directive += " cols=";
                for( size_t c = 0; c < state.columnWidths.size(); ++c )
                {
                    if( c > 0 ) new_directive += ",";
                    char cbuf[32];
                    float cw = state.columnWidths[c];
                    if( std::fabs( cw - std::round( cw ) ) < 0.05f )
                        snprintf( cbuf, sizeof(cbuf), "%.0f", cw );
                    else
                        snprintf( cbuf, sizeof(cbuf), "%.1f", cw );
                    new_directive += cbuf;
                }
            }
            if( !is_default_sort )
            {
                new_directive += " sort=";
                if( !state.sortColumnName.empty() )
                {
                    new_directive += "\"";
                    new_directive += state.sortColumnName;
                    new_directive += "\"";
                }
                else
                {
                    new_directive += std::to_string( state.sortColumn );
                }
                new_directive += ( state.sortDirection == 2 ) ? " desc" : " asc";
            }
            new_directive += " -->";
        }

        int text_len = (int)markdownText.size();
        int t_start = state.textStart;
        int t_end = state.textEnd;
        int r_end = state.rowsEnd;

        // Locate existing table-state directive attached to this table
        int search_start = ( r_end > 0 && r_end <= text_len ) ? r_end : t_start;
        int search_end = ImMin( t_end + 128, text_len );

        int existing_comment_start = -1;
        int existing_comment_end = -1;

        if( search_end > search_start )
        {
            std::string search_slice = markdownText.substr( search_start, search_end - search_start );
            size_t found_pos = search_slice.find( "<!-- table-state:" );
            if( found_pos == std::string::npos )
            {
                found_pos = search_slice.find( "<!-- table: user" );
            }
            if( found_pos == std::string::npos )
            {
                found_pos = search_slice.find( "<!-- table: state" );
            }
            if( found_pos == std::string::npos )
            {
                size_t p = search_slice.find( "<!-- table:" );
                if( p != std::string::npos )
                {
                    std::string sub = search_slice.substr( p );
                    if( sub.find( "default" ) == std::string::npos && sub.find( "preset" ) == std::string::npos )
                    {
                        found_pos = p;
                    }
                }
            }

            if( found_pos != std::string::npos )
            {
                size_t close_pos = search_slice.find( "-->", found_pos );
                if( close_pos != std::string::npos )
                {
                    existing_comment_start = (int)( search_start + found_pos );
                    existing_comment_end = (int)( search_start + close_pos + 3 );

                    // Include surrounding line delimiters
                    int line_start = existing_comment_start;
                    while( line_start > 0 && markdownText[line_start - 1] != '\n' && markdownText[line_start - 1] != '\r' )
                    {
                        if( markdownText[line_start - 1] == ' ' || markdownText[line_start - 1] == '\t' )
                            --line_start;
                        else
                            break;
                    }
                    int line_end = existing_comment_end;
                    if( line_end < text_len && markdownText[line_end] == '\r' ) ++line_end;
                    if( line_end < text_len && markdownText[line_end] == '\n' ) ++line_end;

                    existing_comment_start = line_start;
                    existing_comment_end = line_end;
                }
            }
        }

        std::string result = markdownText;

        if( existing_comment_start >= 0 )
        {
            if( new_directive.empty() )
            {
                result.erase( existing_comment_start, existing_comment_end - existing_comment_start );
            }
            else
            {
                std::string rep = new_directive + "\n";
                result.replace( existing_comment_start, existing_comment_end - existing_comment_start, rep );
            }
        }
        else if( !new_directive.empty() )
        {
            int ins_pos = ( t_end > 0 && t_end <= text_len ) ? t_end : r_end;
            while( ins_pos < text_len && markdownText[ins_pos] != '\n' && markdownText[ins_pos] != '\r' )
                ++ins_pos;
            if( ins_pos < text_len && markdownText[ins_pos] == '\r' ) ++ins_pos;
            if( ins_pos < text_len && markdownText[ins_pos] == '\n' ) ++ins_pos;

            std::string ins = new_directive + "\n";
            result.insert( ins_pos, ins );
        }

        return result;
    }

}

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#define IMGUI_MARKDOWN_READY 1

#if defined(IMGUI_FONT_ROLES_H) && !defined(IMGUI_MARKDOWN_CAPTION_FONT_BRIDGE_DEFINED)
#define IMGUI_MARKDOWN_CAPTION_FONT_BRIDGE_DEFINED
namespace ImGui {
namespace MarkdownDetail {
    inline ImFont* ResolveCaptionFontViaTypographyRole() {
        if( ImFont* explicit_small = ImGui::Detail::GetFontRoleSlot( ImGui::ImFontRole::Small ) ) {
            return explicit_small;
        }
        if( ImGui::GetCurrentContext() ) {
            ImGuiIO& io = ImGui::GetIO();
            if( io.Fonts && (int)ImGui::ImFontRole::Small < io.Fonts->Fonts.Size ) {
                return io.Fonts->Fonts[(int)ImGui::ImFontRole::Small];
            }
        }
        return nullptr;
    }
    struct MarkdownCaptionFontBridgeRunner {
        MarkdownCaptionFontBridgeRunner() {
            GetCaptionFontResolver() = &ResolveCaptionFontViaTypographyRole;
        }
    };
    static MarkdownCaptionFontBridgeRunner s_MarkdownCaptionFontBridgeRunner;
} // namespace MarkdownDetail
} // namespace ImGui
#endif

#if defined(IMGUI_ICONS_H) && defined(CONTRIB_ICONS_AVAILABLE) && !defined(IMGUI_MARKDOWN_ICONS_BRIDGE_DEFINED)
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

#if defined(IMGUI_CLICKABLE_LINKS_READY) && !defined(IMGUI_MARKDOWN_AUTO_LINK_BRIDGED)
#if defined(__has_include)
#if __has_include("../../widgets/imgui_clickable_links.h")
#include "../../widgets/imgui_clickable_links.h"
#elif __has_include("imgui_clickable_links.h")
#include "imgui_clickable_links.h"
#endif
#endif
#endif

#endif // IMGUI_MARKDOWN_H
