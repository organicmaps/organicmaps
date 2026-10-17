#!/usr/bin/env python3
"""Render an Instagram post from a post.toml description.

    python3 build.py releases/2026-07 [--only 4x5] [--slide 2]

Reads <dir>/post.toml, writes HTML into <dir>/html/<fmt>/ and final PNGs into
<dir>/export/<fmt>/. Rendering goes through headless Chrome at 2x device scale,
then Pillow downsamples to the exact Instagram dimensions.
"""

import argparse
import html
import shutil
import subprocess
import sys
import time
import tomllib
from PIL import Image
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from urllib.request import pathname2url

CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

# Instagram canvas sizes. Keys must match the `formats` list in post.toml.
FORMATS = {"4x5": (1080, 1350), "1x1": (1080, 1080), "9x16": (1080, 1920)}

SCALE = 2  # render at 2x, downsample for crisp text
RENDER_TIMEOUT_S = 40
WORKERS = 4

SKILL_DIR = Path(__file__).resolve().parent.parent
BRAND_CSS = SKILL_DIR / "assets" / "brand.css"
SOCIAL_ROOT = SKILL_DIR.parent.parent.parent  # om-social/
BRAND_DIR = SOCIAL_ROOT / "brand"


def furl(path: Path) -> str:
    """Absolute file:// URL. Chrome hangs on top-level data: URLs, so every
    asset reference must be a real file."""
    return "file://" + pathname2url(str(path.resolve()))


def esc(text) -> str:
    return html.escape(str(text), quote=True)


# --------------------------------------------------------------------------
# slide -> HTML body
# --------------------------------------------------------------------------

def render_media(slide: dict, base: Path) -> str:
    media = slide.get("media")
    if not media:
        return ""
    device = slide.get("device", "phone")
    src = (base / media).resolve()
    if not src.exists():
        raise SystemExit(f"missing media: {src}")
    return f'<div class="media"><img class="device {esc(device)}" src="{furl(src)}" alt=""></div>'


def render_footer(slide: dict, index: int, total: int) -> str:
    if slide.get("type") in ("cover", "cta"):
        return ""
    mark = furl(BRAND_DIR / "logo-mark-white.svg")
    return (
        f'<div class="footer"><img src="{mark}" alt=""><span>organicmaps.app</span>'
        f'<span class="spacer"></span><span>{index}/{total}</span></div>'
    )


def render_slide(slide: dict, base: Path, index: int, total: int) -> tuple[str, str]:
    """Return (theme_class, inner_html)."""
    kind = slide.get("type", "feature")
    theme = slide.get("theme") or ("light" if kind == "list" else "green")
    parts = []

    if kind == "cover":
        parts.append(f'<img class="logo" src="{furl(BRAND_DIR / "logo.svg")}" alt="">')
        if slide.get("kicker"):
            parts.append(f'<div class="kicker">{esc(slide["kicker"])}</div>')
        parts.append(f'<h1 class="title">{esc(slide["title"])}</h1>')
        if slide.get("subtitle"):
            parts.append(f'<p class="body">{esc(slide["subtitle"])}</p>')
        parts.append('<div class="rule"></div>')

    elif kind == "cta":
        parts.append(f'<h1 class="title">{esc(slide["title"])}</h1>')
        if slide.get("body"):
            parts.append(f'<p class="body">{esc(slide["body"])}</p>')
        if slide.get("url"):
            parts.append(f'<div class="url">{esc(slide["url"])}</div>')
        badges = slide.get("badges", [])
        if badges:
            imgs = "".join(
                f'<img src="{furl(BRAND_DIR / "badges" / (b + ".png"))}" alt="">' for b in badges
            )
            parts.append(f'<div class="badges">{imgs}</div>')

    else:  # feature | list
        if slide.get("eyebrow"):
            parts.append(f'<div class="eyebrow">{esc(slide["eyebrow"])}</div>')
        parts.append(f'<h1 class="title">{esc(slide["title"])}</h1>')
        if slide.get("body"):
            parts.append(f'<p class="body">{esc(slide["body"])}</p>')
        if slide.get("items"):
            lis = "".join(f"<li>{esc(i)}</li>" for i in slide["items"])
            parts.append(f'<ul class="items">{lis}</ul>')
        parts.append(render_media(slide, base))

    parts.append(render_footer(slide, index, total))
    return theme, "".join(p for p in parts if p)


def layout(slide: dict) -> tuple[str, str]:
    """Portrait mockups bleed off the bottom, landscape ones run full width.

    `bleed` may be false to disable, or a pixel number to override how much of
    the phone is allowed off-canvas — useful when the feature being shown sits
    low in the screenshot.
    """
    device = slide.get("device", "phone") if slide.get("media") else None
    bleed = slide.get("bleed", True)
    if device == "phone" and bleed is not False:
        style = f" style='--bleed:{int(bleed)}px'" if bleed is not True else ""
        return " bleed", style
    if device == "desktop":
        return " wide", ""
    return "", ""


