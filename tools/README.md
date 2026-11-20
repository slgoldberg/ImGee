# ImGee Development Tools

This directory contains developer tooling and build scripts for the ImGee suite:

* **`generate_manual.py`**: Automated PDF manual generator built on Python and ReportLab. It compiles Markdown documents across `docs/`, `extensions/`, `widgets/`, and `themes/` into publication-grade reference manuals with table of contents, running headers and footers, syntax-highlighted code blocks, and 2x Retina screenshots with floating thumbnail layout parity.
* **`render_gallery.cpp`**: Standalone headless renderer for macOS. It creates an offscreen OpenGL context via CGL and renders all 18 ImGee showcase scenes directly into 2x Retina PNG screenshots (`docs/images/fig_*.png`) in sub-second execution time with zero display windows.
* **`fonts/`**: Vendored SIL Open Font License (OFL) monospace typefaces (`RobotoMono-Medium.ttf`, `RobotoMono-Bold.ttf`) used by `generate_manual.py` for code block rendering and typography.

---

## 1. Generating Documentation PDFs

The manual generator requires Python 3 with ReportLab:

```bash
pip install reportlab
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
# Render all 18 showcase figures into docs/images/
./render_gallery
```

Output figures are saved directly to `docs/images/fig_*.png`.
