#!/usr/bin/env python3

import argparse
import copy
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import List, Optional


SVG_NS = "http://www.w3.org/2000/svg"
SEARCH_RESULT_TRANSFORM = "translate(4.5 3.125) scale(1.125)"

GRAPHIC_TAGS = {
    "circle",
    "ellipse",
    "line",
    "path",
    "polygon",
    "polyline",
    "rect",
}


def local_name(tag: str) -> str:
    if tag.startswith("{"):
        return tag.rsplit("}", 1)[1]
    return tag


def normalize_color(value: Optional[str]) -> Optional[str]:
    if value is None:
        return None

    value = value.strip().lower()
    if value == "white":
        return "#fff"
    if value == "#ffffff":
        return "#fff"
    return value


def is_white(value: Optional[str]) -> bool:
    return normalize_color(value) == "#fff"


def is_zero_or_missing(value: Optional[str]) -> bool:
    return value is None or value in {"0", "0.0"}


def is_source_background_shape(element: ET.Element) -> bool:
    tag = local_name(element.tag)
    if tag == "circle":
        return (
            element.attrib.get("cx") == "12"
            and element.attrib.get("cy") == "12"
            and element.attrib.get("r") in {"11", "12"}
        )

    if tag == "rect":
        if (
            is_zero_or_missing(element.attrib.get("x"))
            and is_zero_or_missing(element.attrib.get("y"))
            and element.attrib.get("width") == "24"
            and element.attrib.get("height") == "24"
        ):
            return True

        return (
            element.attrib.get("x") == "1"
            and element.attrib.get("y") == "1"
            and element.attrib.get("width") == "22"
            and element.attrib.get("height") == "22"
        )

    return False


def clean_tag(tag: str) -> str:
    return local_name(tag)


def clean_attrib(attrib: dict[str, str]) -> dict[str, str]:
    result = {}
    for key, value in attrib.items():
        name = local_name(key)
        if name == "id":
            continue
        result[name] = value
    return result


def recolor_to_white(element: ET.Element) -> None:
    if "fill" in element.attrib and normalize_color(element.attrib["fill"]) != "none":
        element.attrib["fill"] = "#fff"
    if "stroke" in element.attrib and normalize_color(element.attrib["stroke"]) != "none":
        element.attrib["stroke"] = "#fff"


def clone_foreground(element: ET.Element, inherited: dict, accept_any_fill: bool = False) -> Optional[ET.Element]:
    tag = local_name(element.tag)
    current = dict(inherited)

    for attr in ("fill", "fill-rule", "stroke", "stroke-width", "stroke-linecap", "stroke-linejoin", "transform"):
        if attr in element.attrib:
            current[attr] = element.attrib[attr]

    if tag in GRAPHIC_TAGS:
        if is_source_background_shape(element):
            return None

        if is_white(current.get("fill")) or accept_any_fill:
            cloned = copy.deepcopy(element)
            cloned.tag = clean_tag(cloned.tag)
            cloned.attrib.clear()
            cloned.attrib.update(clean_attrib(element.attrib))
            for attr in ("fill", "fill-rule"):
                if attr in current and attr not in cloned.attrib:
                    cloned.attrib[attr] = current[attr]
            if accept_any_fill:
                recolor_to_white(cloned)
            return cloned

        return None

    children = []
    for child in element:
        cloned = clone_foreground(child, current, accept_any_fill)
        if cloned is not None:
            children.append(cloned)

    if not children:
        return None

    if tag == "svg":
        group = ET.Element("g")
    else:
        group = ET.Element(clean_tag(element.tag), clean_attrib(element.attrib))
        if accept_any_fill:
            recolor_to_white(group)

    for child in children:
        group.append(child)
    return group


def extract_foreground(source: Path, accept_any_fill: bool = False) -> List[ET.Element]:
    try:
        root = ET.parse(source).getroot()
    except ET.ParseError as e:
        raise ValueError(f"Cannot parse SVG: {e}") from e

    cloned = clone_foreground(root, {}, accept_any_fill)
    if cloned is None:
        return []

    if local_name(cloned.tag) == "g" and not cloned.attrib:
        return list(cloned)
    return [cloned]


