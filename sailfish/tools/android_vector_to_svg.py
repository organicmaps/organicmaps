#!/usr/bin/env python3
"""Converts the Android vector drawables the Sailfish app uses into SVG icons, run by the build:
  sailfish/tools/android_vector_to_svg.py <output dir>
"""

import os
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
sys.path.insert(0, os.path.join(ROOT, "tools", "python"))
from vector_drawable_to_svg import to_svg

# The app resources, then the SDK and branding ones.
RES_DIRS = [
    os.path.join(ROOT, "android/app/src/main/res"),
    os.path.join(ROOT, "android/sdk/src/main/res"),
    os.path.join(ROOT, "android/libs/branding/src/main/res"),
]
# The light/night layer previews.
LAYERS = [
    "ic_layers_outdoors",
    "ic_layers_hiking",
    "ic_layers_cycling",
    "ic_layers_subway",
]
# My position button states.
MY_POSITION = ["ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]
OUTPUTS = [
    ("layers", LAYERS),
    ("myposition", MY_POSITION),
]


def main():
    for subdir, names in OUTPUTS:
        out_dir = os.path.join(sys.argv[1], subdir)
        os.makedirs(out_dir, exist_ok=True)
        for variant, suffix in (("drawable", ""), ("drawable-night", "_night")):
            for name in names:
                paths = [os.path.join(res, variant, name + ".xml") for res in RES_DIRS]
                path = next((p for p in paths if os.path.exists(p)), None)
                # Every icon has a day variant.
                if not path and not suffix:
                    raise FileNotFoundError(name)
                if path:
                    try:
                        tree = to_svg(ET.parse(path).getroot())
                    except ValueError as e:
                        raise ValueError("%s: %s" % (path, e)) from e
                    tree.write(os.path.join(out_dir, name + suffix + ".svg"), encoding="unicode")


if __name__ == "__main__":
    main()
