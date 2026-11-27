#!/usr/bin/env python3
"""
ImGee At-a-Glance Visual Dashboard Generator (tools/generate_dashboard.py)
---------------------------------------------------------------------------
Introspects the ImGee repository live (version, header LOC, git timestamps,
dark + light 2x Retina gallery captures, and compiled PDF page counts) and
emits a completely self-contained docs/index.html with zero external CDN
dependencies -- ready for both offline file:// browsing and GitHub Pages
(https://slgoldberg.github.io/ImGee/).

Usage:
    python3 tools/generate_dashboard.py
"""

import datetime
import html
import json
import os
import re
import subprocess
import sys

REPO_URL = "https://github.com/slgoldberg/ImGee"
PAGES_URL = "https://slgoldberg.github.io/ImGee/"

# Canonical catalog of the 20 feature headers (10 Widgets, 8 Extensions, 1 Themes, 1 Showcase).
# Order inside each category is curated for readability; live stats (LOC, git date, image checks)
# are computed directly from disk on every run.
FEATURE_CATALOG = [
    # --- Opt-In Showcase (1) ---
    {
        "path": "imgee_demo.h",
        "title": "Interactive Showcase Window",
        "category": "Showcase",
        "demoTab": 1,
        "summary": "Opt-in single-header interactive playground mirroring ImGui::ShowDemoWindow with 8 live tabs, programmatic tab/theme helpers, synced DocumentViewer theme, and 1-click Restore Host Style.",
        "apis": ["ShowImGeeDemoWindow", "ShowImGeeDemoContent", "SetImGeeDemoTab", "SetImGeeDemoTheme", "IMGEE_INCLUDE_DEMO"],
        "figures": [
            {"label": "1: Overview", "short": "1", "file": "fig_demo_tab1.png"},
            {"label": "2: Sliders & Knobs", "short": "2", "file": "fig_demo_tab2.png"},
            {"label": "3: Menus & Links", "short": "3", "file": "fig_demo_tab3.png"},
            {"label": "4: Tooltips & Badges", "short": "4", "file": "fig_demo_tab4.png"},
            {"label": "5: Vector Icons", "short": "5", "file": "fig_demo_tab5.png"},
            {"label": "6: Markdown & Tables", "short": "6", "file": "fig_demo_tab6.png"},
            {"label": "7: DocViewer & Find", "short": "7", "file": "fig_demo_tab7.png"},
            {"label": "8: File & Diagnostics", "short": "8", "file": "fig_demo_tab8.png"},
        ],
    },
    # --- Flagship Extensions (8) ---
    {
        "path": "extensions/DocViewer/imgui_doc_viewer.h",
        "title": "Standalone Markdown DocumentViewer",
        "category": "Extensions",
        "demoTab": 7,
        "summary": "Turnkey in-app documentation viewer with 1.0x two-row chrome, auto-parsed ATX TOC jumper, 1-click scroll return, live Find in Doc (Ctrl+F), opacity slider, and 5-theme picker.",
        "apis": ["DocumentViewer", "DocumentViewerConfig", "DocumentViewerState", "OpenDocumentSearch", "StepDocumentSearchMatch", "ParseMarkdownToc"],
        "figures": [
            {"label": "DocumentViewer", "short": "DocViewer", "file": "fig_doc_viewer.png"},
        ],
    },
    {
        "path": "extensions/Markdown/imgui_markdown.h",
        "title": "CommonMark Parser, GFM Tables & Code Blocks",
        "category": "Extensions",
        "demoTab": 6,
        "summary": "Core Markdown renderer with tristate sortable and drag-resizable GFM tables, per-table tooltip metadata overrides, syntax-tinted code blocks with 1-click Copy, and typographic word wrapping.",
        "apis": ["Markdown", "MarkdownConfig", "MarkdownTableState", "table-preset"],
        "figures": [
            {"label": "GFM Tables", "short": "GFM Tables", "file": "fig_gfm_tables.png"},
            {"label": "Code Blocks", "short": "Code Blocks", "file": "fig_code_blocks.png"},
        ],
    },
    {
        "path": "extensions/Markdown/imgui_markdown_ext.h",
        "title": "Markdown Extensions: Callouts, Floats & Style Tags",
        "category": "Extensions",
        "demoTab": 6,
        "summary": "High-level Markdown engine adding GFM alert callout cards ([!NOTE]..[!CAUTION]), mid-paragraph text-wrapping image floats, click-to-zoom lightboxes, semantic style tags, and ButtonMD.",
        "apis": ["Markdown::Text", "TextMD", "ButtonMD", "RegisterMarkdownStyleTag", "RegisterMarkdownTagAlias"],
        "figures": [
            {"label": "Callout Cards", "short": "Callouts", "file": "fig_callout_cards.png"},
            {"label": "Floats & Lightbox", "short": "Floats", "file": "fig_markdown_floats_captions.png"},
        ],
    },
    {
        "path": "extensions/Markdown/imgui_markdown_image_service.h",
        "title": "3-Tier Async Markdown Image Service",
        "category": "Extensions",
        "demoTab": 6,
        "summary": "Non-blocking image resolver and GPU texture cache supporting named in-memory assets, local disk images with subfolder fallbacks, and off-thread HTTP/HTTPS downloads via HostBackend.",
        "apis": ["MarkdownImageService", "ImGee::HostBackend", "ImGee::Shutdown"],
        "figures": [
            {"label": "Floated Figures & Lightbox", "short": "Images", "file": "fig_markdown_floats_captions.png"},
        ],
    },
    {
        "path": "extensions/TimedTooltip/imgui_tooltip_ext.h",
        "title": "Stationary Delayed TimedTooltips",
        "category": "Extensions",
        "demoTab": 4,
        "summary": "Replaces cursor-chasing stock tooltips with dwell-verified stationary cards featuring viewport clamping, base-font isolation, damped zoom scaling, Markdown rendering, and zero-arg EndStationaryTooltip().",
        "apis": ["TimedTooltip::Text", "TimedTooltip::TextMD", "TimedTooltip::TextMDLink", "BeginStationaryTooltip", "EndStationaryTooltip"],
        "figures": [
            {"label": "Stationary TimedTooltips", "short": "Tooltips", "file": "fig_timed_tooltips.png"},
        ],
    },
    {
        "path": "extensions/Typography/imgui_font_roles.h",
        "title": "Semantic Typography & ImFontRole Registry",
        "category": "Extensions",
        "demoTab": 1,
        "summary": "9-slot semantic font role registry (Regular, Title, Small, Bold, BoldLarge, Monospace, Italic, MonospaceMedium, BoldItalic, plus Heading1..3 and Tiny) with 4-tier fallbacks and RAII guards.",
        "apis": ["ImFontRole", "SetFontRole", "GetFont", "ScopedFontRole", "PushFontRole"],
        "figures": [
            {"label": "Typography & Font Roles", "short": "Typography", "file": "fig_typography_styled_text.png"},
        ],
    },
    {
        "path": "extensions/Typography/imgui_styled_text.h",
        "title": "Inline Baseline-Aligned StyledText Engine",
        "category": "Extensions",
        "demoTab": 1,
        "summary": "Lightweight inline XML-style rich text (<b>, <i>, <mono>, <huge>, <small>, <color=#hex>, <badge>, <pill>, <kbd>) with continuous TrueType baseline alignment and printf formatting.",
        "apis": ["StyledText", "StyledTextUnformatted", "StyledTextWrapped"],
        "figures": [
            {"label": "Inline StyledText", "short": "StyledText", "file": "fig_typography_styled_text.png"},
        ],
    },
    {
        "path": "extensions/Viewport/imgui_zoomable_viewport.h",
        "title": "Two-Layer Zoomable Document Viewport",
        "category": "Extensions",
        "demoTab": 7,
        "summary": "Separates crisp 1.0x window chrome from smoothly zoomed scrollable inner content (50%-250%) with proportional font scaling and 1-click zoom reset controls.",
        "apis": ["ZoomableViewportState", "BeginZoomedViewport", "EndZoomedViewport", "ZoomViewportControls"],
        "figures": [
            {"label": "Zoomable Viewport", "short": "Viewport", "file": "fig_doc_viewer.png"},
        ],
    },
    # --- Immediate-Mode Widgets (10) ---
    {
        "path": "widgets/imgui_popup_menus.h",
        "title": "Anchored Popups, SelectPulldown & Menu Strips",
        "category": "Widgets",
        "demoTab": 3,
        "summary": "Item-anchored directional popups (AboveItem/BelowItem), 1-line SelectPulldown with scroll pass-through and Lens UI wheel latching, and zero-callback Hover-Latching Menu Strips.",
        "apis": ["BeginMenuStrip", "BeginStripMenu", "EndStripMenu", "IsMenuStripOpen", "SelectPulldown", "SelectPulldownMD", "BeginAnchoredPopup"],
        "figures": [
            {"label": "Pulldowns & Menu Strip", "short": "Popups", "file": "fig_popup_menus.png"},
        ],
    },
    {
        "path": "widgets/imgui_mouse_wheel_controls.h",
        "title": "Mouse-Wheel Sliders, Combos & Reset-with-Undo",
        "category": "Widgets",
        "demoTab": 2,
        "summary": "Tactile sliders, drags, and combos with Natural List Order wheel stepping, scrollable-container auto-defeat, and abutting 1-click Reset-with-Undo (A/B compare) chasers.",
        "apis": ["SliderWithReset", "PercentSliderWithReset", "MouseWheelCombo", "ComboWithReset", "ResetButton"],
        "figures": [
            {"label": "Sliders & Reset Chasers", "short": "Sliders", "file": "fig_sliders_reset.png"},
        ],
    },
    {
        "path": "widgets/imgui_knobs.h",
        "title": "270-Degree Rotary Knobs & OFF Dimmer Dials",
        "category": "Widgets",
        "demoTab": 2,
        "summary": "Compact vector potentiometers with 270-degree Omega arcs, perimeter ticks, wheel fine/coarse modifiers, two-step tactile OFF detents, and 1-click Reset-with-Undo.",
        "apis": ["Knob", "KnobWithReset", "DimmerKnob", "ImGuiKnobFlags_ShowTicks"],
        "figures": [
            {"label": "Rotary Knobs & Dimmers", "short": "Knobs", "file": "fig_rotary_knobs.png"},
        ],
    },
    {
        "path": "widgets/imgui_toggle_buttons.h",
        "title": "Animated Toggle Switches & Segmented Controls",
        "category": "Widgets",
        "demoTab": 2,
        "summary": "DeltaTime-animated pill switches and joined two-way segmented selectors with clamped wheel toggling, parent scroll protection on labels, and per-state Markdown tooltips.",
        "apis": ["ToggleButton", "ToggleButtons"],
        "figures": [
            {"label": "Animated Toggles", "short": "Toggles", "file": "fig_toggle_switches.png"},
        ],
    },
    {
        "path": "widgets/imgui_icons.h",
        "title": "Zero-Font Procedural Vector Icon Engine",
        "category": "Widgets",
        "demoTab": 5,
        "summary": "Draws 24 antialiased procedural vector icons directly to ImDrawList with zero icon fonts or textures, paired with turnkey IconButton and ButtonIcon pushbuttons.",
        "apis": ["DrawVectorIcon", "IconButton", "ButtonIcon", "RenderIcon"],
        "figures": [
            {"label": "Procedural Vector Icons", "short": "Icons", "file": "fig_vector_icons.png"},
        ],
    },
    {
        "path": "widgets/imgui_badges.h",
        "title": "Badges, Pills, Icon Capsules & 3D Keycaps",
        "category": "Widgets",
        "demoTab": 4,
        "summary": "Auto-contrasting ITU-R BT.601 luminance status badges, rounded pills, named-icon capsules (PillNamed), and beveled 3D keyboard shortcut chips (<kbd>).",
        "apis": ["Badge", "Pill", "PillNamed", "Kbd"],
        "figures": [
            {"label": "Badges, Pills & Keycaps", "short": "Badges", "file": "fig_pills_and_badges.png"},
        ],
    },
    {
        "path": "widgets/imgui_spinners.h",
        "title": "Activity Spinners & Radial Countdown Dials",
        "category": "Widgets",
        "demoTab": 4,
        "summary": "Frame-rate independent ImDrawList activity spinners, progress rings, and 12-o'clock radial countdown timers that scale automatically with ambient font size.",
        "apis": ["Spinner", "ProgressDial", "RadialTimer", "CountdownTimer"],
        "figures": [
            {"label": "Spinners & Countdown Dials", "short": "Spinners", "file": "fig_spinners_and_dials.png"},
        ],
    },
    {
        "path": "widgets/imgui_clickable_links.h",
        "title": "Active-Cursor Clickable Links & Info Triggers",
        "category": "Widgets",
        "demoTab": 3,
        "summary": "Zero-inert-arrow drop-in wrappers (ButtonLink, TextLink, CheckboxLink, RadioButtonLink, MenuItemLink) returning LinkClickResult with automatic click-to-toggle tooltip support.",
        "apis": ["ButtonLink", "TextLink", "CheckboxLink", "RadioButtonLink", "MenuItemLink"],
        "figures": [
            {"label": "Clickable Links", "short": "Links", "file": "fig_clickable_links.png"},
        ],
    },
    {
        "path": "widgets/imgui_copy_button.h",
        "title": "Anti-Twitch Clipboard Copy Buttons",
        "category": "Widgets",
        "demoTab": 3,
        "summary": "One-click clipboard buttons (CopyButton and automatic InputText chaser CopyLastItemButton) with procedural icons and 1.5s transient Copied! visual feedback.",
        "apis": ["CopyButton", "CopyLastItemButton"],
        "figures": [
            {"label": "Clipboard Copy Buttons", "short": "Copy", "file": "fig_copy_button.png"},
        ],
    },
    {
        "path": "widgets/imgui_file_dialog.h",
        "title": "Non-Blocking In-Viewport File Dialogs & Pickers",
        "category": "Widgets",
        "demoTab": 8,
        "summary": "Pure C++11 non-blocking Open/Save modals and inline FilePickerCombo using native POSIX/Win32 directory APIs -- VR-safe, zero console spawning, and overwrite-guarded.",
        "apis": ["OpenFileDialog", "SaveFileDialog", "FilePickerCombo"],
        "figures": [
            {"label": "File Picker & Modal Browser", "short": "File Dialog", "file": "fig_file_dialog.png"},
        ],
    },
    # --- Curated Themes (1) ---
    {
        "path": "themes/imgui_styles.h",
        "title": "Curated Color Themes & Typography Palettes",
        "category": "Themes",
        "demoTab": 1,
        "summary": "Balanced studio and publication themes replacing stock Dear ImGui neon blue: Charcoal (canonical default), Rusty (warm low-glare bronze), and high-contrast Paper Light.",
        "apis": ["ApplyDefaultTheme", "StyleColorsCharcoal", "StyleColorsRusty", "StyleColorsPaperLight"],
        "figures": [
            {"label": "Charcoal & Rusty", "short": "Charcoal & Rusty", "file": "fig_themes_comparison.png", "fixedPath": "images/fig_themes_comparison.png"},
            {"label": "Paper Light", "short": "Paper Light", "file": "fig_themes_comparison.png", "fixedPath": "images/light/fig_themes_comparison.png"},
        ],
    },
]


