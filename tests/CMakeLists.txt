cmake_minimum_required(VERSION 3.16)
project(ImGeeStandaloneTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMGUI_DIR "" CACHE PATH "Path to Dear ImGui source directory containing imgui.h and imgui.cpp")
option(IMGEE_TEST_OBSOLETE_OFF "Compile with IMGUI_DISABLE_OBSOLETE_FUNCTIONS" OFF)

if(NOT IMGUI_DIR OR NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "IMGUI_DIR must point to a valid Dear ImGui directory (got '${IMGUI_DIR}')")
endif()

get_filename_component(IMGEE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# 1. Core Dear ImGui library (compiled without -Werror so older upstream ImGui
#    releases do not trip modern compiler deprecation/bounds diagnostics)
add_library(imgui_core STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
)
target_include_directories(imgui_core SYSTEM PUBLIC "${IMGUI_DIR}")
if(IMGEE_TEST_OBSOLETE_OFF)
    target_compile_definitions(imgui_core PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
endif()
if(WIN32)
    # Disable upstream imgui.cpp's optional imm32/shell32 helpers so the test link
    # proves ImGee itself requires zero extra Win32 import libraries (-lwininet/-lshell32/-limm32)
    target_compile_definitions(imgui_core PRIVATE
        IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
        IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
    )
elseif(APPLE)
    target_link_libraries(imgui_core PUBLIC "-framework CoreFoundation" "-framework ApplicationServices")
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(imgui_core PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
if(MSVC)
    target_compile_options(imgui_core PRIVATE /W0 /EHsc)
else()
    target_compile_options(imgui_core PRIVATE -w)
endif()

# Helper to enforce strict warnings-as-errors across all ImGee test targets
function(imgee_apply_strict_warnings target_name)
    target_include_directories(${target_name} PRIVATE "${IMGEE_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4 /WX /EHsc /Zc:__cplusplus /permissive-
            /external:I "${IMGUI_DIR}" /external:W0
        )
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror)
        if(MINGW)
            # Statically link MinGW C/C++ runtime so .exe has zero extra DLL dependencies
            target_link_options(${target_name} PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
endfunction()

enable_testing()

# 2. Multi-TU Header-Only Suite + Reverse Include-Order + Hostile Macro + Headless Smoke Runner
add_executable(imgee_standalone_tests
    main_smoke.cpp
    tu_reverse_order_knobs.cpp
    tu_reverse_order_toggles_popups.cpp
    tu_hostile_macros.cpp
    tu_standalone_optouts.cpp
)
target_link_libraries(imgee_standalone_tests PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_standalone_tests)
add_test(NAME imgee_standalone_smoke COMMAND imgee_standalone_tests)

# 3. Single-TU External stb_image Mode (-DIMGEE_EXTERNAL_STB_IMAGE + imgee_stb_image.cpp)
add_executable(imgee_external_stbi_test
    tu_external_stbi_a.cpp
    tu_external_stbi_b.cpp
    "${IMGEE_ROOT}/imgee_stb_image.cpp"
)
target_compile_definitions(imgee_external_stbi_test PRIVATE IMGEE_EXTERNAL_STB_IMAGE)
target_link_libraries(imgee_external_stbi_test PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_external_stbi_test)
add_test(NAME imgee_external_stbi COMMAND imgee_external_stbi_test)

# 4. Strict Relative Path Containment Policy (-DIMGEE_FILEPATHS_FORCE_RELATIVE=1)
add_executable(imgee_force_relative_test
    tu_force_relative.cpp
)
target_link_libraries(imgee_force_relative_test PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_force_relative_test)
add_test(NAME imgee_force_relative COMMAND imgee_force_relative_test)

# 5. Silent Table Tooltip Fallback when TimedTooltip is Disabled (-DIMGEE_DISABLE_TIMED_TOOLTIP)
add_executable(imgee_no_tooltip_test
    tu_no_tooltip.cpp
)
target_link_libraries(imgee_no_tooltip_test PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_no_tooltip_test)
add_test(NAME imgee_no_tooltip COMMAND imgee_no_tooltip_test)

# 6. Base-Font Isolation Fallback when Typography is Disabled (-DIMGEE_DISABLE_TYPOGRAPHY)
add_executable(imgee_no_typography_test
    tu_no_typography.cpp
)
target_link_libraries(imgee_no_typography_test PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_no_typography_test)
add_test(NAME imgee_no_typography COMMAND imgee_no_typography_test)

# 7. Standalone Markdown Callouts & Blockquotes without Widgets / Icons (-DIMGEE_DISABLE_WIDGETS -DIMGEE_DISABLE_ICONS)
add_executable(imgee_no_widgets_icons_test
    tu_no_widgets_icons.cpp
)
target_link_libraries(imgee_no_widgets_icons_test PRIVATE imgui_core)
imgee_apply_strict_warnings(imgee_no_widgets_icons_test)
add_test(NAME imgee_no_widgets_icons COMMAND imgee_no_widgets_icons_test)
