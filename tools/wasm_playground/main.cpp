// ============================================================================
// ImGee Live Browser WebAssembly Playground (tools/wasm_playground/main.cpp)
// ----------------------------------------------------------------------------
// Compiles upstream Dear ImGui (v1.92.8, SDL2 + WebGL2) and ImGee's opt-in
// interactive showcase (imgee_demo.h) to single-threaded WebAssembly via
// Emscripten -- zero server, zero SharedArrayBuffer/COOP/COEP headers, crisp
// Retina DPR rendering, native OS clipboard, URL launching, and ?tab=1..8 &
// ?theme=dark|light deep linking.
// ============================================================================

#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL.h>
#include <GLES3/gl3.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#define IMGEE_INCLUDE_DEMO
#include "imgee.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

// ----------------------------------------------------------------------------
// Browser Bridges (EM_JS -- warning-clean under -Wall -Wextra -Werror)
// ----------------------------------------------------------------------------

EM_JS(void, js_copy_clipboard, (const char* text), {
    try {
        var str = UTF8ToString(text);
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(str).catch(function() {});
        }
    } catch (e) {}
});

EM_JS(void, js_open_url, (const char* url), {
    try {
        var str = UTF8ToString(url);
        if (str && (str.indexOf("https://") === 0 || str.indexOf("http://") === 0)) {
            window.open(str, "_blank", "noopener,noreferrer");
        }
    } catch (e) {}
});

EM_JS(int, js_get_initial_tab, (), {
    try {
        var params = new URLSearchParams(window.location.search);
        var raw = params.get("tab");
        if (!raw) return -1;
        var n = parseInt(raw, 10);
        if (n >= 1 && n <= 8) return n - 1;
    } catch (e) {}
    return -1;
});

EM_JS(int, js_get_initial_theme, (), {
    try {
        var params = new URLSearchParams(window.location.search);
        var raw = (params.get("theme") || "").toLowerCase();
        if (raw === "light" || raw === "paper" || raw === "paperlight") return 4;
        if (raw === "rusty") return 1;
        if (raw === "imgui-dark") return 2;
        if (raw === "classic") return 3;
        if (raw === "dark" || raw === "charcoal") return 0;
    } catch (e) {}
    return 0;
});

EM_JS(void, js_sync_shell_theme, (int isLight), {
    try {
        if (window.imgeeSyncShellTheme) {
            window.imgeeSyncShellTheme(Boolean(isLight));
        }
    } catch (e) {}
});

EM_JS(void, js_hide_loader, (), {
    try {
        if (window.imgeeHideLoader) {
            window.imgeeHideLoader();
        }
    } catch (e) {}
});

// ----------------------------------------------------------------------------
// WebGL2 HostBackend (Markdown images & lightboxes) + Clipboard + Links
// ----------------------------------------------------------------------------

static std::string g_ClipboardCache;

static void WasmSetClipboardText(ImGuiContext*, const char* text) {
    g_ClipboardCache = text ? text : "";
    js_copy_clipboard(g_ClipboardCache.c_str());
}

static const char* WasmGetClipboardText(ImGuiContext*) {
    return g_ClipboardCache.c_str();
}

