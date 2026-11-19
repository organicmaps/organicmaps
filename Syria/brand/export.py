"""Renders every brand design to PNG and PDF with headless Chrome, then zips them.

usage: python export.py   (from any folder; needs Chrome and an internet connection for the Google Fonts)
Output: export/*.png, export/*.pdf, export/*.svg and the three zips linked from index.html.
"""
import shutil
import subprocess
import zipfile
from pathlib import Path

BRAND = Path(__file__).resolve().parent
OUT = BRAND / "export"
CHROME_CANDIDATES = [
    Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe"),
    Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
    Path("/usr/bin/google-chrome"),
    Path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"),
]
CHROME = next(str(p) for p in CHROME_CANDIDATES if p.exists())

# (source, output name, CSS width, CSS height, pixel ratio, transparent background)
PNGS = [
    ("mark.svg", "mark.png", 960, 1200, 1, True),
    ("mark-dark.svg", "mark-dark.png", 960, 1200, 1, True),
    ("designs/sticker.html", "sticker-200x100mm.png", 1000, 500, 3, False),
    ("designs/badge.html", "badge-80mm.png", 800, 800, 2, True),
    ("designs/post.html", "post-1080.png", 1080, 1080, 1, False),
    ("brochure-a5.html?page=1", "brochure-a5-page1.png", 559, 794, 3, False),
    ("brochure-a5.html?page=2", "brochure-a5-page2.png", 559, 794, 3, False),
    ("brochure-a5.html?page=3", "brochure-a5-page3.png", 559, 794, 3, False),
    ("brochure-a5.html?page=4", "brochure-a5-page4.png", 559, 794, 3, False),
    ("identity.html", "identity-board.png", 1300, 2900, 1, False),
]
PDFS = [
    ("brochure-a5.html", "brochure-a5.pdf"),
    ("identity.html", "identity-board.pdf"),
    ("designs/sticker.html", "sticker-200x100mm.pdf"),
    ("designs/badge.html", "badge-80mm.pdf"),
    ("designs/post.html", "post-1080.pdf"),
]
SVGS = ["mark.svg", "mark-dark.svg", "qr-website.svg"]


def url(source: str) -> str:
    # ?export hides the download toolbar of the page.
    path, _, query = source.partition("?")
    return (BRAND / path).as_uri() + "?" + (f"{query}&export=1" if query else "export=1")


def chrome(*args: str) -> None:
    subprocess.run([CHROME, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--virtual-time-budget=8000",
                    *args], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main() -> None:
    OUT.mkdir(exist_ok=True)
    for source, name, w, h, ratio, transparent in PNGS:
        extra = ["--default-background-color=00000000"] if transparent else []
        chrome(f"--window-size={w},{h}", f"--force-device-scale-factor={ratio}", *extra,
               f"--screenshot={OUT / name}", url(source))
        print("png", name)
    for source, name in PDFS:
        chrome("--no-pdf-header-footer", f"--print-to-pdf={OUT / name}", url(source))
        print("pdf", name)
    for name in SVGS:
        shutil.copy(BRAND / name, OUT / name)

    pngs = [OUT / n for _, n, *_ in PNGS] + [OUT / n for n in SVGS]
    pdfs = [OUT / n for _, n in PDFS]
    for zip_name, files in (("syria-om-brand-images.zip", pngs), ("syria-om-brand-pdf.zip", pdfs),
                            ("syria-om-brand-kit.zip", pngs + pdfs)):
        with zipfile.ZipFile(OUT / zip_name, "w", zipfile.ZIP_DEFLATED) as z:
            for f in files:
                z.write(f, f.name)
        print("zip", zip_name)


if __name__ == "__main__":
    main()
