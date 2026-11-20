// ============================================================================
// ImGui Contrib Markdown: Default Image Service & Cache (Tiers 1, 2 & 3)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Implements 3-tier image resolution for Markdown rendering:
//   Tier 1: In-memory named textures (via ImgWindow::GetNamedTexture or SetNamedTextureResolver)
//   Tier 2: Local disk image files (searched across CWD, X-Plane root & calling plugin folders)
//   Tier 3: Asynchronous remote HTTP/HTTPS downloads (libcurl/CLI on POSIX, WinINet on Windows)
// ============================================================================

#pragma once

#ifndef IMGUI_MARKDOWN_IMAGE_SERVICE_H
#define IMGUI_MARKDOWN_IMAGE_SERVICE_H

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#endif

#include <string>
#include <vector>
#include <map>
#if !defined(_WIN32)
#include <mutex>
#include <thread>
#endif
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <sys/stat.h>
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#include <algorithm>
#include <utility>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "imgui_markdown.h"

#if defined(__has_include)
    #if __has_include("../../imgee_host_backend.h")
        #include "../../imgee_host_backend.h"
    #elif __has_include("imgee_host_backend.h")
        #include "imgee_host_backend.h"
    #endif
#else
    #include "../../imgee_host_backend.h"
#endif

// If ImgWindow is available (and not explicitly disabled), use its texture upload driver & XPLM path helpers
#if !defined(IMGEE_NO_IMGWINDOW_BRIDGE) && (defined(IMGWINDOW_H) || (defined(__has_include) && __has_include("ImgWindow.h")))
    #if !defined(IMGWINDOW_H)
        #include "ImgWindow.h"
    #endif
    #define IMGEE_HAS_IMGWINDOW 1
    #if defined(__has_include)
        #if __has_include(<XPLMUtilities.h>) && __has_include(<XPLMPlugin.h>)
            #include <XPLMUtilities.h>
            #include <XPLMPlugin.h>
            #define IMGEE_HAS_XPLM_PATHS 1
        #endif
    #else
        #include <XPLMUtilities.h>
        #include <XPLMPlugin.h>
        #define IMGEE_HAS_XPLM_PATHS 1
    #endif
#endif

// Default local path containment policy:
//   1 = Treat leading '/' or '\' on Markdown image URLs as relative paths (default when bridged to ImgWindow / X-Plane)
//   0 = Preserve native OS absolute paths such as /path/to/img.png (default in pure standalone builds)
#ifndef IMGEE_FILEPATHS_FORCE_RELATIVE
    #if defined(IMGEE_HAS_IMGWINDOW)
        #define IMGEE_FILEPATHS_FORCE_RELATIVE 1
    #else
        #define IMGEE_FILEPATHS_FORCE_RELATIVE 0
    #endif
#endif

// Include ImGee's isolated internal stb_image decoder (unless disabled by host)
#if !defined(IMGEE_DISABLE_STB_IMAGE)
    #if defined(__has_include)
        #if __has_include("../../imgee_stbi_internal.h")
            #include "../../imgee_stbi_internal.h"
        #elif __has_include("imgee_stbi_internal.h")
            #include "imgee_stbi_internal.h"
        #endif
    #else
        #include "../../imgee_stbi_internal.h"
    #endif
#endif

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

namespace MarkdownImageService {

inline unsigned char* DecodeImageMemoryInternal(const void* data, size_t len, int* outW, int* outH) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    return ImGeeStbi::LoadFromMemory(data, len, outW, outH);
#else
    (void)data; (void)len;
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    return nullptr;
#endif
}

inline unsigned char* DecodeImageFileInternal(const char* filename, int* outW, int* outH) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    return ImGeeStbi::LoadFromFile(filename, outW, outH);
#else
    (void)filename;
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    return nullptr;
#endif
}

inline void FreeDecodedImageInternal(void* pixels) {
#if defined(IMGEE_HAS_STBI_INTERNAL)
    if (pixels) ImGeeStbi::ImageFree(pixels);
#else
    (void)pixels;
#endif
}

struct MarkdownDiskTextureEntry {
    ImTextureID textureID = (ImTextureID)0;
    ImVec2 size = ImVec2(0.0f, 0.0f);
    bool inFlight = false;
    bool failed = false;
    double lastAttemptTime = 0.0;
    int retryCount = 0;
};

inline std::map<std::string, MarkdownDiskTextureEntry>& GetDiskTextureCache() {
    static std::map<std::string, MarkdownDiskTextureEntry> s_Cache;
    return s_Cache;
}

struct MarkdownRemoteDecodedPayload {
    std::string url;
    uint8_t* pixels = nullptr;
    int width = 0;
    int height = 0;
    bool success = false;
    uint64_t generation = 0;
};

#if defined(_WIN32)
// Use native Win32 CRITICAL_SECTION on Windows so ImGee compiles cleanly on
// MinGW-w64's 'win32' thread model (where std::mutex / std::thread are omitted)
// and avoids introducing a runtime dependency on libwinpthread-1.dll.
struct CompletedMutex {
    CRITICAL_SECTION cs;
    CompletedMutex()  { InitializeCriticalSection(&cs); }
    ~CompletedMutex() { DeleteCriticalSection(&cs); }
    CompletedMutex(const CompletedMutex&) = delete;
    CompletedMutex& operator=(const CompletedMutex&) = delete;
    void lock()   { EnterCriticalSection(&cs); }
    void unlock() { LeaveCriticalSection(&cs); }
};

struct CompletedLockGuard {
    CompletedMutex& mtx;
    explicit CompletedLockGuard(CompletedMutex& m) : mtx(m) { mtx.lock(); }
    ~CompletedLockGuard() { mtx.unlock(); }
    CompletedLockGuard(const CompletedLockGuard&) = delete;
    CompletedLockGuard& operator=(const CompletedLockGuard&) = delete;
};
#else
using CompletedMutex = std::mutex;
using CompletedLockGuard = std::lock_guard<std::mutex>;
#endif

inline CompletedMutex& GetCompletedMutex() {
    static CompletedMutex s_Mutex;
    return s_Mutex;
}

inline std::vector<MarkdownRemoteDecodedPayload>& GetCompletedQueue() {
    static std::vector<MarkdownRemoteDecodedPayload> s_Queue;
    return s_Queue;
}

typedef bool (*RemoteFetchCallback)(const std::string& url, std::vector<uint8_t>& outData);

inline RemoteFetchCallback& GetRemoteFetchCallbackRef() {
    static RemoteFetchCallback s_Callback = nullptr;
    return s_Callback;
}

inline void SetRemoteFetchCallback(RemoteFetchCallback callback) {
    GetRemoteFetchCallbackRef() = callback;
}

inline RemoteFetchCallback GetRemoteFetchCallback() {
    return GetRemoteFetchCallbackRef();
}

inline std::atomic<uint64_t>& GetCacheGeneration() {
    static std::atomic<uint64_t> s_Generation{0};
    return s_Generation;
}