static ImTextureID WasmCreateTextureRGBA8(const unsigned char* pixels, int width, int height, void*) {
    if (!pixels || width <= 0 || height <= 0) {
        return static_cast<ImTextureID>(0);
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return static_cast<ImTextureID>(static_cast<uintptr_t>(tex));
}

static void WasmDeleteTexture(ImTextureID texId, void*) {
    const GLuint tex = static_cast<GLuint>(static_cast<uintptr_t>(texId));
    if (tex != 0) {
        glDeleteTextures(1, &tex);
    }
}

static void WasmMarkdownLinkCallback(ImGui::MarkdownLinkCallbackData data) {
    if (data.isImage || !data.link || data.linkLength <= 0) {
        return;
    }
    std::string url(data.link, static_cast<size_t>(data.linkLength));
    js_open_url(url.c_str());
}

// ----------------------------------------------------------------------------
// Font Registration (TrueType from Emscripten MEMFS -> ImFontRole slots)
// ----------------------------------------------------------------------------

static void LoadCrispFonts(float dpr) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const float rasterDensity = (dpr >= 1.0f && dpr <= 4.0f) ? dpr : 1.0f;
    auto loadFont = [&](const char* path, float pxSize) -> ImFont* {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        cfg.RasterizerDensity = rasterDensity;
#else
        (void)rasterDensity;
#endif
        return io.Fonts->AddFontFromFileTTF(path, pxSize, &cfg);
    };

    const char* regPath      = "/assets/fonts/Sans-Regular.ttf";
    const char* boldPath     = "/assets/fonts/Sans-Bold.ttf";
    const char* italicPath   = "/assets/fonts/Sans-Italic.ttf";
    const char* boldItalPath = "/assets/fonts/Sans-BoldItalic.ttf";
    const char* monoPath     = "/assets/fonts/RobotoMono-Medium.ttf";
    const char* monoBoldPath = "/assets/fonts/RobotoMono-Bold.ttf";

    ImFont* fRegular   = loadFont(regPath, 15.0f);
    ImFont* fTitle     = loadFont(boldPath, 17.0f);
    ImFont* fSmall     = loadFont(regPath, 13.0f);
    ImFont* fBold      = loadFont(boldPath, 15.0f);
    ImFont* fBoldLarge = loadFont(boldPath, 19.0f);
    ImFont* fMono      = loadFont(monoPath, 14.0f);
    ImFont* fItalic    = loadFont(italicPath, 15.0f);
    ImFont* fMonoMed   = loadFont(monoBoldPath, 15.0f);
    ImFont* fBoldItal  = loadFont(boldItalPath, 15.0f);

    if (fRegular) {
        io.FontDefault = fRegular;
        ImGui::SetFontRole(ImGui::ImFontRole::Regular,         fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Title,           fTitle     ? fTitle     : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Small,           fSmall     ? fSmall     : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Bold,            fBold      ? fBold      : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::BoldLarge,       fBoldLarge ? fBoldLarge : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Monospace,       fMono      ? fMono      : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Italic,          fItalic    ? fItalic    : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::MonospaceMedium, fMonoMed   ? fMonoMed   : (fMono ? fMono : fRegular));
        ImGui::SetFontRole(ImGui::ImFontRole::BoldItalic,      fBoldItal  ? fBoldItal  : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Heading1,        fBoldLarge ? fBoldLarge : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Heading2,        fTitle     ? fTitle     : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Heading3,        fBold      ? fBold      : fRegular);
        ImGui::SetFontRole(ImGui::ImFontRole::Tiny,            fSmall     ? fSmall     : fRegular);

        static ImGui::MarkdownConfig s_WasmMdCfg;
        s_WasmMdCfg = ImGui::MarkdownConfig();
        s_WasmMdCfg.regularFont             = fRegular;
        s_WasmMdCfg.boldFont                = fBold;
        s_WasmMdCfg.italicFont              = fItalic;
        s_WasmMdCfg.boldItalicFont          = fBoldItal;
        s_WasmMdCfg.monoFont                = fMono;
        s_WasmMdCfg.h1Font                  = fBoldLarge ? fBoldLarge : fBold;
        s_WasmMdCfg.h2Font                  = fTitle     ? fTitle     : fBold;
        s_WasmMdCfg.h3Font                  = fBold;
        s_WasmMdCfg.headingFormats[0].font  = s_WasmMdCfg.h1Font;
        s_WasmMdCfg.headingFormats[1].font  = s_WasmMdCfg.h2Font;
        s_WasmMdCfg.headingFormats[2].font  = s_WasmMdCfg.h3Font;
        s_WasmMdCfg.linkCallback            = &WasmMarkdownLinkCallback;
        ImGui::SetDefaultMarkdownConfig(&s_WasmMdCfg);
    } else {
        io.Fonts->AddFontDefault();
    }
}

// ----------------------------------------------------------------------------
// Global App State & Main Frame Loop
// ----------------------------------------------------------------------------

