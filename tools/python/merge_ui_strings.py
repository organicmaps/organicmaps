#!/usr/bin/env python3
"""Combine UI translation catalogs for Twine without allowing duplicate keys."""

import re
import sys
from pathlib import Path

UI_CATALOGS = ("strings.txt", "accessibility.txt")
_KEY_RE = re.compile(r"^\[([^\[\]]+)\]$")


def merge_ui_strings(directory: Path) -> str:
    sources = []
    locations = {}
    for name in UI_CATALOGS:
        text = (directory / name).read_text(encoding="utf-8")
        for line_number, line in enumerate(text.splitlines(), 1):
            match = _KEY_RE.fullmatch(line.strip())
            if not match:
                continue
            key = match.group(1)
            location = f"{name}:{line_number}"
            if key in locations:
                raise ValueError(f"Duplicate translation key [{key}] at {locations[key]} and {location}")
            locations[key] = location
        sources.append(text.rstrip())
    return "\n\n".join(sources) + "\n"


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: merge_ui_strings.py CATALOG_DIRECTORY OUTPUT_FILE")
    Path(sys.argv[2]).write_text(merge_ui_strings(Path(sys.argv[1])), encoding="utf-8")
