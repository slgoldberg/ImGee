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
#include <vector>

#if defined(IMGEE_HAS_IMGWINDOW)
#error "IMGEE_HAS_IMGWINDOW must not be defined in pure standalone builds"
#endif

#if !defined(IMGUI_TIMED_TOOLTIP_TYPOGRAPHY_BRIDGE_DEFINED)
#error "Forward include order (TimedTooltip BEFORE Typography in imgee.h) failed to activate TimedTooltip <-> Typography bridge"
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

struct AtlasFonts {
    ImFont* regular16  = nullptr;
    ImFont* bold16     = nullptr;
    ImFont* small9     = nullptr;
    ImFont* baseline14 = nullptr;
    ImFont* compact12  = nullptr;
};

static AtlasFonts PrepareHeadlessFontAtlas(ImGuiIO& io) {
    AtlasFonts fonts;
    ImFontConfig regCfg;
    regCfg.SizePixels = 16.0f;
    fonts.regular16 = io.Fonts->AddFontDefault(&regCfg);

    ImFontConfig boldCfg;
    boldCfg.SizePixels = 16.0f;
    fonts.bold16 = io.Fonts->AddFontDefault(&boldCfg);

    ImFontConfig smallCfg;
    smallCfg.SizePixels = 9.0f;
    fonts.small9 = io.Fonts->AddFontDefault(&smallCfg);

    ImFontConfig base14Cfg;
    base14Cfg.SizePixels = 14.0f;
    fonts.baseline14 = io.Fonts->AddFontDefault(&base14Cfg);

    ImFontConfig comp12Cfg;
    comp12Cfg.SizePixels = 12.0f;
    fonts.compact12 = io.Fonts->AddFontDefault(&comp12Cfg);

    // Intentionally leave io.FontDefault == nullptr to verify X-Plane / ImgWindow fallback behavior
    io.FontDefault = nullptr;

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
    return fonts;
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
    AtlasFonts atlasFonts = PrepareHeadlessFontAtlas(io);
    IMGEE_TEST_CHECK(atlasFonts.regular16 != nullptr && atlasFonts.small9 != nullptr);
    IMGEE_TEST_CHECK(io.FontDefault == nullptr);
    IMGEE_TEST_CHECK(ImGui::TimedTooltip::Detail::GetBaseFontResolver() != nullptr);

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

    // -------------------------------------------------------------------------
    // Wave 1A Test 1: TimedTooltip Base-Font Isolation & perfect_size Parity
    // -------------------------------------------------------------------------
    {
        TimedTooltip::TooltipConfig instantCfg = TimedTooltip::GetConfig();
        instantCfg.hover_delay_cycles = 1;

        ImGuiID idTxtBase   = 0x77000101u;
        ImGuiID idTxtSmall  = 0x77000102u;
        ImGuiID idMdBase    = 0x77000103u;
        ImGuiID idMdSmall   = 0x77000104u;
        ImGuiID idProxyOpen = 0x77000105u;

        auto runSingleTooltipFrame = [&](int mode) {
            ImGui::NewFrame();
            StackSnapshot rootSnap = StackSnapshot::Capture();
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f));
            ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f));
            ImGui::Begin("FontIsolationTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);

            IMGEE_TEST_CHECK(ImGui::GetFont() == atlasFonts.regular16);
            if (mode == 0) {
                (void)TimedTooltip::TextProxy(idTxtBase, true, &instantCfg, "%s",
                    "Stationary base-font isolation check across multiple words for wrapping parity.");
            } else if (mode == 1) {
                (void)TimedTooltip::TextMDProxy(idMdBase, true, &instantCfg, "%s",
                    "Stationary **Markdown** *inline* `code` isolation check across words.");
            } else {
                // Enter a compact caller PushFont(small9) scope (simulating a footer toolbar)
                StackSnapshot smallSnap = StackSnapshot::Capture();
                ImGui::ImGeeCompat::PushFont(atlasFonts.small9);
                IMGEE_TEST_CHECK(ImGui::GetFont() == atlasFonts.small9);
                IMGEE_TEST_CHECK(ImGui::GetFontSize() < 12.0f);

                if (mode == 2) {
                    (void)TimedTooltip::TextProxy(idTxtSmall, true, &instantCfg, "%s",
                        "Stationary base-font isolation check across multiple words for wrapping parity.");
                } else if (mode == 3) {
                    (void)TimedTooltip::TextMDProxy(idMdSmall, true, &instantCfg, "%s",
                        "Stationary **Markdown** *inline* `code` isolation check across words.");
                } else if (mode == 4) {
                    bool opened = TimedTooltip::BeginStationaryTooltipProxy(idProxyOpen, true, &instantCfg);
                    IMGEE_TEST_CHECK(opened);
                    if (opened) {
                        // Inside the tooltip window, the active font MUST be isolated to regular16, NOT small9!
                        IMGEE_TEST_CHECK(ImGui::GetFont() == atlasFonts.regular16);
                        IMGEE_TEST_CHECK(ImGui::GetFontSize() >= 15.0f);
                        ImGui::TextUnformatted("Isolated body copy");
                        TimedTooltip::EndStationaryTooltip(idProxyOpen);
                    }
                }

                // Caller's small9 font must still be active on the caller window before PopFont!
                IMGEE_TEST_CHECK(ImGui::GetFont() == atlasFonts.small9);
                ImGui::PopFont();
                smallSnap.VerifyBalanced("TimedTooltip Base-Font Isolation");
            }

            TimedTooltip::ClearActiveTooltip();
            ImGui::End();
            rootSnap.VerifyBalanced("FontIsolationTestWindow Root");
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        for (int mode = 0; mode < 5; ++mode) {
            runSingleTooltipFrame(mode);
        }

        const auto& stateMap = TimedTooltip::GetTooltipStateMap();
        const auto& stTxtBase  = stateMap.at(idTxtBase);
        const auto& stTxtSmall = stateMap.at(idTxtSmall);
        const auto& stMdBase   = stateMap.at(idMdBase);
        const auto& stMdSmall  = stateMap.at(idMdSmall);

        IMGEE_TEST_CHECK(stTxtBase.perfect_size.x > 0.0f && stTxtBase.perfect_size.y > 0.0f);
        IMGEE_TEST_CHECK(stTxtBase.perfect_size.x == stTxtSmall.perfect_size.x);
        IMGEE_TEST_CHECK(stTxtBase.perfect_size.y == stTxtSmall.perfect_size.y);

        IMGEE_TEST_CHECK(stMdBase.perfect_size.x > 0.0f && stMdBase.perfect_size.y > 0.0f);
        IMGEE_TEST_CHECK(stMdBase.perfect_size.x == stMdSmall.perfect_size.x);
        IMGEE_TEST_CHECK(stMdBase.perfect_size.y == stMdSmall.perfect_size.y);
    }

    // -------------------------------------------------------------------------
    // Wave 1A Test 2: Interactive Markdown Table Tooltips (% Safety, Tristate
    // Sort Cycle, Resize Grips, and <!-- table-preset: ... --> Overrides)
    // -------------------------------------------------------------------------
    {
        struct CapturedTableTips {
            std::vector<std::string> texts;
        } captured;

        const std::string pctTableMd =
            "| Fuel % | Thrust % |\n"
            "| :--- | ---: |\n"
            "| 45% | 92% |\n"
            "| 80% | 65% |\n";

        const std::string suppressedTableMd =
            "| Bus | Volts |\n"
            "| :--- | ---: |\n"
            "| ESS | 28.0V |\n"
            "<!-- table-preset: tooltips=off -->\n";

        const std::string customTipsTableMd =
            "| ICAO | Alt | Mode |\n"
            "| :--- | ---: | :---: |\n"
            "| KSEA | 433 | ILS |\n"
            "<!-- table-preset: col-tooltips=\"Custom **ICAO %s** tip||off\" tip1=\"Custom **Alt** tip\" -->\n";

        auto containsTip = [&](const std::string& needle) -> bool {
            for (const std::string& s : captured.texts) {
                if (s == needle) return true;
            }
            return false;
        };

        ImGui::MarkdownConfig tblCfg;
        tblCfg.userData = &captured;
        tblCfg.tableTooltipCallback = [](ImGuiID, bool, const char* mdText, void* ud) {
            auto* out = static_cast<CapturedTableTips*>(ud);
            if (mdText) out->texts.emplace_back(mdText);
        };

        // Pass A: Verify unsorted tristate header copy, divider grip copy, right-edge grip copy,
        // per-table tooltips=off suppression, and per-column custom/suppressed overrides.
        ImVec2 header0Center(0.0f, 0.0f);
        for (int pass = 0; pass < 2; ++pass) {
            captured.texts.clear();
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddMousePosEvent(50.0f, 45.0f);
            io.AddMouseButtonEvent(0, false);
#else
            io.MousePos = ImVec2(50.0f, 45.0f);
            io.MouseDown[0] = false;
#endif
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
            ImGui::SetNextWindowSize(ImVec2(600.0f, 500.0f));
            ImGui::Begin("TableTooltipsTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
            ImVec2 tableTopLeft = ImGui::GetCursorScreenPos();
            header0Center = ImVec2(tableTopLeft.x + 30.0f, tableTopLeft.y + 12.0f);
            MD::Text(pctTableMd, &tblCfg);

            size_t countAfterPct = captured.texts.size();
            MD::Text(suppressedTableMd, &tblCfg);
            // tooltips=off must emit zero tooltip callbacks!
            IMGEE_TEST_CHECK(captured.texts.size() == countAfterPct);

            size_t countBeforeCustom = captured.texts.size();
            MD::Text(customTipsTableMd, &tblCfg);
            IMGEE_TEST_CHECK(captured.texts.size() > countBeforeCustom);

            // Also render pctTableMd through default TimedTooltip::TextMDProxy (tableTooltipCallback = nullptr)
            // while hovered to prove literal '%' characters never misformat or crash vsnprintf!
            ImGui::MarkdownConfig bridgeCfg;
            MD::Text("| N1 %s %d %% | EGT % |\n| :--- | ---: |\n| 98% | 720C |\n", &bridgeCfg);

            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        }

        IMGEE_TEST_CHECK(containsTip("Sort table by **Fuel %** (A to Z)"));
        IMGEE_TEST_CHECK(containsTip("Sort table by **Thrust %** (A to Z)"));
        IMGEE_TEST_CHECK(containsTip("Drag to resize **Fuel %** column\nDouble-click to auto-fit"));
        IMGEE_TEST_CHECK(containsTip("Drag to resize table width\nDouble-click after resizing to toggle **auto-fit layout**"));
        IMGEE_TEST_CHECK(containsTip("Custom **ICAO %s** tip"));
        IMGEE_TEST_CHECK(containsTip("Custom **Alt** tip"));
        IMGEE_TEST_CHECK(!containsTip("Sort table by **Mode** (A to Z)")); // Suppressed via "off" in col-tooltips!

        // Pass B: Click header 0 three times to verify full tristate sort cycle:
        // Click 1 -> Ascending -> next tip is Descending ("Z to A")
        // Click 2 -> Descending -> next tip is Clear sort ("restore default order")
        // Click 3 -> Unsorted -> next tip is Ascending ("A to Z")
        const char* expectedCycle[3] = {
            "Sort **Fuel %** in descending order (Z to A)",
            "Clear sort on **Fuel %** (restore default order)",
            "Sort table by **Fuel %** (A to Z)"
        };

        for (int step = 0; step < 3; ++step) {
            // Frame 1: Mouse down on header 0
            // Frame 2: Mouse release on header 0 (triggers ButtonBehavior pressed!)
            // Frame 3: Hover frame to inspect updated dynamic tooltip string
            for (int sub = 0; sub < 3; ++sub) {
                captured.texts.clear();
                const bool down = (sub == 0);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
                io.AddMousePosEvent(header0Center.x, header0Center.y);
                io.AddMouseButtonEvent(0, down);
#else
                io.MousePos = header0Center;
                io.MouseDown[0] = down;
#endif
                ImGui::NewFrame();
                ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
                ImGui::SetNextWindowSize(ImVec2(600.0f, 500.0f));
                ImGui::Begin("TableTooltipsTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
                MD::Text(pctTableMd, &tblCfg);
                ImGui::End();
                ImGui::Render();
                CompleteHeadlessRenderTextures();
            }
            IMGEE_TEST_CHECK(containsTip(expectedCycle[step]));
        }
    }

    // -------------------------------------------------------------------------
    // Wave 1B Test 3: Native Blockquotes & GFM Callout Cards (Fence-Awareness,
    // Host Override Precedence, CalcMarkdownSize, and Nesting Torture Test)
    // -------------------------------------------------------------------------
    {
        // 1. Direct preprocessor fence & indented-code safety check
        const std::string rawFenceDoc =
            "> [!NOTE]\n"
            "> Multi-line note body\n\n"
            "> [!INFO] Same-line info alias\n\n"
            "```cpp\n"
            "> [!WARNING] Inside fenced block must remain literal!\n"
            "> Plain quote inside fence\n"
            "```\n\n"
            "    > [!TIP] Four-space indented code line must remain literal!\n\n"
            "> Standard quote line 1\n"
            ">\n"
            ">tight continuation line 2\n";

        std::string preOut;
        bool preChanged = ImGui::PreprocessMarkdownBlockquotesAndAlerts(rawFenceDoc.c_str(), rawFenceDoc.size(), preOut);
        IMGEE_TEST_CHECK(preChanged);
        IMGEE_TEST_CHECK(preOut.find("<callout type=note>") != std::string::npos);
        IMGEE_TEST_CHECK(preOut.find("Same-line info alias") != std::string::npos);
        IMGEE_TEST_CHECK(preOut.find("<blockquote>\nStandard quote line 1\n\ntight continuation line 2\n</blockquote>") != std::string::npos);
        IMGEE_TEST_CHECK(preOut.find("```cpp\n> [!WARNING] Inside fenced block must remain literal!\n> Plain quote inside fence\n```") != std::string::npos);
        IMGEE_TEST_CHECK(preOut.find("    > [!TIP] Four-space indented code line must remain literal!") != std::string::npos);

        // 2. Host RegisterMarkdownTag("callout", ...) override precedence check
        auto savedCalloutHandler = ImGui::GetCustomMarkdownTags()["callout"];
        int customCalloutHits = 0;
        ImGui::RegisterMarkdownTag("callout", [&customCalloutHits](const std::string& inner, const std::string&) {
            customCalloutHits++;
            ImGui::TextUnformatted(inner.c_str());
        });

        ImGui::NewFrame();
        ImGui::Begin("CalloutOverrideTestWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        MD::Text(rawFenceDoc);
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        // Exactly the 2 real alerts ([!NOTE] and [!INFO]) must invoke the custom handler;
        // the fenced [!WARNING] and 4-space indented [!TIP] must NOT!
        IMGEE_TEST_CHECK(customCalloutHits == 2);

        // Restore built-in default callout renderer
        ImGui::RegisterMarkdownTag("callout", savedCalloutHandler);

        // 3. Nesting Torture Test: Callouts & blockquotes containing fenced code (including literal
        //    "</callout>" inside the fence!), interactive GFM tables, and 3-level nested '> > >' quotes!
        const std::string tortureMd =
            "> [!NOTE] Quick preflight advisory.\n\n"
            "> [!TIP]\n"
            "> Multi-line operational tip with **bold** and `code`.\n\n"
            "> [!WARNING] Crosswind exceeds 25 kt.\n\n"
            "> [!CAUTION]\n"
            "> Do not arm autothrottle on contaminated runway.\n\n"
            "<callout type=\"warning\" title=\"Engine Anti-Ice Required\">\n"
            "Explicit XML callout with custom title.\n"
            "</callout>\n\n"
            "<blockquote>\n"
            "Outer explicit blockquote.\n"
            "<blockquote>Inner nested explicit blockquote.</blockquote>\n"
            "Trailing outer blockquote text.\n"
            "</blockquote>\n\n"
            "> [!IMPORTANT] Integrated Avionics Torture Card\n"
            "> Verify nested fenced code, interactive GFM table, and deep quotes inside one callout:\n"
            ">\n"
            "> ```cpp\n"
            "> // Literal closing tag </callout> inside code fence must NOT break outer card!\n"
            "> if (bus_volts > 24.0f) { arm_avionics(); }\n"
            "> ```\n"
            ">\n"
            "> | Bus | Voltage | Load % |\n"
            "> | :--- | ---: | ---: |\n"
            "> | MAIN-1 | 28.1V | 62% |\n"
            "> | ESS-A | 27.9V | 41% |\n"
            ">\n"
            "> > Second-level quote inside callout\n"
            "> > > Third-level nested quote\n"
            "> > [!TIP] Nested GFM callout inside quote inside callout!\n";

        // Verify CalcMarkdownSize & TimedTooltip::TextMD shrink-wrapping on callouts
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::SetNextWindowSize(ImVec2(760.0f, 700.0f));
        ImGui::Begin("TortureCalloutWin", nullptr, ImGuiWindowFlags_NoSavedSettings);

        StackSnapshot snap = StackSnapshot::Capture();
        ImVec2 measuredTorture = MD::CalcMarkdownSize(tortureMd, 520.0f);
        IMGEE_TEST_CHECK(measuredTorture.x > 100.0f && measuredTorture.x <= 540.0f);
        IMGEE_TEST_CHECK(measuredTorture.y > 150.0f);

        MD::TextWrapped(tortureMd);

        TimedTooltip::TooltipConfig ttCfg = TimedTooltip::GetConfig();
        ttCfg.hover_delay_cycles = 1;
        ImGuiID ttCalloutId = 0x77009901u;
        (void)TimedTooltip::TextMDProxy(ttCalloutId, true, &ttCfg, "%s", "> [!TIP] Short callout");
        TimedTooltip::ClearActiveTooltip();

        // Verify short callout in TimedTooltip shrink-wrapped cleanly rather than expanding to 99999px!
        const auto& ttCalloutState = TimedTooltip::GetTooltipStateMap().at(ttCalloutId);
        IMGEE_TEST_CHECK(ttCalloutState.perfect_size.x > 40.0f && ttCalloutState.perfect_size.x < 340.0f);

        snap.VerifyBalanced("Wave 1B Blockquote & Callout Torture Render");

        // Verify ImDrawList CmdBuffer sequential IdxOffset integrity after nested underlay rotations!
        ImDrawList* dl = ImGui::GetWindowDrawList();
        IMGEE_TEST_CHECK(dl != nullptr && dl->CmdBuffer.Size > 0);
        unsigned int expectedIdxOffset = 0;
        for (int ci = 0; ci < dl->CmdBuffer.Size; ++ci) {
            IMGEE_TEST_CHECK(dl->CmdBuffer[ci].IdxOffset == expectedIdxOffset);
            expectedIdxOffset += dl->CmdBuffer[ci].ElemCount;
        }
        IMGEE_TEST_CHECK(expectedIdxOffset == static_cast<unsigned int>(dl->IdxBuffer.Size));

        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();
    }

    // -------------------------------------------------------------------------
    // Wave 1 Peer-Review Regression Suite (Multi-Table grip_id Isolation,
    // FontWindowScale 1.5x Zoom Parity & Cache Invalidation, and Fence/Backtick-
    // Aware Raw Container Scanning)
    // -------------------------------------------------------------------------
    {
        // 1. Multi-table right-edge grip_id isolation in the same window:
        //    Hovering Table 1's right-edge resize grip across hover_delay_cycles
        //    must display Table 1's tooltip without Table 2 resetting hover_cycles!
        const std::string twoTablesMd =
            "| Sys A | Val A |\n"
            "| :--- | ---: |\n"
            "| N1 | 94% |\n"
            "<!-- table-preset: width=200 -->\n\n"
            "| Sys B | Val B |\n"
            "| :--- | ---: |\n"
            "| EGT | 680C |\n"
            "<!-- table-preset: width=280 -->\n";

        TimedTooltip::ClearActiveTooltip();
        ImVec2 table1GripPos(0.0f, 0.0f);

        // Warm-up layout pass to resolve exact OuterRect.Max.x of Table 1:
        std::vector<ImGuiID> gripIdsSeen;
        {
            ImGui::MarkdownConfig idProbeCfg;
            idProbeCfg.userData = &gripIdsSeen;
            idProbeCfg.tableTooltipCallback = [](ImGuiID id, bool, const char* txt, void* ud) {
                if (txt && std::string(txt).find("Drag to resize table width") == 0) {
                    static_cast<std::vector<ImGuiID>*>(ud)->push_back(id);
                }
            };
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
            ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f));
            ImGui::Begin("TwoTableGripWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
            ImVec2 startCursor = ImGui::GetCursorScreenPos();
            table1GripPos = ImVec2(startCursor.x + 200.0f * ImGui::GetVisualScale(), startCursor.y + 14.0f);
            MD::Text(twoTablesMd, &idProbeCfg);
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        }
        IMGEE_TEST_CHECK(gripIdsSeen.size() == 2 && gripIdsSeen[0] != gripIdsSeen[1]);

        const ImGuiID table1GripId = gripIdsSeen[0];
        const ImGuiID table2GripId = gripIdsSeen[1];
        const int delayFrames = TimedTooltip::GetCurrentTooltipConfig().hover_delay_cycles;

        ImGuiID activeIdInsideWin = 0;
        for (int f = 0; f < delayFrames + 3; ++f) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddMousePosEvent(table1GripPos.x, table1GripPos.y);
            io.AddMouseButtonEvent(0, false);
#else
            io.MousePos = table1GripPos;
            io.MouseDown[0] = false;
#endif
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
            ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f));
            ImGui::Begin("TwoTableGripWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
            MD::Text(twoTablesMd); // Uses live TimedTooltip bridge
            activeIdInsideWin = TimedTooltip::GetActiveTooltipID();
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        }

        const auto& ttMapAfterTables = TimedTooltip::GetTooltipStateMap();
        IMGEE_TEST_CHECK(activeIdInsideWin == table1GripId);
        IMGEE_TEST_CHECK(ttMapAfterTables.at(table1GripId).is_displayed);
        IMGEE_TEST_CHECK(ttMapAfterTables.at(table1GripId).hover_cycles >= delayFrames);
        IMGEE_TEST_CHECK(!ttMapAfterTables.at(table2GripId).is_displayed);
        TimedTooltip::ClearActiveTooltip();

        // 2. FontWindowScale 1.5x zoom check (no zoom^2!) & perfect_size zoom invalidation:
        ImGui::SetFontRole(ImGui::ImFontRole::Regular, atlasFonts.baseline14);
        ImGui::SetFontRole(ImGui::ImFontRole::Bold, atlasFonts.baseline14);

        TimedTooltip::TooltipConfig linearZoomCfg = TimedTooltip::GetConfig();
        linearZoomCfg.font_scale_policy = TimedTooltip::TooltipFontScale_LinearInherit;
        linearZoomCfg.hover_delay_cycles = 1;

        const ImGuiID idZoomTxt = 0x77110001u;
        const ImGuiID idZoomMd  = 0x77110002u;
        const ImGuiID idZoomRaw = 0x77110003u;

        // Pass A: Measure at 1.0x window zoom
        ImGui::NewFrame();
        ImGui::Begin("ZoomScaleRegressionWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ImGeeCompat::PushFont(atlasFonts.baseline14);
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        IMGEE_TEST_CHECK(std::abs(TimedTooltip::GetActiveFontScale(&linearZoomCfg) - 1.0f) < 0.001f);

        (void)TimedTooltip::TextProxy(idZoomTxt, true, &linearZoomCfg, "%s", "Zoomed stationary tooltip measurement.");
        TimedTooltip::ClearActiveTooltip();
        ImGui::PopFont();
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        ImGui::NewFrame();
        ImGui::Begin("ZoomScaleRegressionWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ImGeeCompat::PushFont(atlasFonts.baseline14);
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        (void)TimedTooltip::TextMDProxy(idZoomMd, true, &linearZoomCfg, "%s", "Zoomed **Markdown** stationary tooltip.");
        TimedTooltip::ClearActiveTooltip();
        (void)TimedTooltip::BeginStationaryTooltipProxy(idZoomRaw, false, &linearZoomCfg);
        TimedTooltip::GetTooltipStateMap()[idZoomRaw].perfect_size = ImVec2(120.0f, 24.0f);
        ImGui::PopFont();
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        const ImVec2 szTxt1x = TimedTooltip::GetTooltipStateMap().at(idZoomTxt).perfect_size;
        const ImVec2 szMd1x  = TimedTooltip::GetTooltipStateMap().at(idZoomMd).perfect_size;
        IMGEE_TEST_CHECK(szTxt1x.x > 0.0f && szMd1x.x > 0.0f);

        // Pass B: Zoom window to 1.5x -- verify scale is 1.5f (NOT 2.25f!) both with baseline14
        //         and inside a caller PushFont(compact12) scope, and verify perfect_size re-measures!
        ImGui::NewFrame();
        ImGui::Begin("ZoomScaleRegressionWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ImGeeCompat::PushFont(atlasFonts.baseline14);
        ImGui::ImGeeCompat::SetWindowFontScale(1.5f);

        const float scaleAt15x = TimedTooltip::GetActiveFontScale(&linearZoomCfg);
        IMGEE_TEST_CHECK(std::abs(scaleAt15x - 1.5f) < 0.001f);

        ImGui::ImGeeCompat::PushFont(atlasFonts.compact12);
        const float scaleAt15xInCompact12 = TimedTooltip::GetActiveFontScale(&linearZoomCfg);
        IMGEE_TEST_CHECK(std::abs(scaleAt15xInCompact12 - 1.5f) < 0.001f);
        (void)TimedTooltip::TextProxy(idZoomTxt, true, &linearZoomCfg, "%s", "Zoomed stationary tooltip measurement.");
        TimedTooltip::ClearActiveTooltip();
        ImGui::PopFont();

        // Direct BeginStationaryTooltipProxy call must also invalidate stale perfect_size via prev_frame_updated < cur_frame:
        (void)TimedTooltip::BeginStationaryTooltipProxy(idZoomRaw, false, &linearZoomCfg);
        IMGEE_TEST_CHECK(TimedTooltip::GetTooltipStateMap().at(idZoomRaw).perfect_size.x == 0.0f);

        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        ImGui::PopFont();
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        ImGui::NewFrame();
        ImGui::Begin("ZoomScaleRegressionWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ImGeeCompat::PushFont(atlasFonts.baseline14);
        ImGui::ImGeeCompat::SetWindowFontScale(1.5f);
        (void)TimedTooltip::TextMDProxy(idZoomMd, true, &linearZoomCfg, "%s", "Zoomed **Markdown** stationary tooltip.");
        TimedTooltip::ClearActiveTooltip();
        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        ImGui::PopFont();
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        const ImVec2 szTxt15x = TimedTooltip::GetTooltipStateMap().at(idZoomTxt).perfect_size;
        const ImVec2 szMd15x  = TimedTooltip::GetTooltipStateMap().at(idZoomMd).perfect_size;
        IMGEE_TEST_CHECK(szTxt15x.x > szTxt1x.x * 1.35f && szTxt15x.y > szTxt1x.y * 1.30f);
        IMGEE_TEST_CHECK(szMd15x.x  > szMd1x.x  * 1.35f && szMd15x.y  > szMd1x.y  * 1.30f);

        ImGui::ClearFontRoles();

        // 3. Explicit <callout> containing inline `</callout>` and fenced `</callout>` + `> code`:
        const std::string rawContainerEdgeDoc =
            "<callout type=note>Opening line with inline `</callout>` code span\n"
            "Body line mentioning `` `</callout>` `` inline.\n"
            "```html\n"
            "</callout>\n"
            "> literal code line starting with gt inside fence\n"
            "```\n"
            "> Inner quote still inside explicit callout\n"
            "</callout>\n\n"
            "> Top-level quote after explicit callout\n";

        std::string rawContainerOut;
        bool rawContainerChanged = ImGui::PreprocessMarkdownBlockquotesAndAlerts(
            rawContainerEdgeDoc.c_str(), rawContainerEdgeDoc.size(), rawContainerOut);
        IMGEE_TEST_CHECK(rawContainerChanged);
        // The fenced '> literal code line...' and '> Inner quote...' inside <callout> must NOT be rewritten on outer pass!
        IMGEE_TEST_CHECK(rawContainerOut.find("```html\n</callout>\n> literal code line starting with gt inside fence\n```") != std::string::npos);
        IMGEE_TEST_CHECK(rawContainerOut.find("> Inner quote still inside explicit callout\n</callout>") != std::string::npos);
        // Meanwhile, the trailing top-level quote after </callout> MUST be rewritten into <blockquote>!
        IMGEE_TEST_CHECK(rawContainerOut.find("<blockquote>\nTop-level quote after explicit callout\n</blockquote>") != std::string::npos);

        ImGui::NewFrame();
        ImGui::Begin("RawContainerEdgeWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        StackSnapshot edgeSnap = StackSnapshot::Capture();
        MD::TextWrapped(rawContainerEdgeDoc);
        edgeSnap.VerifyBalanced("RawContainerEdgeWin");
        ImGui::End();
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
