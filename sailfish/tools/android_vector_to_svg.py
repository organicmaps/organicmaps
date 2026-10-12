#!/usr/bin/env python3
"""Converts Android vector drawables into the SVG icons of the Sailfish app, run by the build:
  sailfish/tools/android_vector_to_svg.py <output dir>

Only the subset used by those icons is supported: groups with transforms, clip paths, paths with
fill/stroke colors and alphas, and layer lists of an oval shape under a vector.
"""

import os
import sys
import xml.etree.ElementTree as ET

A = "{http://schemas.android.com/apk/res/android}"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
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
# Donate and the social networks of the editor are reused from the menu and editor copies.
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
    | set(HELP)
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

# Android path attribute -> SVG attribute.
PATH_ATTRS = {
    "strokeWidth": "stroke-width",
    "strokeLineCap": "stroke-linecap",
    "strokeLineJoin": "stroke-linejoin",
}


def color(value):
    # Android colors are #RGB, #RRGGBB or #AARRGGBB; SVG takes the alpha separately.
    if value.startswith("@android:color/"):
        return {"white": "#ffffff", "black": "#000000", "holo_blue_light": "#33b5e5"}[value.split("/")[1]], None
    if len(value) == 9:
        return "#" + value[3:], int(value[1:3], 16) / 255
    return value, None


def group_transform(group):
    def num(name, default):
        return float(group.get(A + name, default))

    px, py = num("pivotX", 0), num("pivotY", 0)
    tx, ty = num("translateX", 0), num("translateY", 0)
    sx, sy, rotation = num("scaleX", 1), num("scaleY", 1), num("rotation", 0)
    if (sx, sy, rotation, tx, ty) == (1, 1, 0, 0, 0):
        return None
    # Android applies scale and rotation around the pivot, then the translation.
    return "translate(%g %g) rotate(%g) scale(%g %g) translate(%g %g)" % (tx + px, ty + py, rotation, sx, sy, -px, -py)


def convert(node, out, clip_ids):
    for child in node:
        tag = child.tag
        if tag == "group":
            group = ET.SubElement(out, "g")
            transform = group_transform(child)
            if transform:
                group.set("transform", transform)
            convert(child, group, clip_ids)
        elif tag == "clip-path":
            clip_id = "clip%d" % len(clip_ids)
            clip_ids.append(clip_id)
            clip = ET.SubElement(out, "clipPath", id=clip_id)
            ET.SubElement(clip, "path", d=child.get(A + "pathData"))
            # A clip path applies to the following siblings, so nest them into a clipped group.
            out = ET.SubElement(out, "g", {"clip-path": "url(#%s)" % clip_id})
        elif tag == "path":
            attrs = {"d": child.get(A + "pathData")}
            for kind in ("fill", "stroke"):
                value = child.get(A + kind + "Color")
                rgb, alpha = color(value) if value else ("none", None)
                attrs[kind] = rgb
                # Android multiplies the color's alpha by fillAlpha / strokeAlpha.
                alpha = (1 if alpha is None else alpha) * float(child.get(A + kind + "Alpha", 1))
                if alpha < 1:
                    attrs[kind + "-opacity"] = "%.3f" % alpha
            if child.get(A + "fillType") == "evenOdd":
                attrs["fill-rule"] = "evenodd"
            for android_name, svg_name in PATH_ATTRS.items():
                value = child.get(A + android_name)
                if value is not None:
                    attrs[svg_name] = value.lower()
            ET.SubElement(out, "path", attrs)


def make_white(svg):
    for element in svg.iter():
        for attr in ("fill", "stroke"):
            if element.get(attr) not in (None, "none"):
                element.set(attr, "#ffffff")


def to_svg(root):
    # A layer list stretches every item over the same bounds, so the vector viewport is the canvas.
    items = [item[0] for item in root] if root.tag == "layer-list" else [root]
    vector = next(item for item in items if item.tag == "vector")
    w, h = float(vector.get(A + "viewportWidth")), float(vector.get(A + "viewportHeight"))
    svg = ET.Element(
        "svg", xmlns="http://www.w3.org/2000/svg", width="%g" % w, height="%g" % h, viewBox="0 0 %g %g" % (w, h)
    )
    for item in items:
        if item.tag == "vector":
            convert(item, svg, [])
        elif item.tag == "shape" and item.get(A + "shape") == "oval":
            rgb, alpha = color(item.find("solid").get(A + "color"))
            circle = ET.SubElement(
                svg, "ellipse", cx="%g" % (w / 2), cy="%g" % (h / 2), rx="%g" % (w / 2), ry="%g" % (h / 2), fill=rgb
            )
            if alpha is not None and alpha < 1:
                circle.set("fill-opacity", "%.3f" % alpha)
        else:
            raise ValueError("Unsupported drawable item: " + item.tag)
    ET.indent(svg)
    return ET.ElementTree(svg)


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
                    tree = to_svg(ET.parse(path).getroot())
                    if name in WHITE:
                        make_white(tree.getroot())
                    tree.write(os.path.join(out_dir, name + suffix + ".svg"), encoding="unicode")


if __name__ == "__main__":
    main()
