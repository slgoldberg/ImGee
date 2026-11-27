// ============================================================================
// ImGee Standalone Headless 2x Retina Gallery Generator (tools/render_gallery.cpp)
// ----------------------------------------------------------------------------
// Renders all 17 ImGee widget and extension showcase cards into docs/images/
// using an offscreen macOS CGL + OpenGL 3.2 Core 2x Retina FBO and native
// CoreGraphics / ImageIO PNG export -- zero OS windows, zero focus stealing,
// and zero external image-writing dependencies.
//
// Also emits docs/Gallery_Preview.md as an annotated visual contact sheet.
// ============================================================================

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include "imgee.h"
#include "imgee_demo.h"
#include "imgui_internal.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

// ----------------------------------------------------------------------------
// 1. Filesystem & CoreGraphics PNG Export Helpers
// ----------------------------------------------------------------------------

static bool FileExistsLocal(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

static void EnsureDirExists(const std::string& path) {
    if (path.empty()) return;
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        cur.push_back(path[i]);
        if (path[i] == '/' || i + 1 == path.size()) {
            if (!cur.empty() && cur != "/") {
                (void)mkdir(cur.c_str(), 0755);
            }
        }
    }
}

static void RemoveTreeFiles(const std::vector<std::string>& files, const std::vector<std::string>& dirs) {
    for (size_t i = 0; i < files.size(); ++i) {
        std::remove(files[i].c_str());
    }
    for (size_t i = 0; i < dirs.size(); ++i) {
        rmdir(dirs[i].c_str());
    }
}

static bool WriteRgbaPngTopDown(const std::string& outPath, const uint8_t* rgbaTopDown, int width, int height) {
    if (!rgbaTopDown || width <= 0 || height <= 0) return false;

    CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, outPath.c_str(), kCFStringEncodingUTF8);
    if (!cfPath) return false;
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
    CFRelease(cfPath);
    if (!url) return false;

    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    if (!colorSpace) {
        colorSpace = CGColorSpaceCreateDeviceRGB();
    }

    CGDataProviderRef provider = CGDataProviderCreateWithData(
        nullptr, rgbaTopDown, static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, nullptr);
    CGImageRef image = CGImageCreate(
        static_cast<size_t>(width),
        static_cast<size_t>(height),
        8,
        32,
        static_cast<size_t>(width) * 4u,
        colorSpace,
        static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) | static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Big),
        provider,
        nullptr,
        false,
        kCGRenderingIntentDefault);

    bool ok = false;
    if (image) {
        CFStringRef pngType = CFSTR("public.png");
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, pngType, 1, nullptr);
        if (dest) {
            // Tag 144 DPI (2x Retina) so hi-DPI viewers know physical scale too
            const double dpi = 144.0;
            CFNumberRef dpiNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &dpi);
            const void* keys[2] = { kCGImagePropertyDPIWidth, kCGImagePropertyDPIHeight };
            const void* vals[2] = { dpiNum, dpiNum };
            CFDictionaryRef props = CFDictionaryCreate(
                kCFAllocatorDefault, keys, vals, 2,
                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CGImageDestinationAddImage(dest, image, props);
            ok = CGImageDestinationFinalize(dest);
            if (props) CFRelease(props);
            if (dpiNum) CFRelease(dpiNum);
            CFRelease(dest);
        }
        CGImageRelease(image);
    }

    if (provider) CGDataProviderRelease(provider);
    if (colorSpace) CGColorSpaceRelease(colorSpace);
    CFRelease(url);
    return ok;
}

// Generates a crisp procedural high-DPI flight-envelope schematic PNG on disk
// so Scene #16 exercises ImGee::HostBackend + stb_image end-to-end!
static bool GenerateSchematicDiagramPng(const std::string& pngPath) {
    const int W = 380;
    const int H = 230;
    std::vector<uint8_t> px(static_cast<size_t>(W * H * 4), 255);

    auto putPx = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        size_t idx = static_cast<size_t>((y * W + x) * 4);
        float inv = 1.0f - a;
        px[idx + 0] = static_cast<uint8_t>(std::min(255.0f, px[idx + 0] * inv + r * a));
        px[idx + 1] = static_cast<uint8_t>(std::min(255.0f, px[idx + 1] * inv + g * a));
        px[idx + 2] = static_cast<uint8_t>(std::min(255.0f, px[idx + 2] * inv + b * a));
        px[idx + 3] = 255;
    };

    auto drawLineAA = [&](float x0, float y0, float x1, float y1, float halfW, uint8_t r, uint8_t g, uint8_t b, float alpha = 1.0f) {
        int minX = std::max(0, static_cast<int>(std::floor(std::min(x0, x1) - halfW - 2.0f)));
        int maxX = std::min(W - 1, static_cast<int>(std::ceil(std::max(x0, x1) + halfW + 2.0f)));
        int minY = std::max(0, static_cast<int>(std::floor(std::min(y0, y1) - halfW - 2.0f)));
        int maxY = std::min(H - 1, static_cast<int>(std::ceil(std::max(y0, y1) + halfW + 2.0f)));
        float dx = x1 - x0;
        float dy = y1 - y0;
        float len2 = dx * dx + dy * dy;
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px_c = x + 0.5f;
                float py_c = y + 0.5f;
                float t = (len2 > 1e-5f) ? (((px_c - x0) * dx + (py_c - y0) * dy) / len2) : 0.0f;
                t = std::max(0.0f, std::min(1.0f, t));
                float cx = x0 + t * dx;
                float cy = y0 + t * dy;
                float dist = std::hypot(px_c - cx, py_c - cy);
                float cov = std::max(0.0f, std::min(1.0f, (halfW + 0.65f) - dist));
                if (cov > 0.0f) {
                    putPx(x, y, r, g, b, cov * alpha);
                }
            }
        }
    };

    // Deep slate avionics MFD background with subtle vertical gradient
    for (int y = 0; y < H; ++y) {
        float t = static_cast<float>(y) / static_cast<float>(H - 1);
        uint8_t r = static_cast<uint8_t>(16 + t * 8.0f);
        uint8_t g = static_cast<uint8_t>(24 + t * 10.0f);
        uint8_t b = static_cast<uint8_t>(38 + t * 14.0f);
        for (int x = 0; x < W; ++x) {
            putPx(x, y, r, g, b, 1.0f);
        }
    }

    // Subtle MFD coordinate grid
    const int left = 36, right = W - 24, top = 24, bot = H - 30;
    for (int gx = left; gx <= right; gx += 40) {
        drawLineAA(static_cast<float>(gx), static_cast<float>(top), static_cast<float>(gx), static_cast<float>(bot), 0.5f, 56, 76, 108, 0.45f);
    }
    for (int gy = top; gy <= bot; gy += 32) {
        drawLineAA(static_cast<float>(left), static_cast<float>(gy), static_cast<float>(right), static_cast<float>(gy), 0.5f, 56, 76, 108, 0.45f);
    }

    // Shaded normal-law flight envelope polygon (V_LS .. V_MO, -1.0g .. +2.5g)
    for (int y = top; y <= bot; ++y) {
        float ny = 1.0f - static_cast<float>(y - top) / static_cast<float>(bot - top); // 0 bottom .. 1 top
        for (int x = left; x <= right; ++x) {
            float nx = static_cast<float>(x - left) / static_cast<float>(right - left); // 0 left .. 1 right
            float stallTop = 0.28f + 0.78f * std::pow(std::min(1.0f, nx / 0.52f), 1.45f);
            float upperLim = std::min(0.84f, stallTop);
            float lowerLim = std::max(0.18f, 0.34f - 0.24f * std::min(1.0f, nx / 0.40f));
            if (nx >= 0.16f && nx <= 0.86f && ny >= lowerLim && ny <= upperLim) {
                putPx(x, y, 42, 168, 124, 0.22f);
            }
        }
    }

    // Axes (zero-g reference line + speed/load axes)
    const float zeroGY = top + (bot - top) * 0.62f;
    drawLineAA(static_cast<float>(left), zeroGY, static_cast<float>(right), zeroGY, 0.8f, 120, 148, 188, 0.65f);
    drawLineAA(static_cast<float>(left), static_cast<float>(top), static_cast<float>(left), static_cast<float>(bot), 1.1f, 150, 175, 210, 0.9f);
    drawLineAA(static_cast<float>(left), static_cast<float>(bot), static_cast<float>(right), static_cast<float>(bot), 1.1f, 150, 175, 210, 0.9f);

    // Stall curve + +2.5g ceiling + V_MO barber pole + -1.0g floor
    float prevX = left + (right - left) * 0.16f;
    float prevY = top + (bot - top) * (1.0f - (0.28f + 0.78f * std::pow(0.16f / 0.52f, 1.45f)));
    for (int s = 1; s <= 28; ++s) {
        float nx = 0.16f + (0.52f - 0.16f) * (static_cast<float>(s) / 28.0f);
        float ny = std::min(0.84f, 0.28f + 0.78f * std::pow(nx / 0.52f, 1.45f));
        float cx = left + (right - left) * nx;
        float cy = top + (bot - top) * (1.0f - ny);
        drawLineAA(prevX, prevY, cx, cy, 1.5f, 72, 224, 156, 1.0f);
        prevX = cx;
        prevY = cy;
    }
    const float ceilY = top + (bot - top) * (1.0f - 0.84f);
    const float floorY = top + (bot - top) * (1.0f - 0.18f);
    const float vmoX = left + (right - left) * 0.86f;
    const float vlsX = left + (right - left) * 0.16f;
    drawLineAA(prevX, prevY, vmoX, ceilY, 1.6f, 72, 224, 156, 1.0f);
    drawLineAA(vmoX, ceilY, vmoX, floorY, 1.8f, 245, 166, 35, 1.0f); // Amber V_MO limit
    drawLineAA(left + (right - left) * 0.30f, floorY, vmoX, floorY, 1.5f, 72, 224, 156, 1.0f);
    drawLineAA(vlsX, top + (bot - top) * (1.0f - 0.34f), left + (right - left) * 0.30f, floorY, 1.5f, 72, 224, 156, 1.0f);
    drawLineAA(vlsX, prevY + 72.0f, vlsX, top + (bot - top) * (1.0f - 0.34f), 1.5f, 244, 92, 92, 0.95f);

    // Current aircraft trim operating point pip (cyan diamond / ring)
    const float trimX = left + (right - left) * 0.56f;
    const float trimY = zeroGY;
    for (int a = 0; a < 24; ++a) {
        float a0 = (a * 6.2831853f) / 24.0f;
        float a1 = ((a + 1) * 6.2831853f) / 24.0f;
        drawLineAA(trimX + std::cos(a0) * 6.5f, trimY + std::sin(a0) * 6.5f,
                   trimX + std::cos(a1) * 6.5f, trimY + std::sin(a1) * 6.5f, 1.3f, 96, 205, 255, 1.0f);
    }
    drawLineAA(trimX - 2.0f, trimY, trimX + 2.0f, trimY, 2.0f, 240, 250, 255, 1.0f);

    // Subtle outer border
    drawLineAA(0.5f, 0.5f, W - 0.5f, 0.5f, 0.8f, 78, 96, 126, 1.0f);
    drawLineAA(W - 0.5f, 0.5f, W - 0.5f, H - 0.5f, 0.8f, 78, 96, 126, 1.0f);
    drawLineAA(W - 0.5f, H - 0.5f, 0.5f, H - 0.5f, 0.8f, 78, 96, 126, 1.0f);
    drawLineAA(0.5f, H - 0.5f, 0.5f, 0.5f, 0.8f, 78, 96, 126, 1.0f);

    return WriteRgbaPngTopDown(pngPath, px.data(), W, H);
}

// ----------------------------------------------------------------------------
// 2. Headless Offscreen CGL + OpenGL 3.2 Core 2x Retina Renderer (4x MSAA)
// ----------------------------------------------------------------------------

struct OffscreenGlContext {
    CGLContextObj cglCtx       = nullptr;
    GLuint        msaaFbo      = 0;
    GLuint        msaaColorRbo = 0;
    GLuint        resolveFbo   = 0;
    GLuint        resolveTex   = 0;
    GLuint        shaderProg   = 0;
    GLuint        vao          = 0;
    GLuint        vbo          = 0;
    GLuint        ebo          = 0;
    GLint         uProjMtx     = -1;
    GLint         uTex         = -1;
    GLuint        legacyFontTex = 0;
    int           logicalW     = 1280;
    int           logicalH     = 1100;
    int           scale        = 2;
    int           fbW          = 2560;
    int           fbH          = 2200;
    int           hostCreated  = 0;
    int           hostDeleted  = 0;