struct WasmAppState {
    SDL_Window*   window       = nullptr;
    SDL_GLContext glContext    = nullptr;
    int           cssW         = 1280;
    int           cssH         = 760;
    int           fbW          = 1280;
    int           fbH          = 760;
    float         dpr          = 1.0f;
    int           lastShellLt  = -1;
    bool          loaderHidden = false;
};

static WasmAppState g_App;

static void SyncCanvasSize() {
    double css_w_d = 0.0;
    double css_h_d = 0.0;
    emscripten_get_element_css_size("#canvas", &css_w_d, &css_h_d);
    double dpr_d = emscripten_get_device_pixel_ratio();
    if (dpr_d < 1.0) dpr_d = 1.0;
    if (dpr_d > 3.0) dpr_d = 3.0;

    const int want_css_w = (std::max)(320, static_cast<int>(std::round(css_w_d)));
    const int want_css_h = (std::max)(240, static_cast<int>(std::round(css_h_d)));
    const int want_fb_w  = (std::max)(320, static_cast<int>(std::round(css_w_d * dpr_d)));
    const int want_fb_h  = (std::max)(240, static_cast<int>(std::round(css_h_d * dpr_d)));

    if (want_css_w != g_App.cssW || want_css_h != g_App.cssH ||
        want_fb_w  != g_App.fbW  || want_fb_h  != g_App.fbH) {
        g_App.cssW = want_css_w;
        g_App.cssH = want_css_h;
        g_App.fbW  = want_fb_w;
        g_App.fbH  = want_fb_h;
        g_App.dpr  = static_cast<float>(dpr_d);

        SDL_SetWindowSize(g_App.window, g_App.cssW, g_App.cssH);
        emscripten_set_canvas_element_size("#canvas", g_App.fbW, g_App.fbH);
    }
}

