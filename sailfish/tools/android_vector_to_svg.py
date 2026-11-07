#!/usr/bin/env python3
"""Converts Android vector drawables into the SVG icons of the Sailfish app, run by the build:
  sailfish/tools/android_vector_to_svg.py <output dir>

Only the subset used by those icons is supported: groups with transforms, clip paths, paths with
fill/stroke colors, linear gradients and alphas, vector alphas and color tints, and layer lists of an oval
shape under a vector. Anything else fails the conversion rather than ship a broken icon.
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
    "ic_layers_isoline",
    "ic_layers_hiking",
    "ic_layers_cycling",
    "ic_layers_subway",
    "ic_layers_satellite",
]
# Keys of search::DisplayedCategories.
CATEGORIES = [
    "ic_category_" + key
    for key in (
        "eat",
        "hotel",
        "food",
        "tourism",
        "wifi",
        "transport",
        "fuel",
        "parking",
        "shopping",
        "secondhand",
        "atm",
        "nightlife",
        "children",
        "bank",
        "entertainment",
        "water",
        "hospital",
        "pharmacy",
        "recycling",
        "rv",
        "police",
        "toilet",
        "post",
    )
]
# Place page row icons missing from the Silica theme.
# Cuisine, drive-through and outdoor seating use the editor copies.
PLACE_PAGE = [
    "ic_wheelchair_white",
    "ic_capacity_white",
    "ic_category_bus",
    "ic_category_tram",
    "ic_network_white",
]
# Bookmark list visibility toggles.
BOOKMARKS = ["ic_show", "ic_hide"]
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
# Main menu entries and the recording status button. Android tints the single color ones at runtime, so
# they are made white (WHITE) for Silica to colorize; ic_track_recording_on keeps its two colors.
MENU = [
    "ic_donate",
    "ic_track_recording_off",
    "ic_track_recording_on",
    "ic_track_recording_status",
]
# Route panel and place page routing icons.
ROUTING = [
    "ic_ruler_route",
    "ic_location_arrow_blue",
    "ic_20px_route_planning_tram",
    "ic_20px_route_planning_bus",
] + ["route_point_%02d" % i for i in range(1, 10)]
# Navigation: roundabout turn arrows; the other turn arrows are bitmaps on Android too.
NAVIGATION = ["ic_turn_round"] + ["ic_roundabout_exit_%d" % i for i in range(1, 13)]
# Place editor fields, by the names used in sailfish/place_editor.cpp. The social networks only have white
# variants for the dark theme on Android.
EDITOR = [
    "ic_address",
    "ic_building",
    "ic_cuisine",
    "ic_drive_through_white",
    "ic_email",
    "ic_floor",
    "ic_level_white",
    "ic_operating_hours",
    "ic_operator",
    "ic_outdoor_seating",
    "ic_phone",
    "ic_self_service",
    "ic_street_address",
    "ic_website",
    "ic_website_menu",
    "ic_wifi",
]
SOCIAL = {
    "ic_facebook": "ic_facebook_white",
    "ic_instagram": "ic_instagram_white",
    "ic_line": "ic_line_white",
    "ic_twitterx": "ic_twitterx_white",
    "ic_vk": "ic_vk_white",
}
WHITE = (
    {
        "ic_ruler_route",
        "ic_donate",
        "ic_track_recording_off",
        "ic_track_recording_status",
    }
    | set(EDITOR)
    | set(HELP) - {"ic_telegram", "ic_mastodon"}
)
OUTPUTS = [
    ("layers", LAYERS),
    ("categories", CATEGORIES),
    ("placepage", PLACE_PAGE),
    ("bookmarks", BOOKMARKS),
    ("myposition", MY_POSITION),
    ("help", HELP + HELP_PROMO),
    ("menu", MENU),
    ("routing", ROUTING),
    ("navigation", NAVIGATION),
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