    bool Init(int logW = 1280, int logH = 1100, int retinaScale = 2) {
        logicalW = logW;
        logicalH = logH;
        scale    = retinaScale;
        fbW      = logicalW * scale;
        fbH      = logicalH * scale;

        CGLPixelFormatAttribute attrs[] = {
            kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
            kCGLPFAColorSize,     static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFAAlphaSize,     static_cast<CGLPixelFormatAttribute>(8),
            kCGLPFAAccelerated,
            static_cast<CGLPixelFormatAttribute>(0)
        };
        CGLPixelFormatObj pix = nullptr;
        GLint npix = 0;
        if (CGLChoosePixelFormat(attrs, &pix, &npix) != kCGLNoError || !pix) {
            CGLPixelFormatAttribute swAttrs[] = {
                kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
                static_cast<CGLPixelFormatAttribute>(0)
            };
            if (CGLChoosePixelFormat(swAttrs, &pix, &npix) != kCGLNoError || !pix) {
                std::fprintf(stderr, "[ERROR] CGLChoosePixelFormat failed.\n");
                return false;
            }
        }
        if (CGLCreateContext(pix, nullptr, &cglCtx) != kCGLNoError || !cglCtx) {
            CGLDestroyPixelFormat(pix);
            std::fprintf(stderr, "[ERROR] CGLCreateContext failed.\n");
            return false;
        }
        CGLDestroyPixelFormat(pix);
        CGLSetCurrentContext(cglCtx);

        // 4x MSAA render FBO + single-sample resolve FBO
        glGenFramebuffers(1, &msaaFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo);
        glGenRenderbuffers(1, &msaaColorRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, fbW, fbH);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRbo);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "[ERROR] MSAA offscreen FBO incomplete.\n");
            return false;
        }

        glGenFramebuffers(1, &resolveFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
        glGenTextures(1, &resolveTex);
        glBindTexture(GL_TEXTURE_2D, resolveTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fbW, fbH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "[ERROR] Resolve offscreen FBO incomplete.\n");
            return false;
        }

        const char* vsSrc =
            "#version 150 core\n"
            "uniform mat4 ProjMtx;\n"
            "in vec2 Position;\n"
            "in vec2 UV;\n"
            "in vec4 Color;\n"
            "out vec2 Frag_UV;\n"
            "out vec4 Frag_Color;\n"
            "void main() {\n"
            "    Frag_UV = UV;\n"
            "    Frag_Color = Color;\n"
            "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
            "}\n";

        const char* fsSrc =
            "#version 150 core\n"
            "uniform sampler2D Texture;\n"
            "in vec2 Frag_UV;\n"
            "in vec4 Frag_Color;\n"
            "out vec4 Out_Color;\n"
            "void main() {\n"
            "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
            "}\n";

        auto compileShader = [](GLenum type, const char* src) -> GLuint {
            GLuint sh = glCreateShader(type);
            glShaderSource(sh, 1, &src, nullptr);
            glCompileShader(sh);
            GLint ok = 0;
            glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[512];
                glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
                std::fprintf(stderr, "[ERROR] Shader compile failed: %s\n", log);
                return 0;
            }
            return sh;
        };

        GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
        if (!vs || !fs) return false;

        shaderProg = glCreateProgram();
        glAttachShader(shaderProg, vs);
        glAttachShader(shaderProg, fs);
        glBindAttribLocation(shaderProg, 0, "Position");
        glBindAttribLocation(shaderProg, 1, "UV");
        glBindAttribLocation(shaderProg, 2, "Color");
        glLinkProgram(shaderProg);
        glDeleteShader(vs);
        glDeleteShader(fs);

        uProjMtx = glGetUniformLocation(shaderProg, "ProjMtx");
        uTex     = glGetUniformLocation(shaderProg, "Texture");

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, pos)));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, uv)));
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, col)));
        glBindVertexArray(0);

        return true;
    }

    void UploadLegacyFontAtlasIfNeeded(ImGuiIO& io) {
#if !defined(IMGUI_VERSION_NUM) || (IMGUI_VERSION_NUM < 19200)
        unsigned char* pixels = nullptr;
        int w = 0, h = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        if (pixels && w > 0 && h > 0) {
            if (legacyFontTex == 0) glGenTextures(1, &legacyFontTex);
            glBindTexture(GL_TEXTURE_2D, legacyFontTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            io.Fonts->SetTexID(static_cast<ImTextureID>(static_cast<uintptr_t>(legacyFontTex)));
        }
#else
        (void)io;
#endif
    }

    void RenderImDrawData(ImDrawData* drawData) {
        if (!drawData) return;

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        if (drawData->Textures != nullptr) {
            for (ImTextureData* tex : *drawData->Textures) {
                if (!tex) continue;
                if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates) {
                    GLuint glTex = static_cast<GLuint>(tex->GetTexID());
                    if (tex->Status == ImTextureStatus_WantCreate || glTex == 0) {
                        glGenTextures(1, &glTex);
                        glBindTexture(GL_TEXTURE_2D, glTex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    } else {
                        glBindTexture(GL_TEXTURE_2D, glTex);
                    }
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    if (tex->Format == ImTextureFormat_RGBA32) {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->Width, tex->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->GetPixels());
                    } else {
                        std::vector<uint8_t> rgba(static_cast<size_t>(tex->Width * tex->Height * 4), 255);
                        const uint8_t* a8 = static_cast<const uint8_t*>(tex->GetPixels());
                        for (int i = 0; i < tex->Width * tex->Height; ++i) {
                            rgba[static_cast<size_t>(i) * 4 + 3] = a8[i];
                        }
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->Width, tex->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                    }
                    tex->SetTexID(static_cast<ImTextureID>(glTex));
                    tex->SetStatus(ImTextureStatus_OK);
                } else if (tex->Status == ImTextureStatus_WantDestroy) {
                    GLuint glTex = static_cast<GLuint>(tex->GetTexID());
                    if (glTex != 0) {
                        glDeleteTextures(1, &glTex);
                    }
                    tex->SetTexID(ImTextureID_Invalid);
                    tex->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }
#endif

        glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo);
        glViewport(0, 0, fbW, fbH);
        glDisable(GL_SCISSOR_TEST);
        // Clean transparent backdrop (alpha = 0.0)
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_MULTISAMPLE);

        const float L = drawData->DisplayPos.x;
        const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        const float T = drawData->DisplayPos.y;
        const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        const float ortho[4][4] = {
            { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
            { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
            { 0.0f,              0.0f,             -1.0f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f }
        };

        glUseProgram(shaderProg);
        glUniform1i(uTex, 0);
        glUniformMatrix4fv(uProjMtx, 1, GL_FALSE, &ortho[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vao);

        const ImVec2 clipOff   = drawData->DisplayPos;
        const ImVec2 clipScale = drawData->FramebufferScale;

        for (int n = 0; n < drawData->CmdListsCount; ++n) {
            const ImDrawList* cl = drawData->CmdLists[n];
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cl->VtxBuffer.Size * sizeof(ImDrawVert)), cl->VtxBuffer.Data, GL_STREAM_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cl->IdxBuffer.Size * sizeof(ImDrawIdx)), cl->IdxBuffer.Data, GL_STREAM_DRAW);

            for (int cmd_i = 0; cmd_i < cl->CmdBuffer.Size; ++cmd_i) {
                const ImDrawCmd* pcmd = &cl->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != nullptr) {
                    pcmd->UserCallback(cl, pcmd);
                    continue;
                }
                ImVec4 clipMin((pcmd->ClipRect.x - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.y - clipOff.y) * clipScale.y,
                               (pcmd->ClipRect.z - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.w - clipOff.y) * clipScale.y);
                if (clipMin.z <= clipMin.x || clipMin.w <= clipMin.y) continue;

                glScissor(
                    static_cast<int>(clipMin.x),
                    static_cast<int>(static_cast<float>(fbH) - clipMin.w),
                    static_cast<int>(clipMin.z - clipMin.x),
                    static_cast<int>(clipMin.w - clipMin.y));

#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19140)
                GLuint texId = static_cast<GLuint>(pcmd->GetTexID());
#else
                GLuint texId = static_cast<GLuint>(reinterpret_cast<uintptr_t>(pcmd->TextureId));
#endif
                glBindTexture(GL_TEXTURE_2D, texId);
                const GLenum idxType = (sizeof(ImDrawIdx) == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
                glDrawElementsBaseVertex(
                    GL_TRIANGLES,
                    static_cast<GLsizei>(pcmd->ElemCount),
                    idxType,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pcmd->IdxOffset * sizeof(ImDrawIdx))),
                    static_cast<GLint>(pcmd->VtxOffset));
            }
        }

        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(0);

        // Resolve 4x MSAA into single-sample resolveFbo for glReadPixels
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
        glBlitFramebuffer(0, 0, fbW, fbH, 0, 0, fbW, fbH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
    }

    bool ExportCroppedPng(ImDrawData* drawData, const std::string& outPath, int marginLogicalPx, int& outPxW, int& outPxH, ImVec2& outLogicalSz) {
        if (!drawData) return false;

        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (int n = 0; n < drawData->CmdListsCount; ++n) {
            const ImDrawList* cl = drawData->CmdLists[n];
            for (int ci = 0; ci < cl->CmdBuffer.Size; ++ci) {
                const ImDrawCmd& cmd = cl->CmdBuffer[ci];
                if (cmd.ElemCount == 0 || cmd.UserCallback != nullptr) continue;
                for (unsigned int e = 0; e < cmd.ElemCount; ++e) {
                    const ImDrawIdx idx = cl->IdxBuffer[static_cast<int>(cmd.IdxOffset + e)];
                    const ImVec2 p = cl->VtxBuffer[static_cast<int>(cmd.VtxOffset) + static_cast<int>(idx)].pos;
                    const float cx = std::max(cmd.ClipRect.x, std::min(cmd.ClipRect.z, p.x));
                    const float cy = std::max(cmd.ClipRect.y, std::min(cmd.ClipRect.w, p.y));
                    if (cx < minX) minX = cx;
                    if (cy < minY) minY = cy;
                    if (cx > maxX) maxX = cx;
                    if (cy > maxY) maxY = cy;
                }
            }
        }

        if (maxX <= minX || maxY <= minY) return false;

        int x0 = std::max(0, static_cast<int>(std::floor(minX)) - marginLogicalPx);
        int y0 = std::max(0, static_cast<int>(std::floor(minY)) - marginLogicalPx);
        int x1 = std::min(logicalW, static_cast<int>(std::ceil(maxX)) + marginLogicalPx);
        int y1 = std::min(logicalH, static_cast<int>(std::ceil(maxY)) + marginLogicalPx);

        outLogicalSz = ImVec2(static_cast<float>(x1 - x0), static_cast<float>(y1 - y0));

        const int rx = x0 * scale;
        const int ryTop = y0 * scale;
        const int rw = (x1 - x0) * scale;
        const int rh = (y1 - y0) * scale;
        const int glBottomY = fbH - (ryTop + rh);

        std::vector<uint8_t> bottomUp(static_cast<size_t>(rw * rh * 4));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFbo);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(rx, glBottomY, rw, rh, GL_RGBA, GL_UNSIGNED_BYTE, bottomUp.data());

        // Flip rows bottom-up -> top-down, preserving rendered alpha & transparent margins (alpha = 0)
        std::vector<uint8_t> topDown(static_cast<size_t>(rw * rh * 4));
        const size_t rowBytes = static_cast<size_t>(rw) * 4u;
        for (int row = 0; row < rh; ++row) {
            const uint8_t* srcRow = bottomUp.data() + static_cast<size_t>(rh - 1 - row) * rowBytes;
            uint8_t* dstRow = topDown.data() + static_cast<size_t>(row) * rowBytes;
            std::memcpy(dstRow, srcRow, rowBytes);
        }

        outPxW = rw;
        outPxH = rh;
        return WriteRgbaPngTopDown(outPath, topDown.data(), rw, rh);
    }

    void Shutdown(ImGuiContext* ctx) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
        if (ctx) {
            ImGui::SetCurrentContext(ctx);
            ImVector<ImTextureData*>& texList = ImGui::GetPlatformIO().Textures;
            for (int i = 0; i < texList.Size; ++i) {
                if (ImTextureData* tex = texList[i]) {
                    GLuint glTex = static_cast<GLuint>(tex->GetTexID());
                    if (glTex != 0) glDeleteTextures(1, &glTex);
                    tex->SetTexID(ImTextureID_Invalid);
                    tex->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }
#else
        (void)ctx;
        if (legacyFontTex != 0) glDeleteTextures(1, &legacyFontTex);
#endif
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (shaderProg) glDeleteProgram(shaderProg);
        if (resolveTex) glDeleteTextures(1, &resolveTex);
        if (resolveFbo) glDeleteFramebuffers(1, &resolveFbo);
        if (msaaColorRbo) glDeleteRenderbuffers(1, &msaaColorRbo);
        if (msaaFbo) glDeleteFramebuffers(1, &msaaFbo);
        if (cglCtx) {
            CGLSetCurrentContext(nullptr);
            CGLDestroyContext(cglCtx);
            cglCtx = nullptr;
        }
    }
};

// ----------------------------------------------------------------------------
// 3. Crisp 2x Retina Typography Loader (ImFontRole 9-Slot + Extended Roles)
// ----------------------------------------------------------------------------

struct GalleryFonts {
    ImFont* regular14    = nullptr;
    ImFont* title16      = nullptr;
    ImFont* small12      = nullptr;
    ImFont* bold14       = nullptr;
    ImFont* boldLarge17  = nullptr;
    ImFont* mono13       = nullptr;
    ImFont* italic14     = nullptr;
    ImFont* monoMed14    = nullptr;
    ImFont* boldItalic14 = nullptr;
    ImFont* h1_20        = nullptr;
    ImFont* h2_17        = nullptr;
    ImFont* h3_15        = nullptr;
    ImFont* tiny11       = nullptr;
};

static ImFont* LoadRetinaTtf(ImGuiIO& io, const std::string& path, float logicalSizePx) {
    if (!FileExistsLocal(path)) return nullptr;
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    return io.Fonts->AddFontFromFileTTF(path.c_str(), logicalSizePx, &cfg);
#elif defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19000)
    cfg.RasterizerDensity = 2.0f;
    return io.Fonts->AddFontFromFileTTF(path.c_str(), logicalSizePx, &cfg);
#else
    ImFont* f = io.Fonts->AddFontFromFileTTF(path.c_str(), logicalSizePx * 2.0f, &cfg);
    if (f) f->Scale = 0.5f;
    return f;
#endif
}

