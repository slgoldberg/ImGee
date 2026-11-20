# ImGee Test Suite & Quality Assurance

This directory contains automated regression tests, compilation isolation checks, and multi-translation-unit (TU) validation harnesses for the ImGee suite.

The test harness is built to prove ImGee's core architectural contracts:
1. **100% Header-Only Hygiene:** No symbol collisions, no missing inline guards, and clean compilation across MSVC `/W4 /WX`, GCC `-Wall -Wextra -Werror`, and Clang.
2. **Order Independence:** Headers can be included in any sequence without causing syntax or linkage errors.
3. **Granular Feature Opt-Outs:** Individual subsystems can be disabled via `IMGEE_DISABLE_*` macros without breaking unrelated components.
4. **Coexistence:** Safe interoperation alongside external image decoders (`stb_image`) and hostile platform macros (`min`, `max`).

---

## 1. Test Targets & Translation Units

| Target / File | Purpose & Verification Scope |
| :--- | :--- |
| **`main_smoke.cpp`** | Comprehensive functional test suite. Exercises all widgets (toggles, knobs, pulldowns, dials, spinners, pills, links, copy buttons), extensions (TimedTooltip, Markdown with floating layout and lightbox pan/zoom, Viewport, DocViewer), themes, and typography. |
| **`tu_external_stbi_a.cpp`** / **`b.cpp`** | Verifies that ImGee's internal `stb_image` symbols do not conflict with user applications linking their own independent copy of `stb_image`. |
| **`tu_force_relative.cpp`** | Verifies that headers compile cleanly when included via relative and non-standard search paths. |
| **`tu_hostile_macros.cpp`** | Verifies resilience against hostile preprocessor macros defined before ImGee headers (e.g. `#define min(...)`, `#define max(...)`, `#define wrap(...)`). |
| **`tu_no_tooltip.cpp`** | Validates that widgets, Markdown, and DocViewer degrade cleanly to standard ImGui tooltips when `IMGEE_DISABLE_TIMED_TOOLTIP` is active. |
| **`tu_no_typography.cpp`** | Validates Markdown and widget layout when `IMGEE_DISABLE_TYPOGRAPHY` is active. |
| **`tu_no_widgets_icons.cpp`** | Validates that extensions function when specific widget families or vector icons are excluded. |
| **`tu_reverse_order_knobs.cpp`** | Tests inverse include ordering between knobs, sliders, and core headers. |
| **`tu_reverse_order_toggles_popups.cpp`** | Tests inverse include ordering between toggles, popups, pulldowns, and Markdown. |
| **`tu_standalone_optouts.cpp`** | Tests minimal builds with extensive feature opt-outs. |

---

## 2. Building & Running Tests Locally

Tests require CMake 3.16+ and a path to a Dear ImGui repository:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DIMGUI_DIR=/path/to/imgui ../tests
cmake --build . --config Release
ctest --output-on-failure
```

### Multi-Version Verification
ImGee is continuously tested against multiple Dear ImGui releases, including:
* Modern Dear ImGui (e.g. `v1.92.8+`)
* Legacy Dear ImGui (e.g. `v1.84.2+`)
* Builds with `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` enabled
