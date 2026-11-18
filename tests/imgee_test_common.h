// ============================================================================
// ImGee Standalone Test Helpers (tests/imgee_test_common.h)
// ============================================================================

#pragma once

#include <cstdio>
#include <cstdlib>

#define IMGEE_TEST_CHECK(expr)                                                      \
    do {                                                                            \
        if (!(expr)) {                                                              \
            std::fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr);  \
            std::exit(1);                                                           \
        }                                                                           \
    } while (0)

namespace ImGeeTests {
    void RunReverseOrderKnobsFrame();
    void RunReverseOrderTogglesAndPopupsFrame();
    int  VerifyHostileMacrosCompilation();
    int  VerifyStandaloneOptOuts();
}