static GalleryFonts SetupGalleryTypography(ImGuiIO& io, const std::string& preferredFontDir, ImGui::MarkdownConfig& outMdCfg) {
    GalleryFonts gf;

    std::string regPath  = preferredFontDir + "/Roboto-Regular.ttf";
    std::string boldPath = preferredFontDir + "/Roboto-Bold.ttf";
    std::string italPath = preferredFontDir + "/Roboto-Italic.ttf";
    std::string medPath  = preferredFontDir + "/Roboto-Medium.ttf";
    std::string monoPath = preferredFontDir + "/RobotoMono-Regular.ttf";

    if (!FileExistsLocal(regPath)) {
        // macOS system font fallback if Roboto directory is absent
        regPath  = "/System/Library/Fonts/Supplemental/Arial.ttf";
        boldPath = "/System/Library/Fonts/Supplemental/Arial Bold.ttf";
        italPath = "/System/Library/Fonts/Supplemental/Arial Italic.ttf";
        medPath  = boldPath;
        monoPath = "/System/Library/Fonts/Menlo.ttc";
        if (!FileExistsLocal(regPath)) {
            regPath = "/System/Library/Fonts/SFNS.ttf";
            boldPath = regPath;
            italPath = regPath;
            medPath = regPath;
        }
    }

    // Populate canonical 9-slot atlas order (Slots 0..8) + extended semantic roles:
    gf.regular14    = LoadRetinaTtf(io, regPath,  14.0f); // Slot 0: Regular
    gf.title16      = LoadRetinaTtf(io, medPath,  16.0f); // Slot 1: Title
    gf.small12      = LoadRetinaTtf(io, regPath,  12.0f); // Slot 2: Small
    gf.bold14       = LoadRetinaTtf(io, boldPath, 14.0f); // Slot 3: Bold
    gf.boldLarge17  = LoadRetinaTtf(io, boldPath, 17.0f); // Slot 4: BoldLarge
    gf.mono13       = LoadRetinaTtf(io, monoPath, 13.0f); // Slot 5: Monospace
    gf.italic14     = LoadRetinaTtf(io, italPath, 14.0f); // Slot 6: Italic
    gf.monoMed14    = LoadRetinaTtf(io, monoPath, 14.5f); // Slot 7: MonospaceMedium
    gf.boldItalic14 = LoadRetinaTtf(io, boldPath, 14.0f); // Slot 8: BoldItalic (fallback Bold)
    gf.h1_20        = LoadRetinaTtf(io, boldPath, 20.0f);
    gf.h2_17        = LoadRetinaTtf(io, boldPath, 17.0f);
    gf.h3_15        = LoadRetinaTtf(io, medPath,  15.0f);
    gf.tiny11       = LoadRetinaTtf(io, regPath,  11.0f);

    if (!gf.regular14) {
        std::fprintf(stderr, "[ERROR] Failed to load any TrueType font (checked '%s').\n", preferredFontDir.c_str());
        std::exit(1);
    }

    io.FontDefault = gf.regular14;

    ImGui::SetFontRole(ImGui::ImFontRole::Regular,         gf.regular14);
    ImGui::SetFontRole(ImGui::ImFontRole::Title,           gf.title16 ? gf.title16 : gf.bold14);
    ImGui::SetFontRole(ImGui::ImFontRole::Small,           gf.small12 ? gf.small12 : gf.regular14);
    ImGui::SetFontRole(ImGui::ImFontRole::Bold,            gf.bold14 ? gf.bold14 : gf.regular14);
    ImGui::SetFontRole(ImGui::ImFontRole::BoldLarge,       gf.boldLarge17 ? gf.boldLarge17 : gf.bold14);
    ImGui::SetFontRole(ImGui::ImFontRole::Monospace,       gf.mono13 ? gf.mono13 : gf.regular14);
    ImGui::SetFontRole(ImGui::ImFontRole::Italic,          gf.italic14 ? gf.italic14 : gf.regular14);
    ImGui::SetFontRole(ImGui::ImFontRole::MonospaceMedium, gf.monoMed14 ? gf.monoMed14 : gf.mono13);
    ImGui::SetFontRole(ImGui::ImFontRole::BoldItalic,      gf.boldItalic14 ? gf.boldItalic14 : gf.bold14);
    ImGui::SetFontRole(ImGui::ImFontRole::Heading1,        gf.h1_20 ? gf.h1_20 : gf.boldLarge17);
    ImGui::SetFontRole(ImGui::ImFontRole::Heading2,        gf.h2_17 ? gf.h2_17 : gf.title16);
    ImGui::SetFontRole(ImGui::ImFontRole::Heading3,        gf.h3_15 ? gf.h3_15 : gf.bold14);
    ImGui::SetFontRole(ImGui::ImFontRole::Tiny,            gf.tiny11 ? gf.tiny11 : gf.small12);

    outMdCfg = ImGui::MarkdownConfig{};
    outMdCfg.regularFont     = gf.regular14;
    outMdCfg.boldFont        = gf.bold14;
    outMdCfg.italicFont      = gf.italic14;
    outMdCfg.boldItalicFont  = gf.boldItalic14;
    outMdCfg.monoFont        = gf.mono13;
    outMdCfg.h1Font          = gf.h1_20;
    outMdCfg.h2Font          = gf.h2_17;
    outMdCfg.h3Font          = gf.h3_15;
    outMdCfg.headingFormats[0] = { gf.h1_20, true };
    outMdCfg.headingFormats[1] = { gf.h2_17, true };
    outMdCfg.headingFormats[2] = { gf.h3_15, false };
    outMdCfg.showImageCaptions = true;
    outMdCfg.enableImageLightbox = true;

    ImGui::SetDefaultMarkdownConfig(&outMdCfg);
    return gf;
}

// ----------------------------------------------------------------------------
// 4. Procedural Showcase Cursor Overlay Helper (Hand, ResizeNS, ResizeEW)
// ----------------------------------------------------------------------------

static void StrokePolyCompat(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col, float thickness, ImDrawFlags flags = 0) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19280)
    dl->AddPolyline(pts, count, col, thickness, flags);
#else
    dl->AddPolyline(pts, count, col, flags, thickness);
#endif
}

static void DrawShowcaseCursor(ImDrawList* dl, ImVec2 tipPos, ImGuiMouseCursor cursorType) {
    if (!dl) return;
    const ImU32 shadowCol = IM_COL32(0, 0, 0, 175);
    const ImU32 borderCol = IM_COL32(14, 18, 26, 255);
    const ImU32 fillCol   = IM_COL32(250, 252, 255, 255);

    if (cursorType == ImGuiMouseCursor_Hand) {
        // Exact 15-vertex pointing-hand silhouette partitioned into 3 strictly convex
        // sub-polygons sharing internal diagonals (0--5 and 0--11) so the white fill
        // sits 100% strictly inside the dark StrokePolyCompat outline (zero protrusion!).
        const ImVec2 raw[15] = {
            ImVec2(-1.8f,  9.0f), // 0: index-thumb valley
            ImVec2(-1.8f,  1.8f), // 1: index left shaft
            ImVec2(-0.8f,  0.3f), // 2: index tip-left
            ImVec2( 0.8f,  0.3f), // 3: index tip-right
            ImVec2( 1.8f,  1.8f), // 4: index right shaft
            ImVec2( 1.8f,  6.5f), // 5: index-middle valley
            ImVec2( 4.6f,  6.5f), // 6: middle knuckle
            ImVec2( 7.2f,  7.2f), // 7: ring knuckle
            ImVec2( 9.5f,  8.4f), // 8: pinky knuckle
            ImVec2( 9.7f, 13.8f), // 9: pinky outer wall
            ImVec2( 7.6f, 18.4f), // 10: wrist right
            ImVec2( 0.4f, 18.4f), // 11: wrist left
            ImVec2(-4.0f, 13.8f), // 12: thumb lower base
            ImVec2(-5.6f,  9.8f), // 13: thumb outer tip
            ImVec2(-4.4f,  8.3f)  // 14: thumb upper tip
        };
        ImVec2 shPts[15], fgPts[15];
        for (int i = 0; i < 15; ++i) {
            fgPts[i] = ImVec2(tipPos.x + raw[i].x, tipPos.y + raw[i].y);
            shPts[i] = ImVec2(fgPts[i].x + 1.3f, fgPts[i].y + 1.7f);
        }

        // 1. Soft drop-shadow (exact same 3 convex parts shifted by (+1.3, +1.7))
        const ImVec2 shIndex[6] = { shPts[0], shPts[1], shPts[2], shPts[3], shPts[4], shPts[5] };
        const ImVec2 shPalm[8]  = { shPts[0], shPts[5], shPts[6], shPts[7], shPts[8], shPts[9], shPts[10], shPts[11] };
        const ImVec2 shThumb[5] = { shPts[12], shPts[13], shPts[14], shPts[0], shPts[11] };
        dl->AddConvexPolyFilled(shPalm, 8, shadowCol);
        dl->AddConvexPolyFilled(shThumb, 5, shadowCol);
        dl->AddConvexPolyFilled(shIndex, 6, shadowCol);

        // 2. Crisp white glove fill + unified dark outer silhouette
        const ImVec2 fgIndex[6] = { fgPts[0], fgPts[1], fgPts[2], fgPts[3], fgPts[4], fgPts[5] };
        const ImVec2 fgPalm[8]  = { fgPts[0], fgPts[5], fgPts[6], fgPts[7], fgPts[8], fgPts[9], fgPts[10], fgPts[11] };
        const ImVec2 fgThumb[5] = { fgPts[12], fgPts[13], fgPts[14], fgPts[0], fgPts[11] };
        dl->AddConvexPolyFilled(fgPalm, 8, fillCol);
        dl->AddConvexPolyFilled(fgThumb, 5, fillCol);
        dl->AddConvexPolyFilled(fgIndex, 6, fillCol);
        StrokePolyCompat(dl, fgPts, 15, borderCol, 1.5f, ImDrawFlags_Closed);

        // 3. Subtle finger separation creases
        const ImVec2 f1[2] = { ImVec2(tipPos.x + 1.8f, tipPos.y + 6.5f), ImVec2(tipPos.x + 1.8f, tipPos.y + 10.8f) };
        const ImVec2 f2[2] = { ImVec2(tipPos.x + 4.6f, tipPos.y + 6.6f), ImVec2(tipPos.x + 4.6f, tipPos.y + 11.0f) };
        const ImVec2 f3[2] = { ImVec2(tipPos.x + 7.2f, tipPos.y + 7.3f), ImVec2(tipPos.x + 7.2f, tipPos.y + 11.4f) };
        StrokePolyCompat(dl, f1, 2, borderCol, 1.05f);
        StrokePolyCompat(dl, f2, 2, borderCol, 1.05f);
        StrokePolyCompat(dl, f3, 2, borderCol, 1.05f);
        return;
    }

    if (cursorType == ImGuiMouseCursor_ResizeNS || cursorType == ImGuiMouseCursor_ResizeEW) {
        // Sleek 23x9 / 9x23 double-headed arrow matching Dear ImGui's internal cursor proportions
        // (FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA: ~1.3px thin white shaft, beveled 1px dark outline with
        // zero 45-degree wing miter spikes, and a single subtle un-inflated drop shadow).
        const bool isNS = (cursorType == ImGuiMouseCursor_ResizeNS);
        const float halfLen   = 10.5f;
        const float wingHalf  = 3.8f;
        const float headLen   = 4.2f;
        const float baseOff   = halfLen - headLen; // 6.3f
        const float shaftHalf = 0.65f;             // 1.3px sleek ~1-boxel white shaft

        auto toScreen = [&](ImVec2 origin, float nx, float ny) -> ImVec2 {
            return isNS ? ImVec2(origin.x + nx, origin.y + ny)
                        : ImVec2(origin.x + ny, origin.y + nx);
        };

        auto drawUninflatedParts = [&](ImVec2 origin, ImU32 col, float shaftTuck) {
            const ImVec2 h1a = toScreen(origin,  0.0f,     -halfLen);
            const ImVec2 h1b = toScreen(origin,  wingHalf, -baseOff);
            const ImVec2 h1c = toScreen(origin, -wingHalf, -baseOff);

            const float sy = baseOff + shaftTuck;
            const ImVec2 shaft[4] = {
                toScreen(origin, -shaftHalf, -sy),
                toScreen(origin,  shaftHalf, -sy),
                toScreen(origin,  shaftHalf,  sy),
                toScreen(origin, -shaftHalf,  sy)
            };

            const ImVec2 h2a = toScreen(origin, -wingHalf,  baseOff);
            const ImVec2 h2b = toScreen(origin,  wingHalf,  baseOff);
            const ImVec2 h2c = toScreen(origin,  0.0f,      halfLen);

            dl->AddTriangleFilled(h1a, h1b, h1c, col);
            dl->AddConvexPolyFilled(shaft, 4, col);
            dl->AddTriangleFilled(h2a, h2b, h2c, col);
        };

        // Layer 1: Subtle single drop shadow using exact un-inflated 3 convex parts (zero miter spikes!)
        const ImU32 resizeShadowCol = IM_COL32(0, 0, 0, 105);
        drawUninflatedParts(ImVec2(tipPos.x + 1.05f, tipPos.y + 1.15f), resizeShadowCol, 0.0f);

        // Layer 2: Tight ~0.80px dark outline with beveled tips & beveled 45-degree wing caps (23x9 box, zero bat-ears!)
        {
            const float b = 0.80f;
            const float slantLen = std::sqrt(wingHalf * wingHalf + headLen * headLen);
            const float nx = headLen / slantLen;   // outward normal X on top-right slant
            const float ny = wingHalf / slantLen;  // outward normal Y magnitude on top-right slant
            const float tipCapY    = halfLen + b;
            const float tipBevelX  = b * (1.0f - ny) / nx;
            const float wingCapX   = wingHalf + b;
            const float wingUpperY = halfLen - (nx * wingCapX - b) / ny;
            const float wingLowerY = baseOff - b;
            const float shaftOutX  = shaftHalf + b;
            const float shaftOutY  = baseOff + 0.25f;

            const ImVec2 topHeadBorder[6] = {
                toScreen(tipPos, -tipBevelX, -tipCapY),
                toScreen(tipPos,  tipBevelX, -tipCapY),
                toScreen(tipPos,  wingCapX,  -wingUpperY),
                toScreen(tipPos,  wingCapX,  -wingLowerY),
                toScreen(tipPos, -wingCapX,  -wingLowerY),
                toScreen(tipPos, -wingCapX,  -wingUpperY)
            };
            const ImVec2 botHeadBorder[6] = {
                toScreen(tipPos, -wingCapX,   wingLowerY),
                toScreen(tipPos,  wingCapX,   wingLowerY),
                toScreen(tipPos,  wingCapX,   wingUpperY),
                toScreen(tipPos,  tipBevelX,  tipCapY),
                toScreen(tipPos, -tipBevelX,  tipCapY),
                toScreen(tipPos, -wingCapX,   wingUpperY)
            };
            const ImVec2 shaftBorder[4] = {
                toScreen(tipPos, -shaftOutX, -shaftOutY),
                toScreen(tipPos,  shaftOutX, -shaftOutY),
                toScreen(tipPos,  shaftOutX,  shaftOutY),
                toScreen(tipPos, -shaftOutX,  shaftOutY)
            };
            dl->AddConvexPolyFilled(topHeadBorder, 6, borderCol);
            dl->AddConvexPolyFilled(shaftBorder, 4, borderCol);
            dl->AddConvexPolyFilled(botHeadBorder, 6, borderCol);
        }

        // Layer 3: Crisp 1.3px (~1-boxel) white interior on top
        drawUninflatedParts(tipPos, fillCol, 0.25f);
    }
}