def read_imgee_version(repo_root: str) -> tuple[str, int]:
    imgee_h = os.path.join(repo_root, "imgee.h")
    ver_str = "0.0.0"
    ver_num = 0
    with open(imgee_h, "r", encoding="utf-8", errors="replace") as f:
        src = f.read()
    m_str = re.search(r'#define\s+IMGEE_VERSION\s+"([^"]+)"', src)
    m_num = re.search(r"#define\s+IMGEE_VERSION_NUM\s+(\d+)", src)
    if m_str:
        ver_str = m_str.group(1)
    if m_num:
        ver_num = int(m_num.group(1))
    return ver_str, ver_num


def count_file_lines(path: str) -> int:
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return sum(1 for _ in f)


def count_total_library_headers(repo_root: str) -> int:
    total = 0
    for dirpath, dirnames, filenames in os.walk(repo_root):
        dirnames[:] = [
            d for d in dirnames
            if not d.startswith(".") and not d.startswith("build") and d != "tests"
        ]
        for fn in filenames:
            if fn.endswith(".h"):
                total += 1
    return total


def git_last_updated(repo_root: str, rel_path: str) -> tuple[int, str, str]:
    try:
        out = subprocess.check_output(
            ["git", "log", "-1", "--format=%ct|%cs|%h", "--", rel_path],
            cwd=repo_root,
            stderr=subprocess.DEVNULL,
        ).decode("utf-8", errors="replace").strip()
        if out and "|" in out:
            ts_s, date_s, sha_s = out.split("|", 2)
            return int(ts_s), date_s, sha_s
    except Exception:
        pass
    st = os.stat(os.path.join(repo_root, rel_path))
    ts = int(st.st_mtime)
    dt = datetime.datetime.fromtimestamp(ts).strftime("%Y-%m-%d")
    return ts, dt, "local"


