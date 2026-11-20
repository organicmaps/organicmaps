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
    "ic_layers_satellite",
]
# My position button states.
MY_POSITION = ["ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]
# The logo of the help button and the about page, and the icons of the about page rows, as in about.xml on Android.
# Donate and the social networks of the editor are reused from the menu and editor copies. The Telegram and
# Mastodon logos keep their colors, as on Android.
HELP = [
    "logo",
    "ic_question_mark",
    "ic_report_a_bug",
    "ic_news",
    "ic_telegram",
    "ic_github",
    "ic_website",
    "ic_matrix",
    "ic_mastodon",
    "ic_openstreetmap",
]
# The New Year promo of the help button keeps its colors.
HELP_PROMO = ["ic_christmas_tree"]
# Main menu entries. Android tints them at runtime, so they are made white (WHITE) for Silica to colorize.
MENU = [
    "ic_donate",
]
# Place editor icons that other pages reuse. The social networks only have white variants for the dark theme on
# Android.
EDITOR = [
    "ic_email",
]
SOCIAL = {
    "ic_facebook": "ic_facebook_white",
    "ic_instagram": "ic_instagram_white",
    "ic_twitterx": "ic_twitterx_white",
}
WHITE = (
    {
        "ic_donate",
    }
    | set(EDITOR)
    | set(HELP) - {"ic_telegram", "ic_mastodon"}
)
OUTPUTS = [
    ("layers", LAYERS),
    ("myposition", MY_POSITION),
    ("help", HELP + HELP_PROMO),
    ("menu", MENU),
    ("editor", EDITOR + list(SOCIAL)),
]


def main():
    for subdir, names in OUTPUTS:
        out_dir = os.path.join(sys.argv[1], subdir)
        os.makedirs(out_dir, exist_ok=True)
        for variant, suffix in (("drawable", ""), ("drawable-night", "_night")):
            for name in names:
                source = SOCIAL.get(name, name)
                paths = [os.path.join(res, variant, source + ".xml") for res in RES_DIRS]
                path = next((p for p in paths if os.path.exists(p)), None)
                # Every icon has a day variant.
                if not path and not suffix:
                    raise FileNotFoundError(source)
                if path:
                    try:
                        tree = to_svg(ET.parse(path).getroot(), "#ffffff" if name in WHITE else None)
                    except ValueError as e:
                        raise ValueError("%s: %s" % (path, e)) from e
                    tree.write(os.path.join(out_dir, name + suffix + ".svg"), encoding="unicode")


if __name__ == "__main__":
    main()