static bool s_GalleryLightPass = false;

static void ApplyGalleryTheme() {
    if (s_GalleryLightPass) {
        ImGui::StyleColorsPaperLight();
    } else {
        ImGui::StyleColorsCharcoal();
    }
}

// Subtle card header helper for visual polish across widget showcase cards
static void BeginShowcaseCard(const char* winId, const char* title, const char* subtitle, float cardWidth) {
    ApplyGalleryTheme();
    ImGui::SetNextWindowPos(ImVec2(70.0f, 65.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(cardWidth, 20.0f), ImVec2(cardWidth, 2000.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 9.0f));
    ImGui::Begin(winId, nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoScrollbar);

    if (title && *title) {
        ImGui::PushFontRole(ImGui::ImFontRole::Title);
        ImGui::TextUnformatted(title);
        ImGui::PopFontRole();
    }
    if (subtitle && *subtitle) {
        ImGui::PushFontRole(ImGui::ImFontRole::Small);
        ImGui::TextDisabled("%s", subtitle);
        ImGui::PopFontRole();
    }
    if ((title && *title) || (subtitle && *subtitle)) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
}

static void EndShowcaseCard() {
    ImGui::End();
    ImGui::PopStyleVar(4);
}

static void SimMouseMove(ImGuiIO& io, float x, float y) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
    io.AddMousePosEvent(x, y);
#else
    io.MousePos = ImVec2(x, y);
#endif
}

static void SimMouseButton(ImGuiIO& io, bool down) {
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
    io.AddMouseButtonEvent(0, down);
#else
    io.MouseDown[0] = down;
#endif
}

static void SetSimMouse(ImGuiIO& io, float x, float y, bool down = false) {
    SimMouseMove(io, x, y);
    SimMouseButton(io, down);
}

// ----------------------------------------------------------------------------
// 5. Scene Metadata Record for docs/Gallery_Preview.md
// ----------------------------------------------------------------------------

struct SceneReport {
    int         index = 0;
    std::string title;
    std::string sourceHeader;
    std::string symbols;
    std::string fileName;
    std::string snippet;
    ImVec2      logicalSize = ImVec2(0.0f, 0.0f);
    int         pxW = 0;
    int         pxH = 0;
};

} // namespace

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
    const auto tStart = std::chrono::steady_clock::now();

    std::string fontDir    = "/Users/brat/Documents/imgui4xp-pg-test/imgui4xp/fonts";
    std::string outDir     = "docs/images";
    std::string previewMd  = "build/Gallery_Preview.md";
    std::string themeMode  = "all";

    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--font-dir") == 0) {
            fontDir = argv[++i];
        } else if (std::strcmp(argv[i], "--out-dir") == 0) {
            outDir = argv[++i];
        } else if (std::strcmp(argv[i], "--preview-md") == 0) {
            previewMd = argv[++i];
        } else if (std::strcmp(argv[i], "--theme") == 0) {
            themeMode = argv[++i];
        }
    }

    EnsureDirExists(outDir);

    // Prepare temporary sample directory & schematic diagram for realistic headless scenes
    const std::string sampleRoot = "Workspace_Presets";
    const std::string sampleSub1 = sampleRoot + "/Audio_Chains";
    const std::string sampleSub2 = sampleRoot + "/Layouts";
    EnsureDirExists(sampleSub1);
    EnsureDirExists(sampleSub2);
    const std::vector<std::pair<std::string, size_t>> sampleFiles = {
        { sampleRoot + "/Dark_Studio_2x.json",    4820 },
        { sampleRoot + "/Default_Docking.json",   6340 },
        { sampleRoot + "/HDR_Viewport.ini",       1940 },
        { sampleRoot + "/MultiMonitor_4K.json",   8192 },
        { sampleRoot + "/Telemetry_Overlay.json", 5120 }
    };
    std::vector<std::string> cleanupFiles;
    for (size_t i = 0; i < sampleFiles.size(); ++i) {
        std::ofstream ofs(sampleFiles[i].first.c_str(), std::ios::binary | std::ios::trunc);
        std::string payload(sampleFiles[i].second, ' ');
        ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        cleanupFiles.push_back(sampleFiles[i].first);
    }

    const std::string schematicPngPath = "/tmp/imgee_gallery_envelope_schematic.png";
    if (!GenerateSchematicDiagramPng(schematicPngPath)) {
        std::fprintf(stderr, "[ERROR] Failed to write temporary schematic PNG.\n");
        return 1;
    }
    cleanupFiles.push_back(schematicPngPath);

    OffscreenGlContext gl;
    if (!gl.Init(1280, 1180, 2)) {
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(static_cast<float>(gl.logicalW), static_cast<float>(gl.logicalH));
    io.DisplayFramebufferScale = ImVec2(static_cast<float>(gl.scale), static_cast<float>(gl.scale));
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19200)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#endif

    // Wire ImGee::HostBackend directly to real OpenGL textures!
    ImGee::HostBackend hostBackend;
    hostBackend.UserData = &gl;
    hostBackend.CreateTextureRGBA8 = [](const unsigned char* pixels, int w, int h, void* userData) -> ImTextureID {
        OffscreenGlContext* g = static_cast<OffscreenGlContext*>(userData);
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        if (g) g->hostCreated++;
        return static_cast<ImTextureID>(tex);
    };
    hostBackend.DeleteTexture = [](ImTextureID texId, void* userData) {
        OffscreenGlContext* g = static_cast<OffscreenGlContext*>(userData);
        GLuint tex = static_cast<GLuint>(texId);
        if (tex != 0) {
            glDeleteTextures(1, &tex);
            if (g) g->hostDeleted++;
        }
    };
    ImGee::SetHostBackend(hostBackend);

    // Configure clipboard callbacks in headless mode so CopyButton is active and enabled!
    static std::string s_HeadlessClipboard;
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 19110)
    ImGui::GetPlatformIO().Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) {
        s_HeadlessClipboard = text ? text : "";
    };
    ImGui::GetPlatformIO().Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char* {
        return s_HeadlessClipboard.c_str();
    };
#else
    io.SetClipboardTextFn = [](void*, const char* text) {
        s_HeadlessClipboard = text ? text : "";
    };
    io.GetClipboardTextFn = [](void*) -> const char* {
        return s_HeadlessClipboard.c_str();
    };
