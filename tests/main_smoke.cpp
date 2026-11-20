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
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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

// Valid 69-byte 2x1 RGBA green PNG (used to distinguish subpath vs flattened image resolution)
static const unsigned char kTiny2x1GreenPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0xF4, 0x22, 0x7F,
    0x8A, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
    0x54, 0x78, 0x9C, 0x63, 0x60, 0xF8, 0x0F, 0x81,
    0x00, 0x0F, 0xF9, 0x03, 0xFD, 0xD8, 0x39, 0x71,
    0x41, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E,
    0x44, 0xAE, 0x42, 0x60, 0x82
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
    ImFont* wideBold24 = nullptr;
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

    ImFontConfig wide24Cfg;
    wide24Cfg.SizePixels = 24.0f;
    fonts.wideBold24 = io.Fonts->AddFontDefault(&wide24Cfg);

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
            // Regression suite: quote delimiters after opening emphasis, outer emphasis across links, and nested code spans
            struct FormatRecord {
                ImGui::MarkdownFormatType type;
                int level;
                char emphasisChar;
                bool start;
            };
            std::vector<FormatRecord> formatLog;

            ImGui::MarkdownConfig regCfg = mdCfg;
            regCfg.userData = &formatLog;
            regCfg.formatCallback = [](const ImGui::MarkdownFormatInfo& info, bool start) {
                if (info.config && info.config->userData) {
                    auto* log = static_cast<std::vector<FormatRecord>*>(info.config->userData);
                    log->push_back({ info.type, info.level, info.emphasisChar, start });
                }
                ImGui::defaultMarkdownFormatCallback(info, start);
            };

            // 1. Quotes after opening emphasis delimiter: (*"Italic In Quotes"*) and *"Quoted Italic"*
            formatLog.clear();
            const std::string qDoc = "(*\"Italic In Quotes\"*) and *\"Quoted Italic\"*";
            ImGui::Markdown::Render(qDoc.c_str(), qDoc.length(), regCfg);
            int qItalicCount = 0;
            for (const auto& ev : formatLog) {
                if (ev.type == ImGui::MarkdownFormatType::EMPHASIS && ev.emphasisChar == '*' && ev.level == 1 && ev.start) {
                    ++qItalicCount;
                }
            }
            IMGEE_TEST_CHECK(qItalicCount == 2);

            // 2. Enclosing links preserving outer emphasis: **[Linked Heading](#slug)** and *[Italic Link](url)*
            formatLog.clear();
            const std::string lDoc = "**[Linked Heading](#slug)** and *[Italic Link](url)*";
            ImGui::Markdown::Render(lDoc.c_str(), lDoc.length(), regCfg);
            bool sawBoldLink = false;
            bool sawItalicLink = false;
            for (const auto& ev : formatLog) {
                if (ev.type == ImGui::MarkdownFormatType::EMPHASIS && ev.emphasisChar == '*' && ev.start) {
                    if (ev.level == 2) sawBoldLink = true;
                    if (ev.level == 1) sawItalicLink = true;
                }
            }
            IMGEE_TEST_CHECK(sawBoldLink);
            IMGEE_TEST_CHECK(sawItalicLink);

            // 3. Nested inline code span inside bold emphasis: **Bold text with (`code span`) inside**
            formatLog.clear();
            const std::string cDoc = "**Bold text with (`code span`) inside**";
            ImGui::Markdown::Render(cDoc.c_str(), cDoc.length(), regCfg);
            bool sawOuterBoldStart = false;
            bool sawOuterBoldEnd = false;
            bool sawNestedCodeStart = false;
            bool sawNestedCodeEnd = false;
            for (const auto& ev : formatLog) {
                if (ev.type == ImGui::MarkdownFormatType::EMPHASIS) {
                    if (ev.emphasisChar == '*' && ev.level == 2) {
                        if (ev.start) sawOuterBoldStart = true;
                        else sawOuterBoldEnd = true;
                    } else if (ev.emphasisChar == '`' && ev.level == 1) {
                        if (ev.start) sawNestedCodeStart = true;
                        else sawNestedCodeEnd = true;
                    }
                }
            }
            IMGEE_TEST_CHECK(sawOuterBoldStart);
            IMGEE_TEST_CHECK(sawOuterBoldEnd);
            IMGEE_TEST_CHECK(sawNestedCodeStart);
            IMGEE_TEST_CHECK(sawNestedCodeEnd);
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

    // Verify HostBackend callbacks actually resolved named textures and uploaded our initial disk PNG
    IMGEE_TEST_CHECK(gpuState.namedLookups >= 2);
    IMGEE_TEST_CHECK(gpuState.createdCount == 1);
    IMGEE_TEST_CHECK(gpuState.deletedCount == 0);

    // =========================================================================
    // Wave 2 Regression Tests:
    //  0) Quoted code fences inside > [!TIP] callouts (quote_in_fence)
    //  1) MeasureMarkdownLineWidth & <center> zoom + image + link measurement
    //  2) =68% center / align=right horizontal positioning + pixel zoom + nozoom
    //  3) Figure captions + multi-image same-context lightbox (Esc / [X] / backdrop)
    //  4) MarkdownConfig::baseDir images/ & ../images/ fallback + '..' guard
    //  5) BeginZoomedViewport / EndZoomedViewport 1.0x chrome + Nx content zoom
    // =========================================================================
    {
        // 0. Quoted code fence inside > [!TIP] callout must NOT split callout on > [!WARNING]:
        const std::string quotedFenceAlertDoc =
            "> [!TIP]\n"
            "> Example alert syntax inside a quoted code block:\n"
            "> ~~~markdown\n"
            "> [!WARNING]\n"
            "> Watch out for hydraulic pressure!\n"
            "> ~~~\n"
            "> Still inside the outer TIP callout.\n";

        std::string quotedFenceOut;
        const bool qfChanged = ImGui::PreprocessMarkdownBlockquotesAndAlerts(
            quotedFenceAlertDoc.c_str(), quotedFenceAlertDoc.size(), quotedFenceOut);
        IMGEE_TEST_CHECK(qfChanged);
        IMGEE_TEST_CHECK(quotedFenceOut.find("<callout type=tip>") != std::string::npos);
        IMGEE_TEST_CHECK(quotedFenceOut.find("<callout type=warning>") == std::string::npos);
        IMGEE_TEST_CHECK(quotedFenceOut.find("~~~markdown\n[!WARNING]\nWatch out for hydraulic pressure!\n~~~") != std::string::npos);
        IMGEE_TEST_CHECK(quotedFenceOut.find("Still inside the outer TIP callout.\n</callout>") != std::string::npos);

        // 1 & 2. MeasureMarkdownLineWidth zoom scaling, [label](url), <center>![Alt](<img =68%>)>,
        //        =68% center / align=right positioning, pixel-image zoom scaling, and nozoom:
        float measuredText1x = 0.0f;
        float measuredText15x = 0.0f;
        float measuredLinkLabelOnly = 0.0f;
        float measuredPlainLabel = 0.0f;

        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(820.0f, 640.0f), ImGuiCond_Always);
        ImGui::Begin("Wave2ImageAlignWin", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        StackSnapshot w2Snap = StackSnapshot::Capture();

        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
        measuredText1x = ImGui::MeasureMarkdownLineWidth("Centered **Flight** Advisory", 28, &mdCfg);
        measuredPlainLabel = ImGui::MeasureMarkdownLineWidth("Visible **Link** Label", 22, &mdCfg);
        measuredLinkLabelOnly = ImGui::MeasureMarkdownLineWidth(
            "[Visible **Link** Label](https://example.com/very/long/unrendered/url/path/that/must/not/inflate/width)",
            103, &mdCfg);
        IMGEE_TEST_CHECK(measuredText1x > 40.0f);
        IMGEE_TEST_CHECK(std::fabs(measuredLinkLabelOnly - measuredPlainLabel) <= 1.5f);

        const float fullAvailW = ImGui::GetContentRegionAvail().x;
        const float leftMarginScreenX = ImGui::GetCursorScreenPos().x;
        IMGEE_TEST_CHECK(fullAvailW > 500.0f);

        // Measure ![Alt](<test_logo =68%>) directly:
        const std::string pctImgMd = "![Alt](<test_logo =68%>)";
        const float measuredImg68 = ImGui::MeasureMarkdownLineWidth(pctImgMd.c_str(), pctImgMd.size(), &mdCfg);
        const float expectedW68 = std::round(fullAvailW * 0.68f);
        IMGEE_TEST_CHECK(std::fabs(measuredImg68 - expectedW68) <= 1.0f);

        // Render <center>![Alt](<test_logo =68%>)</center> at 1.0x -> must stay 0.68 * fullAvailW (no post-offset shrinkage!):
        MD::Text("<center>![Alt](<test_logo =68%>)</center>", &mdCfg);
        ImVec2 imgMin, imgMax;
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - expectedW68) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMin.x - leftMarginScreenX) - (fullAvailW - expectedW68) * 0.5f) <= 2.0f);

        // Render inline =68% center modifier:
        MD::Text("![Alt](<test_logo =68% center>)", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - expectedW68) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMin.x - leftMarginScreenX) - std::round((fullAvailW - expectedW68) * 0.5f)) <= 1.5f);

        // Render Pandoc {width=68% align=right} modifier:
        MD::Text("![Alt](test_logo){width=68% align=right}", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - expectedW68) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMin.x - leftMarginScreenX) - std::round(fullAvailW - expectedW68)) <= 1.5f);

        // Pixel-sized image at 1.0x vs 1.5x zoom vs nozoom:
        MD::Text("![Px](<test_logo =100x50>)", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - 100.0f) <= 0.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMax.y - imgMin.y) - 50.0f) <= 0.5f);

        ImGui::ImGeeCompat::SetWindowFontScale(1.5f);
        measuredText15x = ImGui::MeasureMarkdownLineWidth("Centered **Flight** Advisory", 28, &mdCfg);
        IMGEE_TEST_CHECK(std::fabs(measuredText15x - measuredText1x * 1.5f) <= 3.0f);

        // <center>![Alt](<test_logo =68%>)</center> under 1.5x window zoom:
        MD::Text("<center>![Alt](<test_logo =68%>)</center>", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - expectedW68) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMin.x - leftMarginScreenX) - (fullAvailW - expectedW68) * 0.5f) <= 2.0f);

        // Pixel image scales 1.5x with active zoom:
        MD::Text("![PxZoom](<test_logo =100x50>)", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - 150.0f) <= 1.0f);
        IMGEE_TEST_CHECK(std::fabs((imgMax.y - imgMin.y) - 75.0f) <= 1.0f);

        // Opt-out nozoom / scale=fixed stays strict 1:1 100x50 even at 1.5x zoom:
        MD::Text("![PxFixed](<test_logo =100x50 nozoom>)", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - 100.0f) <= 0.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMax.y - imgMin.y) - 50.0f) <= 0.5f);

        MD::Text("![PxFixedAttr](test_logo){width=100 height=50 scale=fixed}", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        IMGEE_TEST_CHECK(std::fabs((imgMax.x - imgMin.x) - 100.0f) <= 0.5f);
        IMGEE_TEST_CHECK(std::fabs((imgMax.y - imgMin.y) - 50.0f) <= 0.5f);

        ImGui::ImGeeCompat::SetWindowFontScale(1.0f);

        // 3a. Centered figure captions beneath centered & right-aligned images + nocaption:
        MD::Text("![Figure 1: **AFDS** Mode Panel](<test_logo =50% center caption>)", &mdCfg);
        ImVec2 capMin, capMax;
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        ImGui::GetLastMarkdownCaptionRect(&capMin, &capMax);
        const float imgCenter = (imgMin.x + imgMax.x) * 0.5f;
        const float capCenter = (capMin.x + capMax.x) * 0.5f;
        IMGEE_TEST_CHECK(capMax.x > capMin.x);
        IMGEE_TEST_CHECK(capMin.y >= imgMax.y);
        IMGEE_TEST_CHECK(std::fabs(capCenter - imgCenter) <= 2.5f);

        // Right-aligned image with custom caption="...":
        MD::Text("![Raw Alt](test_logo){width=40% align=right caption=\"Custom *italic* `NAV1` caption\"}", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
        ImGui::GetLastMarkdownCaptionRect(&capMin, &capMax);
        IMGEE_TEST_CHECK(std::fabs(((capMin.x + capMax.x) * 0.5f) - ((imgMin.x + imgMax.x) * 0.5f)) <= 2.5f);
        IMGEE_TEST_CHECK(capMin.x > leftMarginScreenX + 100.0f); // Not stuck at left margin!

        // Global showImageCaptions = true with per-image nocaption suppression:
        ImGui::MarkdownConfig globalCapCfg = mdCfg;
        globalCapCfg.showImageCaptions = true;
        ImVec2 prevCapMin = capMin;
        MD::Text("![Suppressed Alt](<test_logo =80x40 nocaption>)", &globalCapCfg);
        ImVec2 afterNoCapMin, afterNoCapMax;
        ImGui::GetLastMarkdownCaptionRect(&afterNoCapMin, &afterNoCapMax);
        IMGEE_TEST_CHECK(afterNoCapMin.x == prevCapMin.x && afterNoCapMin.y == prevCapMin.y); // No new caption emitted!

        w2Snap.VerifyBalanced("Wave2ImageAlignWin");
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        // 3b. Multi-Image Same-Context Lightbox: hover Hand cursor, open/dismiss (Esc, [X], backdrop), & ID isolation:
        const std::string twoLightboxDoc =
            "![Primary PFD](test_logo){width=120 height=80 lightbox}\n\n"
            "![Secondary MFD](test_logo){width=120 height=80 align=right lightbox caption=\"**MFD** Map\"}\n";

        auto setMouseAndButton = [&](float mx, float my, bool down) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddMousePosEvent(mx, my);
            io.AddMouseButtonEvent(0, down);
#else
            io.MousePos = ImVec2(mx, my);
            io.MouseDown[0] = down;
#endif
        };

        auto setEscapeKey = [&](bool down) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddKeyEvent(ImGuiKey_Escape, down);
#else
            io.KeyMap[ImGuiKey_Escape] = 27;
            io.KeysDown[27] = down;
#endif
        };

        // Pass A: Discover exact screen rects of image #1 and image #2
        ImVec2 thumb1Center(0.0f, 0.0f), thumb2Center(0.0f, 0.0f);
        setMouseAndButton(5.0f, 5.0f, false);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(30.0f, 30.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Always);
        ImGui::Begin("Wave2LightboxHostWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        MD::Text("![Primary PFD](test_logo){width=120 height=80 lightbox}", &mdCfg);
        ImVec2 r1Min, r1Max;
        ImGui::GetLastMarkdownImageRect(&r1Min, &r1Max);
        thumb1Center = ImVec2((r1Min.x + r1Max.x) * 0.5f, (r1Min.y + r1Max.y) * 0.5f);
        MD::Text("![Secondary MFD](test_logo){width=120 height=80 align=right lightbox caption=\"**MFD** Map\"}", &mdCfg);
        ImVec2 r2Min, r2Max;
        ImGui::GetLastMarkdownImageRect(&r2Min, &r2Max);
        thumb2Center = ImVec2((r2Min.x + r2Max.x) * 0.5f, (r2Min.y + r2Max.y) * 0.5f);
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        auto renderLightboxHostFrame = [&](ImGuiMouseCursor* outCursor = nullptr) {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(30.0f, 30.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Always);
            ImGui::Begin("Wave2LightboxHostWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot lbSnap = StackSnapshot::Capture();
            MD::Text(twoLightboxDoc, &mdCfg);
            if (outCursor) *outCursor = ImGui::GetMouseCursor();
            lbSnap.VerifyBalanced("Wave2LightboxHostWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        // Pass B: Hover thumbnail #1 -> Hand cursor affordance, then click + release to open Lightbox #1
        ImGuiMouseCursor hoveredCursor = ImGuiMouseCursor_Arrow;
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderLightboxHostFrame(&hoveredCursor);
        IMGEE_TEST_CHECK(hoveredCursor == ImGuiMouseCursor_Hand);
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());

        setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
        renderLightboxHostFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        const ImGuiID lbId1 = ImGui::GetActiveMarkdownLightboxID();
        IMGEE_TEST_CHECK(lbId1 != 0);

        // Verify enlarged lightbox image is clamped strictly inside host window [30..790, 30..550]:
        ImVec2 lbImgMin, lbImgMax;
        ImGui::GetActiveMarkdownLightboxImageRect(&lbImgMin, &lbImgMax);
        IMGEE_TEST_CHECK(lbImgMin.x >= 30.0f && lbImgMax.x <= 790.0f);
        IMGEE_TEST_CHECK(lbImgMin.y >= 30.0f && lbImgMax.y <= 550.0f);

        // Dismiss Lightbox #1 via Escape key:
        setEscapeKey(true);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        setEscapeKey(false);
        renderLightboxHostFrame();

        // Pass C: Click + release thumbnail #2 -> opens Lightbox #2 with a DISTINCT salted popup ID!
        setMouseAndButton(thumb2Center.x, thumb2Center.y, false);
        renderLightboxHostFrame();
        setMouseAndButton(thumb2Center.x, thumb2Center.y, true);
        renderLightboxHostFrame();
        setMouseAndButton(thumb2Center.x, thumb2Center.y, false);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        const ImGuiID lbId2 = ImGui::GetActiveMarkdownLightboxID();
        IMGEE_TEST_CHECK(lbId2 != 0 && lbId2 != lbId1);

        // Dismiss Lightbox #2 via [X] close button:
        ImVec2 closeMin, closeMax;
        ImGui::GetActiveMarkdownLightboxCloseRect(&closeMin, &closeMax);
        const ImVec2 closeCenter((closeMin.x + closeMax.x) * 0.5f, (closeMin.y + closeMax.y) * 0.5f);
        setMouseAndButton(closeCenter.x, closeCenter.y, false);
        renderLightboxHostFrame(&hoveredCursor);
        IMGEE_TEST_CHECK(hoveredCursor == ImGuiMouseCursor_Hand);
        setMouseAndButton(closeCenter.x, closeCenter.y, true);
        renderLightboxHostFrame();
        setMouseAndButton(closeCenter.x, closeCenter.y, false);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());

        // Pass D: Re-open Lightbox #1 and dismiss by clicking + releasing dimmed backdrop outside enlarged image:
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderLightboxHostFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
        renderLightboxHostFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());

        // Press top-left corner of host backdrop: stays open while held, closes cleanly on mouse-release:
        setMouseAndButton(45.0f, 80.0f, true);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        setMouseAndButton(45.0f, 80.0f, false);
        renderLightboxHostFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());

        // Pass E: Multi-frame held backdrop click-and-release directly over thumb1Center (and over an
        // underlying button/link) closes cleanly on release and NEVER re-opens or bleeds through!
        bool underlyingBtnTriggered = false;
        int underlyingLinkHits = 0;
        ImGui::MarkdownConfig bleedTestCfg = mdCfg;
        bleedTestCfg.userData = &underlyingLinkHits;
        bleedTestCfg.linkCallback = [](ImGui::MarkdownLinkCallbackData d) {
            if (d.userData) {
                *static_cast<int*>(d.userData) += 1;
            }
        };

        ImVec2 bleedBtnCenter(0.0f, 0.0f);
        ImVec2 bleedLinkCenter(0.0f, 0.0f);
        auto renderBleedTestFrame = [&]() {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(30.0f, 30.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Always);
            ImGui::Begin("Wave2LightboxHostWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            MD::Text(twoLightboxDoc, &bleedTestCfg);
            ImGui::SetCursorScreenPos(ImVec2(48.0f, 220.0f));
            if (ImGui::Button("UnderlyingCockpitAction", ImVec2(150.0f, 28.0f))) {
                underlyingBtnTriggered = true;
            }
            const ImVec2 bMin = ImGui::GetItemRectMin();
            const ImVec2 bMax = ImGui::GetItemRectMax();
            bleedBtnCenter = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);

            ImGui::SetCursorScreenPos(ImVec2(48.0f, 265.0f));
            MD::Text("[UnderlyingChecklistLink](https://example.invalid/checklist)", &bleedTestCfg);
            const ImVec2 lMin = ImGui::GetItemRectMin();
            const ImVec2 lMax = ImGui::GetItemRectMax();
            bleedLinkCenter = ImVec2((lMin.x + lMax.x) * 0.5f, (lMin.y + lMax.y) * 0.5f);
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        // 1. Orphan mouse-release over thumb1Center (mouse pressed elsewhere, released over thumb1Center)
        //    must NOT open Lightbox #1 because clickable thumbnails require matched ButtonBehavior ActiveId:
        setMouseAndButton(45.0f, 480.0f, true);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());

        // 2. Open Lightbox #1 properly with matched press + release on thumb1Center:
        setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
        renderBleedTestFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        ImGui::GetActiveMarkdownLightboxImageRect(&lbImgMin, &lbImgMax);
        // Confirm thumb1Center lies in the dimmed backdrop outside the centered enlarged image:
        IMGEE_TEST_CHECK(thumb1Center.x < lbImgMin.x - 10.0f);

        // 3. Press down on backdrop directly at thumb1Center, hold across 4 frames, then release:
        for (int holdFrame = 0; holdFrame < 4; ++holdFrame) {
            setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
            renderBleedTestFrame();
            IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        }
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderBleedTestFrame();
        // Must close cleanly on the release frame and stay closed on subsequent hover frames!
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());

        // 4. Re-open Lightbox #1, hold & release over underlying button AND over underlying Markdown link:
        setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
        renderBleedTestFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        IMGEE_TEST_CHECK(bleedBtnCenter.x < lbImgMin.x - 10.0f);

        setMouseAndButton(bleedBtnCenter.x, bleedBtnCenter.y, true);
        renderBleedTestFrame();
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        setMouseAndButton(bleedBtnCenter.x, bleedBtnCenter.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        IMGEE_TEST_CHECK(!underlyingBtnTriggered);

        setMouseAndButton(thumb1Center.x, thumb1Center.y, true);
        renderBleedTestFrame();
        setMouseAndButton(thumb1Center.x, thumb1Center.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
        IMGEE_TEST_CHECK(bleedLinkCenter.x < lbImgMin.x - 10.0f);

        setMouseAndButton(bleedLinkCenter.x, bleedLinkCenter.y, true);
        renderBleedTestFrame();
        renderBleedTestFrame();
        setMouseAndButton(bleedLinkCenter.x, bleedLinkCenter.y, false);
        renderBleedTestFrame();
        IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        IMGEE_TEST_CHECK(underlyingLinkHits == 0);

        // Pass F: Multi-line lightbox caption renders with high-contrast backing plate strictly inside
        // host_min..host_max and strictly below the enlarged image (zero overlap):
        {
            const std::string multiLineCaptionDoc =
                "![Overhead Bus Schematic](test_logo){width=120 height=90 lightbox caption=\""
                "**Electrical Bus Tie Overview:** Essential AC Bus 1 and Essential DC Bus 2 automatically "
                "transfer to the standby inverter and emergency ram air turbine generator whenever dual "
                "integrated drive generators drop offline during flight deck abnormal procedures, maintaining "
                "continuous attitude heading reference and flight director instrumentation across all phases.\"}";

            ImVec2 mlThumbCenter(0.0f, 0.0f);
            auto renderMultiLineCaptionLightbox = [&](bool captureThumb = false) {
                ImGui::NewFrame();
                ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(460.0f, 360.0f), ImGuiCond_Always);
                ImGui::Begin("Wave2MultiLineCaptionLbWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
                MD::TextWrapped(multiLineCaptionDoc, &mdCfg);
                if (captureThumb) {
                    ImVec2 mMin, mMax;
                    ImGui::GetLastMarkdownImageRect(&mMin, &mMax);
                    mlThumbCenter = ImVec2((mMin.x + mMax.x) * 0.5f, (mMin.y + mMax.y) * 0.5f);
                }
                ImGui::End();
                ImGui::Render();
                CompleteHeadlessRenderTextures();
            };

            setMouseAndButton(5.0f, 5.0f, false);
            renderMultiLineCaptionLightbox(true);

            setMouseAndButton(mlThumbCenter.x, mlThumbCenter.y, false);
            renderMultiLineCaptionLightbox();
            setMouseAndButton(mlThumbCenter.x, mlThumbCenter.y, true);
            renderMultiLineCaptionLightbox();
            setMouseAndButton(mlThumbCenter.x, mlThumbCenter.y, false);
            renderMultiLineCaptionLightbox();
            IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());

            auto verifyPlateAndVertices = [&]() {
                ImVec2 mlImgMin, mlImgMax, mlPlateMin, mlPlateMax;
                ImGui::GetActiveMarkdownLightboxImageRect(&mlImgMin, &mlImgMax);
                const bool hasPlate = ImGui::GetActiveMarkdownLightboxCaptionPlateRect(&mlPlateMin, &mlPlateMax);
                IMGEE_TEST_CHECK(hasPlate);

                // Backing plate must sit strictly inside host window [40..500, 40..400] and strictly below image:
                IMGEE_TEST_CHECK(mlPlateMin.x >= 40.0f && mlPlateMax.x <= 500.0f);
                IMGEE_TEST_CHECK(mlPlateMin.y >= 40.0f && mlPlateMax.y <= 400.0f);
                IMGEE_TEST_CHECK(mlPlateMin.y >= mlImgMax.y + 4.0f);
                // Multi-line caption must measure >= 3 text lines + vertical plate padding:
                IMGEE_TEST_CHECK((mlPlateMax.y - mlPlateMin.y) >= ImGui::GetTextLineHeight() * 3.0f + 8.0f);

                // Verify backing plate fill vertices exist in DrawData and match mlPlateMin..mlPlateMax:
                const ImDrawData* dd = ImGui::GetDrawData();
                IMGEE_TEST_CHECK(dd != nullptr);
                const ImU32 expectedPlateFill = IM_COL32(10, 14, 22, 245);
                bool foundPlateVerts = false;
                ImVec2 vPlateMin(1e9f, 1e9f), vPlateMax(-1e9f, -1e9f);
                for (int n = 0; n < dd->CmdListsCount; ++n) {
                    const ImDrawList* cl = dd->CmdLists[n];
                    for (int vi = 0; vi < cl->VtxBuffer.Size; ++vi) {
                        const ImDrawVert& v = cl->VtxBuffer[vi];
                        if (v.col == expectedPlateFill) {
                            foundPlateVerts = true;
                            if (v.pos.x < vPlateMin.x) vPlateMin.x = v.pos.x;
                            if (v.pos.y < vPlateMin.y) vPlateMin.y = v.pos.y;
                            if (v.pos.x > vPlateMax.x) vPlateMax.x = v.pos.x;
                            if (v.pos.y > vPlateMax.y) vPlateMax.y = v.pos.y;
                        }
                    }
                }
                IMGEE_TEST_CHECK(foundPlateVerts);
                IMGEE_TEST_CHECK(std::fabs(vPlateMin.x - mlPlateMin.x) <= 1.5f);
                IMGEE_TEST_CHECK(std::fabs(vPlateMin.y - mlPlateMin.y) <= 1.5f);
                IMGEE_TEST_CHECK(std::fabs(vPlateMax.x - mlPlateMax.x) <= 1.5f);
                IMGEE_TEST_CHECK(std::fabs(vPlateMax.y - mlPlateMax.y) <= 1.5f);
            };

            // Verify both Frame 1 (cache miss / VtxBuffer placement) AND Frame 2 (cache hit / direct placement):
            verifyPlateAndVertices();
            renderMultiLineCaptionLightbox();
            verifyPlateAndVertices();

            // Verify top-right hover zoom strip sits strictly to the left of [X], mouse-wheel zoom steps & consumes wheel,
            // and 1-click A/B ToggleResetZoom restores 1.0f <-> custom zoom without closing the lightbox:
            {
                const auto& lbTelem = ImGui::MarkdownDetail::GetLightboxTelemetryState();
                const ImVec2 lbImgCenter(
                    (lbTelem.lightboxImageMin.x + lbTelem.lightboxImageMax.x) * 0.5f,
                    (lbTelem.lightboxImageMin.y + lbTelem.lightboxImageMax.y) * 0.5f);
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                io.AddMousePosEvent(lbImgCenter.x, lbImgCenter.y);
#else
                io.MousePos = lbImgCenter;
#endif
                renderMultiLineCaptionLightbox();
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
                io.AddMouseWheelEvent(0.0f, 3.0f);
#else
                io.MouseWheel = 3.0f;
#endif
                renderMultiLineCaptionLightbox();
                IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());
                IMGEE_TEST_CHECK(lbTelem.zoomStripMax.x > lbTelem.zoomStripMin.x);
                IMGEE_TEST_CHECK(lbTelem.zoomStripMax.x <= lbTelem.closeButtonMin.x);
                IMGEE_TEST_CHECK(std::fabs(lbTelem.lightboxZoom - 1.30f) <= 0.01f);

                // Verify A/B zoom reset/restore state machine:
                auto& lbView = ImGui::MarkdownDetail::GetLightboxViewState();
                IMGEE_TEST_CHECK(lbView.ToggleResetZoom());
                IMGEE_TEST_CHECK(std::fabs(lbView.zoom - 1.0f) <= 0.005f);
                IMGEE_TEST_CHECK(lbView.ToggleResetZoom());
                IMGEE_TEST_CHECK(std::fabs(lbView.zoom - 1.30f) <= 0.01f);
                lbView.ToggleResetZoom();
            }

            // Cleanly close via Escape:
            setEscapeKey(true);
            renderMultiLineCaptionLightbox();
            IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
            setEscapeKey(false);
            renderMultiLineCaptionLightbox();
        }

        // 4. MarkdownConfig::baseDir subfolder (images/) & parent (../, ../images/) lookup + '..' guard:
        auto makeDir = [](const char* path) {
#if defined(_WIN32)
            (void)_mkdir(path);
#else
            (void)mkdir(path, 0755);
#endif
        };
        auto rmDir = [](const char* path) {
#if defined(_WIN32)
            (void)_rmdir(path);
#else
            (void)rmdir(path);
#endif
        };

        makeDir("imgee_ci_docroot");
        makeDir("imgee_ci_docroot/docs");
        makeDir("imgee_ci_docroot/docs/images");
        makeDir("imgee_ci_docroot/images");

        const std::string subImgPath = "imgee_ci_docroot/docs/images/sub_diagram.png";
        const std::string parentSibImgPath = "imgee_ci_docroot/images/parent_diagram.png";
        {
            std::ofstream ofs1(subImgPath.c_str(), std::ios::binary | std::ios::trunc);
            IMGEE_TEST_CHECK(ofs1.good());
            ofs1.write(reinterpret_cast<const char*>(kTinyRedPng), static_cast<std::streamsize>(sizeof(kTinyRedPng)));

            std::ofstream ofs2(parentSibImgPath.c_str(), std::ios::binary | std::ios::trunc);
            IMGEE_TEST_CHECK(ofs2.good());
            ofs2.write(reinterpret_cast<const char*>(kTinyRedPng), static_cast<std::streamsize>(sizeof(kTinyRedPng)));
        }

        ImGui::MarkdownConfig docDirCfg = mdCfg;
        docDirCfg.baseDir = "imgee_ci_docroot/docs";
        const int prevCreated = gpuState.createdCount;

        ImGui::NewFrame();
        ImGui::Begin("Wave2BaseDirWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        // Resolves via baseDir + "/images/" + baseName:
        MD::Text("![Sub](sub_diagram.png)", &docDirCfg);
        // Resolves via baseDir + "/../images/" + baseName:
        MD::Text("![ParentSib](parent_diagram.png)", &docDirCfg);
        // '../imgee_ci_test_pixel.png' contains '..': direct baseDir/../imgee_ci_test_pixel.png does NOT exist
        // (it lives at CWD == ../.. from baseDir!), and fallback MUST NOT double-climb '../..' when '..' is present:
        ImGui::MarkdownImageData travCheck;
        {
            struct ScopedCfg {
                const ImGui::MarkdownConfig* prev;
                explicit ScopedCfg(const ImGui::MarkdownConfig* c) : prev(ImGui::GetActiveMarkdownConfig()) { ImGui::GetActiveMarkdownConfig() = c; }
                ~ScopedCfg() { ImGui::GetActiveMarkdownConfig() = prev; }
            } sc(&docDirCfg);
            travCheck = ImGui::MarkdownImageService::DefaultMarkdownImageCallback({ "Trav", 4, "../imgee_ci_test_pixel.png", 26, nullptr, true });
        }
        IMGEE_TEST_CHECK(!travCheck.isValid);
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        IMGEE_TEST_CHECK(gpuState.createdCount == prevCreated + 2);

        std::remove(subImgPath.c_str());
        std::remove(parentSibImgPath.c_str());
        rmDir("imgee_ci_docroot/docs/images");
        rmDir("imgee_ci_docroot/docs");
        rmDir("imgee_ci_docroot/images");
        rmDir("imgee_ci_docroot");

        // 5. Two-Layer Zoomable Viewport (1.0x chrome + Nx zoomed content + opt-in Ctrl+Wheel + A/B reset):
        ImGui::ZoomableViewportState vpState;
        IMGEE_TEST_CHECK(vpState.enable_ctrl_wheel == false);
        IMGEE_TEST_CHECK(vpState.GetZoomPercent() == 100);
        vpState.SetZoom(0.10f);
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 0.50f) <= 0.001f);
        vpState.SetZoom(9.00f);
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 2.50f) <= 0.001f);
        vpState.SetZoom(1.60f);
        IMGEE_TEST_CHECK(vpState.ToggleResetZoom());
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 1.00f) <= 0.001f);
        IMGEE_TEST_CHECK(vpState.ToggleResetZoom());
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 1.60f) <= 0.001f);
        ImGui::StepViewportZoom(vpState, -0.10f);
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 1.50f) <= 0.001f);

        // Opt in to Ctrl+MouseWheel zoom explicitly and verify 1.0x header/footer isolation + wheel consumption:
        vpState.enable_ctrl_wheel = true;
        float headerFontSz = 0.0f;
        float innerFontSz = 0.0f;
        float footerFontSz = 0.0f;
        for (int pass = 0; pass < 2; ++pass) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
            io.AddMousePosEvent(200.0f, 200.0f);
            io.AddKeyEvent(ImGuiMod_Ctrl, pass == 1);
            if (pass == 1) io.AddMouseWheelEvent(0.0f, 1.0f);