def render_html(slide: dict, base: Path, fmt: str, index: int, total: int) -> str:
    theme, inner = render_slide(slide, base, index, total)
    extra_class, extra_style = layout(slide)
    kind = slide.get("type", "feature") + extra_class
    mark_var = f'url({furl(BRAND_DIR / "logo-mark-white.svg")})'
    return (
        "<!doctype html><html><head><meta charset='utf-8'>"
        f"<link rel='stylesheet' href='{furl(BRAND_CSS)}'>"
        f"<style>:root{{--mark-white:{mark_var}}}</style></head>"
        f"<body class='fmt-{fmt} theme-{theme}'>"
        f"<div class='slide {kind}'{extra_style}>{inner}</div>"
        "</body></html>"
    )


# --------------------------------------------------------------------------
# Chrome
# --------------------------------------------------------------------------

def shoot(html_path: Path, png_path: Path, size: tuple[int, int], profile: Path) -> None:
    """Chrome writes a valid PNG but never exits, so poll for the file and kill."""
    w, h = size
    png_path.unlink(missing_ok=True)
    cmd = [
        CHROME, "--headless", "--disable-gpu", "--no-first-run",
        "--disable-component-update", "--disable-background-networking",
        "--hide-scrollbars", f"--user-data-dir={profile}",
        f"--window-size={w},{h}", f"--force-device-scale-factor={SCALE}",
        f"--screenshot={png_path}", furl(html_path),
    ]
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        deadline = time.time() + RENDER_TIMEOUT_S
        stable = 0
        while time.time() < deadline:
            time.sleep(0.4)
            if png_path.exists() and png_path.stat().st_size > 0:
                stable += 1
                if stable >= 3:  # give the write a moment to finish
                    return
    finally:
        proc.kill()
        proc.wait()
    if not (png_path.exists() and png_path.stat().st_size):
        raise SystemExit(f"chrome produced no screenshot for {html_path}")


def finalize(png_path: Path, size: tuple[int, int]) -> None:
    """Downsample the 2x render to the exact target size and sanity-check it."""
    with Image.open(png_path) as im:
        im = im.convert("RGB")
        if im.size != size:
            im = im.resize(size, Image.LANCZOS)
        extrema = im.getextrema()
        if all(lo == hi for lo, hi in extrema):
            raise SystemExit(f"{png_path.name} rendered as a flat frame — check the HTML")
        im.save(png_path, "PNG", optimize=True)
    with Image.open(png_path) as im:
        assert im.size == size, f"{png_path}: {im.size} != {size}"


# --------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("release_dir")
    ap.add_argument("--only", help="render a single format, e.g. 4x5")
    ap.add_argument("--slide", type=int, help="render a single slide (1-based)")
    args = ap.parse_args()

    base = Path(args.release_dir).resolve()
    post = tomllib.loads((base / "post.toml").read_text())
    slides = post["slides"]
    formats = [args.only] if args.only else post.get("formats", list(FORMATS))
    for f in formats:
        if f not in FORMATS:
            raise SystemExit(f"unknown format {f!r}; known: {', '.join(FORMATS)}")

    jobs = []
    for fmt in formats:
        (base / "html" / fmt).mkdir(parents=True, exist_ok=True)
        (base / "export" / fmt).mkdir(parents=True, exist_ok=True)
        for i, slide in enumerate(slides, start=1):
            if args.slide and i != args.slide:
                continue
            name = f"{i:02d}-{slide.get('type', 'feature')}"
            html_path = base / "html" / fmt / f"{name}.html"
            html_path.write_text(render_html(slide, base, fmt, i, len(slides)))
            jobs.append((fmt, name, html_path, base / "export" / fmt / f"{name}.png"))

    profiles = SOCIAL_ROOT / ".cache" / "om-post-profile"
    shutil.rmtree(profiles, ignore_errors=True)

    def run(job, worker):
        fmt, name, html_path, png_path = job
        shoot(html_path, png_path, FORMATS[fmt], profiles / str(worker))
        finalize(png_path, FORMATS[fmt])
        print(f"  ✓ {fmt}/{name}.png")

    print(f"rendering {len(jobs)} images…")
    with ThreadPoolExecutor(WORKERS) as pool:
        list(pool.map(lambda p: run(p[1], p[0] % WORKERS), enumerate(jobs)))

    shutil.rmtree(profiles, ignore_errors=True)
    print(f"done → {base / 'export'}")


if __name__ == "__main__":
    main()
