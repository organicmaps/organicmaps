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
}


def color(value):
    # Android colors are #RGB, #RRGGBB or #AARRGGBB; SVG takes the alpha separately.
    if value == "@android:color/transparent":
        return "#000000", 0
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


def gradient(attr, defs):
    # Adds the gradient of an aapt:attr to defs, and returns the reference to it.
    (child,) = attr
    if child.tag != "gradient" or child.get(A + "type") != "linear":
        raise ValueError("Unsupported gradient: " + ET.tostring(child, encoding="unicode"))
    gradient_id = "gradient%d" % len(defs)
    coords = {
        svg: child.get(A + name) for svg, name in zip(("x1", "y1", "x2", "y2"), ("startX", "startY", "endX", "endY"))
    }
    element = ET.SubElement(defs, "linearGradient", id=gradient_id, gradientUnits="userSpaceOnUse", **coords)
    for item in child:
        rgb, alpha = color(item.get(A + "color"))
        stop = ET.SubElement(element, "stop", {"offset": item.get(A + "offset"), "stop-color": rgb})
        if alpha is not None and alpha < 1:
            stop.set("stop-opacity", "%.3f" % alpha)
    return "url(#%s)" % gradient_id


def convert(node, out, clip_ids, defs):
    for child in node:
        tag = child.tag
        if tag == "group":
            group = ET.SubElement(out, "g")
            transform = group_transform(child)
            if transform:
                group.set("transform", transform)
            convert(child, group, clip_ids, defs)
        elif tag == "clip-path":
            clip_id = "clip%d" % len(clip_ids)
            clip_ids.append(clip_id)
            clip = ET.SubElement(out, "clipPath", id=clip_id)
            ET.SubElement(clip, "path", d=child.get(A + "pathData"))
            # A clip path applies to the following siblings, so nest them into a clipped group.
            out = ET.SubElement(out, "g", {"clip-path": "url(#%s)" % clip_id})
        elif tag == "path":
            attrs = {"d": child.get(A + "pathData")}
            gradients = {}
            for attr in child:
                if attr.tag != AAPT_ATTR or attr.get("name") not in ("android:fillColor", "android:strokeColor"):
                    raise ValueError("Unsupported path child: " + attr.tag)
                gradients[attr.get("name").removeprefix("android:")] = gradient(attr, defs)
            for kind in ("fill", "stroke"):
                value = child.get(A + kind + "Color")
                if kind + "Color" in gradients:
                    rgb, alpha = gradients[kind + "Color"], None
                else:
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
            if attrs["fill"] == "none" and attrs["stroke"] == "none":
                raise ValueError("Path without a fill or stroke")
            ET.SubElement(out, "path", attrs)
        else:
            raise ValueError("Unsupported vector element: " + tag)


def recolor(svg, rgb):
    for element in svg.iter():
        for attr in ("fill", "stroke"):
            value = element.get(attr)
            if value is not None and value.startswith("url("):
                raise ValueError("A gradient can't be recolored")
            if value not in (None, "none"):
                element.set(attr, rgb)


def to_svg(root, recolor_to=None):
    # A layer list stretches every item over the same bounds, so the vector viewport is the canvas.
    items = [item[0] for item in root] if root.tag == "layer-list" else [root]
    vector = next(item for item in items if item.tag == "vector")
    w, h = float(vector.get(A + "viewportWidth")), float(vector.get(A + "viewportHeight"))
    svg = ET.Element(
        "svg", xmlns="http://www.w3.org/2000/svg", width="%g" % w, height="%g" % h, viewBox="0 0 %g %g" % (w, h)
    )
    defs = ET.SubElement(svg, "defs")
    for item in items:
        if item.tag == "vector":
            content = svg
            if float(item.get(A + "alpha", 1)) < 1:
                content = ET.SubElement(svg, "g", opacity=item.get(A + "alpha"))
            convert(item, content, [], defs)
        elif item.tag == "shape" and item.get(A + "shape") == "oval":
            rgb, alpha = color(item.find("solid").get(A + "color"))
            circle = ET.SubElement(
                svg, "ellipse", cx="%g" % (w / 2), cy="%g" % (h / 2), rx="%g" % (w / 2), ry="%g" % (h / 2), fill=rgb
            )
            if alpha is not None and alpha < 1:
                circle.set("fill-opacity", "%.3f" % alpha)
        else:
            raise ValueError("Unsupported drawable item: " + item.tag)
    if len(defs) == 0:
        svg.remove(defs)
    # A tint replaces the colors; a theme one needs recolor_to.
    tint = vector.get(A + "tint")
    if recolor_to:
        recolor(svg, recolor_to)
    elif tint is not None:
        rgb, alpha = color(tint) if tint.startswith(("#", "@android:color/")) else (None, None)
        if rgb is None or alpha is not None:
            print("Unsupported tint, the colors are kept: " + tint, file=sys.stderr)
        else:
            recolor(svg, rgb)
    ET.indent(svg)
    return ET.ElementTree(svg)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Converts Android XML icons into SVG and prints it")
        print(f"Usage: {sys.argv[0]} <Android vector drawable xml icon>", file=sys.stderr)
        exit(1)
    print(ET.tostring(to_svg(ET.parse(sys.argv[1]).getroot()).getroot(), encoding="unicode"))