def pdf_info(pdf_path: str) -> dict:
    if not os.path.exists(pdf_path):
        return {"exists": False, "pages": 0, "size_kb": 0.0, "size_label": "missing"}
    size_bytes = os.path.getsize(pdf_path)
    with open(pdf_path, "rb") as f:
        raw = f.read()
    # ReportLab writes uncompressed /Type /Page dictionary objects; count exact /Type /Page (not /Pages).
    pages = len(re.findall(rb"/Type\s*/Page(?![a-zA-Z])", raw))
    if size_bytes >= 1024 * 1024:
        size_label = f"{size_bytes / (1024.0 * 1024.0):.2f} MB"
    else:
        size_label = f"{size_bytes / 1024.0:.1f} KB"
    return {
        "exists": True,
        "pages": pages,
        "size_kb": round(size_bytes / 1024.0, 1),
        "size_label": size_label,
    }


def collect_repo_data(repo_root: str) -> dict:
    version, version_num = read_imgee_version(repo_root)
    total_headers = count_total_library_headers(repo_root)
    docs_dir = os.path.join(repo_root, "docs")

    cards = []
    cat_counts = {"Widgets": 0, "Extensions": 0, "Themes": 0, "Showcase": 0}
    cat_loc = {"Widgets": 0, "Extensions": 0, "Themes": 0, "Showcase": 0}
    covered_cards = 0
    missing_assets = []

    for idx, entry in enumerate(FEATURE_CATALOG):
        rel_path = entry["path"]
        abs_path = os.path.join(repo_root, rel_path)
        if not os.path.exists(abs_path):
            raise FileNotFoundError(f"Expected feature header missing: {rel_path}")
        loc = count_file_lines(abs_path)
        ts, date_str, commit_sha = git_last_updated(repo_root, rel_path)

        figs = []
        all_figs_present = True
        for fig in entry["figures"]:
            fn = fig["file"]
            fixed_path = fig.get("fixedPath")
            if fixed_path:
                dark_rel = fixed_path
                light_rel = fixed_path
                if not os.path.exists(os.path.join(docs_dir, fixed_path)):
                    all_figs_present = False
                    missing_assets.append(fixed_path)
            else:
                dark_rel = f"images/{fn}"
                light_rel = f"images/light/{fn}"
                dark_ok = os.path.exists(os.path.join(docs_dir, dark_rel))
                light_ok = os.path.exists(os.path.join(docs_dir, light_rel))
                if not dark_ok:
                    all_figs_present = False
                    missing_assets.append(dark_rel)
                if not light_ok:
                    all_figs_present = False
                    missing_assets.append(light_rel)
                if not light_ok:
                    light_rel = dark_rel
            figs.append({
                "label": fig["label"],
                "short": fig["short"],
                "dark": dark_rel,
                "light": light_rel,
            })

        if all_figs_present and figs:
            covered_cards += 1

        cat = entry["category"]
        cat_counts[cat] = cat_counts.get(cat, 0) + 1
        cat_loc[cat] = cat_loc.get(cat, 0) + loc

        cards.append({
            "id": idx,
            "path": rel_path,
            "filename": os.path.basename(rel_path),
            "title": entry["title"],
            "category": cat,
            "demoTab": entry.get("demoTab", 1),
            "summary": entry["summary"],
            "apis": entry["apis"],
            "loc": loc,
            "updatedTs": ts,
            "updatedDate": date_str,
            "commitSha": commit_sha,
            "githubUrl": f"{REPO_URL}/blob/master/{rel_path}",
            "figures": figs,
        })

    if missing_assets:
        print(f"[WARN] Missing gallery assets: {missing_assets}", file=sys.stderr)

    user_guide_pdf = pdf_info(os.path.join(docs_dir, "ImGee_User_Guide.pdf"))
    manifesto_pdf = pdf_info(os.path.join(docs_dir, "ImGee_UX_Design_Manifesto.pdf"))

    total_feature_loc = sum(cat_loc.values())
    generated_date = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")

    return {
        "version": version,
        "versionNum": version_num,
        "generatedDate": generated_date,
        "repoUrl": REPO_URL,
        "pagesUrl": PAGES_URL,
        "featureHeaderCount": len(cards),
        "totalHeaderCount": total_headers,
        "totalFeatureLoc": total_feature_loc,
        "categoryCounts": cat_counts,
        "categoryLoc": cat_loc,
        "coveredCards": covered_cards,
        "pdfs": {
            "userGuide": user_guide_pdf,
            "manifesto": manifesto_pdf,
        },
        "cards": cards,
    }


