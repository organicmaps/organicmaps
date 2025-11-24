#!/usr/bin/env python3

import fontforge
import os
import glob

SVG_DIR = "."
OUTPUT = "../fonts/organic_maps_emoji.ttf"

font = fontforge.font()
font.fontname = "OrganicMapsEmoji"
font.familyname = "OrganicMapsEmoji"
font.fullname = "OrganicMapsEmoji"
font.em = 1024

for svg_file in glob.glob(os.path.join(SVG_DIR, "*.svg")):
    base = os.path.basename(svg_file)
    name = os.path.splitext(base)[0]

    try:
        codepoint = int(name, 16)
    except ValueError:
        print("Skipping non-hex file:", base)
        continue

    print(f"Importing {svg_file} → U+{codepoint:06X}")

    # --- Default ---
    glyph_default = font.createChar(codepoint, f"u{codepoint:06X}")
    glyph_default.importOutlines(svg_file)
    glyph_default.width = 1024

font.generate(OUTPUT)
print("Done:", OUTPUT)
