#!/usr/bin/env python3

import sys
import xml.etree.ElementTree as ET

A = "{http://schemas.android.com/apk/res/android}"
AAPT_ATTR = "{http://schemas.android.com/aapt}attr"
# Android path attribute -> SVG attribute.
PATH_ATTRS = {
    "strokeWidth": "stroke-width",
    "strokeLineCap": "stroke-linecap",
    "strokeLineJoin": "stroke-linejoin",
    "strokeMiterLimit": "stroke-miterlimit",
}
ANDROID_COLORS = {"transparent": "#00000000", "white": "#ffffff", "black": "#000000", "holo_blue_light": "#33b5e5"}
# Android gradient tileMode -> SVG spreadMethod.
SPREAD_METHODS = {"clamp": "pad", "disabled": "pad", "repeat": "repeat", "mirror": "reflect"}


def color(value):
    # Android colors are #RGB, #ARGB, #RRGGBB or #AARRGGBB; SVG takes the alpha separately.
    if value.startswith("@android:color/"):
        value = ANDROID_COLORS.get(value.removeprefix("@android:color/"), value)
    if not value.startswith("#") or len(value) not in (4, 5, 7, 9):
        raise ValueError("Unsupported color: " + value)
    if len(value) == 5:
        value = "#" + "".join(c * 2 for c in value[1:])
    if len(value) == 9:
        return "#" + value[3:], int(value[1:3], 16) / 255
    return value, 1


def paint(value, tint):
    # The opaque src_in tint of a vector replaces the color and keeps its alpha.
    rgb, alpha = color(value)
    return tint or rgb, alpha


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


def gradient(attr, defs, tint):
    # Adds the gradient of an aapt:attr to defs, and returns the reference to it.
    (child,) = attr
    tile_mode = child.get(A + "tileMode", "clamp")
    if child.tag != "gradient" or child.get(A + "type", "linear") != "linear" or tile_mode not in SPREAD_METHODS:
        raise ValueError("Unsupported gradient: " + ET.tostring(child, encoding="unicode"))
    gradient_id = "gradient%d" % len(defs)
    coords = {
        svg: child.get(A + name, "0")
        for svg, name in zip(("x1", "y1", "x2", "y2"), ("startX", "startY", "endX", "endY"))
    }
    element = ET.SubElement(defs, "linearGradient", id=gradient_id, gradientUnits="userSpaceOnUse", **coords)
    if SPREAD_METHODS[tile_mode] != "pad":
        element.set("spreadMethod", SPREAD_METHODS[tile_mode])
    stops = [(item.get(A + "offset"), item.get(A + "color")) for item in child]
    if not stops:
        # Android defaults the start and end colors to transparent.
        stops = [("0", child.get(A + "startColor", "#00000000"))]
        if child.get(A + "centerColor") is not None:
            stops.append(("0.5", child.get(A + "centerColor")))
        stops.append(("1", child.get(A + "endColor", "#00000000")))
    for offset, value in stops:
        rgb, alpha = paint(value, tint)
        stop = ET.SubElement(element, "stop", {"offset": offset, "stop-color": rgb})
        if alpha < 1:
            stop.set("stop-opacity", "%.3f" % alpha)
    return "url(#%s)" % gradient_id


def convert(node, out, defs, tint):
    for child in node:
        tag = child.tag
        if tag == "group":
            group = ET.SubElement(out, "g")
            transform = group_transform(child)
            if transform:
                group.set("transform", transform)
            convert(child, group, defs, tint)
        elif tag == "clip-path":
            clip_id = "clip%d" % len(defs)
            ET.SubElement(ET.SubElement(defs, "clipPath", id=clip_id), "path", d=child.get(A + "pathData"))
            # A clip path applies to the following siblings, so nest them into a clipped group.
            out = ET.SubElement(out, "g", {"clip-path": "url(#%s)" % clip_id})
        elif tag == "path":
            if float(child.get(A + "trimPathStart", 0)) != 0 or float(child.get(A + "trimPathEnd", 1)) != 1:
                raise ValueError("Unsupported trimmed path")
            attrs = {"d": child.get(A + "pathData")}
            gradients = {}
            for attr in child:
                if attr.tag != AAPT_ATTR or attr.get("name") not in ("android:fillColor", "android:strokeColor"):
                    raise ValueError("Unsupported path child: " + attr.tag)
                gradients[attr.get("name").removeprefix("android:")] = gradient(attr, defs, tint)
            for kind in ("fill", "stroke"):
                value = child.get(A + kind + "Color")
                if kind + "Color" in gradients:
                    rgb, alpha = gradients[kind + "Color"], 1
                else:
                    rgb, alpha = paint(value, tint) if value else ("none", 1)
                attrs[kind] = rgb
                # Android multiplies the color's alpha by fillAlpha / strokeAlpha.
                alpha *= float(child.get(A + kind + "Alpha", 1))
                if alpha < 1:
                    attrs[kind + "-opacity"] = "%.3f" % alpha
                # Android draws a stroke without a width as a hairline of one device pixel.
                if kind == "stroke" and rgb != "none" and alpha > 0 and float(child.get(A + "strokeWidth", 0)) == 0:
                    raise ValueError("Unsupported hairline stroke")
            if child.get(A + "fillType") == "evenOdd":
                attrs["fill-rule"] = "evenodd"
            for android_name, svg_name in PATH_ATTRS.items():
                value = child.get(A + android_name)
                if value is not None:
                    attrs[svg_name] = value.lower()
            ET.SubElement(out, "path", attrs)
        else:
            raise ValueError("Unsupported vector element: " + tag)


