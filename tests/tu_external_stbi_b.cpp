// ============================================================================
// External Single-TU stb_image Mode Verification (TU B)
// ============================================================================

#include "imgee.h"

int ExternalStbiDecodeFromSecondTu(const unsigned char* bytes, size_t len) {
    int w = 0, h = 0;
    unsigned char* px = ImGeeStbi::LoadFromMemory(bytes, len, &w, &h);
    if (!px) return 0;
    bool ok = (w == 1 && h == 1);
    ImGeeStbi::ImageFree(px);
    return ok ? 1 : 0;
}
