// ============================================================================
// Standalone Markdown Callouts & Blockquotes without Widgets / Icons
// Verifies that extensions/Markdown/imgui_markdown_ext.h compiles and renders
// all GFM callouts and nested blockquotes cleanly when IMGEE_DISABLE_WIDGETS
// and IMGEE_DISABLE_ICONS are defined (exercising DrawMinimalCalloutGlyph).
// ============================================================================

#define IMGEE_DISABLE_WIDGETS
#define IMGEE_DISABLE_ICONS
#include "extensions/Markdown/imgui_markdown_ext.h"
#include "imgee_test_common.h"

#include <cstdint>
#include <string>

#if defined(CONTRIB_HAS_ICONS) || defined(CONTRIB_ICONS_AVAILABLE)
#error "Icons must not be active when IMGEE_DISABLE_WIDGETS and IMGEE_DISABLE_ICONS are defined"
#endif

#if defined(CONTRIB_HAS_BADGES)
#error "Badges must not be active when IMGEE_DISABLE_WIDGETS is defined"
#endif

int main() {
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* px = nullptr;
    int fw = 0, fh = 0;
    io.Fonts->GetTexDataAsRGBA32(&px, &fw, &fh);
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)1);
#endif

    const std::string doc =
        "> [!NOTE] Note fallback glyph\n\n"
        "> [!TIP] Tip fallback glyph\n\n"
        "> [!IMPORTANT] Important fallback glyph\n\n"
        "> [!WARNING] Warning fallback glyph\n\n"
        "> [!CAUTION]\n"
        "> Caution fallback glyph with nested table and code:\n"
        "> ```cpp\n"
        "> int ok = 1;\n"
        "> ```\n"
        "> | Col A | Col B |\n"
        "> | :--- | ---: |\n"
        "> | 1 | 2 |\n"
        "> > Nested quote inside caution\n";

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
    ImGui::SetNextWindowSize(ImVec2(640.0f, 520.0f));
    ImGui::Begin("NoWidgetsIconsMarkdownWin", nullptr, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Markdown::TextWrapped(doc);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    IMGEE_TEST_CHECK(dl != nullptr && dl->CmdBuffer.Size > 0 && dl->IdxBuffer.Size > 0);
    unsigned int expectedIdx = 0;
    for (int i = 0; i < dl->CmdBuffer.Size; ++i) {
        IMGEE_TEST_CHECK(dl->CmdBuffer[i].IdxOffset == expectedIdx);
        expectedIdx += dl->CmdBuffer[i].ElemCount;
    }
    IMGEE_TEST_CHECK(expectedIdx == static_cast<unsigned int>(dl->IdxBuffer.Size));

    ImGui::End();
    ImGui::EndFrame();

    ImGui::DestroyContext(ctx);
    std::printf("[PASS] IMGEE_DISABLE_WIDGETS / IMGEE_DISABLE_ICONS standalone callout test succeeded.\n");
    return 0;
}