#endif

    ImGui::ApplyDefaultTheme();
    ImGui::MarkdownConfig mdCfg;
    (void)SetupGalleryTypography(io, fontDir, mdCfg);
    mdCfg.setClipboardTextFn = [](const char* txt, void*) { s_HeadlessClipboard = txt ? txt : ""; };
    mdCfg.baseDir = "/tmp";
    ImGui::SetDefaultMarkdownConfig(&mdCfg);

    gl.UploadLegacyFontAtlasIfNeeded(io);

    auto runAllScenes = [&](bool isLight, const std::string& currentOutDir, std::vector<SceneReport>& currentReports) {
        s_GalleryLightPass = isLight;
        EnsureDirExists(currentOutDir);
        ApplyGalleryTheme();
        currentReports.reserve(18);

        auto finishAndExportScene = [&](SceneReport rep) {
            ImDrawData* dd = ImGui::GetDrawData();
            gl.RenderImDrawData(dd);
            const std::string fullPng = currentOutDir + "/" + rep.fileName;
            if (!gl.ExportCroppedPng(dd, fullPng, 4, rep.pxW, rep.pxH, rep.logicalSize)) {
                std::fprintf(stderr, "[ERROR] Failed exporting '%s'\n", fullPng.c_str());
                std::exit(1);
            }
            currentReports.push_back(rep);
            ImGui::TimedTooltip::ClearActiveTooltip();
            SetSimMouse(io, 2.0f, 2.0f, false);
        };

    // ========================================================================
    // SCENE 1: Toggle Switches & Segmented Rockers (fig_toggle_switches.png)
    // ========================================================================
    {
        bool navLights = false;
        bool autoThrottle = true;
        bool terrainRadar = true;
        int xpdrMode = 1;
        int starterMode = 0;
        ImVec2 handPos(0.0f, 0.0f);

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc1", "Toggle Switches & Dual-State Rockers",
                              "Animated pill switches and segmented binary rockers with clamped mouse-wheel support", 500.0f);

            ImGui::Columns(2, "##sc1_cols", false);
            ImGui::SetColumnWidth(0, 235.0f);

            ImGui::PushFontRole(ImGui::ImFontRole::Bold);
            ImGui::TextUnformatted("Animated Pill Switches");
            ImGui::PopFontRole();
            ImGui::Spacing();

            ImGui::ToggleButton("Landing Lights", &navLights);
            ImGui::ToggleButton("Autothrottle (A/T)", &autoThrottle);

            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.68f, 0.42f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.76f, 0.48f, 1.0f));
            ImGui::ToggleButton("Terrain Overlay", &terrainRadar);
            handPos = ImVec2(ImGui::GetItemRectMin().x + 26.0f, ImGui::GetItemRectMax().y - 3.0f);
            ImGui::PopStyleColor(2);

            ImGui::NextColumn();

            ImGui::PushFontRole(ImGui::ImFontRole::Bold);
            ImGui::TextUnformatted("Segmented Rocker Pairs");
            ImGui::PopFontRole();
            ImGui::Spacing();

            ImGui::ToggleButtons("Transponder", &xpdrMode, "STBY", "TA/RA", ImVec2(54.0f, 0.0f));
            ImGui::ToggleButtons("Ignition", &starterMode, "NORM", "CONT", ImVec2(54.0f, 0.0f));

            ImGui::Columns(1);
            if (f == 2) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), handPos, ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            1,
            "Toggle Switches & Dual-State Rockers",
            "widgets/imgui_toggle_buttons.h",
            "`ImGui::ToggleButton`, `ImGui::ToggleButtons`",
            "fig_toggle_switches.png",
            "ImGui::ToggleButton(\"Autothrottle (A/T)\", &autoThrottle);\n"
            "ImGui::ToggleButtons(\"Transponder\", &xpdrMode, \"STBY\", \"TA/RA\", ImVec2(54.0f, 0.0f));",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 2: Avionics Rotary Knobs (fig_rotary_knobs.png)
    // ========================================================================
    {
        float backlight = 0.85f;
        float comVol    = 0.62f;
        float floodOff  = 0.0f;
        float wxTilt    = 3.5f;
        ImVec2 ewPos(0.0f, 0.0f);
        ImVec2 resetHandPos(0.0f, 0.0f);

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc2", "Rotary Knobs & Dimmer Dials",
                              "Concentric zero-bias needles, OFF detents, graduation ticks, and 1-click vector reset", 560.0f);

            ImGui::Columns(2, "##sc2_cols", false);
            ImGui::SetColumnWidth(0, 265.0f);

            ImGui::KnobPercent("Panel Backlight", &backlight, 1.0f, 0.0f, 1.0f, "%.0f%%", 24.0f);
            ewPos = ImVec2(ImGui::GetItemRectMin().x + 12.0f, ImGui::GetItemRectMin().y + 38.0f);

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::DimmerKnob("Desk Keylight", &floodOff, 0.05f, 1.0f, 0.75f, "%.0f%%", 24.0f, 0.02f, true, 0.0f, ImGuiKnobFlags_ShowTicks);

            ImGui::NextColumn();

            const ImVec2 pCom = ImGui::GetCursorScreenPos();
            ImGui::KnobWithReset("Monitor Gain", &comVol, 0.50f, 0.0f, 1.0f, "%.0f%%", 24.0f, 0.02f, nullptr, ImGuiKnobFlags_None);
            resetHandPos = ImVec2(pCom.x + 95.0f, pCom.y + 34.0f);

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::RotaryDial("Output Trim", &wxTilt, -15.0f, 15.0f, "%+.1f dB", 24.0f, 0.5f, ImGuiKnobFlags_ShowTicks);
            ImGui::Columns(1);

            if (f == 2) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), ewPos, ImGuiMouseCursor_ResizeEW);
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), resetHandPos, ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            2,
            "Rotary Knobs & Dimmer Dials",
            "widgets/imgui_knobs.h",
            "`ImGui::KnobPercent`, `ImGui::KnobWithReset`, `ImGui::DimmerKnob`, `ImGui::RotaryDial`",
            "fig_rotary_knobs.png",
            "ImGui::KnobPercent(\"Panel Backlight\", &backlight, 1.0f, 0.0f, 1.0f, \"%.0f%%\", 24.0f);\n"
            "ImGui::KnobWithReset(\"Monitor Gain\", &comVol, 0.50f, 0.0f, 1.0f, \"%.0f%%\", 24.0f);\n"
            "ImGui::DimmerKnob(\"Desk Keylight\", &floodOff, 0.05f, 1.0f, 0.75f,\n"
            "                  \"%.0f%%\", 24.0f, 0.02f, true, 0.0f, ImGuiKnobFlags_ShowTicks);",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 3: Mouse-Wheel Sliders, Drags & Combos with Reset/Undo (fig_sliders_reset.png)
    // ========================================================================
    {
        float pitchTrim = 2.4f;
        float hudBright = 0.75f;  // == default, with prevHudBright != default -> shows [redo/restore] state!
        float prevHudBright = 0.92f;
        float baroInHg = 30.08f;
        int   camPreset = 2;
        const char* const kPresets[] = { "Pilot Eye (Default)", "Pedestal Close-Up", "Overhead Panel", "External Chase" };
        ImVec2 ewPos(0.0f, 0.0f);

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc3", "Mouse-Wheel Controls with 1-Click Reset & Undo",
                              "Abutting [Reset] and [Restore Previous] buttons with clamped scroll-wheel stepping", 540.0f);

            ImGui::PushItemWidth(300.0f);
            ImGui::SliderWithReset("Pitch Trim", &pitchTrim, 0.0f, -10.0f, 10.0f, "%+.1f deg", 0, 0.2f);
            ewPos = ImVec2(ImGui::GetItemRectMin().x + 68.0f, ImGui::GetItemRectMin().y + 12.0f);

            ImGui::PercentSliderWithReset("HUD Brightness (Undo Ready)", &hudBright, 0.75f, 0.0f, 1.0f, "%.0f%%", 0, 0.02f, nullptr, &prevHudBright);
            ImGui::DragWithReset("Altimeter QNH", &baroInHg, 29.92f, 0.01f, 28.00f, 31.00f, "%.2f inHg", 0, 0.01f);
            ImGui::ComboWithReset("Camera View", &camPreset, 0, kPresets, 4);
            ImGui::PopItemWidth();

            if (f == 2) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), ewPos, ImGuiMouseCursor_ResizeEW);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            3,
            "Mouse-Wheel Controls with 1-Click Reset & Undo",
            "widgets/imgui_mouse_wheel_controls.h",
            "`ImGui::SliderWithReset`, `ImGui::PercentSliderWithReset`, `ImGui::DragWithReset`, `ImGui::ComboWithReset`",
            "fig_sliders_reset.png",
            "ImGui::SliderWithReset(\"Pitch Trim\", &pitchTrim, 0.0f, -10.0f, 10.0f, \"%+.1f deg\", 0, 0.2f);\n"
            "ImGui::PercentSliderWithReset(\"HUD Brightness\", &hudBright, 0.75f, 0.0f, 1.0f,\n"
            "                              \"%.0f%%\", 0, 0.02f, nullptr, &prevHudBright);\n"
            "ImGui::DragWithReset(\"Altimeter QNH\", &baroInHg, 29.92f, 0.01f, 28.00f, 31.00f, \"%.2f inHg\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 4: SelectPulldown & Hover-Latching Menu Strip (fig_popup_menus.png)
    // ========================================================================
    {
        const char* const kNavSrc[] = { "FMS1 (Primary)", "FMS2 (Cross-Side)", "VOR1 / LOC1", "VOR2 / LOC2" };
        const char* const kCamModes[] = { "Cockpit Pilot", "External Circle", "Control Tower", "Fly-By Pass", "Wing Spot" };
        int closedIdx = 0;
        int openIdx   = 1;
        ImVec2 closedBtnPos(0.0f, 0.0f);
        ImVec2 openBtnCenter(0.0f, 0.0f);
        ImVec2 openPopupCursor(0.0f, 0.0f);

        ImGui::MenuStripConfig sc4StripCfg;
        sc4StripCfg.anchor    = ImGuiPopupAnchor_AboveItem;
        sc4StripCfg.align     = ImGuiPopupAlign_Left;
        sc4StripCfg.showCaret = true;

        for (int f = 0; f < 6; ++f) {
            if (f == 1) {
                SimMouseMove(io, openBtnCenter.x, openBtnCenter.y);
            } else if (f == 2) {
                SimMouseButton(io, true);
            } else if (f == 3) {
                SimMouseButton(io, false);
            } else if (f >= 4) {
                SimMouseMove(io, openPopupCursor.x, openPopupCursor.y);
            }

            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc4", "Anchored Pulldowns & Hover-Latching Menu Strips",
                              "SelectPulldown (scroll pass-through / live wheel) and BeginMenuStrip (glide-switch triggers)", 540.0f);

            ImGui::Columns(2, "##sc4_cols", false);
            ImGui::SetColumnWidth(0, 268.0f);
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("CLOSED PULLDOWN (Scroll Pass-Through)");
            ImGui::PopFontRole();
            ImGui::SelectPulldown("##nav_closed", &closedIdx, kNavSrc, 4, nullptr, 224.0f);
            closedBtnPos = ImVec2(ImGui::GetItemRectMin().x + 165.0f, ImGui::GetItemRectMax().y - 5.0f);

            ImGui::Dummy(ImVec2(0.0f, 34.0f));
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("HOVER-LATCHING MENU STRIP (AboveItem)");
            ImGui::PopFontRole();
            if (ImGui::BeginMenuStrip("##sc4_menu_strip", &sc4StripCfg)) {
                if (ImGui::BeginStripMenu("View"))    { ImGui::EndStripMenu(); }
                if (ImGui::BeginStripMenu("Track"))   { ImGui::EndStripMenu(); }
                if (ImGui::BeginStripMenu("Presets")) { ImGui::EndStripMenu(); }
                ImGui::EndMenuStrip();
            }
            ImGui::Dummy(ImVec2(0.0f, 3.0f));
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("Click to latch; glide siblings to switch");
            ImGui::PopFontRole();

            ImGui::NextColumn();
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("OPEN PULLDOWN (Wheel Stepping)");
            ImGui::PopFontRole();
            const ImVec2 pOpen = ImGui::GetCursorScreenPos();
            ImGui::SelectPulldown("##cam_open", &openIdx, kCamModes, 5, nullptr, 224.0f);
            if (f == 0) {
                openBtnCenter = ImVec2(pOpen.x + 90.0f, pOpen.y + ImGui::GetFrameHeight() * 0.5f);
                openPopupCursor = ImVec2(pOpen.x + 175.0f, pOpen.y + ImGui::GetFrameHeight() + 50.0f);
            }
            ImGui::Columns(1);
            ImGui::Dummy(ImVec2(0.0f, 44.0f)); // Generous room inside card framing the anchored popup

            if (f == 5) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), closedBtnPos, ImGuiMouseCursor_Hand);
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), openPopupCursor, ImGuiMouseCursor_ResizeNS);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            4,
            "Anchored Pulldowns & Hover-Latching Menu Strips",
            "widgets/imgui_popup_menus.h",
            "`ImGui::SelectPulldown`, `ImGui::BeginMenuStrip`, `ImGui::BeginStripMenu`",
            "fig_popup_menus.png",
            "ImGui::SelectPulldown(\"##cam_mode\", &camIdx, kCamModes, 5, kCamTooltips, 210.0f);\n"
            "if (ImGui::BeginMenuStrip(\"##bar\", &cfg)) { if (ImGui::BeginStripMenu(\"View\")) { ImGui::EndStripMenu(); } ImGui::EndMenuStrip(); }",
            ImVec2(0, 0), 0, 0
        });

        // Close popup cleanly before next scene
        io.AddKeyEvent(ImGuiKey_Escape, true);
        ImGui::NewFrame(); ImGui::Render();
        io.AddKeyEvent(ImGuiKey_Escape, false);
        SetSimMouse(io, -100.0f, -100.0f, false);
    }

    // ========================================================================
    // SCENE 5: Clickable Links & Active Affordance Controls (fig_clickable_links.png)
    // ========================================================================
    {
        bool autoSyncBaro = true;
        bool pauseTopOfDescent = false;
        ImVec2 linkHandPos(0.0f, 0.0f);

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc5", "Clickable Links & Hand-Affordance Controls",
                              "Zero-inert-arrow text links, procedural external-link icons, and tooltip-toggle buttons", 500.0f);

            ImGui::TextUnformatted("Documentation:");
            ImGui::SameLine();
            const ImVec4 textLinkAccent(0.58f, 0.80f, 1.0f, 1.0f);
            (void)ImGui::TextLink("Quick-Start Flight Briefing", &textLinkAccent);
            ImGui::SameLine(0.0f, 16.0f);
            (void)ImGui::ExternalTextLink("X-Plane SDK Reference", "https://developer.x-plane.com");
            linkHandPos = ImVec2(ImGui::GetItemRectMax().x - 24.0f, ImGui::GetItemRectMax().y - 3.0f);

            ImGui::Spacing();
            (void)ImGui::ButtonLink("Load Approach Briefing");
            ImGui::SameLine();
            (void)ImGui::ButtonLink("Sync SimBrief OFP");

            ImGui::Spacing();
            ImGui::CheckboxLink("Auto-sync standby altimeter baro setting", &autoSyncBaro);
            ImGui::CheckboxLink("Pause simulation 10 NM before Top of Descent", &pauseTopOfDescent);

            if (f == 2) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(), linkHandPos, ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            5,
            "Clickable Links & Hand-Affordance Controls",
            "widgets/imgui_clickable_links.h",
            "`ImGui::TextLink`, `ImGui::ExternalTextLink`, `ImGui::ButtonLink`, `ImGui::CheckboxLink`",
            "fig_clickable_links.png",
            "if (ImGui::ExternalTextLink(\"X-Plane SDK Reference\", \"https://developer.x-plane.com\")) { /* ... */ }\n"
            "ImGui::ButtonLink(\"Load Approach Briefing\");\n"
            "ImGui::CheckboxLink(\"Auto-sync standby altimeter baro setting\", &autoSyncBaro);",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 6: Badges, Status Pills & 3D Keycaps (fig_pills_and_badges.png)
    // ========================================================================
    {
        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc6", "Semantic Badges, Status Pills & 3D Keycaps",
                              "Auto-contrasting annunciators, vector-icon pills, and beveled keyboard shortcut caps", 545.0f);

            ImGui::Badge("DEFAULT", ImGui::ImGuiBadgeVariant_Default); ImGui::SameLine();
            ImGui::Badge("LNAV",    ImGui::ImGuiBadgeVariant_Primary); ImGui::SameLine();
            ImGui::Badge("VNAV",    ImGui::ImGuiBadgeVariant_Success); ImGui::SameLine();
            ImGui::Badge("APU GEN", ImGui::ImGuiBadgeVariant_Warning); ImGui::SameLine();
            ImGui::Badge("MASTER WARN", ImGui::ImGuiBadgeVariant_Danger); ImGui::SameLine();
            ImGui::Badge("ATIS B",  ImGui::ImGuiBadgeVariant_Info);

            ImGui::Spacing();
            ImGui::Pill("CAT III DUAL", ImGui::ImGuiBadgeVariant_Success); ImGui::SameLine();
            ImGui::PillNamed("plane", "AUTOLAND", ImVec4(0.18f, 0.48f, 0.88f, 1.0f)); ImGui::SameLine();
            ImGui::PillNamed("warning", "TERR INHIBIT", ImVec4(0.88f, 0.58f, 0.10f, 1.0f)); ImGui::SameLine();
            ImGui::BadgeNamed("gear", "SYS OK", ImVec4(0.22f, 0.26f, 0.32f, 1.0f)); ImGui::SameLine();
            ImGui::BadgeNamed("info", ImVec4(0.20f, 0.52f, 0.90f, 1.0f));

            ImGui::Spacing();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Hotkeys:"); ImGui::SameLine();
            ImGui::Kbd("Ctrl+S"); ImGui::SameLine();
            ImGui::TextDisabled("Save Preset"); ImGui::SameLine(0.0f, 14.0f);
            ImGui::Kbd("Shift"); ImGui::SameLine(0.0f, 4.0f);
            ImGui::TextUnformatted("+"); ImGui::SameLine(0.0f, 4.0f);
            ImGui::Kbd("R"); ImGui::SameLine();
            ImGui::TextDisabled("Reset View"); ImGui::SameLine(0.0f, 14.0f);
            ImGui::Kbd("Esc");

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            6,
            "Semantic Badges, Status Pills & 3D Keycaps",
            "widgets/imgui_badges.h",
            "`ImGui::Badge`, `ImGui::Pill`, `ImGui::PillNamed`, `ImGui::BadgeNamed`, `ImGui::Kbd`",
            "fig_pills_and_badges.png",
            "ImGui::Badge(\"VNAV\", ImGui::ImGuiBadgeVariant_Success); ImGui::SameLine();\n"
            "ImGui::PillNamed(\"plane\", \"AUTOLAND\", ImVec4(0.18f, 0.48f, 0.88f, 1.0f)); ImGui::SameLine();\n"
            "ImGui::Kbd(\"Ctrl+S\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 7: Procedural Vector Icon Catalog (fig_vector_icons.png)
    // ========================================================================
    {
        struct IconSpec { const char* name; ImVec4 col; };
        const IconSpec kIcons[] = {
            { "undo",          ImVec4(0.75f, 0.85f, 1.00f, 1.0f) },
            { "redo",          ImVec4(0.75f, 0.85f, 1.00f, 1.0f) },
            { "check",         ImVec4(0.30f, 0.86f, 0.52f, 1.0f) },
            { "cross",         ImVec4(0.96f, 0.42f, 0.42f, 1.0f) },
            { "plus",          ImVec4(0.88f, 0.92f, 0.98f, 1.0f) },
            { "minus",         ImVec4(0.88f, 0.92f, 0.98f, 1.0f) },
            { "info",          ImVec4(0.38f, 0.62f, 1.00f, 1.0f) },
            { "lightbulb",     ImVec4(0.28f, 0.85f, 0.52f, 1.0f) },
            { "important",     ImVec4(0.72f, 0.46f, 1.00f, 1.0f) },
            { "warning",       ImVec4(0.98f, 0.68f, 0.18f, 1.0f) },
            { "caution",       ImVec4(0.96f, 0.34f, 0.34f, 1.0f) },
            { "gear",          ImVec4(0.78f, 0.83f, 0.90f, 1.0f) },
            { "star",          ImVec4(0.98f, 0.80f, 0.24f, 1.0f) },
            { "search",        ImVec4(0.72f, 0.84f, 0.98f, 1.0f) },
            { "plane",         ImVec4(0.52f, 0.82f, 1.00f, 1.0f) },
            { "external-link", ImVec4(0.52f, 0.78f, 1.00f, 1.0f) },
            { "circle",        ImVec4(0.36f, 0.82f, 0.56f, 1.0f) },
            { "square",        ImVec4(0.68f, 0.76f, 0.88f, 1.0f) },
            { "caret-up",      ImVec4(0.86f, 0.90f, 0.96f, 1.0f) },
            { "caret-down",    ImVec4(0.86f, 0.90f, 0.96f, 1.0f) },
            { "caret-left",    ImVec4(0.86f, 0.90f, 0.96f, 1.0f) },
            { "caret-right",   ImVec4(0.86f, 0.90f, 0.96f, 1.0f) }
        };
        const int numIcons = static_cast<int>(sizeof(kIcons) / sizeof(kIcons[0]));

        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc7", "Built-In Procedural Vector Icons",
                              "Zero-font, zero-texture antialiased ImDrawList geometry with zero-bias center alignment", 610.0f);

            const int cols = 6;
            const float tileW = 90.0f;
            const float tileH = 54.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 gridOrigin = ImGui::GetCursorScreenPos();

            for (int i = 0; i < numIcons; ++i) {
                const int r = i / cols;
                const int c = i % cols;
                const ImVec2 tMin(gridOrigin.x + c * (tileW + 6.0f), gridOrigin.y + r * (tileH + 6.0f));
                const ImVec2 tMax(tMin.x + tileW, tMin.y + tileH);
                dl->AddRectFilled(tMin, tMax, IM_COL32(28, 32, 40, 210), 5.0f);
                ImGui::MarkdownDetail::AddRectCompat(dl, tMin, tMax, IM_COL32(72, 80, 96, 110), 5.0f, 0, 1.0f);

                const float iconSz = 20.0f;
                const ImVec2 iPos(std::round(tMin.x + (tileW - iconSz) * 0.5f), tMin.y + 8.0f);
                ImGui::DrawVectorIcon(dl, iPos, iconSz, ImGui::ColorConvertFloat4ToU32(kIcons[i].col), kIcons[i].name);

                ImGui::PushFontRole(ImGui::ImFontRole::Tiny);
                const ImVec2 lblSz = ImGui::CalcTextSize(kIcons[i].name);
                dl->AddText(ImVec2(std::round(tMin.x + (tileW - lblSz.x) * 0.5f), tMax.y - lblSz.y - 6.0f),
                            IM_COL32(190, 198, 212, 235), kIcons[i].name);
                ImGui::PopFontRole();
            }
            const int totalRows = (numIcons + cols - 1) / cols;
            ImGui::Dummy(ImVec2(cols * tileW + (cols - 1) * 6.0f, totalRows * tileH + (totalRows - 1) * 6.0f));

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            7,
            "Built-In Procedural Vector Icons",
            "widgets/imgui_icons.h",
            "`ImGui::Icon`, `ImGui::DrawVectorIcon`, `ImGui::IconButton`",
            "fig_vector_icons.png",
            "ImGui::Icon(\"lightbulb\", 18.0f, &greenAccent);\n"
            "ImGui::DrawVectorIcon(ImGui::GetWindowDrawList(), pos, 20.0f, col, \"warning\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 8: Activity Spinners & Radial Countdown Dials (fig_spinners_and_dials.png)
    // ========================================================================
    {
        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 0.28f; // Advance ImGui::GetTime() so spinner arc is mid-sweep!
            ImGui::NewFrame();
            BeginShowcaseCard("##sc8", "Activity Spinners & Radial Countdown Dials",
                              "Frame-rate independent procedural busy arcs, progress rings, and countdown timers", 560.0f);

            ImGui::Columns(3, "##sc8_cols", false);

            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("ACTIVITY SPINNERS");
            ImGui::PopFontRole();
            ImGui::ActivitySpinner("Fetching METAR", 11.0f, 3.0f, IM_COL32(96, 175, 255, 255), 1.4f);
            ImGui::ActivitySpinner("Compiling Terrain", 11.0f, 3.0f, IM_COL32(72, 214, 138, 255), 2.1f);

            ImGui::NextColumn();
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("PROGRESS DIALS");
            ImGui::PopFontRole();
            ImGui::ProgressDial("IRS Align (32%)", 0.32f, 11.0f, 3.0f, IM_COL32(96, 175, 255, 255));
            ImGui::ProgressDial("FMS Load (84%)",  0.84f, 11.0f, 3.0f, IM_COL32(72, 214, 138, 255));

            ImGui::NextColumn();
            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("COUNTDOWN TIMERS");
            ImGui::PopFontRole();
            ImGui::CountdownDial("Hold (21s / 30s)", 21.0f, 30.0f, 11.0f, 3.0f, IM_COL32(245, 178, 48, 255));
            ImGui::CountdownDial("Alert (4s / 30s)",  4.0f, 30.0f, 11.0f, 3.0f, IM_COL32(244, 86, 86, 255));

            ImGui::Columns(1);
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            8,
            "Activity Spinners & Radial Countdown Dials",
            "widgets/imgui_spinners.h",
            "`ImGui::ActivitySpinner`, `ImGui::ProgressDial`, `ImGui::CountdownDial`, `ImGui::RadialTimer`",
            "fig_spinners_and_dials.png",
            "ImGui::ActivitySpinner(\"Fetching METAR\", 11.0f, 3.0f, IM_COL32(96, 175, 255, 255));\n"
            "ImGui::ProgressDial(\"FMS Load (84%)\", 0.84f, 11.0f, 3.0f, IM_COL32(72, 214, 138, 255));\n"
            "ImGui::CountdownDial(\"Hold (21s / 30s)\", 21.0f, 30.0f, 11.0f, 3.0f, IM_COL32(245, 178, 48, 255));",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 9: Clipboard Copy Buttons (fig_copy_button.png)
    // ========================================================================
    {
        ImVec2 secondCopyBtnCenter(0.0f, 0.0f);

        for (int f = 0; f < 5; ++f) {
            if (f == 1) {
                SimMouseMove(io, secondCopyBtnCenter.x, secondCopyBtnCenter.y);
            } else if (f == 2) {
                SimMouseButton(io, true);
            } else if (f == 3) {
                SimMouseButton(io, false);
            } else if (f == 4) {
                SimMouseMove(io, secondCopyBtnCenter.x + 14.0f, secondCopyBtnCenter.y + 6.0f);
            }
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc9", "Clipboard Copy Buttons (Idle & Copied! Feedback)",
                              "Procedural double-sheet icon transitioning smoothly into a green [Copied!] confirmation", 535.0f);

            // Row 1: Idle [Copy]
            ImGui::AlignTextToFramePadding();
            ImGui::PushFontRole(ImGui::ImFontRole::Monospace);
            ImGui::TextColored(ImVec4(0.75f, 0.86f, 1.0f, 1.0f), "sim/cockpit2/autopilot/altitude_dial_ft");
            ImGui::PopFontRole();
            ImGui::SameLine(385.0f);
            ImGui::CopyBufferButton("##cp_idle", "sim/cockpit2/autopilot/altitude_dial_ft", 2.0f);

            // Row 2: Active green [Copied!]
            ImGui::AlignTextToFramePadding();
            ImGui::PushFontRole(ImGui::ImFontRole::Monospace);
            ImGui::TextColored(ImVec4(0.75f, 0.86f, 1.0f, 1.0f), "KSFO 191856Z 28014KT 10SM FEW018 17/10 A3004");
            ImGui::PopFontRole();
            ImGui::SameLine(385.0f);
            ImGui::CopyBufferButton("##cp_active", "KSFO 191856Z 28014KT 10SM FEW018 17/10 A3004", 2.0f);
            if (f == 0) {
                const ImVec2 mn = ImGui::GetItemRectMin();
                const ImVec2 mx = ImGui::GetItemRectMax();
                secondCopyBtnCenter = ImVec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
            }

            if (f == 4) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(),
                                   ImVec2(secondCopyBtnCenter.x + 22.0f, secondCopyBtnCenter.y + 6.0f),
                                   ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }
        SetSimMouse(io, -100.0f, -100.0f, false);

        finishAndExportScene({
            9,
            "Clipboard Copy Buttons (Idle & `Copied!` Feedback)",
            "widgets/imgui_copy_button.h",
            "`ImGui::CopyButton`, `ImGui::CopyBufferButton`, `ImGui::CopyLastItemButton`",
            "fig_copy_button.png",
            "ImGui::TextUnformatted(\"sim/cockpit2/autopilot/altitude_dial_ft\");\n"
            "ImGui::SameLine();\n"
            "ImGui::CopyLastItemButton(\"##copy_dataref\", \"sim/cockpit2/autopilot/altitude_dial_ft\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 10: Non-Blocking File Picker & Modal Browser (fig_file_dialog.png)
    // ========================================================================
    {
        std::string currentProfile = "Default_Docking.json";
        bool browserOpen = true;
        std::string chosenOut;

        ImGui::Detail::FileDialogState& dlgState = ImGui::Detail::GetDialogState("Load Workspace Preset");
        dlgState.was_open = false;

        // OpenFileDialog centers its 560x380 initial modal at DisplaySize * 0.5f -> top-left Y = DisplaySize.y*0.5 - 190.
        // Dock the 560px FilePickerCombo toolbar card 12px cleanly above that modal top edge!
        const float dlgW = 560.0f;
        const float dlgH = 290.0f;
        const float dlgX = io.DisplaySize.x * 0.5f - dlgW * 0.5f;
        const float modalTopY = io.DisplaySize.y * 0.5f - 380.0f * 0.5f;

        for (int f = 0; f < 4; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            ApplyGalleryTheme();
            ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

            ImGui::SetNextWindowPos(ImVec2(dlgX, modalTopY - 58.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(dlgW, 46.0f), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
            ImGui::Begin("##sc10_bar2", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Quick Preset:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(285.0f);
            ImGui::FilePickerCombo("##quick_prof2", &currentProfile, sampleRoot.c_str(), ".json;.ini");
            ImGui::SameLine();
            ImGui::Badge("POSIX / WIN32", ImGui::ImGuiBadgeVariant_Primary);
            ImGui::End();
            ImGui::PopStyleVar();

            if (f >= 1) {
                dlgState.selected_file = "Default_Docking.json";
                std::snprintf(dlgState.filename_input, sizeof(dlgState.filename_input), "%s", "Default_Docking.json");
            }
            ImGui::SetNextWindowSize(ImVec2(dlgW, dlgH), ImGuiCond_Always);
            ImGui::OpenFileDialog("Load Workspace Preset", &browserOpen, &chosenOut, sampleRoot.c_str(), ".json;.ini");

            ImGui::PopStyleColor();
            ImGui::Render();
        }

        finishAndExportScene({
            10,
            "Non-Blocking File Picker & Modal Browser",
            "widgets/imgui_file_dialog.h",
            "`ImGui::FilePickerCombo`, `ImGui::OpenFileDialog`, `ImGui::SaveFileDialog`",
            "fig_file_dialog.png",
            "ImGui::FilePickerCombo(\"##preset\", &currentPreset, presetsDir, \".json;.ini\");\n"
            "if (ImGui::OpenFileDialog(\"Load Workspace Preset\", &browserOpen, &selectedPath, presetsDir, \".json;.ini\")) {\n"
            "    LoadPreset(selectedPath);\n"
            "}",
            ImVec2(0, 0), 0, 0
        });

        // Ensure modal state is closed for subsequent scenes
        browserOpen = false;
        ImGui::NewFrame();
        ImGui::OpenFileDialog("Load Workspace Preset", &browserOpen, &chosenOut, sampleRoot.c_str(), ".json;.ini");
        ImGui::Render();
    }

    // ========================================================================
    // SCENE 11: Curated Color Themes Side-by-Side (Charcoal vs Rusty) (fig_themes_comparison.png)
    // ========================================================================
    {
        auto renderMiniControlPanel = [](const char* winName, const char* heading, const ImVec2& pos) {
            bool masterActive = true;
            float outputGain  = 25.0f;
            float panelDim    = 0.72f;
            bool autoSync     = true;

            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSizeConstraints(ImVec2(295.0f, 10.0f), ImVec2(295.0f, 900.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::Begin(winName, nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

            ImGui::PushFontRole(ImGui::ImFontRole::Small);
            ImGui::TextDisabled("%s", heading);
            ImGui::PopFontRole();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::ToggleButton("Master Bus A", &masterActive);
            ImGui::SameLine(190.0f);
            ImGui::KnobPercent("Level", &panelDim, 1.0f, 0.0f, 1.0f, "%.0f%%", 18.0f);

            ImGui::PushItemWidth(175.0f);
            ImGui::SliderWithReset("Output Trim", &outputGain, 30.0f, 10.0f, 30.0f, "%.0f dB");
            ImGui::PopItemWidth();

            ImGui::CheckboxLink("Sync linked channel group", &autoSync);
            ImGui::Spacing();
            ImGui::Button("Apply Preset", ImVec2(-1.0f, 0.0f));

            ImGui::End();
            ImGui::PopStyleVar(3);
        };

        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();

            if (s_GalleryLightPass) {
                ImGui::StyleColorsPaperLight();
                renderMiniControlPanel("Control Strip -- Paper Light", "ImGui::StyleColorsPaperLight() (Publication)", ImVec2(65.0f, 65.0f));

                ImGui::StyleColorsCharcoal();
                renderMiniControlPanel("Control Strip -- Charcoal", "ImGui::StyleColorsCharcoal() (Studio Slate)", ImVec2(378.0f, 65.0f));
            } else {
                ImGui::StyleColorsCharcoal();
                renderMiniControlPanel("Control Strip -- Charcoal", "ImGui::StyleColorsCharcoal() (Default)", ImVec2(65.0f, 65.0f));

                ImGui::StyleColorsRusty();
                renderMiniControlPanel("Control Strip -- Rusty", "ImGui::StyleColorsRusty() (Warm Bronze)", ImVec2(378.0f, 65.0f));
            }

            ApplyGalleryTheme();
            ImGui::Render();
        }

        finishAndExportScene({
            11,
            s_GalleryLightPass ? "Curated Color Themes (`Paper Light` vs. `Charcoal`)" : "Curated Color Themes (`Charcoal` vs. `Rusty`)",
            "themes/imgui_styles.h",
            "`ImGui::StyleColorsCharcoal`, `ImGui::StyleColorsRusty`, `ImGui::StyleColorsPaperLight`, `ImGui::ApplyDefaultTheme`",
            "fig_themes_comparison.png",
            s_GalleryLightPass ?
                "ImGui::StyleColorsPaperLight(); // Crisp publication high-contrast light\n"
                "ImGui::StyleColorsCharcoal();   // Cool studio slate & platinum accents" :
                "ImGui::StyleColorsCharcoal(); // Cool studio slate & platinum accents\n"
                "ImGui::StyleColorsRusty();    // Warm low-glare bronze & ivory text",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 12: Hover-Delayed Stationary TimedTooltips (fig_timed_tooltips.png)
    // ========================================================================
    {
        bool fdBars = true;
        ImVec2 targetHoverPos(0.0f, 0.0f);
        ImGui::TimedTooltip::TooltipConfig fastTip = ImGui::TimedTooltip::GetConfig();
        fastTip.hover_delay_cycles = 3;
        fastTip.default_wrap_width = 370.0f;

        for (int f = 0; f < 8; ++f) {
            if (f >= 1) {
                SetSimMouse(io, targetHoverPos.x, targetHoverPos.y, false);
            }
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc12", "Stationary Delayed Tooltips (TimedTooltip::TextMD)",
                              "Anti-flicker hover delay, click-to-toggle support, and automatic light-palette callout adaptation", 500.0f);

            ImGui::ToggleButton("Flight Director (FD1)", &fdBars);
            ImGui::SameLine(0.0f, 18.0f);
            ImGui::Badge("CMD A", ImGui::ImGuiBadgeVariant_Success);
            if (f == 0) {
                const ImVec2 mn = ImGui::GetItemRectMin();
                const ImVec2 mx = ImGui::GetItemRectMax();
                targetHoverPos = ImVec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
            }
            ImGui::TimedTooltip::TextMD(
                &fastTip,
                "**Autopilot Command Channel A**\n"
                "Couples lateral `LNAV` and vertical `VNAV PATH` guidance directly to the primary flight control computers.\n\n"
                "> [!TIP]\n"
                "> Callout cards inside yellow sticky-note tooltips automatically switch to deep high-contrast ink accents!");

            ImGui::Dummy(ImVec2(0.0f, 138.0f)); // Keep outer card nicely proportioned behind stationary tooltip

            if (f == 7) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(),
                                   ImVec2(targetHoverPos.x + 8.0f, targetHoverPos.y + 4.0f),
                                   ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            12,
            "Stationary Delayed Tooltips (`TimedTooltip::TextMD`)",
            "extensions/TimedTooltip/imgui_tooltip_ext.h",
            "`ImGui::TimedTooltip::TextMD`, `ImGui::TimedTooltip::Text`, `ImGui::TimedTooltip::TooltipConfig`",
            "fig_timed_tooltips.png",
            "ImGui::Badge(\"CMD A\", ImGui::ImGuiBadgeVariant_Success);\n"
            "ImGui::TimedTooltip::TextMD(\n"
            "    \"**Autopilot Command Channel A**\\n\"\n"
            "    \"> [!TIP]\\n> Callout cards automatically adapt to light sticky-note backgrounds!\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 13: Native Blockquotes & GFM Callout Cards (fig_callout_cards.png)
    // ========================================================================
    {
        const char* kCalloutsMd =
            "> Standard blockquote -- ideal for SOP excerpts, ATIS transcripts, and subtle reference notes.\n"
            "> [!NOTE]\n"
            "> **Dual-Sim Profiles:** Store separate presets for X-Plane 11 and X-Plane 12 photometric lighting.\n"
            "> [!TIP]\n"
            "> Roll the mouse wheel over header `Zoom` controls to scale prose while keeping chrome at `1.0x`.\n"
            "> [!IMPORTANT]\n"
            "> Verify `MarkdownConfig::baseDir` points to your plugin documentation folder for relative assets.\n"
            "> [!WARNING]\n"
            "> Cabin altitude exceeding **10,000 ft** triggers supplemental oxygen deployment advisories.\n"
            "> [!CAUTION]\n"
            "> Disengaging both integrated drive generators (`IDG 1` & `IDG 2`) requires maintenance reset on ground.";

        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc13", "GFM Callout Cards & Native Blockquotes",
                              "All five GitHub-Flavored Markdown alert severities plus quiet slate blockquotes", 600.0f);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 4.0f));
            ImGui::MD::TextWrapped(kCalloutsMd, &mdCfg);
            ImGui::PopStyleVar();

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            13,
            "GFM Callout Cards & Native Blockquotes",
            "extensions/Markdown/imgui_markdown_ext.h",
            "`ImGui::MD::TextWrapped`, `> [!NOTE]`, `> [!TIP]`, `> [!IMPORTANT]`, `> [!WARNING]`, `> [!CAUTION]`",
            "fig_callout_cards.png",
            "ImGui::MD::TextWrapped(\n"
            "    \"> [!NOTE]\\n\"\n"
            "    \"> **Dual-Sim Profiles:** Store separate presets for X-Plane 11 and X-Plane 12.\\n\\n\"\n"
            "    \"> [!WARNING]\\n\"\n"
            "    \"> Cabin altitude exceeding **10,000 ft** triggers oxygen advisories.\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 14: Interactive GFM Tables with Sort & Tooltips (fig_gfm_tables.png)
    // ========================================================================
    {
        const char* kTableMd =
            "| Waypoint | Airway | Alt (ft) | Speed | Status |\n"
            "| :--- | :---: | ---: | ---: | :---: |\n"
            "| **KSFO** | SID | 4,000 | 210 kt | <badge=success>ACTIVE</badge> |\n"
            "| **PORTE** | V25 | 11,000 | 250 kt | <badge=info>CLIMB</badge> |\n"
            "| **MODESTO** | J120 | 23,000 | 290 kt | <badge=primary>ENROUTE</badge> |\n"
            "| **DYAMD** | STAR | 12,000 | 240 kt | <badge=warning>DESCENT</badge> |\n"
            "<!-- table-state: sort=0 dir=asc -->\n";

        ImGui::MarkdownConfig tblCfg = mdCfg;
        ImGui::TimedTooltip::TooltipConfig fastTtCfg;
        fastTtCfg.hover_delay_cycles = 2;
        ImGui::TimedTooltip::SetDefaultTooltipConfig(&fastTtCfg);
        ImVec2 tableTopLeft(0.0f, 0.0f);
        ImVec2 sortHeaderPos(0.0f, 0.0f);

        for (int f = 0; f < 7; ++f) {
            if (f >= 1) {
                SimMouseMove(io, sortHeaderPos.x, sortHeaderPos.y);
            }

            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc14", "Interactive GFM Markdown Tables",
                              "Zebra striping, tristate column sorting, inline badges, and right-edge resize grip", 560.0f);

            tableTopLeft = ImGui::GetCursorScreenPos();
            if (f == 0) {
                sortHeaderPos = ImVec2(tableTopLeft.x + 44.0f, tableTopLeft.y + 14.0f);
            }
            ImGui::MD::TextWrapped(kTableMd, &tblCfg);
            ImGui::Dummy(ImVec2(0.0f, 18.0f));

            if (f == 6) {
                DrawShowcaseCursor(ImGui::GetForegroundDrawList(),
                                   ImVec2(sortHeaderPos.x + 12.0f, sortHeaderPos.y + 4.0f),
                                   ImGuiMouseCursor_Hand);
            }
            EndShowcaseCard();
            ImGui::Render();
        }
        ImGui::TimedTooltip::SetDefaultTooltipConfig(nullptr);
        SetSimMouse(io, -100.0f, -100.0f, false);

        finishAndExportScene({
            14,
            "Interactive GFM Markdown Tables",
            "extensions/Markdown/imgui_markdown.h",
            "`ImGui::MD::TextWrapped` (GFM Pipe Tables, Tristate Sort, Right-Edge Resize Grip)",
            "fig_gfm_tables.png",
            "ImGui::MD::TextWrapped(\n"
            "    \"| Waypoint | Airway | Alt (ft) | Status |\\n\"\n"
            "    \"| :--- | :---: | ---: | :---: |\\n\"\n"
            "    \"| **KSFO** | SID | 4,000 | <badge=success>ACTIVE</badge> |\\n\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 15: Fenced Language-Badged Code Blocks (fig_code_blocks.png)
    // ========================================================================
    {
        const char* kCodeMd =
            "```cpp\n"
            "#include \"imgee.h\"\n\n"
            "void DrawFlightDeckCard(float* qnhInHg, bool* fdEnabled) {\n"
            "    ImGui::ToggleButton(\"Flight Director\", fdEnabled);\n"
            "    ImGui::DragWithReset(\"Baro QNH\", qnhInHg, 29.92f, 0.01f, 28.0f, 31.0f, \"%.2f inHg\");\n"
            "    ImGui::TimedTooltip::TextMD(\"**QNH Setting** -- Scroll wheel steps `0.01 inHg`.\");\n"
            "}\n"
            "```\n";

        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc15", "Fenced Markdown Code Blocks with Header & Copy",
                              "Language badge header bar, monospace rendering, and integrated 1-click [Copy] action", 620.0f);

            ImGui::MD::TextWrapped(kCodeMd, &mdCfg);

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            15,
            "Fenced Markdown Code Blocks with Header & Copy",
            "extensions/Markdown/imgui_markdown.h",
            "`ImGui::MD::TextWrapped` (Fenced Code Blocks + Integrated Copy Header)",
            "fig_code_blocks.png",
            "ImGui::MD::TextWrapped(\n"
            "    \"```cpp\\n\"\n"
            "    \"ImGui::ToggleButton(\\\"Flight Director\\\", fdEnabled);\\n\"\n"
            "    \"```\\n\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 16: Text-Wrapping Markdown Image Floats & Captions (fig_markdown_floats_captions.png)
    // ========================================================================
    {
        const char* kFloatMd =
            "![Flight Envelope](imgee_gallery_envelope_schematic.png){width=220 float=right gap=14x6 caption=\"**Fig 1:** Normal-law flight envelope\"}\n\n"
            "### Flight Envelope Protection\n"
            "Under **Normal Law**, high-speed and angle-of-attack protections continuously monitor air-data inertial "
            "reference channels (`ADR 1..3`) so pitch and roll commands remain strictly inside the green structural envelope "
            "from `V_LS` up to `V_MO`.\n\n"
            "Once prose advances past the bottom of the floated figure and its centered caption, subsequent paragraphs "
            "automatically expand back out to the full container width -- supporting inline `code`, <badge=info>NORMAL LAW</badge>, "
            "and click-to-enlarge lightbox inspection.";

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc16", "Text-Wrapping Markdown Image Floats & Captions",
                              "True magazine-style float=left / float=right figures with centered captions and HostBackend textures", 590.0f);

            ImGui::MD::TextWrapped(kFloatMd, &mdCfg);

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            16,
            "Text-Wrapping Markdown Image Floats & Captions",
            "extensions/Markdown/imgui_markdown.h",
            "`![Alt](img){width=210 float=right caption=\"...\"}`, `ImGee::SetHostBackend`",
            "fig_markdown_floats_captions.png",
            "ImGui::MD::TextWrapped(\n"
            "    \"![Envelope](schematic.png){width=210 float=right caption=\\\"**Fig 1:** Normal-law envelope\\\"}\\n\\n\"\n"
            "    \"### Flight Envelope Protection\\n\"\n"
            "    \"Under **Normal Law**, pitch and roll commands remain strictly inside the structural envelope...\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 17: Semantic Font Roles & Inline StyledText (fig_typography_styled_text.png)
    // ========================================================================
    {
        for (int f = 0; f < 2; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            BeginShowcaseCard("##sc17", "Semantic Typography Registry & Inline StyledText",
                              "9-slot ImFontRole hierarchy paired with lightweight baseline-aligned StyledText spans", 595.0f);

            ImGui::PushFontRole(ImGui::ImFontRole::Heading1);
            ImGui::TextUnformatted("Heading1 (20px Bold) -- Primary Briefing Banner");
            ImGui::PopFontRole();

            ImGui::PushFontRole(ImGui::ImFontRole::Heading2);
            ImGui::TextUnformatted("Heading2 (17px Bold) -- Subsystem Section Header");
            ImGui::PopFontRole();

            ImGui::PushFontRole(ImGui::ImFontRole::Title);
            ImGui::TextUnformatted("Title (16px Medium) -- Dialog & Panel Title");
            ImGui::PopFontRole();

            ImGui::TextUnformatted("Regular (14px) -- Standard body copy, paired with ");
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::PushFontRole(ImGui::ImFontRole::Bold);
            ImGui::TextUnformatted("Bold");
            ImGui::PopFontRole();
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(", ");
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::PushFontRole(ImGui::ImFontRole::Italic);
            ImGui::TextUnformatted("Italic");
            ImGui::PopFontRole();
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(", and ");
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::PushFontRole(ImGui::ImFontRole::Monospace);
            ImGui::TextColored(ImVec4(0.68f, 0.85f, 1.0f, 1.0f), "Monospace 121.50 MHz");
            ImGui::PopFontRole();

            ImGui::SmallText("Small (12px) -- Secondary annotations, units, and subtle status hints");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::StyledText(
                "<icon plane color=#66B8FF /> <b>Cleared ILS Rwy 28R:</b> Maintain <mono color=#7CE3A5>180 KT</mono></color> "
                "to <b>DUMBA</b> <pill color=#1F8E55>LOC* / GS*</pill> -- press <kbd>APP</kbd> to arm.");
            ImGui::StyledText(
                "<icon warning color=#F5A623 /> <color=#F5C469><b> Crosswind Advisory:</b></color> "
                "Surface wind <mono>210@18G24KT</mono> -- <i>verify rudder trim neutral before flare.</i>");

            EndShowcaseCard();
            ImGui::Render();
        }

        finishAndExportScene({
            17,
            "Semantic Typography Registry & Inline `StyledText`",
            "extensions/Typography/imgui_font_roles.h, extensions/Typography/imgui_styled_text.h",
            "`ImGui::ImFontRole`, `ImGui::PushFontRole`, `ImGui::StyledText`, `ImGui::SmallText`",
            "fig_typography_styled_text.png",
            "ImGui::PushFontRole(ImGui::ImFontRole::Heading2);\n"
            "ImGui::TextUnformatted(\"Subsystem Section Header\");\n"
            "ImGui::PopFontRole();\n"
            "ImGui::StyledText(\"<icon plane color=#66B8FF /> <b>Cleared ILS Rwy 28R:</b> \"\n"
            "                  \"Maintain <mono>180 KT</mono> <pill color=#1F8E55>LOC* / GS*</pill>\");",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENE 18: Standalone DocumentViewer (fig_doc_viewer.png)
    // ========================================================================
    {
        const char* kViewerMd =
            "# 1. Flight Envelope Protection\n\n"
            "![Flight Envelope](imgee_gallery_envelope_schematic.png){width=190 float=right gap=14x6 caption=\"**Fig 1:** Normal-law flight envelope\"}\n\n"
            "Under **Normal Law**, redundant air-data inertial reference units (`ADIRU 1..3`) continuously monitor load factor "
            "and airspeed so pitch and roll inputs stay safely bounded between `V_LS` and `V_MO` <badge=info>NORMAL LAW</badge>.\n\n"
            "> [!TIP]\n"
            "> Roll the mouse wheel over `Zoom 115%` or `Opacity: 85%` in the header bar to scale prose from `50%` up to `250%` "
            "while both toolbars and the footer stay crisp at `1.0x`.\n\n"
            "## 2. Autopilot Mode Transitions\n\n"
            "Selecting any heading in the **Section** pulldown -- or clicking an internal `#anchor` link -- scrolls smoothly "
            "to the target section inside the zoomed viewport.\n";

        ImGui::DocumentViewerState dvState;
        dvState.viewport.SetZoom(1.15f);
        dvState.window_opacity = s_GalleryLightPass ? 1.0f : 0.85f;
        dvState.transparent_hud = !s_GalleryLightPass;
        dvState.theme_index = s_GalleryLightPass ? ImGui::DocumentViewerTheme_Light : ImGui::DocumentViewerTheme_Charcoal;

        ImGui::DocumentViewerConfig dvCfg;
        dvCfg.title                 = "User Manual & Reference";
        dvCfg.baseDir               = "/tmp";
        dvCfg.tocWidth              = 228.0f;
        dvCfg.showFitButton         = true;
        dvCfg.showCursorToggle      = true;
        dvCfg.showPopOutButton      = true;
        dvCfg.showCloseButton       = true;
        dvCfg.showFooterCloseButton = true;

        for (int f = 0; f < 3; ++f) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            ApplyGalleryTheme();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 10.0f));
            ImGui::SetNextWindowPos(ImVec2(60.0f, 60.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(732.0f, 368.0f), ImGuiCond_Always);
            ImGui::Begin("##sc18_docviewer", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            (void)ImGui::DocumentViewer("##flight_ops_manual", kViewerMd, dvState, &dvCfg);

            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::Render();
        }

        finishAndExportScene({
            18,
            "Standalone Markdown `DocumentViewer`",
            "extensions/DocViewer/imgui_doc_viewer.h",
            "`ImGui::DocumentViewer`, `ImGui::DocumentViewerState`, `ImGui::DocumentViewerConfig`, `ImGui::ParseMarkdownToc`",
            "fig_doc_viewer.png",
            "static ImGui::DocumentViewerState s_ManualState;\n"
            "ImGui::DocumentViewerConfig cfg;\n"
            "cfg.title = \"User Manual & Reference\";\n"
            "cfg.baseDir = \"assets/docs\";\n"
            "ImGui::DocumentViewer(\"##Manual\", manualMarkdown, s_ManualState, &cfg);",
            ImVec2(0, 0), 0, 0
        });
    }

    // ========================================================================
    // SCENES 19..26: Opt-In Interactive Showcase (imgee_demo.h) -- All 8 Tabs
    // ========================================================================
    {
        static const char* const kDemoTabTitles[8] = {
            "Tab 1: Overview & Themes",
            "Tab 2: Sliders, Toggles & Knobs",
            "Tab 3: Menus, Pulldowns & Links",
            "Tab 4: Tooltips, Badges & Spinners",
            "Tab 5: Vector Icons",
            "Tab 6: File & Diagnostics",
            "Tab 7: DocumentViewer & Find",
            "Tab 8: Markdown & GFM Tables"
        };
        static const float kDemoTabHeights[8] = {
            395.0f, // Tab 1: Overview & Themes
            425.0f, // Tab 2: Sliders, Toggles & Knobs
            395.0f, // Tab 3: Menus, Pulldowns & Links
            415.0f, // Tab 4: Tooltips, Badges & Spinners (includes live dwell/max sliders + TextMDLink)
            355.0f, // Tab 5: Vector Icons (8 cols x 3 rows at 1140px)
            465.0f, // Tab 6: File & Diagnostics (with live file preview pane)
            500.0f, // Tab 7: DocumentViewer & Find
            565.0f  // Tab 8: Markdown & GFM Tables (shows full preview + fenced code block)
        };

        const int targetDemoTheme = s_GalleryLightPass ? 4 : 0; // 4 = Paper Light, 0 = Charcoal
        const char* demoScopeId   = s_GalleryLightPass ? "##gallery_imgee_demo_light" : "##gallery_imgee_demo_dark";
        const char* hostWinTitle  = s_GalleryLightPass
            ? "ImGee Interactive Showcase (v" IMGEE_VERSION ")##sc_demo_host_light"
            : "ImGee Interactive Showcase (v" IMGEE_VERSION ")##sc_demo_host_dark";
        for (int tabIdx = 0; tabIdx < 8; ++tabIdx) {
            ImGeeDemoDetail::GetForcedTabIndex() = tabIdx;
            for (int f = 0; f < 4; ++f) {
                io.DeltaTime = 1.0f / 60.0f;
                ImGui::NewFrame();
                ApplyGalleryTheme();

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(1140.0f, kDemoTabHeights[tabIdx]), ImGuiCond_Always);
                if (ImGui::Begin(hostWinTitle, nullptr,
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse)) {
                    const ImGuiID demoId = ImGui::GetID(demoScopeId);
                    ImGeeDemoDetail::ImGeeDemoState& demoSt = ImGeeDemoDetail::GetOrCreateDemoState(demoId);
                    ImGeeDemoDetail::ApplyDemoTheme(demoSt, targetDemoTheme);
                    if (tabIdx == 5 && f == 0) {
                        demoSt.picker_file = "imgee.h";
                        demoSt.file_preview_path = "imgee.h";
                        demoSt.file_preview_content =
                            "// ImGee: Master Suite Umbrella Header (imgee.h)\n"
                            "#define IMGEE_VERSION     \"" IMGEE_VERSION "\"\n"
                            "// Single-header ergonomic UI toolkit for Dear ImGui (v1.84-v1.92+)";
                    }
                    if (tabIdx == 6 && f == 0) {
                        std::snprintf(demoSt.doc_state.search_query, sizeof(demoSt.doc_state.search_query), "%s", "ImGee");
                        ImGui::OpenDocumentSearch(demoSt.doc_state);
                        demoSt.doc_state.search_focus_requested = false; // Avoid blinking caret artifacts
                    }
                    ImGui::ShowImGeeDemoContent(demoScopeId);
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
                ImGui::Render();
            }

            char fileBuf[64];
            std::snprintf(fileBuf, sizeof(fileBuf), "fig_demo_tab%d.png", tabIdx + 1);
            finishAndExportScene({
                19 + tabIdx,
                std::string("Interactive Showcase -- ") + kDemoTabTitles[tabIdx],
                "imgee_demo.h",
                "`ImGui::ShowImGeeDemoWindow`, `ImGui::ShowImGeeDemoContent`",
                fileBuf,
                "#include \"imgee_demo.h\"\n"
                "ImGui::ShowImGeeDemoWindow(&show_demo);",
                ImVec2(0, 0), 0, 0
            });
        }
        ImGeeDemoDetail::GetForcedTabIndex() = -1;
        ApplyGalleryTheme();
    }

    }; // runAllScenes

    std::vector<SceneReport> darkReports;
    std::vector<SceneReport> lightReports;

    if (themeMode == "dark" || themeMode == "all") {
        runAllScenes(false, outDir, darkReports);
    }
    if (themeMode == "light" || themeMode == "all") {
        const std::string lightOutDir = (themeMode == "light") ? outDir : (outDir + "/light");
        runAllScenes(true, lightOutDir, lightReports);
    }

    // Exercise ImGee::Shutdown() end-to-end and verify GPU texture cleanup
    ImGee::Shutdown();
    if (gl.hostCreated <= 0 || gl.hostDeleted != gl.hostCreated) {
        std::fprintf(stderr, "[ERROR] HostBackend texture leak check failed (created=%d, deleted=%d).\n",
                     gl.hostCreated, gl.hostDeleted);
        return 1;
    }

    gl.Shutdown(ctx);
    ImGui::DestroyContext(ctx);
    RemoveTreeFiles(cleanupFiles, { sampleSub2, sampleSub1, sampleRoot });

    const auto tEnd = std::chrono::steady_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    // ========================================================================
    // Emit Annotated Visual Review Sheet(s)
    // ========================================================================
    auto emitPreviewMd = [&](const std::string& mdPath, const std::string& currentOutDir,
                             const std::vector<SceneReport>& currentReports, bool isLight) {
        if (currentReports.empty()) return true;
        const size_t slashPos = mdPath.find_last_of("/\\");
        if (slashPos != std::string::npos) {
            EnsureDirExists(mdPath.substr(0, slashPos));
        }

        const std::string imgRelPrefix =
            (mdPath.compare(0, 5, "docs/") == 0) ? (isLight ? "images/light/" : "images/") : ("../" + currentOutDir + "/");

        std::ofstream md(mdPath.c_str(), std::ios::trunc);
        if (!md.good()) {
            std::fprintf(stderr, "[ERROR] Could not open '%s' for writing.\n", mdPath.c_str());
            return false;
        }

        md << "# ImGee 2x Retina Widget & Extension Visual Gallery (" << (isLight ? "Paper Light" : "Dark Studio") << ")\n\n";
        md << "> [!NOTE]\n";
        md << "> Auto-generated headlessly by `tools/render_gallery.cpp` via offscreen macOS `CGL` + 4x MSAA OpenGL `2x` Retina FBO (`DisplayFramebufferScale = 2.0x`) in **"
           << static_cast<int>(std::round(elapsedMs)) << " ms** (" << currentReports.size()
           << " scenes, `HostBackend` textures created/freed: `" << gl.hostCreated << "/" << gl.hostDeleted << "`).\n\n";

        md << "### Complete Showcase Catalog\n\n";
        md << "| # | Showcase Scene | Source Header | Retina PNG |\n";
        md << "| :---: | :--- | :--- | :---: |\n";
        for (size_t i = 0; i < currentReports.size(); ++i) {
            const SceneReport& r = currentReports[i];
            md << "| " << r.index << " | **" << r.title << "** | `" << r.sourceHeader
               << "` | `" << r.pxW << " x " << r.pxH << "` |\n";
        }
        md << "\n<!-- pagebreak -->\n\n";

        for (size_t i = 0; i < currentReports.size(); ++i) {
            const SceneReport& r = currentReports[i];
            md << "<!-- keep-together -->\n";
            md << "## " << r.index << ". " << r.title << "\n\n";
            md << "* **Header:** `" << r.sourceHeader << "`  \n";
            md << "* **Primary Symbols:** " << r.symbols << "  \n";
            md << "* **Output File:** `" << currentOutDir << "/" << r.fileName << "` -- Logical `"
               << static_cast<int>(r.logicalSize.x) << " x " << static_cast<int>(r.logicalSize.y)
               << "` -> Retina **`" << r.pxW << " x " << r.pxH << " px`** (`144 DPI`)\n\n";
            md << "```cpp\n" << r.snippet << "\n```\n\n";
            md << "![" << r.title << "](" << imgRelPrefix << r.fileName << ")\n";
            md << "<!-- /keep-together -->\n\n";
            if (i + 1 < currentReports.size()) {
                md << "---\n\n";
            }
        }
        return true;
    };

    if (!darkReports.empty()) {
        emitPreviewMd(previewMd, outDir, darkReports, false);
    }
    if (!lightReports.empty()) {
        const std::string lightPreviewMd = (themeMode == "light") ? previewMd : "build/Gallery_Preview_Light.md";
        const std::string lightOutDir = (themeMode == "light") ? outDir : (outDir + "/light");
        emitPreviewMd(lightPreviewMd, lightOutDir, lightReports, true);
    }

    std::printf("[OK] Rendered %zu dark + %zu light 2x Retina gallery scenes in %.1f ms (HostBackend textures %d/%d freed).\n",
                darkReports.size(), lightReports.size(), elapsedMs, gl.hostCreated, gl.hostDeleted);
    return 0;
}
