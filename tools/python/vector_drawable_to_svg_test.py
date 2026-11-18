#!/usr/bin/env python3
"""Unit tests for the Android vector drawable to SVG converter."""

import unittest
import xml.etree.ElementTree as ET

from vector_drawable_to_svg import to_svg

NS = 'xmlns:android="http://schemas.android.com/apk/res/android" xmlns:aapt="http://schemas.android.com/aapt"'


def vector(body, attrs=""):
    return f'<vector {NS} android:viewportWidth="24" android:viewportHeight="24" {attrs}>{body}</vector>'


def convert(xml, recolor_to=None):
    return to_svg(ET.fromstring(xml), recolor_to).getroot()


def find(svg, tag):
    return svg.findall(".//{*}" + tag)


class ConverterTest(unittest.TestCase):
    def test_clipped_siblings_and_gradient(self):
        group = """<group android:translateX="{x}"><clip-path android:pathData="M0,0h{x}v24h-{x}z"/>
            <path android:pathData="M0,0h24v24h-24z" android:fillColor="{color}"/></group>"""
        gradient = """<path android:pathData="M0,0h24v24h-24z"><aapt:attr name="android:fillColor">
            <gradient android:endX="24" android:tileMode="mirror">
            <item android:offset="0" android:color="#80FF0000"/><item android:offset="1" android:color="#0000FF"/>
            </gradient></aapt:attr></path>"""
        # The clipped siblings are in two vectors, whose clip paths share the IDs of one SVG.
        first, second = vector(group.format(x=12, color="#FF0000") + gradient), vector(
            group.format(x=6, color="#0000FF")
        )
        svg = convert(f"<layer-list {NS}><item>{first}</item><item>{second}</item></layer-list>")
        clips = {clip.get("id"): clip.find("{*}path").get("d") for clip in find(svg, "clipPath")}
        groups = [g for g in find(svg, "g") if g.get("clip-path")]
        self.assertEqual(len(clips), 2)
        for g, d, fill in zip(groups, ("M0,0h12v24h-12z", "M0,0h6v24h-6z"), ("#FF0000", "#0000FF")):
            self.assertEqual(clips[g.get("clip-path")[5:-1]], d)
            self.assertEqual(g.find("{*}path").get("fill"), fill)
        (linear,) = find(svg, "linearGradient")
        coords = {name: linear.get(name) for name in ("x1", "y1", "x2", "y2", "spreadMethod")}
        self.assertEqual(coords, {"x1": "0", "y1": "0", "x2": "24", "y2": "0", "spreadMethod": "reflect"})
        stops = [(s.get("offset"), s.get("stop-color"), s.get("stop-opacity")) for s in linear]
        self.assertEqual(stops, [("0", "#FF0000", "0.502"), ("1", "#0000FF", None)])

    def test_gradient_attribute_colors(self):
        svg = convert(vector("""<path android:pathData="M0,0h24v24h-24z"><aapt:attr name="android:fillColor">
            <gradient android:endY="24" android:startColor="#F00" android:centerColor="#0F0" android:endColor="#00F"
            android:tileMode="disabled"/>
            </aapt:attr></path>"""))
        stops = [(s.get("offset"), s.get("stop-color")) for s in find(svg, "stop")]
        self.assertEqual(stops, [("0", "#F00"), ("0.5", "#0F0"), ("1", "#00F")])
        self.assertIsNone(find(svg, "linearGradient")[0].get("spreadMethod"))
        # The start and end colors default to transparent, and the tint applies to the stops.
        svg = convert(
            vector(
                """<path android:pathData="M0,0h24v24h-24z"><aapt:attr name="android:fillColor">
            <gradient android:endY="24" android:centerColor="#F00"/></aapt:attr></path>""",
                'android:tint="#00F"',
            )
        )
        stops = [(s.get("offset"), s.get("stop-color"), s.get("stop-opacity")) for s in find(svg, "stop")]
        self.assertEqual(stops, [("0", "#00F", "0.000"), ("0.5", "#00F", None), ("1", "#00F", "0.000")])

    def test_colors(self):
        svg = convert(vector('<path android:pathData="M0,0h24v24z" android:fillColor="#8F00"/>'))
        (path,) = find(svg, "path")
        self.assertEqual((path.get("fill"), path.get("fill-opacity")), ("#FF0000", "0.533"))
        svg = convert(vector('<path android:pathData="M0,0h24v24z" android:fillColor="@android:color/transparent"/>'))
        (path,) = find(svg, "path")
        self.assertEqual((path.get("fill"), path.get("fill-opacity")), ("#000000", "0.000"))
        with self.assertRaisesRegex(ValueError, "@color/accent"):
            convert(vector('<path android:pathData="M0,0h24v24z" android:fillColor="@color/accent"/>'))
        with self.assertRaisesRegex(ValueError, "Unsupported drawable: selector"):
            convert(f"<selector {NS}/>")

    def test_paths(self):
        svg = convert(vector("""<path android:pathData="M0,0h24"/>
            <path android:pathData="M0,0L24,24L0,24" android:strokeColor="#000" android:strokeWidth="2"
                android:strokeMiterLimit="1"/>
            <path android:pathData="M0,0h24" android:strokeColor="#00000000"/>"""))
        unpainted, mitered, invisible = find(svg, "path")
        self.assertEqual((unpainted.get("fill"), unpainted.get("stroke")), ("none", "none"))
        self.assertEqual(mitered.get("stroke-miterlimit"), "1")
        self.assertEqual(invisible.get("stroke-opacity"), "0.000")
        for path in (
            '<path android:pathData="M0,0h24" android:strokeColor="#000" android:strokeWidth="2" '
            'android:trimPathEnd="0.5"/>',
            '<path android:pathData="M0,0h24" android:strokeColor="#000"/>',
        ):
            with self.assertRaises(ValueError):
                convert(vector(path))

    def test_layers(self):
        oval = '<shape android:shape="oval"><solid android:color="#00FF00"/></shape>'
        tinted = vector(
            '<path android:pathData="M0,0h24v24z" android:fillColor="#80000000"/>', 'android:tint="#FFFFFF00"'
        )
        svg = convert(f"<layer-list {NS}><item>{oval}</item><item>{tinted}</item></layer-list>")
        (ellipse,) = find(svg, "ellipse")
        (path,) = find(svg, "path")
        self.assertEqual((ellipse.get("fill"), path.get("fill")), ("#00FF00", "#FFFF00"))
        self.assertEqual(path.get("fill-opacity"), "0.502")
        recolored = convert(f"<layer-list {NS}><item>{oval}</item><item>{tinted}</item></layer-list>", "#ffffff")
        self.assertEqual([e.get("fill") for e in find(recolored, "ellipse") + find(recolored, "path")], ["#ffffff"] * 2)
        for layers in (
            f'<item android:gravity="center">{oval}</item><item>{tinted}</item>',
            f'<item android:drawable="@drawable/ic_oval"/><item>{tinted}</item>',
            f"<item/><item>{tinted}</item>",
        ):
            with self.assertRaisesRegex(ValueError, "Unsupported layer: "):
                convert(f"<layer-list {NS}>{layers}</layer-list>")
        with self.assertRaisesRegex(ValueError, "2 vector viewports"):
            convert(f"<layer-list {NS}><item>{tinted}</item><item>{vector('').replace('24', '48')}</item></layer-list>")
        for shape in (
            '<shape android:shape="oval"><solid android:color="#0F0"/><stroke android:width="1"/></shape>',
            '<shape><solid android:color="#0F0"/></shape>',
        ):
            with self.assertRaisesRegex(ValueError, "Unsupported drawable item"):
                convert(f"<layer-list {NS}><item>{shape}</item><item>{tinted}</item></layer-list>")
        for tint in (
            'android:tint="#FFF" android:tintMode="multiply"',
            'android:tint="#80FFFFFF"',
            'android:tint="?attr/colorAccent"',
        ):
            with self.assertRaises(ValueError):
                convert(vector("", tint))


if __name__ == "__main__":
    unittest.main()
