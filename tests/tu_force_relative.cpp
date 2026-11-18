// ============================================================================
// IMGEE_FILEPATHS_FORCE_RELATIVE=1 Policy Verification
// Verifies that leading '/' on Markdown image URLs is resolved strictly as a
// relative path under baseDir / CWD rather than an absolute OS root path.
// ============================================================================

#define IMGEE_FILEPATHS_FORCE_RELATIVE 1
#include "imgee.h"
#include "imgee_test_common.h"

#include <cstdint>
#include <fstream>
#include <string>

#if IMGEE_FILEPATHS_FORCE_RELATIVE != 1
#error "IMGEE_FILEPATHS_FORCE_RELATIVE override was not honored"
#endif

static const unsigned char kForceRelPng[] = {
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

int main() {
    const std::string fileName = "imgee_force_rel_pixel.png";
    {
        std::ofstream ofs(fileName.c_str(), std::ios::binary | std::ios::trunc);
        IMGEE_TEST_CHECK(ofs.good());
        ofs.write(reinterpret_cast<const char*>(kForceRelPng), static_cast<std::streamsize>(sizeof(kForceRelPng)));
    }

    uintptr_t nextTex = 200;
    ImGee::HostBackend backend;
    backend.UserData = &nextTex;
    backend.CreateTextureRGBA8 = [](const unsigned char*, int w, int h, void* ud) -> ImTextureID {
        if (w <= 0 || h <= 0) return (ImTextureID)0;
        auto* p = static_cast<uintptr_t*>(ud);
        return (ImTextureID)((*p)++);
    };
    backend.DeleteTexture = [](ImTextureID, void*) {};
    ImGee::SetHostBackend(backend);

    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(640.0f, 480.0f);
    io.Fonts->AddFontDefault();
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* px = nullptr;
    int fw = 0, fh = 0;
    io.Fonts->GetTexDataAsRGBA32(&px, &fw, &fh);
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)1);
#endif

    ImGui::MarkdownConfig cfg;
    cfg.baseDir = ".";
    ImGui::SetDefaultMarkdownConfig(&cfg);

    // Leading '/' must strip cleanly and resolve relative to baseDir ("."):
    const std::string slashedUrl = "/" + fileName;
    ImGui::MarkdownLinkCallbackData cbData = {
        "", 0, slashedUrl.c_str(), static_cast<int>(slashedUrl.size()), nullptr, true
    };

    ImGui::NewFrame();
    ImGui::Begin("ForceRel");
    ImGui::MarkdownImageData img = ImGui::MarkdownImageService::DefaultMarkdownImageCallback(cbData);
    IMGEE_TEST_CHECK(img.isValid && img.user_texture_id != (ImTextureID)0);
    ImGui::End();
    ImGui::EndFrame();

    ImGee::Shutdown();
    ImGui::SetDefaultMarkdownConfig(nullptr);
    ImGui::DestroyContext(ctx);
    std::remove(fileName.c_str());

    std::printf("[PASS] IMGEE_FILEPATHS_FORCE_RELATIVE=1 policy test succeeded.\n");
    return 0;
}
