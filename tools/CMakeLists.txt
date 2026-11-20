cmake_minimum_required(VERSION 3.16)
project(ImGeeGalleryGenerator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(IMGEE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(DEFAULT_PEER_IMGUI "${IMGEE_ROOT}/../imgui4xp-pg-test/src/imgui" ABSOLUTE)

set(IMGUI_DIR "${DEFAULT_PEER_IMGUI}" CACHE PATH "Path to Dear ImGui source directory containing imgui.h and imgui.cpp")

if(NOT IMGUI_DIR OR NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "IMGUI_DIR must point to a valid Dear ImGui directory (got '${IMGUI_DIR}')")
endif()

if(NOT APPLE)
    message(FATAL_ERROR "render_gallery currently uses macOS CGL offscreen FBO + CoreGraphics/ImageIO")
endif()

add_library(imgui_gallery_core STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
)
target_include_directories(imgui_gallery_core SYSTEM PUBLIC "${IMGUI_DIR}")
target_compile_options(imgui_gallery_core PRIVATE -w)

add_executable(render_gallery
    render_gallery.cpp
    "${IMGEE_ROOT}/imgee_stb_image.cpp"
)
target_include_directories(render_gallery PRIVATE "${IMGEE_ROOT}")
target_compile_definitions(render_gallery PRIVATE GL_SILENCE_DEPRECATION)
target_compile_options(render_gallery PRIVATE -Wall -Wextra -Werror)
target_link_libraries(render_gallery PRIVATE
    imgui_gallery_core
    "-framework OpenGL"
    "-framework CoreGraphics"
    "-framework ImageIO"
    "-framework CoreFoundation"
    "-framework ApplicationServices"
)
