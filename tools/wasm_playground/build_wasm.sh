#!/usr/bin/env bash
# ============================================================================
# Build ImGee Live WebAssembly Playground (tools/wasm_playground/build_wasm.sh)
# ----------------------------------------------------------------------------
# Usage:
#   tools/wasm_playground/build_wasm.sh <path/to/imgui> <output_dir>
#
# Requires Emscripten (em++) on PATH (e.g. via emsdk or mymindstorm/setup-emsdk).
# Outputs index.html, index.js, index.wasm, and index.data into <output_dir>.
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMGUI_DIR="${1:-${IMGUI_DIR:-}}"
OUT_DIR="${2:-$REPO_ROOT/build-wasm/playground}"

if [ -z "$IMGUI_DIR" ] || [ ! -f "$IMGUI_DIR/imgui.h" ]; then
    echo "[ERROR] Please pass path to upstream Dear ImGui directory (containing imgui.h) as arg 1 or IMGUI_DIR." >&2
    exit 1
fi

if ! command -v em++ >/dev/null 2>&1; then
    echo "[ERROR] em++ not found on PATH. Install/activate emsdk first." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

OBJ_DIR="$WORK_DIR/obj"
STAGE_ASSETS="$WORK_DIR/assets"
mkdir -p "$OBJ_DIR" "$STAGE_ASSETS/fonts" "$STAGE_ASSETS/samples"

