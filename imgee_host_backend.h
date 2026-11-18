// ============================================================================
// ImGee: Host Backend & Platform Integration Contract (imgee_host_backend.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE)
//
// Defines the lightweight, optional C-callable host backend table used by ImGee
// subsystems (such as MarkdownImageService and StyledText) to upload/destroy
// GPU textures, resolve named in-memory textures, resolve relative asset paths,
// query host scaling, and coordinate shutdown cleanup.
//
// - Pure Standalone Dear ImGui Hosts (GLFW / SDL / Win32 / Metal / Vulkan):
//   Register texture upload/destroy callbacks once via ImGee::SetHostBackend()
//   and call ImGee::Shutdown() prior to tearing down your graphics device.
// - X-Plane / ImgWindow Hosts:
//   Zero configuration required -- when ImgWindow.h is available (and
//   IMGEE_NO_IMGWINDOW_BRIDGE is not defined), ImGee bridges to ImgWindow
//   automatically out of the box.
// ============================================================================

#pragma once

#ifndef IMGEE_HOST_BACKEND_H
#define IMGEE_HOST_BACKEND_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include "imgui.h"
#include <cstddef>
#include <vector>
#include <algorithm>

namespace ImGui {

#ifndef IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_NAMED_TEXTURE_RESOLVER_DEFINED
    namespace Detail {
        typedef ImTextureID (*NamedTextureResolverFn)(const char* name, ImVec2* outSize);
        inline NamedTextureResolverFn& GetNamedTextureResolver() {
            static NamedTextureResolverFn s_Resolver = nullptr;
            return s_Resolver;
        }
    }
#endif
#ifndef IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
#define IMGUI_SET_NAMED_TEXTURE_RESOLVER_DEFINED
    inline void SetNamedTextureResolver(Detail::NamedTextureResolverFn fn) {
        Detail::GetNamedTextureResolver() = fn;
    }
#endif

} // namespace ImGui

namespace ImGee {

    struct HostBackend {
        // Upload RGBA8 pixel buffer (width * height * 4 bytes) and return Dear ImGui texture handle
        ImTextureID (*CreateTextureRGBA8)(const unsigned char* pixels, int width, int height, void* userData) = nullptr;
        // Destroy texture previously created via CreateTextureRGBA8
        void        (*DeleteTexture)(ImTextureID texId, void* userData) = nullptr;
        // Optional: resolve a named in-memory/host texture alias (e.g. ![Alt](logo) or <img name="logo" />)
        ImTextureID (*ResolveNamedTexture)(const char* name, ImVec2* outSize, void* userData) = nullptr;
        // Optional: resolve a relative asset URL/path to an existing filesystem path
        bool        (*ResolveAssetPath)(const char* relativeUrl, char* outPath, size_t outPathCap, void* userData) = nullptr;
        // Optional: return UI/extension scale factor for the active window (defaults to 1.0f)
        float       (*GetScaleFactor)(void* userData) = nullptr;
        // Optional: register a shutdown callback with the host lifecycle manager
        void        (*AddShutdownHandler)(void (*handler)(), void* userData) = nullptr;
        void*       UserData = nullptr;
    };

    namespace Detail {
        typedef void (*ShutdownHandlerFn)();

        inline HostBackend& GetHostBackendStorage() {
            static HostBackend s_Backend;
            return s_Backend;
        }

        inline std::vector<ShutdownHandlerFn>& GetShutdownHandlers() {
            static std::vector<ShutdownHandlerFn> s_Handlers;
            return s_Handlers;
        }

        struct HostShutdownRegistration {
            void  (*addFn)(void (*)(), void*) = nullptr;
            void* userData = nullptr;
        };

        inline HostShutdownRegistration& GetLastHostShutdownRegistration() {
            static HostShutdownRegistration s_Last;
            return s_Last;
        }
    } // namespace Detail

    /** Registers an internal or application teardown callback invoked by ImGee::Shutdown(). */
    inline void AddShutdownHandler(void (*handler)()) {
        if (!handler) return;
        auto& handlers = Detail::GetShutdownHandlers();
        if (std::find(handlers.begin(), handlers.end(), handler) == handlers.end()) {
            handlers.push_back(handler);
        }
    }

    /** Purges ImGee subsystem caches (such as Markdown GPU textures) prior to graphics teardown. */
    inline void Shutdown() {
        auto& handlers = Detail::GetShutdownHandlers();
        for (auto fn : handlers) {
            if (fn) fn();
        }
    }

    /** Returns the active host backend configuration. */
    inline const HostBackend& GetHostBackend() {
        return Detail::GetHostBackendStorage();
    }

    /** Installs or updates the active host backend callbacks. */
    inline void SetHostBackend(const HostBackend& backend) {
        Detail::GetHostBackendStorage() = backend;
        if (backend.AddShutdownHandler) {
            auto& last = Detail::GetLastHostShutdownRegistration();
            if (last.addFn != backend.AddShutdownHandler || last.userData != backend.UserData) {
                last.addFn = backend.AddShutdownHandler;
                last.userData = backend.UserData;
                backend.AddShutdownHandler(&Shutdown, backend.UserData);
            }
        }
    }

    /** Queries the host backend scale factor (or 1.0f if unset). */
    inline float GetHostScaleFactor() {
        const HostBackend& backend = Detail::GetHostBackendStorage();
        if (backend.GetScaleFactor) {
            float s = backend.GetScaleFactor(backend.UserData);
            if (s > 0.0f) return s;
        }
        return 1.0f;
    }

} // namespace ImGee

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGEE_HOST_BACKEND_H
