// ============================================================================
// ImGee Headless Smoke & Stack-Integrity Test Runner (tests/main_smoke.cpp)
// ============================================================================

#define IMGEE_ENABLE_GLOBAL_NAMESPACES
#include "imgee.h"
#include "imgui_internal.h"
#include "imgee_test_common.h"

#include <cstdint>
#include <fstream>
#include <string>

#if defined(IMGEE_HAS_IMGWINDOW)
#error "IMGEE_HAS_IMGWINDOW must not be defined in pure standalone builds"
#endif

#if IMGEE_FILEPATHS_FORCE_RELATIVE != 0
#error "IMGEE_FILEPATHS_FORCE_RELATIVE must default to 0 in pure standalone builds"
#endif

namespace {

// Valid 67-byte 1x1 RGBA red PNG
static const unsigned char kTinyRedPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
    0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
    0x54, 0x78, 0x9C, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
    0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x89, 0x99,
    0x3D, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

struct MockGpuState {
    uintptr_t nextHandle = 100;
    int createdCount = 0;
    int deletedCount = 0;
    int namedLookups = 0;
    int shutdownCalls = 0;
};

struct StackSnapshot {
    int colorSize;
    int styleVarSize;
    int fontSize;
    int groupSize;

    static StackSnapshot Capture() {
        ImGuiContext& g = *GImGui;
        StackSnapshot s;
        s.colorSize    = g.ColorStack.Size;
        s.styleVarSize = g.StyleVarStack.Size;
        s.fontSize     = g.FontStack.Size;
        s.groupSize    = g.GroupStack.Size;
        return s;
    }

    void VerifyBalanced(const char* sectionName) const {
        ImGuiContext& g = *GImGui;
        if (g.ColorStack.Size != colorSize ||
            g.StyleVarStack.Size != styleVarSize ||
            g.FontStack.Size != fontSize ||
            g.GroupStack.Size != groupSize) {
            std::fprintf(stderr,
                "[FAIL] Stack imbalance in section '%s': Color(%d->%d) StyleVar(%d->%d) Font(%d->%d) Group(%d->%d)\n",
                sectionName,
                colorSize, g.ColorStack.Size,
                styleVarSize, g.StyleVarStack.Size,
                fontSize, g.FontStack.Size,
                groupSize, g.GroupStack.Size);
            std::exit(1);
        }
    }
};

static void PrepareHeadlessFontAtlas(ImGuiIO& io) {
    io.Fonts->AddFontDefault();
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    IMGEE_TEST_CHECK(pixels != nullptr && width > 0 && height > 0);
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)1);
#endif
}

static void CompleteHeadlessRenderTextures() {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->Textures) {
        for (ImTextureData* tex : *drawData->Textures) {
            if (tex && tex->Status != ImTextureStatus_OK && tex->Status != ImTextureStatus_Destroyed) {
                if (tex->Status == ImTextureStatus_WantDestroy) {
                    tex->SetTexID(ImTextureID_Invalid);
                    tex->SetStatus(ImTextureStatus_Destroyed);
                } else {
                    tex->SetTexID((ImTextureID)(uintptr_t)(10 + tex->UniqueID));
                    tex->SetStatus(ImTextureStatus_OK);
                }
            }
        }
    }