def extract_search_result_foreground(source: Path) -> List[ET.Element]:
    foreground = extract_foreground(source)
    if foreground:
        return foreground

    return extract_foreground(source, accept_any_fill=True)


def indent(element: ET.Element, level: int = 0) -> None:
    children = list(element)
    if not children:
        return

    indent_text = "\n" + " " * (level + 1)
    element.text = indent_text
    for child in children:
        indent(child, level + 1)
        child.tail = indent_text
    children[-1].tail = "\n" + " " * level


def make_search_result_svg(foreground: List[ET.Element]) -> ET.Element:
    svg = ET.Element(
        "svg",
        {
            "width": "36",
            "height": "36",
            "version": "1.1",
            "viewBox": "0 0 36 36",
            "xml:space": "preserve",
            "xmlns": SVG_NS,
        },
    )

    defs = ET.SubElement(svg, "defs")
    filter_element = ET.SubElement(
        defs,
        "filter",
        {
            "id": "filter1",
            "x": "-.147",
            "y": "-.147",
            "width": "1.294",
            "height": "1.294",
            "color-interpolation-filters": "sRGB",
        },
    )
    ET.SubElement(filter_element, "feGaussianBlur", {"stdDeviation": "1.5925"})

    gradient = ET.SubElement(
        defs,
        "linearGradient",
        {
            "id": "linearGradient1",
            "x1": "18",
            "x2": "18",
            "y1": "3",
            "y2": "29",
            "gradientUnits": "userSpaceOnUse",
        },
    )
    ET.SubElement(gradient, "stop", {"stop-color": "#3fa8f3", "offset": "0"})
    ET.SubElement(gradient, "stop", {"stop-color": "#2089d4", "offset": "1"})

    ET.SubElement(svg, "circle", {"cx": "18", "cy": "18", "r": "13", "filter": "url(#filter1)", "opacity": ".35"})
    ET.SubElement(
        svg,
        "circle",
        {
            "cx": "18",
            "cy": "16",
            "r": "13",
            "fill": "url(#linearGradient1)",
            "stroke": "#fff",
            "stroke-width": "2",
        },
    )

    group = ET.SubElement(svg, "g", {"transform": SEARCH_RESULT_TRANSFORM})
    for element in foreground:
        group.append(element)

    indent(svg)
    return svg


def write_svg(svg: ET.Element, output: Path) -> None:
    data = ET.tostring(svg, encoding="unicode", short_empty_elements=True)
    output.write_text(f'<?xml version="1.0" encoding="UTF-8"?>\n{data}\n', encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate derived Organic Maps SVG symbols from regular symbol SVGs."
    )
    parser.add_argument("symbol", choices=["search_result"], help="Symbol type to generate.")
    parser.add_argument("input", type=Path, help="Input SVG, for example bank-m.svg.")
    parser.add_argument("output", type=Path, help="Output SVG, for example search-result-bank.svg.")
    parser.add_argument("--force", action="store_true", help="Overwrite output if it already exists.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = args.input
    output_path = args.output

    if not input_path.is_file():
        print(f"Input SVG does not exist: {input_path}", file=sys.stderr)
        return 1

    if not output_path.parent.is_dir():
        print(f"Output directory does not exist: {output_path.parent}", file=sys.stderr)
        return 1

    if output_path.exists() and not args.force:
        print(f"Output SVG already exists, use --force to overwrite: {output_path}", file=sys.stderr)
        return 1

    try:
        foreground = extract_search_result_foreground(input_path)
    except ValueError as e:
        print(e, file=sys.stderr)
        return 1

    if not foreground:
        print(f"Foreground white pictogram was not found in: {input_path}", file=sys.stderr)
        return 1

    if args.symbol == "search_result":
        write_svg(make_search_result_svg(foreground), output_path)
        return 0

    print(f"Unsupported symbol type: {args.symbol}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