# Stage authentic Regular, Bold, Italic, and BoldItalic sans TrueType fonts
# (prefers installed system Roboto or Liberation Sans 2; falls back to ImGui's Roboto-Medium.ttf)
pick_first_existing() {
    for candidate in "$@"; do
        if [ -f "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done
    return 1
}

FALLBACK_SANS="$IMGUI_DIR/misc/fonts/Roboto-Medium.ttf"
if [ ! -f "$FALLBACK_SANS" ]; then
    echo "[ERROR] Expected fallback font $FALLBACK_SANS not found." >&2
    exit 1
fi

FONT_REG="$(pick_first_existing \
    /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/Roboto-Regular.ttf \
    /usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf \
    /usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf \
    /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf \
    "$FALLBACK_SANS")"
FONT_BOLD="$(pick_first_existing \
    /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/Roboto-Bold.ttf \
    /usr/share/fonts/truetype/roboto/hinted/Roboto-Bold.ttf \
    /usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf \
    /usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf \
    "$FALLBACK_SANS")"
FONT_ITAL="$(pick_first_existing \
    /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/Roboto-Italic.ttf \
    /usr/share/fonts/truetype/roboto/hinted/Roboto-Italic.ttf \
    /usr/share/fonts/truetype/liberation2/LiberationSans-Italic.ttf \
    /usr/share/fonts/truetype/liberation/LiberationSans-Italic.ttf \
    "$FALLBACK_SANS")"
FONT_BI="$(pick_first_existing \
    /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/Roboto-BoldItalic.ttf \
    /usr/share/fonts/truetype/roboto/hinted/Roboto-BoldItalic.ttf \
    /usr/share/fonts/truetype/liberation2/LiberationSans-BoldItalic.ttf \
    /usr/share/fonts/truetype/liberation/LiberationSans-BoldItalic.ttf \
    "$FALLBACK_SANS")"

echo "[WASM] Staging TrueType fonts:"
echo "  Regular:    $FONT_REG"
echo "  Bold:       $FONT_BOLD"
echo "  Italic:     $FONT_ITAL"
echo "  BoldItalic: $FONT_BI"
cp "$FONT_REG"  "$STAGE_ASSETS/fonts/Sans-Regular.ttf"
cp "$FONT_BOLD" "$STAGE_ASSETS/fonts/Sans-Bold.ttf"
cp "$FONT_ITAL" "$STAGE_ASSETS/fonts/Sans-Italic.ttf"
cp "$FONT_BI"   "$STAGE_ASSETS/fonts/Sans-BoldItalic.ttf"
cp "$REPO_ROOT/tools/fonts/RobotoMono-Medium.ttf" "$STAGE_ASSETS/fonts/RobotoMono-Medium.ttf"
cp "$REPO_ROOT/tools/fonts/RobotoMono-Bold.ttf"   "$STAGE_ASSETS/fonts/RobotoMono-Bold.ttf"

# Stage a crisp sample image and tiny sample files for Tab 6/7/8 demos
if [ -f "$REPO_ROOT/docs/images/fig_vector_icons.png" ]; then
    cp "$REPO_ROOT/docs/images/fig_vector_icons.png" "$STAGE_ASSETS/sample_icons.png"
fi

cat > "$STAGE_ASSETS/samples/README.md" <<'EOF'
# ImGee WebAssembly Sample Workspace

These virtual files live inside Emscripten MEMFS so you can test `FilePickerCombo`,
`OpenFileDialog`, and `SaveFileDialog` live in Tab 8 without touching your local disk.
EOF

cat > "$STAGE_ASSETS/samples/checklist.md" <<'EOF'
# Preflight Checklist
- [x] Battery & Avionics ON
- [x] Barometric Altimeter Set (29.92 / 1013)
- [x] ImGee Interactive Showcase Ready
EOF

cat > "$STAGE_ASSETS/samples/imgee_snippet.h" <<'EOF'
#pragma once
#define IMGEE_INCLUDE_DEMO
#include "imgee.h"

inline void DrawMyImGeeDemo(bool* p_open) {
    ImGui::ShowImGeeDemoWindow(p_open);
}
EOF

echo "[WASM] Compiling Dear ImGui core + SDL2/OpenGL3 backends..."
IMGUI_SRCS=(
    "$IMGUI_DIR/imgui.cpp"
    "$IMGUI_DIR/imgui_draw.cpp"
    "$IMGUI_DIR/imgui_tables.cpp"
    "$IMGUI_DIR/imgui_widgets.cpp"
    "$IMGUI_DIR/backends/imgui_impl_sdl2.cpp"
    "$IMGUI_DIR/backends/imgui_impl_opengl3.cpp"
)

IMGUI_OBJS=()
for src in "${IMGUI_SRCS[@]}"; do
    base="$(basename "$src" .cpp)"
    obj="$OBJ_DIR/${base}.o"
    em++ -std=c++17 -O2 -sUSE_SDL=2 \
        -I"$IMGUI_DIR" -I"$IMGUI_DIR/backends" \
        -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS \
        -c "$src" -o "$obj"
    IMGUI_OBJS+=("$obj")
done

echo "[WASM] Compiling ImGee playground main.cpp (-Wall -Wextra -Wsign-conversion -Werror)..."
MAIN_OBJ="$OBJ_DIR/main.o"
em++ -std=c++17 -O2 -Wall -Wextra -Wsign-conversion -Werror \
    -sUSE_SDL=2 \
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS \
    -I"$REPO_ROOT" \
    -isystem "$IMGUI_DIR" \
    -isystem "$IMGUI_DIR/backends" \
    -c "$SCRIPT_DIR/main.cpp" -o "$MAIN_OBJ"

echo "[WASM] Linking WebGL2 playground bundle -> $OUT_DIR/index.html ..."
em++ -O2 \
    "$MAIN_OBJ" "${IMGUI_OBJS[@]}" \
    -sUSE_SDL=2 \
    -sUSE_WEBGL2=1 \
    -sFULL_ES3=1 \
    -sWASM=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sNO_EXIT_RUNTIME=1 \
    -sEXPORTED_RUNTIME_METHODS="['UTF8ToString']" \
    --shell-file "$SCRIPT_DIR/shell.html" \
    --preload-file "$STAGE_ASSETS@/assets" \
    -o "$OUT_DIR/index.html"

for f in index.html index.js index.wasm index.data; do
    if [ ! -s "$OUT_DIR/$f" ]; then
        echo "[ERROR] Expected WASM output missing or empty: $OUT_DIR/$f" >&2
        exit 1
    fi
done

ls -lh "$OUT_DIR/index.html" "$OUT_DIR/index.js" "$OUT_DIR/index.wasm" "$OUT_DIR/index.data"
echo "[OK] ImGee WebAssembly Playground built cleanly in $OUT_DIR"