def vector_tint(vector):
    tint = vector.get(A + "tint")
    if tint is None:
        return None
    mode = vector.get(A + "tintMode", "src_in")
    rgb, alpha = color(tint)
    if alpha < 1 or mode != "src_in":
        raise ValueError("Unsupported tint: %s with %s" % (tint, mode))
    return rgb


def recolor(svg, rgb):
    for element in svg.iter():
        for attr in ("fill", "stroke"):
            value = element.get(attr)
            if value is not None and value.startswith("url("):
                raise ValueError("A gradient can't be recolored")
            if value not in (None, "none"):
                element.set(attr, rgb)


def is_solid_oval(item):
    return (
        item.tag == "shape"
        and item.get(A + "shape") == "oval"
        and item.find("solid") is not None
        and {child.tag for child in item} <= {"solid", "size"}
    )


def to_svg(root, recolor_to=None):
    """Converts a vector drawable, or a layer list of vectors and solid ovals, into an SVG tree.

    Layer items must be inline and unpositioned, so that Android stretches each of them over the whole drawable, and
    the vectors must share their viewport, which becomes the size of the SVG. The opaque tint of a vector applies to its
    colors. recolor_to (e.g. "#ffffff") replaces every color of the icon instead.
    Raises ValueError for anything else.
    """
    if root.tag == "layer-list":
        items = []
        for item in root:
            if item.attrib or len(item) != 1:
                raise ValueError("Unsupported layer: only an unpositioned item with an inline drawable is supported")
            items.append(item[0])
    elif root.tag == "vector":
        items = [root]
    else:
        raise ValueError("Unsupported drawable: " + root.tag)
    viewports = {
        (float(item.get(A + "viewportWidth")), float(item.get(A + "viewportHeight")))
        for item in items
        if item.tag == "vector"
    }
    if len(viewports) != 1:
        raise ValueError("Unsupported layers: %d vector viewports" % len(viewports))
    ((w, h),) = viewports
    svg = ET.Element(
        "svg", xmlns="http://www.w3.org/2000/svg", width="%g" % w, height="%g" % h, viewBox="0 0 %g %g" % (w, h)
    )
    defs = ET.SubElement(svg, "defs")
    for item in items:
        if item.tag == "vector":
            content = svg
            if float(item.get(A + "alpha", 1)) < 1:
                content = ET.SubElement(svg, "g", opacity=item.get(A + "alpha"))
            convert(item, content, defs, None if recolor_to else vector_tint(item))
        elif is_solid_oval(item):
            rgb, alpha = color(item.find("solid").get(A + "color"))
            circle = ET.SubElement(
                svg, "ellipse", cx="%g" % (w / 2), cy="%g" % (h / 2), rx="%g" % (w / 2), ry="%g" % (h / 2), fill=rgb
            )
            if alpha < 1:
                circle.set("fill-opacity", "%.3f" % alpha)
        else:
            kind = item.get(A + "shape", "rectangle") + " shape" if item.tag == "shape" else item.tag
            raise ValueError("Unsupported drawable item: " + kind)
    if len(defs) == 0:
        svg.remove(defs)
    if recolor_to:
        recolor(svg, recolor_to)
    ET.indent(svg)
    return ET.ElementTree(svg)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Converts Android XML icons into SVG and prints it")
        print(f"Usage: {sys.argv[0]} <Android vector drawable xml icon>", file=sys.stderr)
        exit(1)
    print(ET.tostring(to_svg(ET.parse(sys.argv[1]).getroot()).getroot(), encoding="unicode"))