#endif
}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    IMGEE_TEST_CHECK(ImGeeTests::VerifyHostileMacrosCompilation() == 42);
    IMGEE_TEST_CHECK(ImGeeTests::VerifyStandaloneOptOuts() == 7);

    // Verify isolated ImGeeStbi decodes our 1x1 PNG cleanly in memory
    {
        int w = 0, h = 0;
        unsigned char* decoded = ImGeeStbi::LoadFromMemory(kTinyRedPng, sizeof(kTinyRedPng), &w, &h);
        IMGEE_TEST_CHECK(decoded != nullptr && w == 1 && h == 1);
        IMGEE_TEST_CHECK(decoded[0] == 255 && decoded[1] == 0 && decoded[2] == 0 && decoded[3] == 255);
        ImGeeStbi::ImageFree(decoded);
    }

    // Write temporary PNG file to test MarkdownConfig::baseDir + HostBackend texture upload/teardown
    const std::string tempPngName = "imgee_ci_test_pixel.png";
    {
        std::ofstream ofs(tempPngName.c_str(), std::ios::binary | std::ios::trunc);
        IMGEE_TEST_CHECK(ofs.good());
        ofs.write(reinterpret_cast<const char*>(kTinyRedPng), static_cast<std::streamsize>(sizeof(kTinyRedPng)));
    }

    MockGpuState gpuState;
    ImGee::HostBackend backend;
    backend.UserData = &gpuState;
    backend.CreateTextureRGBA8 = [](const unsigned char* pixels, int width, int height, void* ud) -> ImTextureID {
        auto* st = static_cast<MockGpuState*>(ud);
        if (!pixels || width <= 0 || height <= 0) return (ImTextureID)0;
        st->createdCount++;
        return (ImTextureID)(st->nextHandle++);
    };
    backend.DeleteTexture = [](ImTextureID texId, void* ud) {
        auto* st = static_cast<MockGpuState*>(ud);
        if (texId != (ImTextureID)0) {
            st->deletedCount++;
        }
    };
    backend.ResolveNamedTexture = [](const char* name, ImVec2* outSize, void* ud) -> ImTextureID {
        auto* st = static_cast<MockGpuState*>(ud);
        if (name && std::string(name) == "test_logo") {
            st->namedLookups++;
            if (outSize) *outSize = ImVec2(16.0f, 16.0f);
            return (ImTextureID)(uintptr_t)77;
        }
        return (ImTextureID)0;
    };
    backend.GetScaleFactor = [](void*) -> float { return 1.0f; };
    ImGee::SetHostBackend(backend);
    ImGee::AddShutdownHandler([]() {});

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    IMGEE_TEST_CHECK(ctx != nullptr);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 800.0f);
    io.DeltaTime = 1.0f / 60.0f;
    PrepareHeadlessFontAtlas(io);

    // Verify all curated theme presets maintain balanced stacks and valid style colors
    ImGui::StyleColorsCharcoal();
    ImGui::StyleColorsRusty();
    ImGui::ApplyDefaultTheme();

    float sliderVal = 0.5f;
    int   intVal    = 42;
    int   comboIdx  = 1;
    bool  flagVal   = true;
    static const char* const kComboItems[] = { "Cockpit", "Chase", "Circle", "Tower" };
    static const char* const kComboTips[]  = { "**Cockpit**", "**Chase**", "**Circle**", "**Tower**" };

    ImGui::MarkdownConfig mdCfg;
    mdCfg.baseDir = ".";

    const std::string sampleMarkdown =
        "# Avionics Status Briefing\n"
        "## Flight Controls & Telemetry\n"
        "Configure **autopilot**, *yaw damper*, ~~legacy~~, ++active++, and `NAV1` modes.\n\n"
        "- Primary bus: <badge=success>ONLINE</badge> <pill=info>v1.0.0</pill>\n"
        "- Shortcut: Press <kbd>Ctrl+G</kbd> or click :gear: / `↺`.\n\n"
        "<center>Centered Flight-Deck Advisory</center>\n\n"
        "| Subsystem | Channel | Status |\n"
        "| :--- | :---: | ---: |\n"
        "| FMS-1 | Dual | Nominal |\n"
        "|XPDR | Mode-S | 1200 |\n\n"
        "```cpp\n"
        "ImGui::SliderWithReset(\"Gain\", &gain, 1.0f, 0.0f, 2.0f);\n"
        "```\n\n"
        "![Named](test_logo) ![Local](imgee_ci_test_pixel.png =16x16)\n";

    for (int frame = 0; frame < 3; ++frame) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
        io.AddMousePosEvent(120.0f, 120.0f);
#else
        io.MousePos = ImVec2(120.0f, 120.0f);
