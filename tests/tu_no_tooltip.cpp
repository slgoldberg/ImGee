// ============================================================================
// IMGEE_DISABLE_TIMED_TOOLTIP Silent-Fallback Verification (UX Rule #6)
// Verifies that when TimedTooltip is omitted, interactive Markdown tables
// compile cleanly, default to silent (never calling stock ImGui::SetTooltip),
// and still honor custom MarkdownConfig::tableTooltipCallback hooks.
// ============================================================================

#define IMGEE_DISABLE_TIMED_TOOLTIP
#include "imgee.h"
#include "imgui_internal.h"
#include "imgee_test_common.h"

#include <string>

#if defined(IMGUI_TOOLTIP_EXT_H)
#error "IMGEE_DISABLE_TIMED_TOOLTIP failed to exclude imgui_tooltip_ext.h"
#endif

#if defined(IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED)
#error "IMGUI_TIMED_TOOLTIP_MD_BRIDGE_DEFINED must not be defined when TimedTooltip is disabled"
#endif

#if defined(IMGUI_TIMED_TOOLTIP_TYPOGRAPHY_BRIDGE_DEFINED)
#error "IMGUI_TIMED_TOOLTIP_TYPOGRAPHY_BRIDGE_DEFINED must not be defined when TimedTooltip is disabled"
#endif

int main() {
    IMGEE_TEST_CHECK(ImGui::MarkdownDetail::GetTableTooltipFn() == nullptr);

    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(640.0f, 480.0f);
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

    const std::string tableMd =
        "| Fuel % | Status |\n"
        "| :--- | ---: |\n"
        "| 75% | OK |\n";

    // 1. Silent fallback check: hovering header without TimedTooltip and without
    //    custom callback must NOT spawn stock instant-pop "##Tooltip_00"!
    for (int f = 0; f < 2; ++f) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
        io.AddMousePosEvent(35.0f, 22.0f);
#else
        io.MousePos = ImVec2(35.0f, 22.0f);
#endif
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
        ImGui::Begin("SilentFallbackWin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
        ImGui::Markdown::Text(tableMd);
        ImGui::End();
        ImGui::EndFrame();
    }
    IMGEE_TEST_CHECK(ImGui::FindWindowByName("##Tooltip_00") == nullptr);

    // 2. Custom host callback check: when host supplies tableTooltipCallback, it
    //    receives dynamic tristate strings cleanly even without TimedTooltip!
    int callbackCount = 0;
    bool sawFuelSort = false;
    struct CbCtx { int* count; bool* sawFuel; } cbCtx{ &callbackCount, &sawFuelSort };

    ImGui::MarkdownConfig customCfg;
    customCfg.userData = &cbCtx;
    customCfg.tableTooltipCallback = [](ImGuiID, bool, const char* mdText, void* ud) {
        auto* c = static_cast<CbCtx*>(ud);
        (*c->count)++;
        if (mdText && std::string(mdText) == "Sort table by **Fuel %** (A to Z)") {
            *c->sawFuel = true;
        }
    };

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
    ImGui::Begin("SilentFallbackWin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Markdown::Text(tableMd, &customCfg);
    ImGui::End();
    ImGui::EndFrame();

    IMGEE_TEST_CHECK(callbackCount > 0 && sawFuelSort);

    ImGui::DestroyContext(ctx);
    std::printf("[PASS] IMGEE_DISABLE_TIMED_TOOLTIP silent-fallback test succeeded.\n");
    return 0;
}