def build_html(data: dict) -> str:
    ver = html.escape(data["version"])
    feat_n = data["featureHeaderCount"]
    tot_h = data["totalHeaderCount"]
    tot_loc = data["totalFeatureLoc"]
    cov_n = data["coveredCards"]
    cov_pct = int(round((100.0 * cov_n / feat_n) if feat_n else 0.0))
    cc = data["categoryCounts"]
    cl = data["categoryLoc"]
    ug = data["pdfs"]["userGuide"]
    ux = data["pdfs"]["manifesto"]

    def pct(n: int) -> float:
        return round((100.0 * n / tot_loc) if tot_loc else 0.0, 2)

    loc_segments = [
        ("Extensions", cl["Extensions"], pct(cl["Extensions"]), "seg-ext"),
        ("Widgets", cl["Widgets"], pct(cl["Widgets"]), "seg-wid"),
        ("Showcase", cl["Showcase"], pct(cl["Showcase"]), "seg-sho"),
        ("Themes", cl["Themes"], pct(cl["Themes"]), "seg-thm"),
    ]
    bar_html = "\n".join(
        f'<div class="dist-seg {cls}" style="width:{p}%" title="{name}: {loc:,} LOC ({p:.1f}%)"></div>'
        for name, loc, p, cls in loc_segments
    )
    legend_html = "\n".join(
        f'<span class="dist-leg"><i class="dot {cls}"></i>{name} <b>{loc:,}</b> ({p:.0f}%)</span>'
        for name, loc, p, cls in loc_segments
    )

    data_json = json.dumps(data, ensure_ascii=True, separators=(",", ":")).replace("</", "<\\/")

    return f"""<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ImGee v{ver} -- At-a-Glance Visual Toolkit Dashboard</title>
<meta name="description" content="Self-contained visual catalog and live metrics dashboard for ImGee v{ver}: header-only ergonomic widgets, extensions, curated themes, and interactive showcase for Dear ImGui.">
<style>
:root {{
  --bg: #181a1f;
  --bg-elev: #20232a;
  --bg-card: #242832;
  --bg-stage: #15171c;
  --bg-pill: #2c313d;
  --bg-pill-hover: #363d4c;
  --border: #323846;
  --border-strong: #464f63;
  --text: #e8ecf4;
  --text-muted: #98a2b6;
  --text-dim: #737d92;
  --accent: #4c9aff;
  --accent-soft: rgba(76, 154, 255, 0.16);
  --accent-text: #85beff;
  --ext-col: #4c9aff;
  --wid-col: #38c172;
  --sho-col: #f59e0b;
  --thm-col: #a855f7;
  --shadow: 0 10px 28px rgba(0, 0, 0, 0.35);
  --radius: 12px;
  --mono: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
  --sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
}}
html[data-theme="light"] {{
  --bg: #f4f6fa;
  --bg-elev: #ffffff;
  --bg-card: #ffffff;
  --bg-stage: #eef1f6;
  --bg-pill: #e8edf5;
  --bg-pill-hover: #dbe3f0;
  --border: #d7dfec;
  --border-strong: #b6c3d8;
  --text: #18202c;
  --text-muted: #526076;
  --text-dim: #6f7e95;
  --accent: #2563eb;
  --accent-soft: rgba(37, 99, 235, 0.11);
  --accent-text: #1d4ed8;
  --ext-col: #2563eb;
  --wid-col: #16a34a;
  --sho-col: #d97706;
  --thm-col: #9333ea;
  --shadow: 0 8px 24px rgba(15, 23, 42, 0.08);
}}
* {{ box-sizing: border-box; }}
html, body {{
  margin: 0;
  padding: 0;
  background: var(--bg);
  color: var(--text);
  font-family: var(--sans);
  line-height: 1.5;
  -webkit-font-smoothing: antialiased;
}}
a {{ color: var(--accent-text); text-decoration: none; }}
a:hover {{ text-decoration: underline; }}

.topbar {{
  position: sticky;
  top: 0;
  z-index: 40;
  background: var(--bg-elev);
  border-bottom: 1px solid var(--border);
  backdrop-filter: blur(10px);
}}
.topbar-inner {{
  max-width: 1380px;
  margin: 0 auto;
  padding: 12px 24px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}}
.brand {{
  display: flex;
  align-items: center;
  gap: 10px;
}}
.brand-logo {{
  width: 32px;
  height: 32px;
  border-radius: 8px;
  background: linear-gradient(135deg, #2563eb, #38bdf8);
  color: #fff;
  font-weight: 800;
  font-size: 14px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  letter-spacing: -0.02em;
}}
.brand h1 {{
  margin: 0;
  font-size: 18px;
  font-weight: 700;
  letter-spacing: -0.01em;
}}
.ver-pill {{
  display: inline-flex;
  align-items: center;
  padding: 2px 9px;
  border-radius: 999px;
  background: var(--accent-soft);
  color: var(--accent-text);
  font-family: var(--mono);
  font-size: 12px;
  font-weight: 600;
}}
.top-actions {{
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}}
.btn {{
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 7px 12px;
  border-radius: 8px;
  border: 1px solid var(--border);
  background: var(--bg-pill);
  color: var(--text);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease;
  text-decoration: none;
}}
.btn:hover {{
  background: var(--bg-pill-hover);
  border-color: var(--border-strong);
  text-decoration: none;
}}
.btn-primary {{
  background: var(--accent);
  border-color: var(--accent);
  color: #fff;
}}
.btn-primary:hover {{
  filter: brightness(1.08);
  color: #fff;
}}

.wrap {{
  max-width: 1380px;
  margin: 0 auto;
  padding: 24px;
}}

.hero {{
  background: var(--bg-elev);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 22px 24px;
  margin-bottom: 20px;
}}
.hero-lead {{
  margin: 0 0 18px 0;
  color: var(--text-muted);
  font-size: 15px;
  max-width: 920px;
}}
.kpi-grid {{
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
  margin-bottom: 16px;
}}
@media (max-width: 960px) {{
  .kpi-grid {{ grid-template-columns: repeat(2, minmax(0, 1fr)); }}
}}
@media (max-width: 560px) {{
  .kpi-grid {{ grid-template-columns: 1fr; }}
}}
.kpi {{
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 14px 16px;
}}
.kpi-label {{
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--text-dim);
  font-weight: 700;
  margin-bottom: 4px;
}}
.kpi-val {{
  font-size: 24px;
  font-weight: 800;
  letter-spacing: -0.02em;
  line-height: 1.2;
}}
.kpi-sub {{
  font-size: 12px;
  color: var(--text-muted);
  margin-top: 4px;
}}
.kpi-links {{
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin-top: 4px;
  font-size: 12.5px;
}}
.dist-wrap {{
  padding-top: 4px;
}}
.dist-bar {{
  display: flex;
  height: 10px;
  border-radius: 999px;
  overflow: hidden;
  background: var(--bg-stage);
  border: 1px solid var(--border);
}}
.dist-seg {{ height: 100%; }}
.seg-ext {{ background: var(--ext-col); }}
.seg-wid {{ background: var(--wid-col); }}
.seg-sho {{ background: var(--sho-col); }}
.seg-thm {{ background: var(--thm-col); }}
.dist-legend {{
  display: flex;
  flex-wrap: wrap;
  gap: 14px;
  margin-top: 8px;
  font-size: 12px;
  color: var(--text-muted);
}}
.dist-leg {{ display: inline-flex; align-items: center; gap: 6px; }}
.dot {{
  width: 9px;
  height: 9px;
  border-radius: 50%;
  display: inline-block;
}}

.controls {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  margin-bottom: 18px;
}}
.filter-pills {{
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
}}
.pill-btn {{
  padding: 6px 12px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: var(--bg-elev);
  color: var(--text-muted);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.14s ease;
}}
.pill-btn:hover {{
  color: var(--text);
  border-color: var(--border-strong);
}}
.pill-btn.active {{
  background: var(--accent);
  border-color: var(--accent);
  color: #fff;
}}
.search-sort {{
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}}
.search-box {{
  position: relative;
}}
.search-box input {{
  width: 280px;
  max-width: 100%;
  padding: 8px 34px 8px 12px;
  border-radius: 8px;
  border: 1px solid var(--border);
  background: var(--bg-elev);
  color: var(--text);
  font-size: 13px;
  outline: none;
}}
.search-box input:focus {{
  border-color: var(--accent);
  box-shadow: 0 0 0 3px var(--accent-soft);
}}
.search-clear {{
  position: absolute;
  right: 8px;
  top: 50%;
  transform: translateY(-50%);
  border: none;
  background: transparent;
  color: var(--text-dim);
  cursor: pointer;
  font-size: 15px;
  padding: 2px 5px;
  display: none;
}}
.search-clear.visible {{ display: inline-block; }}
select.sort-select {{
  padding: 8px 10px;
  border-radius: 8px;
  border: 1px solid var(--border);
  background: var(--bg-elev);
  color: var(--text);
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
}}
.result-meta {{
  font-size: 12.5px;
  color: var(--text-muted);
  margin-bottom: 12px;
}}

.grid {{
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(380px, 1fr));
  gap: 18px;
}}
@media (max-width: 480px) {{
  .grid {{ grid-template-columns: 1fr; }}
}}
.card {{
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  transition: transform 0.16s ease, border-color 0.16s ease, box-shadow 0.16s ease;
}}
.card:hover {{
  transform: translateY(-2px);
  border-color: var(--border-strong);
  box-shadow: var(--shadow);
}}
.card-stage {{
  position: relative;
  background: var(--bg-stage);
  border-bottom: 1px solid var(--border);
  aspect-ratio: 16 / 10;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 10px;
  cursor: zoom-in;
  overflow: hidden;
}}
.card-stage img {{
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
  border-radius: 6px;
  display: block;
}}
.zoom-hint {{
  position: absolute;
  right: 10px;
  bottom: 8px;
  padding: 2px 7px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 600;
  background: rgba(15, 18, 24, 0.72);
  color: #e8ecf4;
  opacity: 0;
  transition: opacity 0.15s ease;
  pointer-events: none;
}}
.card-stage:hover .zoom-hint {{ opacity: 1; }}
.fig-bar {{
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
  padding: 7px 12px;
  background: var(--bg-elev);
  border-bottom: 1px solid var(--border);
}}
.fig-bar-label {{
  font-size: 11px;
  font-weight: 700;
  color: var(--text-dim);
  margin-right: 4px;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}}
.fig-tab {{
  padding: 2px 8px;
  border-radius: 6px;
  border: 1px solid var(--border);
  background: var(--bg-pill);
  color: var(--text-muted);
  font-size: 11.5px;
  font-weight: 600;
  cursor: pointer;
}}
.fig-tab:hover {{ color: var(--text); border-color: var(--border-strong); }}
.fig-tab.active {{
  background: var(--accent);
  border-color: var(--accent);
  color: #fff;
}}
.card-body {{
  padding: 14px 16px 16px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  flex: 1;
}}
.card-head {{
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10px;
}}
.card-title {{
  margin: 0;
  font-size: 15.5px;
  font-weight: 700;
  line-height: 1.3;
}}
.cat-badge {{
  flex-shrink: 0;
  padding: 2px 8px;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.03em;
}}
.cat-Widgets {{ background: rgba(56, 193, 114, 0.16); color: var(--wid-col); }}
.cat-Extensions {{ background: rgba(76, 154, 255, 0.16); color: var(--ext-col); }}
.cat-Themes {{ background: rgba(168, 85, 247, 0.16); color: var(--thm-col); }}
.cat-Showcase {{ background: rgba(245, 158, 11, 0.18); color: var(--sho-col); }}

.card-meta {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  flex-wrap: wrap;
  font-size: 12px;
  font-family: var(--mono);
  color: var(--text-dim);
}}
.card-meta a {{
  color: var(--accent-text);
  word-break: break-all;
}}
.card-summary {{
  margin: 0;
  font-size: 13.2px;
  color: var(--text-muted);
}}
.api-chips {{
  display: flex;
  flex-wrap: wrap;
  gap: 5px;
  margin-top: auto;
  padding-top: 6px;
}}
.api-chip {{
  font-family: var(--mono);
  font-size: 11px;
  padding: 2px 7px;
  border-radius: 6px;
  background: var(--bg-pill);
  border: 1px solid var(--border);
  color: var(--text);
}}
.btn-live {{
  background: #16a34a;
  border-color: #16a34a;
  color: #fff;
}}
.btn-live:hover {{
  background: #15803d;
  border-color: #15803d;
  color: #fff;
}}
.card-foot {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding-top: 8px;
  margin-top: 4px;
  border-top: 1px solid var(--border);
  font-size: 12px;
}}
.try-live-link {{
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-weight: 600;
  color: var(--wid-col);
  text-decoration: none;
  white-space: nowrap;
}}
.try-live-link:hover {{
  text-decoration: underline;
}}

.empty-state {{
  text-align: center;
  padding: 48px 20px;
  background: var(--bg-elev);
  border: 1px dashed var(--border-strong);
  border-radius: var(--radius);
  color: var(--text-muted);
}}

/* Lightbox Modal */
.lb-backdrop {{
  position: fixed;
  inset: 0;
  z-index: 100;
  background: rgba(10, 12, 17, 0.86);
  display: none;
  align-items: center;
  justify-content: center;
  padding: 20px;
}}
.lb-backdrop.open {{ display: flex; }}
.lb-panel {{
  background: var(--bg-elev);
  border: 1px solid var(--border-strong);
  border-radius: 14px;
  max-width: min(1280px, 96vw);
  max-height: 94vh;
  width: 100%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  box-shadow: 0 24px 64px rgba(0, 0, 0, 0.55);
}}
.lb-header {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 16px;
  border-bottom: 1px solid var(--border);
  flex-wrap: wrap;
}}
.lb-title-wrap {{
  display: flex;
  flex-direction: column;
  gap: 2px;
}}
.lb-title {{
  font-size: 15px;
  font-weight: 700;
}}
.lb-sub {{
  font-size: 12px;
  font-family: var(--mono);
  color: var(--text-muted);
}}
.lb-controls {{
  display: flex;
  align-items: center;
  gap: 8px;
}}
.lb-tabs {{
  display: none;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
  padding: 8px 16px;
  background: var(--bg-card);
  border-bottom: 1px solid var(--border);
}}
.lb-tabs.visible {{ display: flex; }}
.lb-stage {{
  flex: 1;
  min-height: 220px;
  padding: 16px;
  background: var(--bg-stage);
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: auto;
}}
.lb-stage img {{
  max-width: 100%;
  max-height: 74vh;
  object-fit: contain;
  border-radius: 8px;
  box-shadow: 0 12px 32px rgba(0, 0, 0, 0.35);
}}
.lb-footer {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 9px 16px;
  border-top: 1px solid var(--border);
  font-size: 12px;
  color: var(--text-muted);
}}

footer {{
  max-width: 1380px;
  margin: 12px auto 32px;
  padding: 16px 24px 0;
  border-top: 1px solid var(--border);
  color: var(--text-dim);
  font-size: 12.5px;
  display: flex;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 10px;
}}
</style>
</head>
<body>

<header class="topbar">
  <div class="topbar-inner">
    <div class="brand">
      <span class="brand-logo">IG</span>
      <h1>ImGee Visual Toolkit Dashboard</h1>
      <span class="ver-pill">v{ver}</span>
    </div>
    <div class="top-actions">
      <a id="livePlaygroundTopBtn" class="btn btn-live" href="{PAGES_URL}playground/" target="_blank" rel="noopener">&#9654; Try Live Playground</a>
      <a class="btn" href="ImGee_User_Guide.pdf" target="_blank" rel="noopener">User Guide PDF ({ug["pages"]}p)</a>
      <a class="btn" href="ImGee_UX_Design_Manifesto.pdf" target="_blank" rel="noopener">UX Manifesto PDF ({ux["pages"]}p)</a>
      <a class="btn" href="{REPO_URL}" target="_blank" rel="noopener">GitHub Repo</a>
      <button id="themeToggleBtn" class="btn btn-primary" type="button" title="Toggle Dark Studio / Paper Light">Theme: Dark Studio</button>
    </div>
  </div>
</header>

<main class="wrap">
  <section class="hero">
    <p class="hero-lead">
      Header-only ergonomic widgets, stationary <code>TimedTooltip</code>s, CommonMark + sortable GFM tables,
      procedural vector icons, curated themes, and a turnkey <code>DocumentViewer</code> for Dear ImGui (<code>v1.84-v1.92+</code>).
      Switch globally between <b>Dark Studio</b> and <b>Paper Light</b> 2x Retina captures, filter by category or API symbol,
      or click any card to inspect full-resolution screenshots (including all 8 interactive Demo tabs).
    </p>
    <div class="kpi-grid">
      <div class="kpi">
        <div class="kpi-label">Feature Headers</div>
        <div class="kpi-val">{feat_n} <span style="font-size:14px;font-weight:600;color:var(--text-muted);">({tot_h} total .h)</span></div>
        <div class="kpi-sub">{cc["Widgets"]} Widgets / {cc["Extensions"]} Extensions / {cc["Themes"]} Themes / {cc["Showcase"]} Showcase</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">Total Feature LOC</div>
        <div class="kpi-val">{tot_loc:,}</div>
        <div class="kpi-sub">100% header-only C++11/17 (zero .cpp required)</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">2x Retina Coverage</div>
        <div class="kpi-val">{cov_n} / {feat_n} <span style="font-size:14px;font-weight:600;color:var(--wid-col);">({cov_pct}%)</span></div>
        <div class="kpi-sub">Dark Studio + Paper Light headless captures</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">Compiled PDF Manuals</div>
        <div class="kpi-links">
          <a href="ImGee_User_Guide.pdf" target="_blank" rel="noopener">ImGee_User_Guide.pdf -- {ug["pages"]} pages ({ug["size_label"]})</a>
          <a href="ImGee_UX_Design_Manifesto.pdf" target="_blank" rel="noopener">ImGee_UX_Design_Manifesto.pdf -- {ux["pages"]} pages ({ux["size_label"]})</a>
        </div>
      </div>
    </div>
    <div class="dist-wrap">
      <div class="dist-bar" aria-label="Lines of code distribution by category">
        {bar_html}
      </div>
      <div class="dist-legend">
        {legend_html}
      </div>
    </div>
  </section>

  <section class="controls" aria-label="Filter and sort controls">
    <div class="filter-pills" id="categoryPills">
      <button class="pill-btn active" data-cat="All" type="button">All ({feat_n})</button>
      <button class="pill-btn" data-cat="Showcase" type="button">Showcase ({cc["Showcase"]})</button>
      <button class="pill-btn" data-cat="Extensions" type="button">Extensions ({cc["Extensions"]})</button>
      <button class="pill-btn" data-cat="Widgets" type="button">Widgets ({cc["Widgets"]})</button>
      <button class="pill-btn" data-cat="Themes" type="button">Themes ({cc["Themes"]})</button>
    </div>
    <div class="search-sort">
      <div class="search-box">
        <input id="searchInput" type="text" placeholder="Filter headers, titles, or APIs (/) ..." aria-label="Filter headers">
        <button id="searchClear" class="search-clear" type="button" aria-label="Clear search">&times;</button>
      </div>
      <select id="sortSelect" class="sort-select" aria-label="Sort headers">
        <option value="category">Sort: Category</option>
        <option value="recent">Sort: Recently Updated</option>
        <option value="loc">Sort: Largest LOC</option>
        <option value="alpha">Sort: A-Z</option>
      </select>
    </div>
  </section>

  <div class="result-meta" id="resultMeta">Showing {feat_n} of {feat_n} feature headers</div>
  <section class="grid" id="cardsGrid"></section>
</main>

<div class="lb-backdrop" id="lightbox" role="dialog" aria-modal="true" aria-labelledby="lbTitle">
  <div class="lb-panel">
    <div class="lb-header">
      <div class="lb-title-wrap">
        <span class="lb-title" id="lbTitle"></span>
        <span class="lb-sub" id="lbSub"></span>
      </div>
      <div class="lb-controls">
        <a class="btn btn-live" id="lbLiveLink" href="{PAGES_URL}playground/" target="_blank" rel="noopener">&#9654; Try Live</a>
        <button class="btn" id="lbThemeBtn" type="button">Dark Studio</button>
        <button class="btn" id="lbPrevBtn" type="button" title="Previous figure (Left arrow)">&larr;</button>
        <button class="btn" id="lbNextBtn" type="button" title="Next figure (Right arrow)">&rarr;</button>
        <button class="btn" id="lbCloseBtn" type="button" title="Close (Esc)">Close (Esc)</button>
      </div>
    </div>
    <div class="lb-tabs" id="lbTabs"></div>
    <div class="lb-stage">
      <img id="lbImg" src="" alt="">
    </div>
    <div class="lb-footer">
      <span id="lbCaption"></span>
      <span>Tip: Press <b>Left / Right</b> to switch figures, <b>Esc</b> to close.</span>
    </div>
  </div>
</div>

<footer>
  <span>ImGee v{ver} (<code>IMGEE_VERSION_NUM {data["versionNum"]}</code>) -- BSD 3-Clause License</span>
  <span>Auto-generated by <code>tools/generate_dashboard.py</code> on {data["generatedDate"]} -- Works offline (<code>file://</code>) and on <a href="{PAGES_URL}" target="_blank" rel="noopener">GitHub Pages</a></span>
</footer>

<script>
const DASHBOARD_DATA = {data_json};

(function() {{
  const state = {{
    theme: "dark",
    category: "All",
    query: "",
    sort: "category",
    cardFigIndex: {{}},
    lbOpen: false,
    lbCardId: null,
    lbFigIdx: 0,
    lbTheme: "dark"
  }};

  try {{
    const saved = localStorage.getItem("imgee_dashboard_theme");
    if (saved === "light" || saved === "dark") state.theme = saved;
  }} catch (e) {{}}

  const htmlEl = document.documentElement;
  const themeBtn = document.getElementById("themeToggleBtn");
  const liveTopBtn = document.getElementById("livePlaygroundTopBtn");
  const pillsWrap = document.getElementById("categoryPills");
  const searchInput = document.getElementById("searchInput");
  const searchClear = document.getElementById("searchClear");
  const sortSelect = document.getElementById("sortSelect");
  const resultMeta = document.getElementById("resultMeta");
  const grid = document.getElementById("cardsGrid");

  const lb = document.getElementById("lightbox");
  const lbTitle = document.getElementById("lbTitle");
  const lbSub = document.getElementById("lbSub");
  const lbTabs = document.getElementById("lbTabs");
  const lbImg = document.getElementById("lbImg");
  const lbCaption = document.getElementById("lbCaption");
  const lbLiveLink = document.getElementById("lbLiveLink");
  const lbThemeBtn = document.getElementById("lbThemeBtn");
  const lbPrevBtn = document.getElementById("lbPrevBtn");
  const lbNextBtn = document.getElementById("lbNextBtn");
  const lbCloseBtn = document.getElementById("lbCloseBtn");

  const cardById = new Map(DASHBOARD_DATA.cards.map(c => [c.id, c]));

  function playgroundUrl(tabNum, themeMode) {{
    const isFile = window.location.protocol === "file:";
    const base = isFile ? (DASHBOARD_DATA.pagesUrl + "playground/") : "playground/";
    const params = [];
    if (tabNum >= 1 && tabNum <= 8) params.push("tab=" + tabNum);
    params.push("theme=" + (themeMode === "light" ? "light" : "dark"));
    return base + "?" + params.join("&");
  }}

  function cardDemoTab(card, figIdx) {{
    if (card.path === "imgee_demo.h") return (figIdx || 0) + 1;
    return card.demoTab || 1;
  }}

  function applyGlobalTheme(mode) {{
    state.theme = mode;
    htmlEl.setAttribute("data-theme", mode);
    themeBtn.textContent = mode === "dark" ? "Theme: Dark Studio" : "Theme: Paper Light";
    if (liveTopBtn) liveTopBtn.href = playgroundUrl(0, mode);
    try {{ localStorage.setItem("imgee_dashboard_theme", mode); }} catch (e) {{}}
    renderCards();
  }}

  function escHtml(s) {{
    return String(s).replace(/[&<>"']/g, ch => ({{
      "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
    }}[ch]));
  }}

  function filteredCards() {{
    const q = state.query.trim().toLowerCase();
    let list = DASHBOARD_DATA.cards.filter(c => {{
      if (state.category !== "All" && c.category !== state.category) return false;
      if (!q) return true;
      if (c.path.toLowerCase().includes(q)) return true;
      if (c.title.toLowerCase().includes(q)) return true;
      if (c.summary.toLowerCase().includes(q)) return true;
      if (c.category.toLowerCase().includes(q)) return true;
      return c.apis.some(a => a.toLowerCase().includes(q));
    }});

    if (state.sort === "recent") {{
      list = list.slice().sort((a, b) => (b.updatedTs - a.updatedTs) || (a.id - b.id));
    }} else if (state.sort === "loc") {{
      list = list.slice().sort((a, b) => (b.loc - a.loc) || (a.id - b.id));
    }} else if (state.sort === "alpha") {{
      list = list.slice().sort((a, b) => a.title.localeCompare(b.title));
    }} else {{
      list = list.slice().sort((a, b) => a.id - b.id);
    }}
    return list;
  }}

  function renderCards() {{
    const list = filteredCards();
    const total = DASHBOARD_DATA.cards.length;
    const themeLabel = state.theme === "dark" ? "Dark Studio" : "Paper Light";
    resultMeta.textContent = `Showing ${{list.length}} of ${{total}} feature headers (${{themeLabel}} captures)`;

    if (!list.length) {{
      grid.innerHTML = `<div class="empty-state" style="grid-column:1/-1;">No headers match <b>${{escHtml(state.query)}}</b> in <b>${{escHtml(state.category)}}</b>.</div>`;
      return;
    }}

    const parts = list.map(c => {{
      const figIdx = state.cardFigIndex[c.id] || 0;
      const fig = c.figures[figIdx] || c.figures[0];
      const imgSrc = state.theme === "light" ? fig.light : fig.dark;

      let figBar = "";
      if (c.figures.length > 1) {{
        const barTitle = c.path === "imgee_demo.h" ? "Tabs:" : "Views:";
        const btns = c.figures.map((f, i) =>
          `<button class="fig-tab${{i === figIdx ? " active" : ""}}" type="button" data-card="${{c.id}}" data-fig="${{i}}" title="${{escHtml(f.label)}}">${{escHtml(f.short)}}</button>`
        ).join("");
        figBar = `<div class="fig-bar"><span class="fig-bar-label">${{barTitle}}</span>${{btns}}</div>`;
      }}

      const apisHtml = c.apis.map(a => `<span class="api-chip">${{escHtml(a)}}</span>`).join("");
      const liveTab = cardDemoTab(c, figIdx);
      const liveHref = playgroundUrl(liveTab, state.theme);

      return `
        <article class="card" data-card-id="${{c.id}}">
          <div class="card-stage" data-zoom-card="${{c.id}}" title="Click to enlarge ${{escHtml(fig.label)}}">
            <img src="${{escHtml(imgSrc)}}" alt="${{escHtml(c.title)}} -- ${{escHtml(fig.label)}}" loading="lazy">
            <span class="zoom-hint">Click to zoom</span>
          </div>
          ${{figBar}}
          <div class="card-body">
            <div class="card-head">
              <h2 class="card-title">${{escHtml(c.title)}}</h2>
              <span class="cat-badge cat-${{escHtml(c.category)}}">${{escHtml(c.category)}}</span>
            </div>
            <div class="card-meta">
              <a href="${{escHtml(c.githubUrl)}}" target="_blank" rel="noopener">${{escHtml(c.path)}}</a>
              <span>${{c.loc.toLocaleString()}} LOC &middot; ${{escHtml(c.updatedDate)}}</span>
            </div>
            <p class="card-summary">${{escHtml(c.summary)}}</p>
            <div class="api-chips">${{apisHtml}}</div>
            <div class="card-foot">
              <span style="color:var(--text-dim);">Demo Tab ${{liveTab}}</span>
              <a class="try-live-link" href="${{escHtml(liveHref)}}" target="_blank" rel="noopener" title="Open Tab ${{liveTab}} in the live 60fps C++ WebAssembly Playground">&#9654; Try Live</a>
            </div>
          </div>
        </article>`;
    }});

    grid.innerHTML = parts.join("");
  }}

  function renderLightbox() {{
    if (!state.lbOpen || state.lbCardId === null) return;
    const c = cardById.get(state.lbCardId);
    if (!c) return;
    const fig = c.figures[state.lbFigIdx] || c.figures[0];
    const src = state.lbTheme === "light" ? fig.light : fig.dark;

    lbTitle.textContent = `${{c.title}} -- ${{fig.label}}`;
    lbSub.textContent = `${{c.path}}  |  ${{c.loc.toLocaleString()}} LOC  |  ${{state.lbTheme === "dark" ? "Dark Studio" : "Paper Light"}} (2x Retina)`;
    if (lbLiveLink) lbLiveLink.href = playgroundUrl(cardDemoTab(c, state.lbFigIdx), state.lbTheme);
    lbThemeBtn.textContent = state.lbTheme === "dark" ? "Switch to Paper Light" : "Switch to Dark Studio";
    lbImg.src = src;
    lbImg.alt = `${{c.title}} -- ${{fig.label}}`;
    lbCaption.textContent = `${{fig.label}} (${{state.lbFigIdx + 1}} / ${{c.figures.length}}) -- ${{src}}`;

    const multi = c.figures.length > 1;
    lbPrevBtn.style.display = multi ? "" : "none";
    lbNextBtn.style.display = multi ? "" : "none";

    if (multi) {{
      lbTabs.classList.add("visible");
      lbTabs.innerHTML = c.figures.map((f, i) =>
        `<button class="fig-tab${{i === state.lbFigIdx ? " active" : ""}}" type="button" data-lb-fig="${{i}}">${{escHtml(f.label)}}</button>`
      ).join("");
    }} else {{
      lbTabs.classList.remove("visible");
      lbTabs.innerHTML = "";
    }}
  }}

  function openLightbox(cardId) {{
    const c = cardById.get(cardId);
    if (!c) return;
    state.lbOpen = true;
    state.lbCardId = cardId;
    state.lbFigIdx = state.cardFigIndex[cardId] || 0;
    state.lbTheme = state.theme;
    lb.classList.add("open");
    renderLightbox();
  }}

  function closeLightbox() {{
    state.lbOpen = false;
    lb.classList.remove("open");
  }}

  function stepLightbox(delta) {{
    if (!state.lbOpen || state.lbCardId === null) return;
    const c = cardById.get(state.lbCardId);
    if (!c || c.figures.length <= 1) return;
    const n = c.figures.length;
    state.lbFigIdx = (state.lbFigIdx + delta + n) % n;
    state.cardFigIndex[c.id] = state.lbFigIdx;
    renderLightbox();
    renderCards();
  }}

  themeBtn.addEventListener("click", () => {{
    applyGlobalTheme(state.theme === "dark" ? "light" : "dark");
  }});

  pillsWrap.addEventListener("click", e => {{
    const btn = e.target.closest(".pill-btn");
    if (!btn) return;
    state.category = btn.getAttribute("data-cat") || "All";
    pillsWrap.querySelectorAll(".pill-btn").forEach(b => b.classList.toggle("active", b === btn));
    renderCards();
  }});

  searchInput.addEventListener("input", () => {{
    state.query = searchInput.value;
    searchClear.classList.toggle("visible", state.query.length > 0);
    renderCards();
  }});
  searchClear.addEventListener("click", () => {{
    searchInput.value = "";
    state.query = "";
    searchClear.classList.remove("visible");
    renderCards();
    searchInput.focus();
  }});

  sortSelect.addEventListener("change", () => {{
    state.sort = sortSelect.value;
    renderCards();
  }});

  grid.addEventListener("click", e => {{
    const tabBtn = e.target.closest(".fig-tab");
    if (tabBtn) {{
      const cid = Number(tabBtn.getAttribute("data-card"));
      const fidx = Number(tabBtn.getAttribute("data-fig"));
      state.cardFigIndex[cid] = fidx;
      renderCards();
      return;
    }}
    const stage = e.target.closest("[data-zoom-card]");
    if (stage) {{
      openLightbox(Number(stage.getAttribute("data-zoom-card")));
    }}
  }});

  lbTabs.addEventListener("click", e => {{
    const btn = e.target.closest("[data-lb-fig]");
    if (!btn) return;
    const fidx = Number(btn.getAttribute("data-lb-fig"));
    state.lbFigIdx = fidx;
    if (state.lbCardId !== null) state.cardFigIndex[state.lbCardId] = fidx;
    renderLightbox();
    renderCards();
  }});

  lbThemeBtn.addEventListener("click", () => {{
    state.lbTheme = state.lbTheme === "dark" ? "light" : "dark";
    renderLightbox();
  }});
  lbPrevBtn.addEventListener("click", () => stepLightbox(-1));
  lbNextBtn.addEventListener("click", () => stepLightbox(+1));
  lbCloseBtn.addEventListener("click", closeLightbox);
  lb.addEventListener("click", e => {{
    if (e.target === lb) closeLightbox();
  }});

  document.addEventListener("keydown", e => {{
    if (state.lbOpen) {{
      if (e.key === "Escape") closeLightbox();
      else if (e.key === "ArrowLeft") stepLightbox(-1);
      else if (e.key === "ArrowRight") stepLightbox(+1);
      return;
    }}
    if (e.key === "/" && document.activeElement !== searchInput) {{
      e.preventDefault();
      searchInput.focus();
      searchInput.select();
    }} else if (e.key === "Escape" && document.activeElement === searchInput) {{
      searchInput.blur();
    }}
  }});

  applyGlobalTheme(state.theme);
}})();
</script>
</body>
</html>
"""


def main() -> int:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)
    out_path = os.path.join(repo_root, "docs", "index.html")

    data = collect_repo_data(repo_root)
    html_text = build_html(data)

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(html_text)

    print(
        f"[Dashboard Generator] Generated {out_path} "
        f"(ImGee v{data['version']}, {data['featureHeaderCount']} feature headers / "
        f"{data['totalHeaderCount']} total .h, {data['totalFeatureLoc']:,} LOC, "
        f"coverage {data['coveredCards']}/{data['featureHeaderCount']}, "
        f"User Guide {data['pdfs']['userGuide']['pages']}p, "
        f"UX Manifesto {data['pdfs']['manifesto']['pages']}p)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