#endif
        ImGui::NewFrame();

        StackSnapshot rootSnap = StackSnapshot::Capture();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::SetNextWindowSize(ImVec2(900.0f, 700.0f));
        ImGui::Begin("ImGee Headless Smoke Window", nullptr, ImGuiWindowFlags_NoSavedSettings);

        {
            StackSnapshot s = StackSnapshot::Capture();
            MD::Text(sampleMarkdown, &mdCfg);
            (void)ImGui::ButtonMD("**Arm** :plane:", ImVec2(120.0f, 0.0f));
            s.VerifyBalanced("MarkdownExt & ButtonMD");
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
            IMGEE_TEST_CHECK(!GImGui->CurrentWindow->DC.IsSameLine);
#endif
        }

        {
            StackSnapshot s = StackSnapshot::Capture();
            ImGui::StyledText("Bus <b>A</b> <i>status</i>: <badge=primary>OK</badge> <icon gear /> <img name=\"test_logo\" width=12 height=12 />");
            ImGui::StyledTextWrapped("Wrapped <b>styled</b> telemetry <color=#44cc88>nominal</color>.");
            s.VerifyBalanced("Typography & StyledText");
        }

        {
            StackSnapshot s = StackSnapshot::Capture();
            ImGui::Icon("gear");
            ImGui::SameLine();
            (void)ImGui::IconButton("##cfg_btn", "gear", ImVec2(0, 0), "Configure");
            ImGui::SameLine();
            (void)ImGui::ButtonIcon("Reset", "undo", ImVec2(0, 0), false, "Restore baseline");
            ImGui::Badge("ACTIVE", ImGui::ImGuiBadgeVariant_Success);
            ImGui::SameLine();
            ImGui::Pill("GPS", ImGui::ImGuiBadgeVariant_Info);
            ImGui::SameLine();
            ImGui::Kbd("Alt+R");
            ImGui::Spinner("##spin");
            ImGui::RadialTimer("Hold", 0.65f);
            ImGui::CountdownTimer("Window", 3.5f, 10.0f);
            (void)ImGui::CopyButton("##copy_test", "N450GA");
            (void)ImGui::ButtonLink("Briefing");
            TimedTooltip::Text("Plain stationary tooltip.");
            ImGui::TextLink("Quick Reference");
            TimedTooltip::TextMD("Rich **Markdown** stationary tooltip.");
            s.VerifyBalanced("Core Widgets & TimedTooltip");
        }

        {
            StackSnapshot s = StackSnapshot::Capture();
            (void)ImGui::MouseWheelSlider("Gain", &sliderVal, 0.0f, 1.0f);
            (void)ImGui::PercentSlider("Dimmer", &sliderVal, 0.0f, 1.0f);
            (void)ImGui::SliderWithReset("Pitch", &sliderVal, 0.5f, 0.0f, 1.0f);
            (void)ImGui::SliderWithReset("Channel", &intVal, 42, 0, 100);
            (void)ImGui::DragWithReset("Trim", &sliderVal, 0.5f, 0.01f, -1.0f, 1.0f);
            (void)ImGui::MouseWheelCombo("Mode", &comboIdx, kComboItems, 4, -1, false, kComboTips);
            (void)ImGui::ComboWithReset("Preset", &comboIdx, 1, kComboItems, 4, -1, true, nullptr, kComboTips);
            (void)ImGui::MouseWheelCheckbox("Autothrottle", &flagVal);
            s.VerifyBalanced("MouseWheelControls & Reset-with-Undo");
        }

        {
            StackSnapshot s = StackSnapshot::Capture();
            ImGeeTests::RunReverseOrderKnobsFrame();
            s.VerifyBalanced("ReverseOrder Knobs");
        }

        {
            StackSnapshot s = StackSnapshot::Capture();
            ImGeeTests::RunReverseOrderTogglesAndPopupsFrame();
            s.VerifyBalanced("ReverseOrder Toggles & Popups");
        }

        ImGui::End();
        rootSnap.VerifyBalanced("Frame Root");

        ImGui::Render();
        CompleteHeadlessRenderTextures();
    }

    // Verify HostBackend callbacks actually resolved named textures and uploaded our disk PNG
    IMGEE_TEST_CHECK(gpuState.namedLookups >= 2);
    IMGEE_TEST_CHECK(gpuState.createdCount == 1);
    IMGEE_TEST_CHECK(gpuState.deletedCount == 0);

    // Verify ImGee::Shutdown() purges cached GPU textures via HostBackend::DeleteTexture
    ImGee::Shutdown();
    IMGEE_TEST_CHECK(gpuState.deletedCount == 1);

    ImGui::DestroyContext(ctx);
    std::remove(tempPngName.c_str());

    std::printf("[PASS] ImGee standalone smoke & stack-integrity suite succeeded (ImGui %s).\n", IMGUI_VERSION);
    return 0;
}