inline void ClearTextureCache() {
    GetCacheGeneration().fetch_add(1);
    {
        CompletedLockGuard lock(GetCompletedMutex());
        for (auto& item : GetCompletedQueue()) {
            if (item.pixels) {
                FreeDecodedImageInternal(item.pixels);
                item.pixels = nullptr;
            }
        }
        GetCompletedQueue().clear();
    }

    for (auto& pair : GetDiskTextureCache()) {
        if (pair.second.textureID) {
#if defined(IMGEE_HOST_BACKEND_H)
            const ImGee::HostBackend& backend = ImGee::GetHostBackend();
            if (backend.DeleteTexture) {
                backend.DeleteTexture(pair.second.textureID, backend.UserData);
                continue;
            }
#endif
#if defined(IMGEE_HAS_IMGWINDOW)
            ImgWindow::DeleteTexture(pair.second.textureID);
#endif
        }
    }
    GetDiskTextureCache().clear();
}

inline ImTextureID UploadTextureRGBA8Internal(const unsigned char* pixels, int width, int height) {
    if (!pixels || width <= 0 || height <= 0) return (ImTextureID)0;
#if defined(IMGEE_HOST_BACKEND_H)
    const ImGee::HostBackend& backend = ImGee::GetHostBackend();
    if (backend.CreateTextureRGBA8) {
        return backend.CreateTextureRGBA8(pixels, width, height, backend.UserData);
    }
#endif
#if defined(IMGEE_HAS_IMGWINDOW)
    return ImgWindow::CreateTexture(pixels, width, height);
#else
    return (ImTextureID)0;
#endif
}

inline void ProcessPendingTextures() {
    std::vector<MarkdownRemoteDecodedPayload> completed;
    {
        CompletedLockGuard lock(GetCompletedMutex());
        if (!GetCompletedQueue().empty()) {
            completed.swap(GetCompletedQueue());
        }
    }

    if (completed.empty()) {
        return;
    }

    for (auto& item : completed) {
        if (item.generation != GetCacheGeneration().load()) {
            if (item.pixels) {
                FreeDecodedImageInternal(item.pixels);
                item.pixels = nullptr;
            }
            continue;
        }

        auto& entry = GetDiskTextureCache()[item.url];
        entry.inFlight = false;

        if (item.success && item.pixels && item.width > 0 && item.height > 0) {
            ImTextureID texID = UploadTextureRGBA8Internal(item.pixels, item.width, item.height);
            FreeDecodedImageInternal(item.pixels);
            item.pixels = nullptr;

            if (texID) {
                entry.textureID = texID;
                entry.size = ImVec2(static_cast<float>(item.width), static_cast<float>(item.height));
                entry.failed = false;
                entry.retryCount = 0;
            } else {
                entry.failed = true;
                entry.lastAttemptTime = ImGui::GetTime();
            }
        } else {
            if (item.pixels) {
                FreeDecodedImageInternal(item.pixels);
                item.pixels = nullptr;
            }
            entry.failed = true;
            entry.lastAttemptTime = ImGui::GetTime();
        }
    }
}

#if !defined(_WIN32)
typedef void CURL;
typedef int CURLcode;
typedef int CURLoption;
typedef int CURLINFO;

#define IMGEE_CURLE_OK 0
#define IMGEE_CURLOPT_URL 10002
#define IMGEE_CURLOPT_FOLLOWLOCATION 52
#define IMGEE_CURLOPT_USERAGENT 10018
#define IMGEE_CURLOPT_TIMEOUT 13
#define IMGEE_CURLOPT_WRITEFUNCTION 20011
#define IMGEE_CURLOPT_WRITEDATA 10001
#define IMGEE_CURLINFO_RESPONSE_CODE 0x200002

typedef CURL* (*curl_easy_init_t)();
typedef CURLcode (*curl_easy_setopt_t)(CURL*, CURLoption, ...);
typedef CURLcode (*curl_easy_perform_t)(CURL*);
typedef void (*curl_easy_cleanup_t)(CURL*);
typedef CURLcode (*curl_easy_getinfo_t)(CURL*, CURLINFO, ...);

inline size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* mem = reinterpret_cast<std::vector<uint8_t>*>(userp);
    auto* bytes = reinterpret_cast<uint8_t*>(contents);
    mem->insert(mem->end(), bytes, bytes + total);
    return total;
}

inline bool FetchRemoteBytesCurl(const std::string& url, std::vector<uint8_t>& outBuffer) {
#if defined(__APPLE__) || defined(__MACH__)
    const char* dylibNames[] = { "libcurl.4.dylib", "libcurl.dylib", "/usr/lib/libcurl.4.dylib", nullptr };
#else
    const char* dylibNames[] = { "libcurl.so.4", "libcurl.so", nullptr };
#endif

    void* handle = nullptr;
    for (int i = 0; dylibNames[i] != nullptr; ++i) {
        handle = dlopen(dylibNames[i], RTLD_LAZY);
        if (handle) break;
    }
    if (!handle) return false;

    auto p_init = (curl_easy_init_t)dlsym(handle, "curl_easy_init");
    auto p_setopt = (curl_easy_setopt_t)dlsym(handle, "curl_easy_setopt");
    auto p_perform = (curl_easy_perform_t)dlsym(handle, "curl_easy_perform");
    auto p_cleanup = (curl_easy_cleanup_t)dlsym(handle, "curl_easy_cleanup");
    auto p_getinfo = (curl_easy_getinfo_t)dlsym(handle, "curl_easy_getinfo");

    if (!p_init || !p_setopt || !p_perform || !p_cleanup) {
        dlclose(handle);
        return false;
    }

    CURL* curl = p_init();
    if (!curl) {
        dlclose(handle);
        return false;
    }

    outBuffer.clear();
    p_setopt(curl, IMGEE_CURLOPT_URL, url.c_str());
    p_setopt(curl, IMGEE_CURLOPT_FOLLOWLOCATION, 1L);
    p_setopt(curl, IMGEE_CURLOPT_USERAGENT, "ImGee/1.0.0");
    p_setopt(curl, IMGEE_CURLOPT_TIMEOUT, 10L);
    p_setopt(curl, IMGEE_CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    p_setopt(curl, IMGEE_CURLOPT_WRITEDATA, &outBuffer);

    CURLcode res = p_perform(curl);
    long http_code = 200;
    if (res == IMGEE_CURLE_OK && p_getinfo) {
        p_getinfo(curl, IMGEE_CURLINFO_RESPONSE_CODE, &http_code);
    }

    p_cleanup(curl);
    dlclose(handle);

    return (res == IMGEE_CURLE_OK && http_code >= 200 && http_code < 300 && !outBuffer.empty());
}

inline bool FetchRemoteBytesCLI(const std::string& url, std::vector<uint8_t>& outBuffer) {
    if (url.find_first_of("'\"`;\r\n") != std::string::npos) {
        return false;
    }
    std::string cmd = "curl -s -L --max-time 10 '" + url + "'";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return false;

    uint8_t chunk[4096];
    size_t n = 0;
    outBuffer.clear();
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        outBuffer.insert(outBuffer.end(), chunk, chunk + n);
    }
    int status = pclose(fp);
    return (status == 0 && !outBuffer.empty());
}
#endif

