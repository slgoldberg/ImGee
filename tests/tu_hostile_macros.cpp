// ============================================================================
// Hostile Pre-Include Environment Verification:
// Simulates a host TU that includes <windows.h> WITHOUT NOMINMAX or
// WIN32_LEAN_AND_MEAN and declares `using namespace std;` in C++17 mode
// BEFORE including ImGee headers.
// ============================================================================

#include <cstddef>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

using namespace std;

#include "imgee.h"
#include "imgee_test_common.h"

// Verify ImGee restored the caller's min/max macro state via pop_macro:
#if !defined(min) || !defined(max)
#error "ImGee failed to restore caller min/max macros after #include \"imgee.h\""
#endif

namespace ImGeeTests {

int VerifyHostileMacrosCompilation() {
    int low = min(42, 99);
    int high = max(10, 42);
    return (low == 42 && high == 42) ? 42 : 0;
}

} // namespace ImGeeTests
