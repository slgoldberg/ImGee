# ImGee Development Tools

This directory contains developer tooling and build scripts for the ImGee suite:

* **`generate_manual.py`**: Automated PDF manual generator built on Python, ReportLab, and Pillow. It compiles Markdown documents across `docs/`, `extensions/`, `widgets/`, and `themes/` into publication-grade reference manuals with table of contents, running headers and footers, syntax-highlighted code blocks, and 2x Retina screenshots -- automatically preferring `docs/images/light/fig_*.png` light-theme specimen cards on white PDF pages when available (falling back to `docs/images/fig_*.png`).
* **`generate_dashboard.py`**: Zero-dependency Python script that introspects the repository (`IMGEE_VERSION`, header counts, exact LOC, git dates, screenshot coverage, and PDF page counts) and emits the self-contained `docs/index.html` At-a-Glance Visual Dashboard (works both offline via `file://` and live at `https://slgoldberg.github.io/ImGee/`). Run alongside `generate_manual.py` before every release.
* **`render_gallery.cpp`**: Standalone headless renderer for macOS. It creates an offscreen OpenGL context via CGL and renders all 26 ImGee showcase scenes (18 widget/extension cards + all 8 `imgee_demo.h` tabs) into 2x Retina RGBA PNGs with 4px tight transparent margins for both Dark Studio (`docs/images/fig_*.png`) and Paper Light (`docs/images/light/fig_*.png`) palettes with zero display windows.
* **`fonts/`**: Vendored SIL Open Font License (OFL) monospace typefaces (`RobotoMono-Medium.ttf`, `RobotoMono-Bold.ttf`) used by `generate_manual.py` for code block rendering and typography.

---

## 1. Generating Documentation PDFs

The manual generator requires Python 3 with ReportLab and Pillow:

```bash
pip install reportlab pillow
```

### Build All Manuals
To compile both the User Guide and the UX Design Manifesto in one run:

```bash
python3 tools/generate_manual.py
```

This outputs:
* `docs/ImGee_User_Guide.pdf`
* `docs/ImGee_UX_Design_Manifesto.pdf`

### Individual Manual Targets
```bash
# Build only the ImGee User Guide:
python3 tools/generate_manual.py --imgee

# Build only the UX Design Manifesto:
python3 tools/generate_manual.py --manifesto

# Compile custom Markdown files into a standalone PDF:
python3 tools/generate_manual.py -o docs/Custom_Manual.pdf doc1.md doc2.md
```

---

## 2. Headless Gallery Screenshot Generator

The screenshot generator uses an offscreen Core Graphics / OpenGL FBO pipeline to produce crisp 2x Retina captures of ImGee widgets and extensions.

### Building `render_gallery` (macOS)
```bash
mkdir -p build-gallery && cd build-gallery
cmake -DCMAKE_BUILD_TYPE=Release -DIMGUI_DIR=/path/to/imgui ../tools
cmake --build . --config Release
```

### Running the Generator
```bash
# Render all 18 showcase figures for both Dark Studio (docs/images/) and Paper Light (docs/images/light/):
./render_gallery --theme all   # (default)

# Or render a single palette pass:
./render_gallery --theme dark
./render_gallery --theme light
```

Dark figures are saved to `docs/images/fig_*.png` and Paper Light specimen cards are saved to `docs/images/light/fig_*.png` (picked up automatically by `generate_manual.py` for white-page PDF manuals).
