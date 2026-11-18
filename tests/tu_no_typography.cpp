// ============================================================================
// IMGEE_DISABLE_TYPOGRAPHY Base-Font Fallback Verification
// Verifies that TimedTooltip compiles cleanly without Typography and still
// isolates tooltip fonts via io.FontDefault -> io.Fonts->Fonts[0] when
// io.FontDefault == nullptr.
// ============================================================================

#define IMGEE_DISABLE_TYPOGRAPHY
#include "extensions/TimedTooltip/imgui_tooltip_ext.h"
#include "imgee_test_common.h"

#if defined(IMGUI_FONT_ROLES_H)
#error "IMGEE_DISABLE_TYPOGRAPHY failed to exclude imgui_font_roles.h"
#endif

#if defined(IMGUI_TIMED_TOOLTIP_TYPOGRAPHY_BRIDGE_DEFINED)
#error "Typography bridge must not be defined when Typography is disabled"
#endif

int main() {
    IMGEE_TEST_CHECK(ImGui::TimedTooltip::Detail::GetBaseFontResolver() == nullptr);

    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(640.0f, 480.0f);
    io.DeltaTime = 1.0f / 60.0f;

    ImFontConfig regCfg;
    regCfg.SizePixels = 16.0f;
    ImFont* reg16 = io.Fonts->AddFontDefault(&regCfg);

    ImFontConfig smallCfg;
    smallCfg.SizePixels = 9.0f;
    ImFont* small9 = io.Fonts->AddFontDefault(&smallCfg);

    // Leave io.FontDefault null (X-Plane plugin pattern)
    io.FontDefault = nullptr;

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* px = nullptr;
    int fw = 0, fh = 0;
    io.Fonts->GetTexDataAsRGBA32(&px, &fw, &fh);
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)1);
#endif

    ImGui::TimedTooltip::TooltipConfig cfg = ImGui::TimedTooltip::GetConfig();
    cfg.hover_delay_cycles = 1;

    ImGui::NewFrame();
    ImGui::Begin("NoTypographyFallbackWin");

    // Verify fallback selects Fonts[0] (reg16) even inside PushFont(small9) when io.FontDefault == nullptr
    ImGui::ImGeeCompat::PushFont(small9);
    IMGEE_TEST_CHECK(ImGui::TimedTooltip::Detail::ResolveBaseTooltipFont(&cfg) == reg16);
    bool opened = ImGui::TimedTooltip::BeginStationaryTooltipProxy(0x880001u, true, &cfg);
    IMGEE_TEST_CHECK(opened);
    if (opened) {
        IMGEE_TEST_CHECK(ImGui::GetFont() == reg16);
        ImGui::TimedTooltip::EndStationaryTooltip(0x880001u);
    }
    ImGui::PopFont();

    // Verify io.FontDefault takes priority over Fonts[0] when explicitly assigned
    io.FontDefault = small9;
    IMGEE_TEST_CHECK(ImGui::TimedTooltip::Detail::ResolveBaseTooltipFont(&cfg) == small9);
    io.FontDefault = nullptr;

    ImGui::End();
    ImGui::EndFrame();

    ImGui::DestroyContext(ctx);
    std::printf("[PASS] IMGEE_DISABLE_TYPOGRAPHY base-font fallback test succeeded.\n");
    return 0;
}