#if defined(_WIN32)
inline bool FetchRemoteBytesWin(const std::string& url, std::vector<uint8_t>& outBuffer) {
    HMODULE hWinINet = LoadLibraryA("wininet.dll");
    if (!hWinINet) return false;

    typedef void* (__stdcall *InternetOpenA_t)(const char*, unsigned long, const char*, const char*, unsigned long);
    typedef void* (__stdcall *InternetOpenUrlA_t)(void*, const char*, const char*, unsigned long, unsigned long, size_t);
    typedef int   (__stdcall *InternetReadFile_t)(void*, void*, unsigned long, unsigned long*);
    typedef int   (__stdcall *InternetCloseHandle_t)(void*);

    auto p_Open    = reinterpret_cast<InternetOpenA_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetOpenA")));
    auto p_OpenUrl = reinterpret_cast<InternetOpenUrlA_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetOpenUrlA")));
    auto p_Read    = reinterpret_cast<InternetReadFile_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetReadFile")));
    auto p_Close   = reinterpret_cast<InternetCloseHandle_t>(reinterpret_cast<void*>(GetProcAddress(hWinINet, "InternetCloseHandle")));

    if (!p_Open || !p_OpenUrl || !p_Read || !p_Close) {
        FreeLibrary(hWinINet);
        return false;
    }

    void* hInternet = p_Open("ImGee/1.0.0", 0, nullptr, nullptr, 0);
    if (!hInternet) {
        FreeLibrary(hWinINet);
        return false;
    }

    // INTERNET_FLAG_RELOAD (0x80000000) | INTERNET_FLAG_SECURE (0x00800000) | INTERNET_FLAG_NO_CACHE_WRITE (0x04000000)
    unsigned long flags = 0x80000000UL | 0x04000000UL;
    if (url.rfind("https://", 0) == 0) {
        flags |= 0x00800000UL;
    }
    void* hUrl = p_OpenUrl(hInternet, url.c_str(), nullptr, 0, flags, 0);
    if (!hUrl) {
        p_Close(hInternet);
        FreeLibrary(hWinINet);
        return false;
    }

    outBuffer.clear();
    char chunk[4096];
    unsigned long bytesRead = 0;
    while (p_Read(hUrl, chunk, sizeof(chunk), &bytesRead) && bytesRead > 0) {
        outBuffer.insert(outBuffer.end(), chunk, chunk + bytesRead);
    }

    p_Close(hUrl);
    p_Close(hInternet);
    FreeLibrary(hWinINet);
    return !outBuffer.empty();
}
#endif

inline bool FetchRemoteBytesDefault(const std::string& url, std::vector<uint8_t>& outBuffer) {
#if defined(_WIN32)
    if (FetchRemoteBytesWin(url, outBuffer)) return true;
#else
    if (FetchRemoteBytesCurl(url, outBuffer)) return true;
    if (FetchRemoteBytesCLI(url, outBuffer)) return true;
#endif
    return false;
}

inline void FetchRemoteImageWorker(std::string url, uint64_t gen) {
    if (GetCacheGeneration().load() != gen) {
        return;
    }

    // Detect shields.io vector badge URLs lacking a raster extension and query the PNG raster variant:
    std::string fetchUrl = url;
    if ((fetchUrl.find("img.shields.io/") != std::string::npos ||
         fetchUrl.find("shields.io/") != std::string::npos) &&
        fetchUrl.find(".png") == std::string::npos &&
        fetchUrl.find(".jpg") == std::string::npos &&
        fetchUrl.find(".jpeg") == std::string::npos) {
        fetchUrl += ".png";
    }

    std::vector<uint8_t> rawBytes;
    bool fetchOk = false;

    if (GetRemoteFetchCallback()) {
        fetchOk = GetRemoteFetchCallback()(fetchUrl, rawBytes);
    }
    if (!fetchOk && GetCacheGeneration().load() == gen) {
        fetchOk = FetchRemoteBytesDefault(fetchUrl, rawBytes);
    }

    // Fallback: if modified fetchUrl failed, retry with original url
    if (!fetchOk && fetchUrl != url && GetCacheGeneration().load() == gen) {
        if (GetRemoteFetchCallback()) {
            fetchOk = GetRemoteFetchCallback()(url, rawBytes);
        }
        if (!fetchOk && GetCacheGeneration().load() == gen) {
            fetchOk = FetchRemoteBytesDefault(url, rawBytes);
        }
    }

    if (GetCacheGeneration().load() != gen) {
        return;
    }

    MarkdownRemoteDecodedPayload payload;
    payload.url = url;
    payload.generation = gen;

    if (fetchOk && !rawBytes.empty()) {
        int w = 0, h = 0;
        uint8_t* pixels = DecodeImageMemoryInternal(rawBytes.data(), rawBytes.size(), &w, &h);
        if (GetCacheGeneration().load() != gen) {
            if (pixels) FreeDecodedImageInternal(pixels);
            return;
        }
        if (pixels && w > 0 && h > 0) {
            payload.pixels = pixels;
            payload.width = w;
            payload.height = h;
            payload.success = true;
        } else {
            if (pixels) FreeDecodedImageInternal(pixels);
            payload.success = false;
        }
    } else {
        payload.success = false;
    }

    if (GetCacheGeneration().load() != gen) {
        if (payload.pixels) FreeDecodedImageInternal(payload.pixels);
        return;
    }

    {
        CompletedLockGuard lock(GetCompletedMutex());
        if (GetCacheGeneration().load() == gen) {
            GetCompletedQueue().push_back(payload);
        } else if (payload.pixels) {
            FreeDecodedImageInternal(payload.pixels);
        }
    }
}

#if defined(_WIN32)
struct Win32FetchWorkerArgs {
    std::string url;
    uint64_t generation;
};

inline DWORD WINAPI Win32FetchWorkerProc(LPVOID lpParam) {
    Win32FetchWorkerArgs* args = static_cast<Win32FetchWorkerArgs*>(lpParam);
    if (args) {
        std::string workerUrl = std::move(args->url);
        uint64_t workerGen = args->generation;
        delete args;
        try {
            FetchRemoteImageWorker(workerUrl, workerGen);
        } catch (...) {
        }
    }
    return 0;
}

inline bool SpawnDetachedFetchWorker(const std::string& url, uint64_t currentGen) {
    // Ensure main-thread static initialization before the worker thread starts
    (void)GetCompletedMutex();
    (void)GetCompletedQueue();
    (void)GetCacheGeneration();
    Win32FetchWorkerArgs* args = nullptr;
    try {
        args = new Win32FetchWorkerArgs{url, currentGen};
    } catch (...) {
        return false;
    }
    HANDLE hThread = CreateThread(NULL, 0, &Win32FetchWorkerProc, args, 0, NULL);
    if (!hThread) {
        delete args;
        return false;
    }
    CloseHandle(hThread);
    return true;
}
#else
inline bool SpawnDetachedFetchWorker(const std::string& url, uint64_t currentGen) {
    try {
        std::thread([url, currentGen]() {
            FetchRemoteImageWorker(url, currentGen);
        }).detach();
        return true;
    } catch (...) {
        return false;
    }
}
#endif