#else
            io.MousePos = ImVec2(200.0f, 200.0f);
            io.KeyCtrl = (pass == 1);
            io.MouseWheel = (pass == 1) ? 1.0f : 0.0f;
#endif
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(600.0f, 420.0f), ImGuiCond_Always);
            ImGui::Begin("Wave2ZoomViewportWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot vpSnap = StackSnapshot::Capture();

            headerFontSz = ImGui::GetFontSize();
            ImGui::TextUnformatted("1.0x Header Chrome");
            ImGui::SameLine();
            (void)ImGui::ZoomViewportControls(vpState);

            if (ImGui::BeginZoomedViewport("##DocBody", vpState, ImVec2(0.0f, -30.0f))) {
                if (pass == 1) {
                    // BeginZoomedViewport must zero io.MouseWheel after consuming Ctrl+MouseWheel zoom!
                    IMGEE_TEST_CHECK(std::fabs(ImGui::GetIO().MouseWheel) <= 0.0001f);
                }
                innerFontSz = ImGui::GetFontSize();
                MD::TextWrapped("# Zoomed Document Body\nParagraph scaled cleanly inside child viewport.", &mdCfg);
            }
            ImGui::EndZoomedViewport();

            footerFontSz = ImGui::GetFontSize();
            ImGui::TextDisabled("1.0x Footer Chrome");

            vpSnap.VerifyBalanced("Wave2ZoomViewportWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        }

        // Ctrl+Wheel (+1 notch == +0.05) stepped 1.50 -> 1.55 inside hovered viewport!
        IMGEE_TEST_CHECK(std::fabs(vpState.zoom - 1.55f) <= 0.001f);
        IMGEE_TEST_CHECK(std::fabs(innerFontSz - headerFontSz * 1.55f) <= 0.5f);
        // Header and footer chrome remain strictly 1.0x!
        IMGEE_TEST_CHECK(std::fabs(footerFontSz - headerFontSz) <= 0.01f);

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddMouseWheelEvent(0.0f, 0.0f);
#else
        io.KeyCtrl = false;
        io.MouseWheel = 0.0f;
#endif
    }

    // =========================================================================
    // Wave 3 Regression Tests:
    //  0) <img .../> placeholder width parity & images/subdir/foo.png priority
    //  1) float=left mid-paragraph wrap + expansion below maxY
    //  2) float=right (=35%) flush-right placement + right-inset wrap + expansion
    //  3) Short text beside tall float (end-of-scope clearance, <callout>, CalcSize)
    //  4) Explicit <clear/> & <!-- clear --> float clearing + narrow-viewport fallback
    // =========================================================================
    {
        auto makeDir = [](const char* path) {
#if defined(_WIN32)
            (void)_mkdir(path);
#else
            (void)mkdir(path, 0755);
#endif
        };
        auto rmDir = [](const char* path) {
#if defined(_WIN32)
            (void)_rmdir(path);
#else
            (void)rmdir(path);
#endif
        };

        // 0a. <img src="missing_asset.png" /> placeholder width parity with ![Alt](missing_asset.png):
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_Always);
        ImGui::Begin("Wave3PolishAndFloatsWin", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        StackSnapshot w3Snap = StackSnapshot::Capture();

        const char* mdMissing = "![Alt](nonexistent_remote_preview.png)";
        const char* htmlMissing = "<img src=\"nonexistent_remote_preview.png\" />";
        const float wMdPh = ImGui::MeasureMarkdownLineWidth(mdMissing, std::strlen(mdMissing), &mdCfg);
        const float wHtmlPh = ImGui::MeasureMarkdownLineWidth(htmlMissing, std::strlen(htmlMissing), &mdCfg);
        IMGEE_TEST_CHECK(wHtmlPh > 40.0f);
        IMGEE_TEST_CHECK(std::fabs(wHtmlPh - wMdPh) <= 1.0f);

        // 0b. Subpath priority: images/subdir/chart.png (2x1) MUST win over flattened images/chart.png (1x1)!
        makeDir("imgee_ci_subprio");
        makeDir("imgee_ci_subprio/docs");
        makeDir("imgee_ci_subprio/docs/images");
        makeDir("imgee_ci_subprio/docs/images/subdir");
        const std::string flatChartPath = "imgee_ci_subprio/docs/images/chart.png";
        const std::string subChartPath  = "imgee_ci_subprio/docs/images/subdir/chart.png";
        {
            std::ofstream ofsFlat(flatChartPath.c_str(), std::ios::binary | std::ios::trunc);
            IMGEE_TEST_CHECK(ofsFlat.good());
            ofsFlat.write(reinterpret_cast<const char*>(kTinyRedPng), static_cast<std::streamsize>(sizeof(kTinyRedPng)));

            std::ofstream ofsSub(subChartPath.c_str(), std::ios::binary | std::ios::trunc);
            IMGEE_TEST_CHECK(ofsSub.good());
            ofsSub.write(reinterpret_cast<const char*>(kTiny2x1GreenPng), static_cast<std::streamsize>(sizeof(kTiny2x1GreenPng)));
        }

        ImGui::MarkdownConfig subPrioCfg = mdCfg;
        subPrioCfg.baseDir = "imgee_ci_subprio/docs";
        ImGui::MarkdownImageData subResolved;
        {
            struct ScopedCfg {
                const ImGui::MarkdownConfig* prev;
                explicit ScopedCfg(const ImGui::MarkdownConfig* c) : prev(ImGui::GetActiveMarkdownConfig()) { ImGui::GetActiveMarkdownConfig() = c; }
                ~ScopedCfg() { ImGui::GetActiveMarkdownConfig() = prev; }
            } sc(&subPrioCfg);
            subResolved = ImGui::MarkdownImageService::DefaultMarkdownImageCallback({ "Chart", 5, "subdir/chart.png", 16, nullptr, true });
        }
        IMGEE_TEST_CHECK(subResolved.isValid);
        IMGEE_TEST_CHECK(std::fabs(subResolved.native_size.x - 2.0f) <= 0.1f && std::fabs(subResolved.native_size.y - 1.0f) <= 0.1f);

        std::remove(subChartPath.c_str());
        std::remove(flatChartPath.c_str());
        rmDir("imgee_ci_subprio/docs/images/subdir");
        rmDir("imgee_ci_subprio/docs/images");
        rmDir("imgee_ci_subprio/docs");
        rmDir("imgee_ci_subprio");

        // 1. float=left + mid-paragraph expansion once y >= maxY:
        const float baseLeftX = ImGui::GetCursorScreenPos().x;
        const float fullW = ImGui::GetContentRegionAvail().x;
        const std::string floatLeftDoc =
            "![Diagram](<test_logo =120x42 float=left gap=14x4>)\n\n"
            "Autopilot flight director guidance continuously computes lateral roll commands and vertical pitch targets "
            "across climb cruise descent and approach phases while monitoring dual redundant air data inertial reference "
            "channels and autothrottle speed envelope protections throughout every segment of the flight plan.";
        MD::TextWrapped(floatLeftDoc, &mdCfg);

        ImVec2 flMin, flMax;
        ImGui::GetLastMarkdownImageRect(&flMin, &flMax);
        IMGEE_TEST_CHECK(std::fabs(flMin.x - baseLeftX) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs((flMax.x - flMin.x) - 120.0f) <= 1.0f);
        const float flClearY = flMax.y + 4.0f;

        const auto& leftLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(leftLines.size() >= 4);
        bool sawWrappedBesideLeft = false;
        bool sawExpandedBelowLeft = false;
        for (size_t li = 0; li < leftLines.size(); ++li) {
            if (leftLines[li].min.y < flMax.y - 2.0f) {
                sawWrappedBesideLeft = true;
                IMGEE_TEST_CHECK(leftLines[li].min.x >= baseLeftX + 120.0f + 14.0f - 2.0f);
            } else if (leftLines[li].min.y >= flClearY + 1.0f) {
                sawExpandedBelowLeft = true;
                IMGEE_TEST_CHECK(std::fabs(leftLines[li].min.x - baseLeftX) <= 2.0f);
            }
        }
        IMGEE_TEST_CHECK(sawWrappedBesideLeft && sawExpandedBelowLeft);

        // 2. float=right (=35%) flush-right placement + right-inset wrap + expansion below maxY:
        const std::string floatRightDoc =
            "![Panel](<test_logo =35%x42 float=right gap=12x4>)\n\n"
            "Primary hydraulic actuators receive pressurized fluid from engine driven pumps and electric standby pumps "
            "ensuring full aileron elevator and rudder authority during asymmetric thrust or single system degraded "
            "operations all the way down to touchdown rollout and taxiway turnoff.";
        // Note: =35% with explicit height=42 via Pandoc attributes:
        const std::string floatRightAttrDoc =
            "![Panel](test_logo){width=35% height=42 float=right gap=12x4}\n\n"
            "Primary hydraulic actuators receive pressurized fluid from engine driven pumps and electric standby pumps "
            "ensuring full aileron elevator and rudder authority during asymmetric thrust or single system degraded "
            "operations all the way down to touchdown rollout and taxiway turnoff while secondary accumulators maintain "
            "parking brake pressure and nosewheel steering damping across extended gate turnaround operations.";
        (void)floatRightDoc;
        MD::TextWrapped(floatRightAttrDoc, &mdCfg);

        ImVec2 frMin, frMax;
        ImGui::GetLastMarkdownImageRect(&frMin, &frMax);
        const float expectedRightW = std::round(fullW * 0.35f);
        IMGEE_TEST_CHECK(std::fabs((frMax.x - frMin.x) - expectedRightW) <= 1.5f);
        IMGEE_TEST_CHECK(std::fabs(frMin.x - std::round(baseLeftX + fullW - expectedRightW)) <= 2.0f);
        const float frClearY = frMax.y + 4.0f;

        const auto& rightLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(rightLines.size() >= 4);
        bool sawWrappedBesideRight = false;
        bool sawExpandedBelowRight = false;
        for (size_t li = 0; li < rightLines.size(); ++li) {
            if (rightLines[li].min.y < frMax.y - 2.0f) {
                sawWrappedBesideRight = true;
                IMGEE_TEST_CHECK(std::fabs(rightLines[li].min.x - baseLeftX) <= 2.0f);
                IMGEE_TEST_CHECK(rightLines[li].max.x <= frMin.x - 12.0f + 2.0f);
            } else if (rightLines[li].min.y >= frClearY + 1.0f) {
                if (rightLines[li].max.x > frMin.x + 40.0f) {
                    sawExpandedBelowRight = true;
                }
            }
        }
        IMGEE_TEST_CHECK(sawWrappedBesideRight && sawExpandedBelowRight);

        // 3. Short text beside a tall floated image:
        //  (a) Top-level end-of-scope clearance prevents overlap with subsequent widgets
        MD::TextWrapped("![Tall](test_logo){width=90 height=160 float=left}\n\nShort caption note.", &mdCfg);
        ImVec2 tallMin, tallMax;
        ImGui::GetLastMarkdownImageRect(&tallMin, &tallMax);
        const float cursorAfterTallFloat = ImGui::GetCursorScreenPos().y;
        IMGEE_TEST_CHECK(cursorAfterTallFloat >= tallMax.y);

        //  (b) Inside <callout type=note>: card group height encompasses full 150px floated image!
        const std::string calloutTallFloatDoc =
            "> [!NOTE]\n"
            "> ![Badge](test_logo){width=80 height=150 float=right}\n"
            "> Brief note beside tall float.\n";
        MD::TextWrapped(calloutTallFloatDoc, &mdCfg);
        const ImVec2 calloutCardSz = ImGui::GetItemRectSize();
        IMGEE_TEST_CHECK(calloutCardSz.y >= 150.0f);

        //  (c) MD::CalcMarkdownSize encompasses full height of a tall floated image:
        const ImVec2 calcFloatSz = MD::CalcMarkdownSize(
            ImGui::GetID("##w3_calc_float"), 0,
            "![TallCalc](test_logo){width=80 height=175 float=left}\n\nOne short line.",
            400.0f, 1.0f, &mdCfg);
        IMGEE_TEST_CHECK(calcFloatSz.y >= 175.0f);

        // 4. Explicit <clear/> and <!-- clear --> advance cursor Y below active floats:
        MD::TextWrapped(
            "![Clr1](test_logo){width=90 height=130 float=left}\n\n"
            "Beside float.\n"
            "<clear/>\n"
            "Line after explicit clear tag.", &mdCfg);
        ImVec2 clrMin, clrMax;
        ImGui::GetLastMarkdownImageRect(&clrMin, &clrMax);
        const auto& clrLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(clrLines.size() >= 2);
        IMGEE_TEST_CHECK(clrLines.front().min.y < clrMax.y);
        IMGEE_TEST_CHECK(clrLines.front().min.x >= baseLeftX + 90.0f);
        IMGEE_TEST_CHECK(clrLines.back().min.y >= clrMax.y);
        IMGEE_TEST_CHECK(std::fabs(clrLines.back().min.x - baseLeftX) <= 2.0f);

        MD::TextWrapped(
            "![Clr2](test_logo){width=90 height=120 float=left}\n\n"
            "Beside float two.\n"
            "<!-- clear -->\n"
            "Line after HTML clear comment.", &mdCfg);
        ImGui::GetLastMarkdownImageRect(&clrMin, &clrMax);
        const auto& clrCommentLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(clrCommentLines.size() >= 2);
        IMGEE_TEST_CHECK(clrCommentLines.back().min.y >= clrMax.y);
        IMGEE_TEST_CHECK(std::fabs(clrCommentLines.back().min.x - baseLeftX) <= 2.0f);

        w3Snap.VerifyBalanced("Wave3PolishAndFloatsWin");
        ImGui::End();

        // 5. Responsive narrow-viewport fallback: when remaining text width < 90px * scale,
        //    float gracefully falls back to block layout (text renders BELOW image at full width):
        ImGui::SetNextWindowPos(ImVec2(680.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(180.0f, 300.0f), ImGuiCond_Always);
        ImGui::Begin("Wave3NarrowFallbackWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        const float narrowLeftX = ImGui::GetCursorScreenPos().x;
        MD::TextWrapped("![WideInNarrow](test_logo){width=110 height=50 float=left}\n\nFallback prose below image.", &mdCfg);
        ImVec2 nfMin, nfMax;
        ImGui::GetLastMarkdownImageRect(&nfMin, &nfMax);
        const auto& nfLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(!nfLines.empty());
        IMGEE_TEST_CHECK(nfLines.front().min.y >= nfMax.y - 0.5f);
        IMGEE_TEST_CHECK(std::fabs(nfLines.front().min.x - narrowLeftX) <= 2.0f);
        ImGui::End();

        ImGui::Render();
        CompleteHeadlessRenderTextures();
    }

    // ========================================================================
    // Wave 3 Peer-Review Polish Regression Tests:
    //  1. CalcMarkdownSize() default MarkdownConfig inheritance, caller window
    //     zoom parity, and non-leakage across passes when font_scale <= 0.0f
    //  2. Word-wrapped <center>...</center> preserving outer float=left &
    //     float=right bounds across all wrapped lines
    //  3. Tall floated image + > [!NOTE] callout non-overlap in both wide
    //     (narrows beside float) and narrow (< 220px clears below float) layouts
    //  4. Unordered & ordered list items starting immediately with a custom
    //     inline tag beside float=left
    //  5. Wrapped <style bg=...> background pill coordinates matching the
    //     wrapped text line rather than the pre-wrap cursor position
    // ========================================================================
    {
        io.DeltaTime = 1.0f / 60.0f;
        io.MousePos = ImVec2(0.0f, 0.0f);
        io.MouseDown[0] = false;
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(640.0f, 960.0f), ImGuiCond_Always);
        ImGui::Begin("Wave3PeerReviewPolishWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        StackSnapshot w3PolishSnap = StackSnapshot::Capture();

        const float polishLeftX = ImGui::GetCursorScreenPos().x;
        const float polishFullW = ImGui::GetContentRegionAvail().x;

        // 1. CalcMarkdownSize() default MarkdownConfig + zoom parity + non-leakage:
        {
            ImGui::SetDefaultMarkdownConfig(nullptr);
            const ImGuiID calcId = ImGui::GetID("##w3_polish_calc");
            const ImVec2 szNoDefBold = MD::CalcMarkdownSize(calcId, 0, "**Reset**", 0.0f, 0.0f, nullptr);

            ImGui::MarkdownConfig defBoldCfg;
            defBoldCfg.boldFont = atlasFonts.wideBold24;
            defBoldCfg.wrapWidth = 42.0f; // Ensure wrap_width == 0.0f with md_config == nullptr resets wrapWidth to 0.0f!
            ImGui::SetDefaultMarkdownConfig(&defBoldCfg);

            const ImVec2 szWithDefBold1x = MD::CalcMarkdownSize(calcId, 0, "**Reset**", 0.0f, 0.0f, nullptr);
            IMGEE_TEST_CHECK(szWithDefBold1x.x > szNoDefBold.x * 1.3f);

            // Explicit font_scale = 2.0f on pass 0 (and fresh pass 1) must scale ~2x (never 4x!) and NOT leak:
            const ImVec2 szFreshPass1_2x = MD::CalcMarkdownSize(calcId, 1, "**Reset**", 0.0f, 2.0f, nullptr);
            IMGEE_TEST_CHECK(szFreshPass1_2x.x > szWithDefBold1x.x * 1.85f);
            IMGEE_TEST_CHECK(szFreshPass1_2x.x < szWithDefBold1x.x * 2.2f);

            const ImVec2 szExplicit2x = MD::CalcMarkdownSize(calcId, 0, "**Reset**", 0.0f, 2.0f, nullptr);
            IMGEE_TEST_CHECK(szExplicit2x.x > szWithDefBold1x.x * 1.85f);
            IMGEE_TEST_CHECK(szExplicit2x.x < szWithDefBold1x.x * 2.2f);

            const ImVec2 szAfter2x = MD::CalcMarkdownSize(calcId, 0, "**Reset**", 0.0f, 0.0f, nullptr);
            IMGEE_TEST_CHECK(std::fabs(szAfter2x.x - szWithDefBold1x.x) <= 0.5f);
            IMGEE_TEST_CHECK(std::fabs(szAfter2x.y - szWithDefBold1x.y) <= 0.5f);

            // Inherit caller window's 1.5x zoom when font_scale <= 0.0f:
            ImGui::ImGeeCompat::SetWindowFontScale(1.5f);
            const ImVec2 szInherited15x = MD::CalcMarkdownSize(calcId, 0, "**Reset**", 0.0f, 0.0f, nullptr);
            ImGui::ImGeeCompat::SetWindowFontScale(1.0f);
            IMGEE_TEST_CHECK(szInherited15x.x > szWithDefBold1x.x * 1.4f);
            IMGEE_TEST_CHECK(szInherited15x.x < szWithDefBold1x.x * 1.6f);

            ImGui::SetDefaultMarkdownConfig(nullptr);
        }

        // 2. Word-wrapped <center>...</center> beside active float=left AND float=right:
        {
            const std::string wrappedCenterFloatDoc =
                "![FL](test_logo){width=100 height=95 float=left gap=10x4}\n"
                "![FR](test_logo){width=100 height=95 float=right gap=10x4}\n"
                "<center>Long centered advisory sentence that wraps cleanly across multiple lines while staying strictly inside both left and right active float margins throughout the entire block.</center>";
            MD::TextWrapped(wrappedCenterFloatDoc, &mdCfg);

            const auto& cfLines = ImGui::GetLastMarkdownRenderedLines();
            IMGEE_TEST_CHECK(cfLines.size() >= 2);
            const float expectedMinX = polishLeftX + 100.0f + 10.0f - 2.0f;
            const float expectedMaxX = polishLeftX + polishFullW - (100.0f + 10.0f) + 2.0f;
            for (size_t li = 0; li < cfLines.size(); ++li) {
                IMGEE_TEST_CHECK(cfLines[li].min.x >= expectedMinX);
                IMGEE_TEST_CHECK(cfLines[li].max.x <= expectedMaxX);
            }
        }

        // 3a. Tall floated image + > [!NOTE] callout in a wide window (narrows cleanly beside float):
        {
            const std::string wideFloatCalloutDoc =
                "![TallFl](test_logo){width=120 height=160 float=left gap=12x4}\n\n"
                "> [!NOTE]\n"
                "> Multi-line advisory callout card beside a tall floated diagram that narrows cleanly inside the remaining column without overlapping the float.\n";
            MD::TextWrapped(wideFloatCalloutDoc, &mdCfg);

            ImVec2 tallFlMin, tallFlMax;
            ImGui::GetLastMarkdownImageRect(&tallFlMin, &tallFlMax);
            const auto& wideCalloutLines = ImGui::GetLastMarkdownRenderedLines();
            IMGEE_TEST_CHECK(wideCalloutLines.size() >= 2);
            // Confirm callout sits vertically alongside the float and horizontally strictly to its right:
            IMGEE_TEST_CHECK(wideCalloutLines.front().min.y < tallFlMax.y - 10.0f);
            for (size_t li = 0; li < wideCalloutLines.size(); ++li) {
                IMGEE_TEST_CHECK(wideCalloutLines[li].min.x >= tallFlMax.x + 12.0f + 6.0f);
                IMGEE_TEST_CHECK(wideCalloutLines[li].max.x <= polishLeftX + polishFullW + 2.0f);
            }
        }

        // 4. Unordered & ordered list items starting immediately with a custom inline tag beside float=left:
        {
            static std::vector<float> s_probeStartXs;
            s_probeStartXs.clear();
            ImGui::RegisterMarkdownTag("w3probe", [](const std::string& inner, const std::string& /*param*/) {
                s_probeStartXs.push_back(ImGui::GetCursorScreenPos().x);
                ImGui::TextUnformatted(inner.c_str());
            });

            const std::string listTagFloatDoc =
                "![ListFl](test_logo){width=100 height=70 float=left gap=12x4}\n\n"
                "* <w3probe>Tagged bullet item</w3probe>\n"
                "1. <w3probe>Tagged ordered item</w3probe>";
            MD::TextWrapped(listTagFloatDoc, &mdCfg);

            IMGEE_TEST_CHECK(s_probeStartXs.size() == 2);
            IMGEE_TEST_CHECK(s_probeStartXs[0] >= polishLeftX + 100.0f + 12.0f + 8.0f);
            IMGEE_TEST_CHECK(s_probeStartXs[1] >= polishLeftX + 100.0f + 12.0f + 8.0f);
        }

        // 5. Wrapped <style bg=...> pill background rectangle lands on the wrapped text line (not line 1):
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const int vtxBefore = dl->VtxBuffer.Size;

            ImGui::MarkdownConfig pillWrapCfg = mdCfg;
            pillWrapCfg.wrapWidth = 240.0f;
            MD::TextWrapped("Prefix words filling nearly all of line one <style color=#FFEE44 bg=#225588>HighlightedPillSpan</style>", &pillWrapCfg);

            const auto& pillLines = ImGui::GetLastMarkdownRenderedLines();
            IMGEE_TEST_CHECK(!pillLines.empty());
            const float line1BottomY = pillLines.front().max.y;

            const ImU32 expectedBgCol   = IM_COL32(0x22, 0x55, 0x88, 0xFF);
            const ImU32 expectedTextCol = IM_COL32(0xFF, 0xEE, 0x44, 0xFF);

            ImVec2 bgMin(1e9f, 1e9f), bgMax(-1e9f, -1e9f);
            ImVec2 txtMin(1e9f, 1e9f), txtMax(-1e9f, -1e9f);
            bool sawBg = false;
            bool sawTxt = false;
            for (int vi = vtxBefore; vi < dl->VtxBuffer.Size; ++vi) {
                const ImDrawVert& v = dl->VtxBuffer[vi];
                if (v.col == expectedBgCol) {
                    sawBg = true;
                    if (v.pos.x < bgMin.x) bgMin.x = v.pos.x;
                    if (v.pos.y < bgMin.y) bgMin.y = v.pos.y;
                    if (v.pos.x > bgMax.x) bgMax.x = v.pos.x;
                    if (v.pos.y > bgMax.y) bgMax.y = v.pos.y;
                } else if (v.col == expectedTextCol) {
                    sawTxt = true;
                    if (v.pos.x < txtMin.x) txtMin.x = v.pos.x;
                    if (v.pos.y < txtMin.y) txtMin.y = v.pos.y;
                    if (v.pos.x > txtMax.x) txtMax.x = v.pos.x;
                    if (v.pos.y > txtMax.y) txtMax.y = v.pos.y;
                }
            }
            IMGEE_TEST_CHECK(sawBg && sawTxt);
            // Pill background must sit on line 2 (>= line1BottomY - 2.0f) near polishLeftX, matching wrapped text:
            IMGEE_TEST_CHECK(bgMin.y >= line1BottomY - 2.0f);
            IMGEE_TEST_CHECK(std::fabs(bgMin.x - polishLeftX) <= 6.0f);
            IMGEE_TEST_CHECK(std::fabs(bgMin.y - txtMin.y) <= 5.0f);
            IMGEE_TEST_CHECK(bgMin.x <= txtMin.x + 1.0f && bgMax.x >= txtMax.x - 1.0f);
        }

        w3PolishSnap.VerifyBalanced("Wave3PeerReviewPolishWin");
        ImGui::End();

        // 3b. Tall floated image + > [!NOTE] callout in a medium-narrow window where < 220px * scale
        //     remains beside the float: callout auto-clears below the float (zero X/Y overlap):
        ImGui::SetNextWindowPos(ImVec2(680.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(310.0f, 420.0f), ImGuiCond_Always);
        ImGui::Begin("Wave3CalloutClearNarrowWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        const std::string narrowFloatCalloutDoc =
            "![TallNarrowFl](test_logo){width=120 height=90 float=left gap=12x4}\n\n"
            "> [!NOTE]\n"
            "> Callout clears cleanly below the float when remaining width is below 220px.\n";
        MD::TextWrapped(narrowFloatCalloutDoc, &mdCfg);

        ImVec2 narrowFlMin, narrowFlMax;
        ImGui::GetLastMarkdownImageRect(&narrowFlMin, &narrowFlMax);
        const auto& narrowCalloutLines = ImGui::GetLastMarkdownRenderedLines();
        IMGEE_TEST_CHECK(!narrowCalloutLines.empty());
        for (size_t li = 0; li < narrowCalloutLines.size(); ++li) {
            IMGEE_TEST_CHECK(narrowCalloutLines[li].min.y >= narrowFlMax.y - 0.5f);
        }
        ImGui::End();

        ImGui::Render();
        CompleteHeadlessRenderTextures();
    }

    // ========================================================================
    // Zero-Bias Vector Icon & Fallback Glyph Symmetry Regression Tests:
    //  Verifies that symmetric procedural icons (info, important, warning,
    //  caution, lightbulb, plus, cross, plane) and standalone fallback callout
    //  glyphs emit horizontally symmetric VtxBuffer extents AND per-vertex
    //  mirror geometry around center.x within < 0.05f px (catching any +0.5f
    //  ImDrawList::AddLine bias immediately, even on inner stems!).
    // ========================================================================
    {
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f), ImGuiCond_Always);
        ImGui::Begin("ZeroBiasVectorSymmetryWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        auto verifySymmetricVertices = [&](int vStart, int vEnd, float centerX, const char* label) {
            IMGEE_TEST_CHECK(vEnd > vStart);
            float minX = 1e9f;
            float maxX = -1e9f;
            double sumDx = 0.0;
            for (int vi = vStart; vi < vEnd; ++vi) {
                const float vx = dl->VtxBuffer[vi].pos.x;
                if (vx < minX) minX = vx;
                if (vx > maxX) maxX = vx;
                sumDx += static_cast<double>(vx - centerX);
            }
            const float leftExtent = centerX - minX;
            const float rightExtent = maxX - centerX;
            const float extentAsym = std::fabs(leftExtent - rightExtent);
            const float meanDx = static_cast<float>(sumDx / static_cast<double>(vEnd - vStart));
            if (extentAsym >= 0.05f || std::fabs(meanDx) >= 0.05f) {
                std::fprintf(stderr,
                    "[FAIL] Asymmetric icon '%s': left=%.4f right=%.4f extentAsym=%.4f meanDx=%.4f\n",
                    label, leftExtent, rightExtent, extentAsym, meanDx);
            }
            IMGEE_TEST_CHECK(extentAsym < 0.05f);
            IMGEE_TEST_CHECK(std::fabs(meanDx) < 0.05f);

            // Also verify every vertex (including inner 'i' / '!' stems) has a horizontal mirror partner:
            for (int vi = vStart; vi < vEnd; ++vi) {
                const ImVec2 p = dl->VtxBuffer[vi].pos;
                const float targetX = 2.0f * centerX - p.x;
                bool foundMirror = false;
                for (int vj = vStart; vj < vEnd; ++vj) {
                    const ImVec2 q = dl->VtxBuffer[vj].pos;
                    if (std::fabs(q.x - targetX) < 0.05f && std::fabs(q.y - p.y) < 0.05f) {
                        foundMirror = true;
                        break;
                    }
                }
                if (!foundMirror) {
                    std::fprintf(stderr,
                        "[FAIL] Missing mirror vertex in '%s' for (%.4f, %.4f) around centerX=%.4f\n",
                        label, p.x, p.y, centerX);
                }
                IMGEE_TEST_CHECK(foundMirror);
            }
        };

        const char* kSymmetricIcons[] = {
            "info", "important", "warning", "caution", "lightbulb", "plus", "cross", "plane"
        };
        const float iconSz = 16.0f;
        const ImU32 iconCol = IM_COL32(240, 245, 255, 255);
        for (size_t idx = 0; idx < sizeof(kSymmetricIcons) / sizeof(kSymmetricIcons[0]); ++idx) {
            const ImVec2 iconPos(40.0f + static_cast<float>(idx) * 28.0f, 50.0f);
            const float cx = iconPos.x + iconSz * 0.5f;
            const int v0 = dl->VtxBuffer.Size;
            const bool drew = ImGui::DrawVectorIcon(dl, iconPos, iconSz, iconCol, kSymmetricIcons[idx]);
            IMGEE_TEST_CHECK(drew);
            verifySymmetricVertices(v0, dl->VtxBuffer.Size, cx, kSymmetricIcons[idx]);
        }

        // Also verify standalone fallback callout glyphs (Note, Important, Warning, Caution):
        const ImGui::Detail::CalloutKind kSymmetricFallbacks[] = {
            ImGui::Detail::CalloutKind::Note,
            ImGui::Detail::CalloutKind::Important,
            ImGui::Detail::CalloutKind::Warning,
            ImGui::Detail::CalloutKind::Caution
        };
        const char* kFallbackLabels[] = {
            "fallback:note", "fallback:important", "fallback:warning", "fallback:caution"
        };
        for (size_t idx = 0; idx < 4; ++idx) {
            const ImVec2 gPos(40.0f + static_cast<float>(idx) * 28.0f, 90.0f);
            const float cx = gPos.x + iconSz * 0.5f;
            const int v0 = dl->VtxBuffer.Size;
            ImGui::Detail::DrawMinimalCalloutGlyph(dl, gPos, iconSz, iconCol, kSymmetricFallbacks[idx]);
            verifySymmetricVertices(v0, dl->VtxBuffer.Size, cx, kFallbackLabels[idx]);
        }

        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();
    }

    // ========================================================================
    // Read-Only Indicator TimedTooltip Click-to-Toggle Regression Tests:
    //  Verifies multi-frame held-click toggle ON -> stay open across release ->
    //  dismiss on second click across ActivitySpinner, ProgressDial,
    //  CountdownDial, Badge, Kbd, and Icon (inside a movable window where
    //  Badge / Kbd trigger ActiveId == MoveId during multi-frame holds), while
    //  confirming ButtonMD("<icon undo /> <badge>OK</badge> Reset") preserves
    //  standard action-button tooltip behavior without falsely auto-toggling.
    // ========================================================================
    {
        auto setMouseInput = [&](float mx, float my, bool down) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddMousePosEvent(mx, my);
            io.AddMouseButtonEvent(0, down);
#else
            io.MousePos = ImVec2(mx, my);
            io.MouseDown[0] = down;
#endif
        };

        ImGui::TimedTooltip::TooltipConfig roTipCfg;
        roTipCfg.hover_delay_cycles = 60;
        roTipCfg.grace_period_cycles = 3;

        ImVec2 itemCenters[7];
        bool tipShown[7] = { false, false, false, false, false, false, false };
        bool btnMdActivated = false;

        auto renderIndicatorFrame = [&](bool captureCenters = false, ImGuiMouseCursor* outCursor = nullptr) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();

            // Movable window (no ImGuiWindowFlags_NoMove) so non-ButtonBehavior items
            // (Badge / Kbd) assign ActiveId = MoveId on mouse-down!
            ImGui::SetNextWindowPos(ImVec2(25.0f, 25.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(480.0f, 360.0f), ImGuiCond_Always);
            ImGui::Begin("ReadOnlyIndicatorTooltipToggleWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot roSnap = StackSnapshot::Capture();

            auto captureCenter = [&](int slot) {
                if (captureCenters) {
                    const ImVec2 mn = ImGui::GetItemRectMin();
                    const ImVec2 mx = ImGui::GetItemRectMax();
                    itemCenters[slot] = ImVec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
                }
            };

            ImGui::ActivitySpinner("##ro_spin", 9.0f);
            captureCenter(0);
            tipShown[0] = ImGui::TimedTooltip::Text(&roTipCfg, "ActivitySpinner tooltip");

            ImGui::ProgressDial("##ro_prog", 0.65f, 9.0f);
            captureCenter(1);
            tipShown[1] = ImGui::TimedTooltip::TextMD(&roTipCfg, "**ProgressDial** tooltip");

            ImGui::CountdownDial("##ro_cnt", 12.0f, 30.0f, 9.0f);
            captureCenter(2);
            tipShown[2] = ImGui::TimedTooltip::Text(&roTipCfg, "CountdownDial tooltip");

            ImGui::Badge("NAV");
            captureCenter(3);
            tipShown[3] = ImGui::TimedTooltip::TextMD(&roTipCfg, "**Badge** tooltip");

            ImGui::Kbd("Ctrl+G");
            captureCenter(4);
            tipShown[4] = ImGui::TimedTooltip::Text(&roTipCfg, "Kbd shortcut tooltip");

            ImGui::Icon("info", 16.0f);
            captureCenter(5);
            tipShown[5] = ImGui::TimedTooltip::TextMD(&roTipCfg, "**Icon** tooltip");

            if (ImGui::MD::Button("<icon undo /> <badge>OK</badge> Reset")) {
                btnMdActivated = true;
            }
            captureCenter(6);
            tipShown[6] = ImGui::TimedTooltip::TextMD(&roTipCfg, "**Reset** button tooltip");

            if (outCursor) {
                *outCursor = ImGui::GetMouseCursor();
            }

            roSnap.VerifyBalanced("ReadOnlyIndicatorTooltipToggleWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        // Discover item centers with mouse parked away from the window:
        setMouseInput(2.0f, 2.0f, false);
        renderIndicatorFrame(true);

        const char* kIndicatorNames[6] = {
            "ActivitySpinner", "ProgressDial", "CountdownDial", "Badge", "Kbd", "Icon"
        };
        for (int idx = 0; idx < 6; ++idx) {
            const ImVec2 c = itemCenters[idx];
            ImGuiMouseCursor cur = ImGuiMouseCursor_Arrow;

            // 1. Brief hover (< hover_delay_cycles == 60): Hand cursor shown, tooltip not yet visible
            setMouseInput(c.x, c.y, false);
            renderIndicatorFrame(false, &cur);
            renderIndicatorFrame(false, &cur);
            IMGEE_TEST_CHECK(cur == ImGuiMouseCursor_Hand);
            IMGEE_TEST_CHECK(!tipShown[idx]);

            // 2. Click and hold across 6 frames (> grace_period_cycles == 3):
            //    Must pop immediately on Frame 1 AND stay displayed across all held frames!
            for (int hf = 0; hf < 6; ++hf) {
                setMouseInput(c.x, c.y, true);
                renderIndicatorFrame();
                if (!tipShown[idx]) {
                    std::fprintf(stderr, "[FAIL] '%s' tooltip dropped on held frame %d\n", kIndicatorNames[idx], hf);
                }
                IMGEE_TEST_CHECK(tipShown[idx]);
            }

            // 3. Release mouse button and hover 2 frames: tooltip stays displayed!
            setMouseInput(c.x, c.y, false);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(tipShown[idx]);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(tipShown[idx]);

            // 4. Second multi-frame click dismisses the tooltip immediately and keeps it closed:
            setMouseInput(c.x, c.y, true);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(!tipShown[idx]);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(!tipShown[idx]);
            setMouseInput(c.x, c.y, false);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(!tipShown[idx]);
        }

        // Verify ButtonMD("<icon undo /> <badge>OK</badge> Reset") does NOT falsely auto-toggle
        // its trailing action-button tooltip on click before hover_delay_cycles:
        {
            const ImVec2 bc = itemCenters[6];
            btnMdActivated = false;
            setMouseInput(bc.x, bc.y, false);
            renderIndicatorFrame();
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(!tipShown[6]);

            for (int hf = 0; hf < 4; ++hf) {
                setMouseInput(bc.x, bc.y, true);
                renderIndicatorFrame();
                IMGEE_TEST_CHECK(!tipShown[6]);
            }
            setMouseInput(bc.x, bc.y, false);
            renderIndicatorFrame();
            IMGEE_TEST_CHECK(btnMdActivated);
            IMGEE_TEST_CHECK(!tipShown[6]);
        }

        setMouseInput(2.0f, 2.0f, false);
        renderIndicatorFrame();
    }

    // ========================================================================
    // SelectPulldown Open-Button Cursor & Mouse-Wheel Consistency Tests:
    //  1. Closed + hovering opener button: Hand cursor, wheel pass-through
    //     (neither mutates *current_item nor consumes io.MouseWheel).
    //  2. Open + hovering opener button: ResizeNS cursor, wheel steps
    //     *current_item in place ("click to unlock, then scroll in place").
    //  3. Open + hovering popup list item: ResizeNS cursor, wheel steps
    //     *current_item; closing restores Hand cursor on opener button.
    // ========================================================================
    {
        auto setMouseAndWheel = [&](float mx, float my, bool down, float wheelY = 0.0f) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
            io.AddMousePosEvent(mx, my);
            io.AddMouseButtonEvent(0, down);
            if (wheelY != 0.0f) {
                io.AddMouseWheelEvent(0.0f, wheelY);
            }
#else
            io.MousePos = ImVec2(mx, my);
            io.MouseDown[0] = down;
            io.MouseWheel = wheelY;
#endif
        };

        static const char* const kModes[4] = { "Circle", "Spot", "Tower", "Chase" };
        int selectedMode = 0;
        ImVec2 btnMin(0.0f, 0.0f), btnMax(0.0f, 0.0f);
        ImGuiMouseCursor observedCursor = ImGuiMouseCursor_Arrow;
        float unconsumedWheelAfterWidget = 0.0f;
        bool stepChanged = false;

        auto renderPulldownFrame = [&](bool captureBtnRect = false) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(30.0f, 30.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(360.0f, 280.0f), ImGuiCond_Always);
            ImGui::Begin("SelectPulldownCursorWheelWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot pdSnap = StackSnapshot::Capture();

            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            stepChanged = ImGui::SelectPulldown("##cam_mode", &selectedMode, kModes, 4, nullptr, 150.0f);
            if (captureBtnRect) {
                btnMin = p0;
                btnMax = ImVec2(p0.x + 150.0f, p0.y + ImGui::GetFrameHeight());
            }
            unconsumedWheelAfterWidget = io.MouseWheel;
            observedCursor = ImGui::GetMouseCursor();

            pdSnap.VerifyBalanced("SelectPulldownCursorWheelWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        setMouseAndWheel(2.0f, 2.0f, false, 0.0f);
        renderPulldownFrame(true);
        const ImVec2 btnCenter((btnMin.x + btnMax.x) * 0.5f, (btnMin.y + btnMax.y) * 0.5f);

        // State 1: Closed + hovering opener button -> Hand cursor & wheel pass-through!
        setMouseAndWheel(btnCenter.x, btnCenter.y, false, 0.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_Hand);

        setMouseAndWheel(btnCenter.x, btnCenter.y, false, -1.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_Hand);
        IMGEE_TEST_CHECK(!stepChanged);
        IMGEE_TEST_CHECK(selectedMode == 0);
        IMGEE_TEST_CHECK(std::fabs(unconsumedWheelAfterWidget - (-1.0f)) < 0.01f);

        // Click + release opener button to unlock/open the pulldown without moving the mouse:
        setMouseAndWheel(btnCenter.x, btnCenter.y, true, 0.0f);
        renderPulldownFrame();
        setMouseAndWheel(btnCenter.x, btnCenter.y, false, 0.0f);
        renderPulldownFrame();

        // State 2: Open + hovering opener button -> ResizeNS cursor & wheel steps *current_item!
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_ResizeNS);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_ResizeNS);

        setMouseAndWheel(btnCenter.x, btnCenter.y, false, -1.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_ResizeNS);
        IMGEE_TEST_CHECK(stepChanged);
        IMGEE_TEST_CHECK(selectedMode == 1);
        IMGEE_TEST_CHECK(std::fabs(unconsumedWheelAfterWidget) < 0.01f);

        // State 3: Open + hovering popup item below opener button -> ResizeNS cursor & wheel steps *current_item!
        const ImVec2 popupItemPos(btnCenter.x, btnMax.y + ImGui::GetTextLineHeightWithSpacing() * 1.2f);
        setMouseAndWheel(popupItemPos.x, popupItemPos.y, false, 0.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_ResizeNS);

        setMouseAndWheel(popupItemPos.x, popupItemPos.y, false, -1.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_ResizeNS);
        IMGEE_TEST_CHECK(stepChanged);
        IMGEE_TEST_CHECK(selectedMode == 2);
        IMGEE_TEST_CHECK(std::fabs(unconsumedWheelAfterWidget) < 0.01f);

        // Close popup by clicking outside, then re-hover opener button -> returns to Hand cursor!
        setMouseAndWheel(320.0f, 240.0f, true, 0.0f);
        renderPulldownFrame();
        setMouseAndWheel(320.0f, 240.0f, false, 0.0f);
        renderPulldownFrame();

        setMouseAndWheel(btnCenter.x, btnCenter.y, false, 0.0f);
        renderPulldownFrame();
        IMGEE_TEST_CHECK(observedCursor == ImGuiMouseCursor_Hand);
    }

    // =========================================================================
    // StyledText <color=#hex> Delimiter & Standalone DocumentViewer Suite
    // =========================================================================
    {
        // 1. Verify <color=#hex>...</color> and <col=#hex>...</col> parse tagName cleanly without space:
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(560.0f, 220.0f), ImGuiCond_Always);
        ImGui::Begin("StyledTextColorEqWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
        StackSnapshot stSnap = StackSnapshot::Capture();

        ImDrawList* stDl = ImGui::GetWindowDrawList();
        const int vtxBefore = stDl->VtxBuffer.Size;
        ImGui::StyledText("<color=#F5C469><b>Crosswind Advisory:</b></color> <col=#66B8FF>210@18KT</col>");
        const int vtxAfter = stDl->VtxBuffer.Size;
        IMGEE_TEST_CHECK(vtxAfter > vtxBefore);

        // Verify amber vertex color (#F5C469 -> R=0xF5, G=0xC4, B=0x69) was pushed onto rendered glyphs
        const ImU32 expectedAmber = IM_COL32(0xF5, 0xC4, 0x69, 0xFF);
        bool sawAmberGlyph = false;
        for (int vi = vtxBefore; vi < vtxAfter; ++vi) {
            if (stDl->VtxBuffer[vi].col == expectedAmber) {
                sawAmberGlyph = true;
                break;
            }
        }
        IMGEE_TEST_CHECK(sawAmberGlyph);

        stSnap.VerifyBalanced("StyledTextColorEqWin");
        ImGui::End();
        ImGui::Render();
        CompleteHeadlessRenderTextures();

        // 2. Verify ParseMarkdownToc & MakeMarkdownHeadingSlug (skipping code fences & > callouts, deduplicating slugs):
        const char* kSampleManual =
            "# Flight Deck Operations Manual\n\n"
            "Introductory overview with [Jump to Emergency](#3-emergency-procedures).\n\n"
            "```python\n"
            "# Not a heading inside backtick fence!\n"
            "## Neither is this!\n"
            "```\n\n"
            "~~~bash\n"
            "# Not a heading inside tilde fence!\n"
            "~~~\n\n"
            "> [!NOTE]\n"
            "> # Heading inside blockquote should be ignored in top-level TOC\n\n"
            "## 1. Normal Procedures ##\n\n"
            "Paragraph line 1.\n\nParagraph line 2.\n\nParagraph line 3.\n\n"
            "### Preflight Inspection\n\n"
            "Detailed preflight items.\n\n\n\n"
            "### Preflight Inspection\n\n"
            "Duplicate heading name verifying -1 slug suffix.\n\n\n\n"
            "#### Deep H4 Excluded at Default Level 3\n\n"
            "## 2. Autopilot Modes\n\n"
            "Lateral and vertical guidance modes.\n\n"
            "Paragraph A.\n\nParagraph B.\n\nParagraph C.\n\nParagraph D.\n\nParagraph E.\n\n"
            "Paragraph F.\n\nParagraph G.\n\nParagraph H.\n\nParagraph I.\n\nParagraph J.\n\n"
            "## 3. Emergency Procedures\n\n"
            "Engine fire and dual generator failure memory items.\n\n"
            "Final paragraph.\n";

        std::vector<ImGui::DocumentTocEntry> parsedToc;
        std::string firstH1;
        ImGui::ParseMarkdownToc(kSampleManual, std::strlen(kSampleManual), parsedToc, 3, &firstH1);
        IMGEE_TEST_CHECK(firstH1 == "Flight Deck Operations Manual");
        IMGEE_TEST_CHECK(parsedToc.size() == 6);
        if (parsedToc.size() == 6) {
            IMGEE_TEST_CHECK(parsedToc[0].level == 1 && parsedToc[0].slug == "flight-deck-operations-manual");
            IMGEE_TEST_CHECK(parsedToc[1].level == 2 && parsedToc[1].title == "1. Normal Procedures" && parsedToc[1].slug == "1-normal-procedures");
            IMGEE_TEST_CHECK(parsedToc[2].level == 3 && parsedToc[2].slug == "preflight-inspection");
            IMGEE_TEST_CHECK(parsedToc[3].level == 3 && parsedToc[3].slug == "preflight-inspection-1");
            IMGEE_TEST_CHECK(parsedToc[4].level == 2 && parsedToc[4].slug == "2-autopilot-modes");
            IMGEE_TEST_CHECK(parsedToc[5].level == 2 && parsedToc[5].slug == "3-emergency-procedures");
        }

        // 3. Verify ApplyDocumentViewerTheme preserves window_opacity when transparent_hud is enabled,
        //    and enforces 100% solid WindowBg.w == 1.0f & Border.w == 1.0f across all 5 presets when false:
        ImGuiStyle savedStyle = ImGui::GetStyle();
        ImGui::ApplyDocumentViewerTheme(ImGui::DocumentViewerTheme_Rusty, true, 0.72f);
        IMGEE_TEST_CHECK(std::fabs(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w - 0.72f) <= 0.005f);
        ImGui::ApplyDocumentViewerTheme(ImGui::DocumentViewerTheme_Light, true, 0.65f);
        IMGEE_TEST_CHECK(std::fabs(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w - 0.65f) <= 0.005f);
        for (int s6ThemeIdx = 0; s6ThemeIdx < ImGui::DocumentViewerTheme_COUNT; ++s6ThemeIdx) {
            ImGui::ApplyDocumentViewerTheme(s6ThemeIdx, false, 0.65f);
            IMGEE_TEST_CHECK(std::fabs(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w - 1.0f) <= 0.001f);
            IMGEE_TEST_CHECK(std::fabs(ImGui::GetStyle().Colors[ImGuiCol_Border].w - 1.0f) <= 0.001f);
        }
        ImGui::GetStyle() = savedStyle;

        // 4. Multi-frame headless DocumentViewer render, zoom A/B toggle, & anchor jump verification:
        ImGui::DocumentViewerState docState;
        docState.viewport.SetZoom(1.25f);
        ImGui::DocumentViewerConfig docCfg;
        docCfg.showFitButton         = true;
        docCfg.showCursorToggle      = true;
        docCfg.showPopOutButton      = true;
        docCfg.showVrButton          = true;
        docCfg.showCloseButton       = true;
        docCfg.showReloadButton      = true;
        docCfg.showFooterCloseButton = true;

        auto renderViewerFrame = [&]() {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(720.0f, 260.0f), ImGuiCond_Always);
            ImGui::Begin("DocViewerSmokeWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot dvSnap = StackSnapshot::Capture();

            const bool stillOpen = ImGui::DocumentViewer("##smoke_manual", kSampleManual, docState, &docCfg);
            IMGEE_TEST_CHECK(stillOpen);
            // Parent window font scale must remain strictly 1.0x after DocumentViewer returns!
            IMGEE_TEST_CHECK(std::fabs(ImGui::GetCurrentWindow()->FontWindowScale - 1.0f) <= 0.001f);

            dvSnap.VerifyBalanced("DocViewerSmokeWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        };

        renderViewerFrame();
        renderViewerFrame();
        IMGEE_TEST_CHECK(docState.toc.size() == 6);
        IMGEE_TEST_CHECK(docState.toc[5].content_y > docState.toc[1].content_y + 80.0f);

        // Verify zoom A/B reset & restore memory:
        IMGEE_TEST_CHECK(docState.viewport.ToggleResetZoom());
        IMGEE_TEST_CHECK(std::fabs(docState.viewport.zoom - 1.0f) <= 0.001f);
        IMGEE_TEST_CHECK(docState.viewport.ToggleResetZoom());
        IMGEE_TEST_CHECK(std::fabs(docState.viewport.zoom - 1.25f) <= 0.001f);

        // Verify JumpToDocumentAnchor("#3-emergency-procedures") scrolls inner viewport down to section #5:
        IMGEE_TEST_CHECK(ImGui::JumpToDocumentAnchor(docState, "#3-emergency-procedures"));
        renderViewerFrame();
        renderViewerFrame();
        IMGEE_TEST_CHECK(docState.last_scroll_y > 50.0f);
        IMGEE_TEST_CHECK(docState.active_toc_index >= 4);

        // Verify 1-click Scroll-to-Top & Return-to-Reading-Spot (ToggleDocumentScrollReset):
        const float savedReadingY = docState.last_scroll_y;
        IMGEE_TEST_CHECK(ImGui::ToggleDocumentScrollReset(docState));
        renderViewerFrame();
        renderViewerFrame();
        IMGEE_TEST_CHECK(docState.last_scroll_y <= 1.0f);
        IMGEE_TEST_CHECK(std::fabs(docState.saved_scroll_y - savedReadingY) <= 0.5f);
        IMGEE_TEST_CHECK(docState.active_toc_index == 0);

        // 1-click return to reading spot restores exact savedReadingY:
        IMGEE_TEST_CHECK(ImGui::ToggleDocumentScrollReset(docState));
        renderViewerFrame();
        renderViewerFrame();
        IMGEE_TEST_CHECK(std::fabs(docState.last_scroll_y - savedReadingY) <= 1.0f);
        IMGEE_TEST_CHECK(docState.active_toc_index >= 4);

        // Jump back to top heading (#0):
        IMGEE_TEST_CHECK(ImGui::JumpToDocumentHeading(docState, 0));
        renderViewerFrame();
        renderViewerFrame();
        IMGEE_TEST_CHECK(docState.last_scroll_y <= 5.0f);
        IMGEE_TEST_CHECK(docState.active_toc_index == 0);

        // 5. Verify HTML <img src="..." align="right|left"> float parity & <br clear="all"/> termination:
        {
            const auto htmlRight = ImGui::MarkdownImageService::ParseMarkdownImageModifiers(
                "src=\"imgee_ci_test_pixel.png\" align=\"right\" width=\"380\" alt=\"Toggle Card\"");
            IMGEE_TEST_CHECK(htmlRight.clean_url == "imgee_ci_test_pixel.png");
            IMGEE_TEST_CHECK(htmlRight.float_mode == ImGui::MarkdownImageFloat::Right);
            IMGEE_TEST_CHECK(std::fabs(htmlRight.width - 380.0f) <= 0.01f);
            IMGEE_TEST_CHECK(htmlRight.alt_text == "Toggle Card");

            const auto htmlLeft = ImGui::MarkdownImageService::ParseMarkdownImageModifiers(
                "src=\"imgee_ci_test_pixel.png\" align=\"left\" width=\"360\"");
            IMGEE_TEST_CHECK(htmlLeft.float_mode == ImGui::MarkdownImageFloat::Left);

            // Explicit nofloat / float=none overrides HTML align="right":
            const auto htmlNoFloat = ImGui::MarkdownImageService::ParseMarkdownImageModifiers(
                "src=\"imgee_ci_test_pixel.png\" align=\"right\" nofloat width=\"380\"");
            IMGEE_TEST_CHECK(htmlNoFloat.float_mode == ImGui::MarkdownImageFloat::None);
            IMGEE_TEST_CHECK(htmlNoFloat.align == ImGui::MarkdownImageAlign::Right);

            // Standard Markdown angle-bracket ![alt](<url align=right>) keeps non-floating block alignment:
            const auto mdBlockRight = ImGui::MarkdownImageService::ParseMarkdownImageModifiers(
                "<imgee_ci_test_pixel.png align=right width=200>");
            IMGEE_TEST_CHECK(mdBlockRight.float_mode == ImGui::MarkdownImageFloat::None);
            IMGEE_TEST_CHECK(mdBlockRight.align == ImGui::MarkdownImageAlign::Right);

            // Live render: <img ... align="right" .../> wraps adjacent prose and <br clear="all"/> clears past float bottom:
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(520.0f, 300.0f), ImGuiCond_Always);
            ImGui::Begin("HtmlImgFloatWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot hfSnap = StackSnapshot::Capture();

            const float startY = ImGui::GetCursorScreenPos().y;
            const std::string htmlFloatMd =
                "<img src=\"test_logo\" align=\"right\" width=\"140\" height=\"110\" alt=\"Compact Card\"/>\n\n"
                "Short wrapped text beside right-floated HTML image.\n\n"
                "<br clear=\"all\"/>";
            ImGui::MD::TextWrapped(htmlFloatMd);
            const float endY = ImGui::GetCursorScreenPos().y;
            ImVec2 imgMin(0.0f, 0.0f), imgMax(0.0f, 0.0f);
            ImGui::GetLastMarkdownImageRect(&imgMin, &imgMax);
            // Right-floated image sits near the right edge of the 520px window while <br clear="all"/> advances Y >= 110px:
            IMGEE_TEST_CHECK(imgMin.x > 300.0f);
            IMGEE_TEST_CHECK(endY >= startY + 110.0f);

            hfSnap.VerifyBalanced("HtmlImgFloatWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();
        }

        // 6. Verify Small caption height (< Regular), float-scope isolation on captions beneath floated figures,
        //    balanced wide-window Lightbox caption reading width, and Ouroboros Easter-egg vector icons:
        {
            // 6a. Ouroboros Easter-egg aliases, raw 4-byte UTF-8 glyphs, & DrawVectorIcon / MD / StyledText vertex emission:
            IMGEE_TEST_CHECK(ImGui::NormalizeIconName("\xF0\x9F\x90\x89") == "ouroboros");
            IMGEE_TEST_CHECK(ImGui::NormalizeIconName("\xF0\x9F\x90\xB2") == "ouroboros-alt");
            IMGEE_TEST_CHECK(ImGui::NormalizeIconName(":ouroboros:") == "ouroboros");
            IMGEE_TEST_CHECK(ImGui::NormalizeIconName("gee-crest") == "ouroboros");
            IMGEE_TEST_CHECK(ImGui::NormalizeIconName("dragon-alt") == "ouroboros-alt");
            IMGEE_TEST_CHECK(ImGui::IsVectorIcon("\xF0\x9F\x90\x89"));
            IMGEE_TEST_CHECK(ImGui::IsVectorIcon("\xF0\x9F\x90\xB2"));
            IMGEE_TEST_CHECK(ImGui::IsVectorIcon(":ouroboros:"));
            IMGEE_TEST_CHECK(ImGui::IsVectorIcon("ouroboros-alt"));

            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(640.0f, 420.0f), ImGuiCond_Always);
            ImGui::Begin("CaptionAndOuroborosSmokeWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
            StackSnapshot s6Snap = StackSnapshot::Capture();

            ImDrawList* s6Dl = ImGui::GetWindowDrawList();
            const int s6Vtx0 = s6Dl->VtxBuffer.Size;
            IMGEE_TEST_CHECK(ImGui::DrawVectorIcon(s6Dl, ImGui::GetCursorScreenPos(), 18.0f, IM_COL32_WHITE, "\xF0\x9F\x90\x89"));
            const int s6Vtx1 = s6Dl->VtxBuffer.Size;
            IMGEE_TEST_CHECK(s6Vtx1 > s6Vtx0);
            IMGEE_TEST_CHECK(ImGui::DrawVectorIcon(s6Dl, ImVec2(ImGui::GetCursorScreenPos().x + 24.0f, ImGui::GetCursorScreenPos().y), 18.0f, IM_COL32_WHITE, "\xF0\x9F\x90\xB2"));
            const int s6Vtx2 = s6Dl->VtxBuffer.Size;
            IMGEE_TEST_CHECK(s6Vtx2 > s6Vtx1);
            ImGui::Dummy(ImVec2(48.0f, 20.0f));

            const int s6VtxMd0 = s6Dl->VtxBuffer.Size;
            ImGui::MD::Text("\xF0\x9F\x90\x89 :ouroboros-alt: `<icon gee-crest/>`");
            IMGEE_TEST_CHECK(s6Dl->VtxBuffer.Size > s6VtxMd0);

            const int s6VtxSt0 = s6Dl->VtxBuffer.Size;
            ImGui::StyledText("Crest \xF0\x9F\x90\x89 \xF0\x9F\x90\xB2");
            IMGEE_TEST_CHECK(s6Dl->VtxBuffer.Size > s6VtxSt0);

            // 6b. Inline figure caption height (including **bold** and `code`) is strictly smaller than Regular line height:
            const float s6RegLineH = ImGui::GetTextLineHeight();
            ImGui::MD::TextWrapped("![**Fig 1:** Compact `V_MO` caption](test_logo){width=240 height=60 caption}");
            ImVec2 s6CapMin(0.0f, 0.0f), s6CapMax(0.0f, 0.0f);
            ImGui::GetLastMarkdownCaptionRect(&s6CapMin, &s6CapMax);
            const float s6CapH = s6CapMax.y - s6CapMin.y;
            IMGEE_TEST_CHECK(s6CapH > 4.0f && s6CapH < s6RegLineH - 0.25f);

            // 6c. Float-scope non-interference: active left float must NOT shift or narrow a right-floated figure's caption:
            const std::string s6DualFloatMd =
                "![Left Card](test_logo){width=160 height=120 float=left}\n\n"
                "![Right Card Multi-Word Wrapped Caption Text Under Figure](test_logo){width=160 height=70 float=right caption}\n\n"
                "<br clear=\"all\"/>";
            ImGui::MD::TextWrapped(s6DualFloatMd);
            ImVec2 s6RightImgMin(0.0f, 0.0f), s6RightImgMax(0.0f, 0.0f);
            ImVec2 s6RightCapMin(0.0f, 0.0f), s6RightCapMax(0.0f, 0.0f);
            ImGui::GetLastMarkdownImageRect(&s6RightImgMin, &s6RightImgMax);
            ImGui::GetLastMarkdownCaptionRect(&s6RightCapMin, &s6RightCapMax);
            IMGEE_TEST_CHECK(s6RightCapMin.x >= s6RightImgMin.x - 1.5f);
            IMGEE_TEST_CHECK(s6RightCapMax.x <= s6RightImgMax.x + 1.5f);

            s6Snap.VerifyBalanced("CaptionAndOuroborosSmokeWin");
            ImGui::End();
            ImGui::Render();
            CompleteHeadlessRenderTextures();

            // 6d. Balanced Lightbox caption reading measure on wide windows (clamped to ~0.68 * avail_lb_w):
            const std::string s6WideLbMd =
                "![Wide Window Lightbox](test_logo){width=120 height=80 lightbox caption=\""
                "**Balanced Reading Measure:** On wide desktop viewports, lightbox captions wrap comfortably around "
                "sixty-eight percent of the available overlay width so multi-clause technical descriptions form "
                "even, readable lines instead of stretching edge-to-edge with a two-word orphan tail.\"}";
            ImVec2 s6ThumbCenter(0.0f, 0.0f);
            auto renderWideLbFrame = [&](bool captureThumb) {
                ImGui::NewFrame();
                ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(1100.0f, 480.0f), ImGuiCond_Always);
                ImGui::Begin("WideLightboxCaptionMeasureWin", nullptr, ImGuiWindowFlags_NoSavedSettings);
                ImGui::MD::TextWrapped(s6WideLbMd);
                if (captureThumb) {
                    ImVec2 mMin(0.0f, 0.0f), mMax(0.0f, 0.0f);
                    ImGui::GetLastMarkdownImageRect(&mMin, &mMax);
                    s6ThumbCenter = ImVec2((mMin.x + mMax.x) * 0.5f, (mMin.y + mMax.y) * 0.5f);
                }
                ImGui::End();
                ImGui::Render();
                CompleteHeadlessRenderTextures();
            };

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
            auto setS6Mouse = [&](float mx, float my, bool down) {
                io.AddMousePosEvent(mx, my);
                io.AddMouseButtonEvent(0, down);
            };
#else
            auto setS6Mouse = [&](float mx, float my, bool down) {
                io.MousePos = ImVec2(mx, my);
                io.MouseDown[0] = down;
            };
#endif
            setS6Mouse(5.0f, 5.0f, false);
            renderWideLbFrame(true);
            setS6Mouse(s6ThumbCenter.x, s6ThumbCenter.y, false);
            renderWideLbFrame(false);
            setS6Mouse(s6ThumbCenter.x, s6ThumbCenter.y, true);
            renderWideLbFrame(false);
            setS6Mouse(s6ThumbCenter.x, s6ThumbCenter.y, false);
            renderWideLbFrame(false);
            IMGEE_TEST_CHECK(ImGui::IsMarkdownLightboxOpen());

            ImVec2 s6PlateMin(0.0f, 0.0f), s6PlateMax(0.0f, 0.0f);
            IMGEE_TEST_CHECK(ImGui::GetActiveMarkdownLightboxCaptionPlateRect(&s6PlateMin, &s6PlateMax));
            const float s6AvailLbW = 1100.0f - 40.0f;
            const float s6TargetCapW = s6AvailLbW * 0.68f;
            const float s6PlateW = s6PlateMax.x - s6PlateMin.x;
            const float s6PlateH = s6PlateMax.y - s6PlateMin.y;
            IMGEE_TEST_CHECK(s6PlateW <= s6TargetCapW + 26.0f);
            IMGEE_TEST_CHECK(s6PlateH >= s6RegLineH * 1.45f);

            // Close via [X] close button click:
            ImVec2 s6CloseMin(0.0f, 0.0f), s6CloseMax(0.0f, 0.0f);
            ImGui::GetActiveMarkdownLightboxCloseRect(&s6CloseMin, &s6CloseMax);
            const ImVec2 s6CloseCenter((s6CloseMin.x + s6CloseMax.x) * 0.5f, (s6CloseMin.y + s6CloseMax.y) * 0.5f);
            setS6Mouse(s6CloseCenter.x, s6CloseCenter.y, true);
            renderWideLbFrame(false);
            setS6Mouse(s6CloseCenter.x, s6CloseCenter.y, false);
            renderWideLbFrame(false);
            IMGEE_TEST_CHECK(!ImGui::IsMarkdownLightboxOpen());
        }
    }

    // Verify ImGee::Shutdown() purges all 4 cached GPU textures via HostBackend::DeleteTexture
    IMGEE_TEST_CHECK(gpuState.createdCount == 4);
    IMGEE_TEST_CHECK(gpuState.deletedCount == 0);
    ImGee::Shutdown();
    IMGEE_TEST_CHECK(gpuState.deletedCount == 4);

    ImGui::DestroyContext(ctx);
    std::remove(tempPngName.c_str());

    std::printf("[PASS] ImGee standalone smoke & stack-integrity suite succeeded (ImGui %s).\n", IMGUI_VERSION);
    return 0;
}