static void MainLoopTick() {
    SyncCanvasSize();

    // Runtime-exact mouse coordinate normalization across Emscripten SDL2 DPR behaviors
    int sdlW = 0;
    int sdlH = 0;
    SDL_GetWindowSize(g_App.window, &sdlW, &sdlH);
    const float mouseScaleX = (sdlW > 0 && g_App.cssW > 0) ? (static_cast<float>(g_App.cssW) / static_cast<float>(sdlW)) : 1.0f;
    const float mouseScaleY = (sdlH > 0 && g_App.cssH > 0) ? (static_cast<float>(g_App.cssH) / static_cast<float>(sdlH)) : 1.0f;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (std::fabs(mouseScaleX - 1.0f) > 0.01f || std::fabs(mouseScaleY - 1.0f) > 0.01f) {
            if (ev.type == SDL_MOUSEMOTION) {
                ev.motion.x = static_cast<Sint32>(std::round(static_cast<float>(ev.motion.x) * mouseScaleX));
                ev.motion.y = static_cast<Sint32>(std::round(static_cast<float>(ev.motion.y) * mouseScaleY));
            } else if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
                ev.button.x = static_cast<Sint32>(std::round(static_cast<float>(ev.button.x) * mouseScaleX));
                ev.button.y = static_cast<Sint32>(std::round(static_cast<float>(ev.button.y) * mouseScaleY));
            }
        }
        ImGui_ImplSDL2_ProcessEvent(&ev);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    // Enforce logical CSS DisplaySize + physical Retina DisplayFramebufferScale
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(g_App.cssW), static_cast<float>(g_App.cssH));
    io.DisplayFramebufferScale = ImVec2(
        static_cast<float>(g_App.fbW) / static_cast<float>(g_App.cssW),
        static_cast<float>(g_App.fbH) / static_cast<float>(g_App.cssH));

    ImGui::NewFrame();

    // Full-viewport host window containing the complete 8-tab ImGee Interactive Showcase
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 10.0f));

    const ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##ImGeeWasmPlaygroundHost", nullptr, hostFlags)) {
        ImGui::ShowImGeeDemoContent("##ImGeeWasmDemo");
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

    // Keep the outer HTML top navigation bar matched to Dark Studio or Paper Light
    const ImVec4 winBg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const float bgLum = 0.299f * winBg.x + 0.587f * winBg.y + 0.114f * winBg.z;
    const int isLightNow = (bgLum > 0.55f) ? 1 : 0;
    if (isLightNow != g_App.lastShellLt) {
        g_App.lastShellLt = isLightNow;
        js_sync_shell_theme(isLightNow);
    }

    ImGui::Render();

    glViewport(0, 0, g_App.fbW, g_App.fbH);
    glClearColor(winBg.x, winBg.y, winBg.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(g_App.window);

    if (!g_App.loaderHidden) {
        g_App.loaderHidden = true;
        js_hide_loader();
    }
}

} // namespace

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "[ImGee WASM] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Point working directory at preloaded sample files so Tab 8 FilePickerCombo / OpenFileDialog has real content
    (void)chdir("/assets/samples");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    double css_w_d = 1280.0;
    double css_h_d = 760.0;
    emscripten_get_element_css_size("#canvas", &css_w_d, &css_h_d);
    double dpr_d = emscripten_get_device_pixel_ratio();
    if (dpr_d < 1.0) dpr_d = 1.0;
    if (dpr_d > 3.0) dpr_d = 3.0;
    g_App.cssW = (std::max)(320, static_cast<int>(std::round(css_w_d)));
    g_App.cssH = (std::max)(240, static_cast<int>(std::round(css_h_d)));
    g_App.fbW  = (std::max)(320, static_cast<int>(std::round(css_w_d * dpr_d)));
    g_App.fbH  = (std::max)(240, static_cast<int>(std::round(css_h_d * dpr_d)));
    g_App.dpr  = static_cast<float>(dpr_d);

    const SDL_WindowFlags winFlags = static_cast<SDL_WindowFlags>(
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    g_App.window = SDL_CreateWindow(
        "ImGee Interactive Playground",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_App.cssW, g_App.cssH, winFlags);
    if (!g_App.window) {
        std::fprintf(stderr, "[ImGee WASM] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    emscripten_set_canvas_element_size("#canvas", g_App.fbW, g_App.fbH);

    g_App.glContext = SDL_GL_CreateContext(g_App.window);
    if (!g_App.glContext) {
        std::fprintf(stderr, "[ImGee WASM] SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(g_App.window, g_App.glContext);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    pio.Platform_SetClipboardTextFn = &WasmSetClipboardText;
    pio.Platform_GetClipboardTextFn = &WasmGetClipboardText;

    // Apply canonical Charcoal default before taking the Demo's host-style snapshot
    ImGui::ApplyDefaultTheme();

    // Calibrate default TimedTooltip timing for smooth 60fps browser interaction
    static ImGui::TimedTooltip::TooltipConfig s_WasmTooltipCfg = ImGui::TimedTooltip::GetConfig();
    s_WasmTooltipCfg.hover_delay_cycles  = 50;  // ~0.83s deliberate hover dwell at 60fps
    s_WasmTooltipCfg.max_display_cycles  = 900; // ~15s reading time before auto-dismiss
    s_WasmTooltipCfg.grace_period_cycles = 24;  // ~0.4s slip forgiveness
    ImGui::TimedTooltip::SetDefaultTooltipConfig(&s_WasmTooltipCfg);

    ImGui_ImplSDL2_InitForOpenGL(g_App.window, g_App.glContext);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    LoadCrispFonts(g_App.dpr);

    // Wire WebGL2 texture host backend and browser URL opener for Markdown links
    ImGee::HostBackend hostBackend;
    hostBackend.CreateTextureRGBA8 = &WasmCreateTextureRGBA8;
    hostBackend.DeleteTexture      = &WasmDeleteTexture;
    ImGee::SetHostBackend(hostBackend);
    ImGui::SetDefaultMarkdownLinkCallback(&WasmMarkdownLinkCallback);

    // Honor ?tab=1..8 and ?theme=dark|light query parameters on load
    const int initialTab   = js_get_initial_tab();
    const int initialTheme = js_get_initial_theme();
    if (initialTab >= 0) {
        ImGui::SetImGeeDemoTab(initialTab);
    }
    if (initialTheme >= 0) {
        ImGui::SetImGeeDemoTheme(initialTheme);
    }

    emscripten_set_main_loop(&MainLoopTick, 0, true);
    return 0;
}