struct MarkdownImageModifiers {
    std::string clean_url;
    float width = -1.0f;
    float height = -1.0f;
    bool width_is_percent = false;
    float width_percent = 0.0f;
    bool height_is_percent = false;
    float height_percent = 0.0f;
    bool has_border = false;
    ImVec4 border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    ImGui::MarkdownImageAlign align = ImGui::MarkdownImageAlign::None;
    bool disable_zoom_scale = false;
    int caption_mode = 0;         // 0 = default, 1 = force show, -1 = force hide (nocaption)
    std::string custom_caption;
    std::string alt_text;
    int lightbox_mode = 0;        // 0 = default, 1 = force enable, -1 = force disable (nolightbox)
    ImGui::MarkdownImageFloat float_mode = ImGui::MarkdownImageFloat::None;
    bool has_explicit_float = false;
    float float_gap_x = -1.0f;
    float float_gap_y = -1.0f;
};

inline std::string TrimMarkdownModifierWhitespace(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n')) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r' || str[end - 1] == '\n')) {
        --end;
    }
    return str.substr(start, end - start);
}

inline std::string LowerAsciiCopy(const std::string& in) {
    std::string out = in;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

inline bool ApplySingleMarkdownImageModifier(MarkdownImageModifiers& mods, const std::string& raw_key, const std::string& val, bool has_eq) {
    const std::string key = LowerAsciiCopy(raw_key);
    const std::string vlow = LowerAsciiCopy(val);

    if (key.empty() && has_eq) {
        // Shorthand dimension token: =68%, =200x100, =200, =x100
        if (!val.empty() && val.back() == '%') {
            mods.width_is_percent = true;
            mods.width_percent = static_cast<float>(std::atof(val.c_str()));
        } else {
            size_t x_pos = val.find_first_of("xX");
            if (x_pos != std::string::npos) {
                std::string sw = val.substr(0, x_pos);
                std::string sh = val.substr(x_pos + 1);
                if (!sw.empty()) mods.width = static_cast<float>(std::atof(sw.c_str()));
                if (!sh.empty()) mods.height = static_cast<float>(std::atof(sh.c_str()));
            } else if (!val.empty()) {
                mods.width = static_cast<float>(std::atof(val.c_str()));
            }
        }
        return true;
    }

    if (key == "src") {
        mods.clean_url = val;
        return true;
    }
    if (key == "alt" || key == "title") {
        if (has_eq && mods.alt_text.empty()) {
            mods.alt_text = val;
        }
        return true;
    }
    if (key == "width" || key == "w") {
        if (!val.empty() && val.back() == '%') {
            mods.width_is_percent = true;
            mods.width_percent = static_cast<float>(std::atof(val.c_str()));
        } else if (!val.empty()) {
            mods.width = static_cast<float>(std::atof(val.c_str()));
        }
        return true;
    }
    if (key == "height" || key == "h") {
        if (!val.empty() && val.back() == '%') {
            mods.height_is_percent = true;
            mods.height_percent = static_cast<float>(std::atof(val.c_str()));
        } else if (!val.empty()) {
            mods.height = static_cast<float>(std::atof(val.c_str()));
        }
        return true;
    }
    if (key == "border") {
        if (!has_eq || val.empty() || vlow == "1" || vlow == "true" || vlow == "on") {
            mods.has_border = true;
            mods.border_col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        } else if (vlow == "0" || vlow == "false" || vlow == "none" || vlow == "off") {
            mods.has_border = false;
        } else {
            mods.has_border = true;
            ImGui::ParseMarkdownColor(val, mods.border_col);
        }
        return true;
    }

    // Horizontal alignment: left, center/centered/centre, right, align=...
    if (!has_eq && key == "left") {
        mods.align = ImGui::MarkdownImageAlign::Left;
        return true;
    }
    if (!has_eq && (key == "center" || key == "centered" || key == "centre")) {
        mods.align = ImGui::MarkdownImageAlign::Center;
        return true;
    }
    if (!has_eq && key == "right") {
        mods.align = ImGui::MarkdownImageAlign::Right;
        return true;
    }
    if (key == "align") {
        if (vlow == "left") mods.align = ImGui::MarkdownImageAlign::Left;
        else if (vlow == "center" || vlow == "centered" || vlow == "centre" || vlow == "middle") mods.align = ImGui::MarkdownImageAlign::Center;
        else if (vlow == "right") mods.align = ImGui::MarkdownImageAlign::Right;
        else if (vlow == "none" || vlow == "inline") mods.align = ImGui::MarkdownImageAlign::None;
        return true;
    }

    // Zoom scaling opt-out: nozoom, no-zoom, scale=fixed, scale=1:1
    if (!has_eq && (key == "nozoom" || key == "no-zoom" || key == "no_zoom" || key == "fixed")) {
        mods.disable_zoom_scale = true;
        return true;
    }
    if (key == "scale") {
        if (vlow == "fixed" || vlow == "1:1" || vlow == "1" || vlow == "none" || vlow == "off") {
            mods.disable_zoom_scale = true;
        } else if (vlow == "auto" || vlow == "zoom" || vlow == "on") {
            mods.disable_zoom_scale = false;
        }
        return true;
    }

    // Figure captions: caption, figcaption, caption="...", nocaption
    if (!has_eq && (key == "caption" || key == "figcaption")) {
        mods.caption_mode = 1;
        return true;
    }
    if (!has_eq && (key == "nocaption" || key == "no-caption" || key == "no_caption")) {
        mods.caption_mode = -1;
        mods.custom_caption.clear();
        return true;
    }
    if (key == "caption" || key == "figcaption") {
        if (!has_eq || val.empty() || vlow == "1" || vlow == "true" || vlow == "on" || vlow == "auto") {
            mods.caption_mode = 1;
        } else if (vlow == "0" || vlow == "false" || vlow == "none" || vlow == "off") {
            mods.caption_mode = -1;
            mods.custom_caption.clear();
        } else {
            mods.caption_mode = 1;
            mods.custom_caption = val;
        }
        return true;
    }

    // In-context lightbox: lightbox, zoom, expand, nolightbox
    if (!has_eq && (key == "lightbox" || key == "zoom" || key == "expand" || key == "enlarge")) {
        mods.lightbox_mode = 1;
        return true;
    }
    if (!has_eq && (key == "nolightbox" || key == "no-lightbox" || key == "no_lightbox" || key == "noexpand" || key == "no-expand")) {
        mods.lightbox_mode = -1;
        return true;
    }
    if (key == "lightbox" || key == "expand") {
        if (!has_eq || val.empty() || vlow == "1" || vlow == "true" || vlow == "on" || vlow == "auto") {
            mods.lightbox_mode = 1;
        } else if (vlow == "0" || vlow == "false" || vlow == "none" || vlow == "off") {
            mods.lightbox_mode = -1;
        }
        return true;
    }
    if (key == "zoom" && has_eq) {
        if (vlow == "0" || vlow == "false" || vlow == "none" || vlow == "off") {
            mods.lightbox_mode = -1;
        } else if (vlow == "1" || vlow == "true" || vlow == "on" || vlow == "lightbox") {
            mods.lightbox_mode = 1;
        }
        return true;
    }

    // Text-wrapping image floats: float=left|right, float-left|right, wrap=left|right, gap=..., margin=...
    if (!has_eq && (key == "float-left" || key == "float_left" || key == "wrap-left" || key == "wrap_left")) {
        mods.float_mode = ImGui::MarkdownImageFloat::Left;
        mods.has_explicit_float = true;
        return true;
    }
    if (!has_eq && (key == "float-right" || key == "float_right" || key == "wrap-right" || key == "wrap_right")) {
        mods.float_mode = ImGui::MarkdownImageFloat::Right;
        mods.has_explicit_float = true;
        return true;
    }
    if (!has_eq && (key == "nofloat" || key == "no-float" || key == "no_float" || key == "nowrap" || key == "no-wrap")) {
        mods.float_mode = ImGui::MarkdownImageFloat::None;
        mods.has_explicit_float = true;
        return true;
    }
    if (key == "float" || key == "wrap") {
        if (vlow == "left") {
            mods.float_mode = ImGui::MarkdownImageFloat::Left;
            mods.has_explicit_float = true;
        } else if (vlow == "right") {
            mods.float_mode = ImGui::MarkdownImageFloat::Right;
            mods.has_explicit_float = true;
        } else if (vlow == "none" || vlow == "off" || vlow == "false" || vlow == "0") {
            mods.float_mode = ImGui::MarkdownImageFloat::None;
            mods.has_explicit_float = true;
        }
        return true;
    }
    if ((key == "gap" || key == "margin") && has_eq && !val.empty()) {
        size_t x_pos = val.find_first_of("xX");
        if (x_pos != std::string::npos) {
            std::string gx = val.substr(0, x_pos);
            std::string gy = val.substr(x_pos + 1);
            if (!gx.empty()) mods.float_gap_x = static_cast<float>(std::atof(gx.c_str()));
            if (!gy.empty()) mods.float_gap_y = static_cast<float>(std::atof(gy.c_str()));
        } else {
            mods.float_gap_x = static_cast<float>(std::atof(val.c_str()));
        }
        return true;
    }
    if ((key == "hgap" || key == "hmargin") && has_eq && !val.empty()) {
        mods.float_gap_x = static_cast<float>(std::atof(val.c_str()));
        return true;
    }
    if ((key == "vgap" || key == "vmargin") && has_eq && !val.empty()) {
        mods.float_gap_y = static_cast<float>(std::atof(val.c_str()));
        return true;
    }

    return false;
}

inline void ParseMarkdownImageModifierList(const std::string& attr_str, MarkdownImageModifiers& mods, bool stop_on_html_close) {
    size_t p = 0;
    const size_t n = attr_str.size();
    while (p < n) {
        while (p < n && (attr_str[p] == ' ' || attr_str[p] == '\t' || attr_str[p] == '\r' || attr_str[p] == '\n' || attr_str[p] == '{' || attr_str[p] == '}')) {
            ++p;
        }
        if (p >= n) break;
        if (stop_on_html_close && (attr_str[p] == '/' || attr_str[p] == '>')) {
            ++p;
            continue;
        }

        // Ignore standalone quoted title strings (e.g. ![alt](url "Tooltip title"))
        if (attr_str[p] == '"' || attr_str[p] == '\'') {
            char q = attr_str[p++];
            while (p < n && attr_str[p] != q) {
                if (attr_str[p] == '\\' && p + 1 < n) ++p;
                ++p;
            }
            if (p < n) ++p;
            continue;
        }

        size_t key_start = p;
        while (p < n && attr_str[p] != '=' && attr_str[p] != ' ' && attr_str[p] != '\t' && attr_str[p] != '\r' && attr_str[p] != '\n' && attr_str[p] != '}') {
            if (stop_on_html_close && (attr_str[p] == '/' || attr_str[p] == '>')) break;
            ++p;
        }
        std::string key = attr_str.substr(key_start, p - key_start);

        bool has_eq = false;
        std::string val;
        if (p < n && attr_str[p] == '=') {
            has_eq = true;
            ++p;
            while (p < n && (attr_str[p] == ' ' || attr_str[p] == '\t')) ++p;
            if (p < n && (attr_str[p] == '"' || attr_str[p] == '\'')) {
                char q = attr_str[p++];
                while (p < n && attr_str[p] != q) {
                    if (attr_str[p] == '\\' && p + 1 < n && (attr_str[p + 1] == q || attr_str[p + 1] == '\\')) {
                        ++p;
                    }
                    val.push_back(attr_str[p++]);
                }
                if (p < n && attr_str[p] == q) ++p;
            } else {
                size_t vstart = p;
                while (p < n && attr_str[p] != ' ' && attr_str[p] != '\t' && attr_str[p] != '\r' && attr_str[p] != '\n' && attr_str[p] != '}') {
                    if (stop_on_html_close && (attr_str[p] == '/' || attr_str[p] == '>')) break;
                    ++p;
                }
                val = attr_str.substr(vstart, p - vstart);
            }
        }

        ApplySingleMarkdownImageModifier(mods, key, val, has_eq);
    }
}

inline MarkdownImageModifiers ParseMarkdownImageModifiers(const std::string& raw_input) {
    MarkdownImageModifiers mods;
    std::string s = TrimMarkdownModifierWhitespace(raw_input);
    if (s.empty()) return mods;

    // 1. HTML <img ... /> attribute string containing "src=" (synthesized by imgui_markdown_ext.h)
    const std::string s_low = LowerAsciiCopy(s);
    size_t src_pos = s_low.find("src=");
    if (src_pos != std::string::npos && (src_pos == 0 || s[src_pos - 1] == ' ' || s[src_pos - 1] == '\t')) {
        ParseMarkdownImageModifierList(s, mods, true);
        // Match HTML4 / GitHub GFM <img align="left|right"> float semantics unless explicitly overridden:
        if (!mods.has_explicit_float) {
            if (mods.align == ImGui::MarkdownImageAlign::Left) {
                mods.float_mode = ImGui::MarkdownImageFloat::Left;
            } else if (mods.align == ImGui::MarkdownImageAlign::Right) {
                mods.float_mode = ImGui::MarkdownImageFloat::Right;
            }
        }
        return mods;
    }

    // 2. CommonMark / Extended URL & Pandoc attribute block syntax
    std::string url;
    std::string remainder;

    if (s.front() == '<') {
        char q = '\0';
        size_t end_angle = std::string::npos;
        for (size_t k = 1; k < s.size(); ++k) {
            if (q != '\0') {
                if (s[k] == q && s[k - 1] != '\\') q = '\0';
                continue;
            }
            if (s[k] == '"' || s[k] == '\'') { q = s[k]; continue; }
            if (s[k] == '>') { end_angle = k; break; }
        }

        std::string inside = (end_angle != std::string::npos) ? s.substr(1, end_angle - 1) : s.substr(1);
        std::string after = (end_angle != std::string::npos) ? s.substr(end_angle + 1) : std::string();
        inside = TrimMarkdownModifierWhitespace(inside);

        // Tokenize inside <...> (quote-aware) and peel trailing recognized modifiers from the right
        // so both <pic.png =68% center> and <folder with spaces/pic.png =68% center> preserve URLs cleanly!
        struct SpanTok { size_t start; size_t end; std::string key; std::string val; bool has_eq; };
        std::vector<SpanTok> toks;
        size_t tp = 0;
        while (tp < inside.size()) {
            while (tp < inside.size() && (inside[tp] == ' ' || inside[tp] == '\t')) ++tp;
            if (tp >= inside.size()) break;
            size_t t_start = tp;
            size_t k_start = tp;
            while (tp < inside.size() && inside[tp] != '=' && inside[tp] != ' ' && inside[tp] != '\t') ++tp;
            std::string t_key = inside.substr(k_start, tp - k_start);
            bool t_eq = false;
            std::string t_val;
            if (tp < inside.size() && inside[tp] == '=') {
                t_eq = true;
                ++tp;
                if (tp < inside.size() && (inside[tp] == '"' || inside[tp] == '\'')) {
                    char tq = inside[tp++];
                    size_t vs = tp;
                    while (tp < inside.size() && inside[tp] != tq) {
                        if (inside[tp] == '\\' && tp + 1 < inside.size()) ++tp;
                        ++tp;
                    }
                    t_val = inside.substr(vs, tp - vs);
                    if (tp < inside.size() && inside[tp] == tq) ++tp;
                } else {
                    size_t vs = tp;
                    while (tp < inside.size() && inside[tp] != ' ' && inside[tp] != '\t') ++tp;
                    t_val = inside.substr(vs, tp - vs);
                }
            }
            toks.push_back({ t_start, tp, t_key, t_val, t_eq });
        }

        size_t mod_first_tok = toks.size();
        while (mod_first_tok > 1) {
            MarkdownImageModifiers probe;
            const SpanTok& cand = toks[mod_first_tok - 1];
            if (ApplySingleMarkdownImageModifier(probe, cand.key, cand.val, cand.has_eq)) {
                --mod_first_tok;
            } else {
                break;
            }
        }

        if (mod_first_tok < toks.size()) {
            url = TrimMarkdownModifierWhitespace(inside.substr(0, toks[mod_first_tok].start));
            remainder = inside.substr(toks[mod_first_tok].start);
            if (!after.empty()) {
                remainder += ' ';
                remainder += after;
            }
        } else {
            url = inside;
            remainder = after;
        }
    } else if (s.front() == '"' || s.front() == '\'') {
        char quote = s.front();
        size_t end_quote = s.find(quote, 1);
        if (end_quote != std::string::npos) {
            url = s.substr(1, end_quote - 1);
            remainder = s.substr(end_quote + 1);
        } else {
            url = s.substr(1);
        }
    } else {
        size_t sep = s.find_first_of(" \t{");
        if (sep != std::string::npos) {
            url = s.substr(0, sep);
            remainder = s.substr(sep);
        } else {
            url = s;
        }
    }

    mods.clean_url = url;
    remainder = TrimMarkdownModifierWhitespace(remainder);
    if (!remainder.empty()) {
        ParseMarkdownImageModifierList(remainder, mods, false);
    }
    return mods;
}

inline void ApplyMarkdownImageModifiers(ImGui::MarkdownImageData& img, const MarkdownImageModifiers& mods, const ImVec2& native_size, bool preserve_existing_border = false) {
    const float avail_w = ImGui::MarkdownDetail::ResolveFullLineAvailWidth(ImGui::GetActiveMarkdownConfig());

    if (mods.has_border) {
        img.border_col = mods.border_col;
    } else if (!preserve_existing_border) {
        img.border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const float base_w = (native_size.x > 0.0f) ? native_size.x : 64.0f;
    const float base_h = (native_size.y > 0.0f) ? native_size.y : 64.0f;
    img.native_size = ImVec2(base_w, base_h);
    img.align = mods.align;
    img.captionMode = mods.caption_mode;
    img.customCaption = mods.custom_caption;
    img.altOverride = mods.alt_text;
    img.lightboxMode = mods.lightbox_mode;
    img.floatMode = mods.float_mode;
    img.floatGapX = mods.float_gap_x;
    img.floatGapY = mods.float_gap_y;

    float target_w = base_w;
    float target_h = base_h;
    const float aspect = (base_w > 0.0f) ? (base_h / base_w) : 1.0f;

    if (mods.width_is_percent) {
        const float ref_w = (avail_w > 10.0f) ? avail_w : (base_w > 0.0f ? base_w : 100.0f);
        target_w = std::round(ref_w * (mods.width_percent / 100.0f));
        if (mods.height > 0.0f) {
            target_h = mods.height;
        } else if (mods.height_is_percent) {
            target_h = std::round(ref_w * (mods.height_percent / 100.0f));
        } else {
            target_h = std::round(target_w * aspect);
        }
    } else {
        if (mods.width > 0.0f && mods.height > 0.0f) {
            target_w = mods.width;
            target_h = mods.height;
        } else if (mods.width > 0.0f) {
            target_w = mods.width;
            target_h = std::round(target_w * aspect);
        } else if (mods.height > 0.0f) {
            target_h = mods.height;
            target_w = (aspect > 0.0f) ? std::round(target_h / aspect) : base_w;
        }

        if (!mods.disable_zoom_scale) {
            const float zoom = ImGui::MarkdownDetail::GetActiveContentZoom();
            if (zoom > 0.0f && std::fabs(zoom - 1.0f) > 0.001f) {
                target_w = std::round(target_w * zoom);
                target_h = std::round(target_h * zoom);
            }
        }
    }

    // Auto-constraint to available viewport width (max-width: 100%; height: auto)
    if (avail_w > 10.0f && target_w > avail_w && target_w > 0.0f) {
        const float scale = avail_w / target_w;
        target_w = avail_w;
        target_h = std::round(target_h * scale);
    }

    if (target_w < 1.0f) target_w = 1.0f;
    if (target_h < 1.0f) target_h = 1.0f;

    img.size = ImVec2(target_w, target_h);
}

inline ImGui::MarkdownImageData DefaultMarkdownImageCallback(ImGui::MarkdownLinkCallbackData data) {
    if (!data.link || data.linkLength <= 0) {
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    std::string raw_input(data.link, (size_t)data.linkLength);
    MarkdownImageModifiers mods = ParseMarkdownImageModifiers(raw_input);
    const std::string& url = mods.clean_url;

    if (url.empty()) {
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    ProcessPendingTextures();

    // 1. Tier 1: Check Named Texture Resolver or ImgWindow named textures:
    ImVec2 nativeSize(0.0f, 0.0f);
    ImVec2 uv0(0.0f, 0.0f);
    ImVec2 uv1(1.0f, 1.0f);
    ImVec4 tintCol(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 borderCol(0.0f, 0.0f, 0.0f, 0.0f);
    ImTextureID namedTex = (ImTextureID)0;

    std::string key = url;
    if (key.rfind("texture://", 0) == 0) {
        key = key.substr(10);
    }

#if defined(IMGEE_HAS_IMGWINDOW)
    namedTex = ImgWindow::GetNamedTexture(key, &nativeSize, &uv0, &uv1, &tintCol, &borderCol);
    if (!namedTex && key != url) {
        namedTex = ImgWindow::GetNamedTexture(url, &nativeSize, &uv0, &uv1, &tintCol, &borderCol);
    }
#endif
#if defined(IMGEE_HOST_BACKEND_H)
    if (!namedTex) {
        const ImGee::HostBackend& backend = ImGee::GetHostBackend();
        if (backend.ResolveNamedTexture) {
            namedTex = backend.ResolveNamedTexture(key.c_str(), &nativeSize, backend.UserData);
            if (!namedTex && key != url) {
                namedTex = backend.ResolveNamedTexture(url.c_str(), &nativeSize, backend.UserData);
            }
        }
    }
#endif
    if (!namedTex && Detail::GetNamedTextureResolver()) {
        namedTex = Detail::GetNamedTextureResolver()(key.c_str(), &nativeSize);
        if (!namedTex && key != url) {
            namedTex = Detail::GetNamedTextureResolver()(url.c_str(), &nativeSize);
        }
    }

    if (namedTex) {
        ImGui::MarkdownImageData namedData;
        namedData.isValid = true;
        namedData.useLinkCallback = false;
        namedData.user_texture_id = namedTex;
        namedData.size = nativeSize;
        namedData.uv0 = uv0;
        namedData.uv1 = uv1;
        namedData.tint_col = tintCol;
        namedData.border_col = borderCol;
        bool hasStoredBorder = (borderCol.w > 0.0f);
        ApplyMarkdownImageModifiers(namedData, mods, nativeSize, hasStoredBorder);
        return namedData;
    }

    // Resolve optional active MarkdownConfig::baseDir for local relative paths:
    bool isRemoteUrl = (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0);
    const ImGui::MarkdownConfig* activeCfg = ImGui::GetActiveMarkdownConfig()
                                             ? ImGui::GetActiveMarkdownConfig()
                                             : ImGui::GetDefaultMarkdownConfig();
    std::string activeBaseDir = (!isRemoteUrl && activeCfg && activeCfg->baseDir && activeCfg->baseDir[0] != '\0')
                                ? std::string(activeCfg->baseDir)
                                : std::string();
    while (activeBaseDir.size() > 1 && (activeBaseDir.back() == '/' || activeBaseDir.back() == '\\')) {
        activeBaseDir.pop_back();
    }
    const std::string cacheKey = activeBaseDir.empty() ? url : (activeBaseDir + "|" + url);

    // 2. Check Disk & Remote Texture Cache
    auto it = GetDiskTextureCache().find(cacheKey);
    if (it != GetDiskTextureCache().end()) {
        if (it->second.inFlight) {
            ImGui::MarkdownImageData loadingData;
            loadingData.isValid = false;
            loadingData.isLoading = true;
            return loadingData;
        }
        if (it->second.failed) {
            double now = ImGui::GetTime();
            if (it->second.retryCount >= 3 || (now - it->second.lastAttemptTime < 60.0)) {
                ImGui::MarkdownImageData invalid;
                invalid.isValid = false;
                invalid.isLoading = false;
                return invalid;
            }
        } else if (it->second.textureID) {
            ImGui::MarkdownImageData img;
            img.isValid = true;
            img.isLoading = false;
            img.user_texture_id = it->second.textureID;
            img.size = it->second.size;
            ApplyMarkdownImageModifiers(img, mods, it->second.size);
            return img;
        }
    }

    // 3. Tier 3: Remote Web URLs
    if (isRemoteUrl) {
        auto& entry = GetDiskTextureCache()[cacheKey];
        entry.inFlight = true;
        entry.failed = false;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;

        uint64_t currentGen = GetCacheGeneration().load();
        if (!SpawnDetachedFetchWorker(url, currentGen)) {
            entry.inFlight = false;
            entry.failed = true;
            entry.lastAttemptTime = ImGui::GetTime();
            ImGui::MarkdownImageData invalid;
            invalid.isValid = false;
            return invalid;
        }

        ImGui::MarkdownImageData loadingData;
        loadingData.isValid = false;
        loadingData.isLoading = true;
        return loadingData;
    }

    // 4. Tier 2: Resolve File Path on Disk
    //    (baseDir -> HostBackend::ResolveAssetPath -> CWD -> X-Plane Root / Calling Plugin Folders)
    std::string resolvedPath;
    bool fileFound = false;

    auto checkCandidate = [](const std::string& candidate, struct stat& st_out) -> bool {
        return (!candidate.empty() && stat(candidate.c_str(), &st_out) == 0 && S_ISREG(st_out.st_mode));
    };

    const bool hasWinDrivePrefix = (url.size() >= 3 && std::isalpha(static_cast<unsigned char>(url[0])) &&
                                    url[1] == ':' && (url[2] == '/' || url[2] == '\\'));
    const bool hasLeadingSlash = (!url.empty() && (url[0] == '/' || url[0] == '\\'));
    const bool isAbsoluteOsPath = hasWinDrivePrefix || hasLeadingSlash;

    std::string cleanUrl = url;
    if (cleanUrl.rfind("./", 0) == 0 || cleanUrl.rfind(".\\", 0) == 0) {
        cleanUrl = cleanUrl.substr(2);
    }
#if IMGEE_FILEPATHS_FORCE_RELATIVE
    while (!cleanUrl.empty() && (cleanUrl[0] == '/' || cleanUrl[0] == '\\')) {
        cleanUrl.erase(0, 1);
    }
    if (hasWinDrivePrefix) {
        cleanUrl.clear(); // Disallow raw drive-letter absolute paths under strict relative mode
    }
#endif

    struct stat st;

#if !IMGEE_FILEPATHS_FORCE_RELATIVE
    // In standalone mode (IMGEE_FILEPATHS_FORCE_RELATIVE == 0), preserve native OS absolute paths directly:
    if (isAbsoluteOsPath && checkCandidate(url, st)) {
        resolvedPath = url;
        fileFound = true;
    }
#else
    (void)isAbsoluteOsPath;
#endif

    // (a) Resolve relative to MarkdownConfig::baseDir (when configured), including common
    //     documentation subfolder/parent layouts (baseDir/images/<file>, baseDir/../<url>, baseDir/../images/<file>)
    //     when cleanUrl contains no '..' parent-traversal segments:
    if (!fileFound && !activeBaseDir.empty() && !cleanUrl.empty()) {
        auto hasParentTraversalSegment = [](const std::string& p) -> bool {
            size_t pos = 0;
            while (pos < p.size()) {
                size_t next_sep = p.find_first_of("/\\", pos);
                std::string seg = (next_sep == std::string::npos) ? p.substr(pos) : p.substr(pos, next_sep - pos);
                if (seg == "..") return true;
                if (next_sep == std::string::npos) break;
                pos = next_sep + 1;
            }
            return false;
        };

        std::string candidate = activeBaseDir + "/" + cleanUrl;
        if (checkCandidate(candidate, st)) {
            resolvedPath = candidate;
            fileFound = true;
        } else if (!hasParentTraversalSegment(cleanUrl)) {
            size_t slash_pos = cleanUrl.find_last_of("/\\");
            std::string baseName = (slash_pos == std::string::npos) ? cleanUrl : cleanUrl.substr(slash_pos + 1);

            const std::string fallbacks[] = {
                (cleanUrl != baseName) ? (activeBaseDir + "/images/" + cleanUrl) : std::string(),
                !baseName.empty() ? (activeBaseDir + "/images/" + baseName) : std::string(),
                activeBaseDir + "/../" + cleanUrl,
                (cleanUrl != baseName) ? (activeBaseDir + "/../images/" + cleanUrl) : std::string(),
                !baseName.empty() ? (activeBaseDir + "/../images/" + baseName) : std::string()
            };

            for (size_t fi = 0; fi < sizeof(fallbacks) / sizeof(fallbacks[0]); ++fi) {
                if (!fallbacks[fi].empty() && checkCandidate(fallbacks[fi], st)) {
                    resolvedPath = fallbacks[fi];
                    fileFound = true;
                    break;
                }
            }
        }
    }

    // (b) Resolve via ImGee::HostBackend::ResolveAssetPath (when registered)
#if defined(IMGEE_HOST_BACKEND_H)
    if (!fileFound && !cleanUrl.empty()) {
        const ImGee::HostBackend& backend = ImGee::GetHostBackend();
        if (backend.ResolveAssetPath) {
            char hostBuf[1024] = {0};
            if (backend.ResolveAssetPath(cleanUrl.c_str(), hostBuf, sizeof(hostBuf), backend.UserData) && hostBuf[0] != '\0') {
                std::string candidate(hostBuf);
                if (checkCandidate(candidate, st)) {
                    resolvedPath = candidate;
                    fileFound = true;
                }
            }
        }
    }
#endif

    // (c) Resolve relative to Current Working Directory
    if (!fileFound && !cleanUrl.empty()) {
#if IMGEE_FILEPATHS_FORCE_RELATIVE
        if (checkCandidate(cleanUrl, st)) {
            resolvedPath = cleanUrl;
            fileFound = true;
        }
#else
        if (checkCandidate(url, st)) {
            resolvedPath = url;
            fileFound = true;
        } else if (cleanUrl != url && checkCandidate(cleanUrl, st)) {
            resolvedPath = cleanUrl;
            fileFound = true;
        }
#endif
    }

#if defined(IMGEE_HAS_XPLM_PATHS)
    // In XPLM fallback, also strip a leading slash if IMGEE_FILEPATHS_FORCE_RELATIVE was overridden to 0
    std::string xplmRelUrl = cleanUrl;
    while (!xplmRelUrl.empty() && (xplmRelUrl[0] == '/' || xplmRelUrl[0] == '\\')) {
        xplmRelUrl.erase(0, 1);
    }

    // (d) Try relative to X-Plane system root
    if (!fileFound && !xplmRelUrl.empty()) {
        char sysPath[512] = {0};
        XPLMGetSystemPath(sysPath);
        std::string sPath(sysPath);
        if (!sPath.empty() && sPath.back() != '/' && sPath.back() != '\\') {
            sPath += '/';
        }
        std::string candidate = sPath + xplmRelUrl;
        if (checkCandidate(candidate, st)) {
            resolvedPath = candidate;
            fileFound = true;
        }
    }

    // (e) Try relative to calling plugin directory (e.g. Resources/plugins/<Plugin>/)
    if (!fileFound && !xplmRelUrl.empty()) {
        XPLMPluginID myId = XPLMGetMyID();
        if (myId != XPLM_NO_PLUGIN_ID) {
            char pluginFilePath[512] = {0};
            XPLMGetPluginInfo(myId, nullptr, pluginFilePath, nullptr, nullptr);
            std::string pPath(pluginFilePath);
            size_t slash = pPath.find_last_of("/\\");
            if (slash != std::string::npos) {
                std::string pluginDir = pPath.substr(0, slash);
                size_t prevSlash = pluginDir.find_last_of("/\\");
                std::string parentDir = (prevSlash != std::string::npos) ? pluginDir.substr(0, prevSlash) : pluginDir;

                std::vector<std::string> searchDirs = {
                    pluginDir,
                    parentDir,
                    parentDir + "/Resources",
                    parentDir + "/images",
                    parentDir + "/notes"
                };

                for (const auto& d : searchDirs) {
                    std::string candidate = d + "/" + xplmRelUrl;
                    if (checkCandidate(candidate, st)) {
                        resolvedPath = candidate;
                        fileFound = true;
                        break;
                    }
                }
            }
        }
    }
#endif

    if (!fileFound) {
        auto& entry = GetDiskTextureCache()[cacheKey];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    int width = 0, height = 0;
    unsigned char* pixels = DecodeImageFileInternal(resolvedPath.c_str(), &width, &height);
    if (!pixels || width <= 0 || height <= 0) {
        if (pixels) FreeDecodedImageInternal(pixels);
        auto& entry = GetDiskTextureCache()[cacheKey];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    ImTextureID texID = UploadTextureRGBA8Internal(pixels, width, height);
    FreeDecodedImageInternal(pixels);

    if (!texID) {
        auto& entry = GetDiskTextureCache()[cacheKey];
        entry.inFlight = false;
        entry.failed = true;
        entry.lastAttemptTime = ImGui::GetTime();
        entry.retryCount++;
        ImGui::MarkdownImageData invalid;
        invalid.isValid = false;
        return invalid;
    }

    MarkdownDiskTextureEntry successEntry;
    successEntry.textureID = texID;
    successEntry.size = ImVec2(static_cast<float>(width), static_cast<float>(height));
    successEntry.inFlight = false;
    successEntry.failed = false;
    successEntry.retryCount = 0;
    GetDiskTextureCache()[cacheKey] = successEntry;

    ImGui::MarkdownImageData img;
    img.isValid = true;
    img.isLoading = false;
    img.user_texture_id = texID;
    img.size = successEntry.size;
    ApplyMarkdownImageModifiers(img, mods, successEntry.size);
    return img;
}

inline void EnableDefaultImageService(bool enable = true) {
    if (enable) {
        ImGui::SetDefaultMarkdownImageCallback(&DefaultMarkdownImageCallback);
    } else {
        if (ImGui::GetDefaultMarkdownImageCallback() == &DefaultMarkdownImageCallback) {
            ImGui::SetDefaultMarkdownImageCallback(nullptr);
        }
    }
}

namespace MarkdownImageDetail {
    struct AutoMarkdownImageServiceInit {
        AutoMarkdownImageServiceInit() {
            EnableDefaultImageService(true);
#if defined(IMGEE_HOST_BACKEND_H)
            ImGee::AddShutdownHandler(&ClearTextureCache);
#endif
        }
    };
    static AutoMarkdownImageServiceInit s_AutoMarkdownImageServiceInit;
}

} // namespace MarkdownImageService
} // namespace ImGui

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

#endif // IMGUI_MARKDOWN_IMAGE_SERVICE_H
