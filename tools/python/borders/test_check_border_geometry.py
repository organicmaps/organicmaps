from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
from shapely.geometry import MultiPolygon, Polygon

from check_border_geometry import (Dataset, check, compare, cross_ring_contacts, rectangle_mask,
                                   point_contacts_fit, ring_contact_kinds)
from test_packed_borders import container, packed_fixture
from packed_borders import PackedBorders
from border_geometry import ring_coverage
from shapely.ops import unary_union
import border_comparison


def dataset(regions, packed=False):
    names, shapes, invalid, contacts, contact_kinds = [], [], [], {}, {}
    for name, points in regions.items():
        filename = name + ".poly"
        points = np.asarray(points, dtype=float)
        polygon = Polygon(points)
        names.append(filename)
        shapes.append(ring_coverage(points, polygon.is_valid))
        if not polygon.is_valid:
            invalid.append({"file": filename, "ring": "1"})
            contact_kinds[filename, "1"] = ring_contact_kinds(points)
            contacts[filename, "1"] = unary_union(list(contact_kinds[filename, "1"].values()))
    return Dataset(names, shapes, invalid, [item["file"] for item in invalid], {}, contacts,
                   contact_kinds=contact_kinds if packed else {})


class BorderGeometryCheckTests(unittest.TestCase):
    def test_valid_region_split_keeps_coverage_and_is_not_metadata_failure(self):
        old = dataset({"A": [(0, 0), (2, 0), (2, 1), (0, 1)]})
        new = dataset({"West": [(0, 0), (1, 0), (1, 1), (0, 1)],
                       "East": [(1, 0), (2, 0), (2, 1), (1, 1)]})
        report = compare(new, old, .01)
        self.assertFalse(report["regressions"])
        self.assertEqual(report["removed_names"], ["A.poly"])

    def test_new_gap_and_overlap_fail(self):
        a = [(0, 0), (1, 0), (1, 1), (0, 1)]
        old = dataset({"A": a, "B": [(1, 0), (2, 0), (2, 1), (1, 1)]})
        for x, key in [(1.001, "introduced_uncovered_parts"), (.999, "introduced_overlap_parts")]:
            new = dataset({"A": a, "B": [(x, 0), (2, 0), (2, 1), (x, 1)]})
            report = compare(new, old, .01)
            self.assertTrue(report["regressions"])
            self.assertEqual(len(report[key]), 1)

    def test_moving_an_inherited_crossing_does_not_increase_its_defect_count(self):
        bowtie = [(0, 0), (2, 2), (0, 2), (2, 0)]
        old = dataset({"A": bowtie}, packed=True)
        self.assertFalse(compare(old, old, .01)["regressions"])
        new = dataset({"A": [(0, 0), (2, 2), (0, 2), (2.001, 0)]}, packed=True)
        report = compare(new, old, .01)
        self.assertFalse(report["new_invalid_ring_contacts"])
        # Moving the crossing still cannot introduce an uncovered area.
        self.assertTrue(report["introduced_uncovered_parts"])

    def test_new_crossing_in_invalid_ring_fails_even_with_identical_parity_coverage(self):
        square = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0)]
        retrace = [(1, 0), (0, 0)]
        old = dataset({"A": square + retrace}, packed=True)
        # Traversing this bowtie twice cancels its filled area, but its edges
        # introduce a new proper crossing outside the inherited retrace.
        loop = [(-1, -1), (-3, -3), (-1, -3), (-3, -1), (-1, -1)]
        new = dataset({"A": square + retrace + loop + loop[1:] + [(0, 0)]}, packed=True)
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        report = compare(new, old, .01)
        self.assertTrue(report["new_invalid_ring_contacts"])
        self.assertEqual(report["introduced_uncovered_parts"], [])
        self.assertGreater(report["new_invalid_ring_contacts"][0]["increased_kinds"]["proper_crossings"]["current"], 0)

    def test_backtrack_growth_fails_despite_unchanged_coverage_and_component_count(self):
        square = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0)]
        for packed in (False, True):
            for direction in (1, -1):
                old = dataset({"A": square + [(direction, 0), (0, 0)]}, packed=packed)
                new = dataset({"A": square + [(direction * 100, 0), (0, 0)]}, packed=packed)
                self.assertTrue(old.shapes[0].equals(new.shapes[0]))
                self.assertTrue(compare(new, old, .01)["new_invalid_ring_contacts"])

    def test_distant_zero_area_loop_replacement_fails_same_crossing_count(self):
        square = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0)]

        def ring(offset):
            loop = [(-1 + offset, -1), (-3 + offset, -3), (-1 + offset, -3),
                    (-3 + offset, -1), (-1 + offset, -1)]
            return square + loop + loop[1:] + [(0, 0)]

        old = dataset({"A": ring(0)}, packed=True)
        new = dataset({"A": ring(-8)}, packed=True)
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        self.assertTrue(compare(new, old, .01)["new_invalid_ring_contacts"])
        # Even a small contact movement remains spatially strict in source data.
        old, new = dataset({"A": ring(0)}), dataset({"A": ring(-.001)})
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        self.assertTrue(compare(new, old, .01)["new_invalid_ring_contacts"])

    def test_inherited_point_matching_is_one_to_one_and_not_greedy(self):
        from shapely.geometry import MultiPoint
        old = MultiPoint([(0, 0), (.0035, 0)])
        self.assertTrue(point_contacts_fit(MultiPoint([(.001, 0), (-.0025, 0)]), old))
        old = MultiPoint([(0, 0), (10, 0)])
        self.assertFalse(point_contacts_fit(MultiPoint([(.001, 0), (.002, 0)]), old))

    def test_repairing_a_retraced_line_may_leave_inherited_point_contacts(self):
        square = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0)]
        old = dataset({"A": square + [(3, 0), (0, 0)]})
        new = dataset({"A": square + [(0, 1), (0, 0), (0, 2), (0, 0)]})
        # Exercise the endpoint inheritance explicitly, without changing the
        # coverage or adding a proper crossing to either ring.
        from shapely.geometry import LineString, MultiPoint, GeometryCollection
        key = "A.poly", "1"
        old.contact_kinds[key] = {"proper_crossings": GeometryCollection(),
                                  "point_touches": GeometryCollection(),
                                  "backtracks": LineString([(0, 0), (0, 3)])}
        new.contact_kinds[key] = {"proper_crossings": GeometryCollection(),
                                  "point_touches": MultiPoint([(0, 1), (0, 2)]),
                                  "backtracks": GeometryCollection()}
        old.contacts[key] = old.contact_kinds[key]["backtracks"]
        new.contacts[key] = new.contact_kinds[key]["point_touches"]
        self.assertFalse(compare(new, old, .01)["regressions"])

    def test_subdividing_inherited_retraced_contact_preserves_physical_defect(self):
        old = dataset({"A": [(0, 0), (3, 0), (3, 3), (0, 3), (0, 0), (3, 0), (0, 0)]})
        new = dataset({"A": [(0, 0), (3, 0), (3, 3), (0, 3), (0, 0), (1, 0), (3, 0), (0, 0)]})
        report = compare(new, old, .01)
        self.assertFalse(report["new_invalid_ring_contacts"])
        self.assertFalse(report["regressions"])

    def test_inherited_invalid_multi_outer_cannot_add_hidden_internal_overlap(self):
        left = [(0, 0), (2, 0), (2, 2), (0, 2)]
        old_right = [(2, 0), (4, 0), (4, 2), (2, 2)]
        new_right = [(1, 0), (4, 0), (4, 2), (1, 2)]

        def multiple(right):
            polygons = [Polygon(left), Polygon(right)]
            assembled = MultiPolygon(polygons)
            self.assertFalse(assembled.is_valid)
            contacts = {("A.poly", *pair): value for pair, value in cross_ring_contacts([
                (str(index), np.asarray(points), False, True) for index, points in enumerate((left, right))]).items()}
            return Dataset(["A.poly"], [polygons[0].union(polygons[1])], [], ["A.poly"], {}, {}, contacts)

        old, new = multiple(old_right), multiple(new_right)
        self.assertTrue(new.shapes[0].equals(old.shapes[0]))
        self.assertFalse(compare(old, old, .01)["regressions"])
        report = compare(new, old, .01)
        self.assertTrue(report["new_invalid_file_contacts"])
        self.assertTrue(report["regressions"])

    def test_old_physical_ambiguity_is_inherited_across_rename(self):
        regions = {"A": [(0, 0), (2, 0), (2, 1), (0, 1)], "B": [(1, 0), (3, 0), (3, 1), (1, 1)]}
        old = dataset(regions)
        new = dataset({"RenamedA": regions["A"], "RenamedB": regions["B"]})
        self.assertFalse(compare(new, old, .01)["regressions"])

    def test_wrapped_metadata_rect_retains_both_halves(self):
        mask = rectangle_mask((178, -2, 182, 2))
        from shapely.geometry import Point
        self.assertTrue(mask.covers(Point(-179, 0)))
        self.assertTrue(mask.covers(Point(179, 0)))
        self.assertFalse(mask.covers(Point(0, 0)))

    def test_cli_reports_new_metadata_clipping_and_exits_one(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            source = directory / "borders"
            source.mkdir()
            (source / "A.poly").write_text("A\n1\n0 0\n1 0\n1 1\n0 1\n0 0\nEND\nEND\n")
            regions = {"A": [(0, 0), (1, 0), (1, 1), (0, 1)]}
            old, new = directory / "old.bin", directory / "new.bin"
            old.write_bytes(packed_fixture(regions))
            new.write_bytes(packed_fixture(regions, {"A": (0, 0, .9, 1)}))
            report = directory / "report.json"
            result = subprocess.run([sys.executable, str(Path(__file__).with_name("check_border_geometry.py")),
                                     "--source", str(source), "--packed", str(new), "--baseline-source", str(source),
                                     "--baseline-packed", str(old), "--report", str(report)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn("packed_paths: introduced_uncovered_parts", result.stderr)
            self.assertIn("new_metadata_clipping", result.stderr)
            import json
            value = json.loads(report.read_text())
            self.assertEqual(len(value["new_metadata_clipping"]), 1)
            self.assertTrue(value["packed_paths"]["introduced_uncovered_parts"])

    def test_checker_ignores_missing_or_opaque_display_sections(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            source = directory / "borders"
            source.mkdir()
            (source / "A.poly").write_text("A\n1\n0 0\n1 0\n1 1\n0 1\n0 0\nEND\nEND\n")
            old, new = directory / "old.bin", directory / "new.bin"
            old.write_bytes(packed_fixture({"A": [(0, 0), (1, 0), (1, 1), (0, 1)]}))
            decoded = PackedBorders(old)
            for display in (None, b"\xffnot geometry"):
                sections = [(tag, decoded.data[offset:offset + size])
                            for tag, (offset, size) in decoded.sections.items() if not tag.startswith("t")]
                if display is not None:
                    sections.append(("t0", display))
                new.write_bytes(container(sections))
                report = check(source, new, source, old)
                self.assertFalse(report["regressions"])
                self.assertNotIn("packed_triangles", report)
                self.assertNotIn("triangles", report["current_packed_metadata"])
            result = subprocess.run([sys.executable, str(Path(__file__).with_name("check_border_geometry.py")),
                                     "--source", str(source), "--packed", str(new), "--baseline-source", str(source),
                                     "--baseline-packed", str(old), "--report", str(directory / "report.json")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            for message in ("Reading current source borders", "Reading baseline packed geometry",
                            "Comparing source geometry", "Comparing packed outer paths", "No border geometry regressions"):
                self.assertIn(message, result.stderr)
            self.assertNotIn("triangle", result.stderr.lower())

    def test_existing_two_owner_overlap_does_not_hide_a_new_third_owner(self):
        rectangle = [(0, 0), (2, 0), (2, 1), (0, 1)]
        old = dataset({"A": rectangle, "B": rectangle})
        new = dataset({"A": rectangle, "B": rectangle, "C": rectangle})
        report = compare(new, old, .01)
        self.assertTrue(report["regressions"])
        self.assertTrue(report["introduced_overlap_parts"])
        # Removing the third owner is an improvement, rather than a new gap.
        self.assertFalse(compare(old, new, .01)["regressions"])

    def test_established_overlap_skips_multiplicity_without_a_changed_third_owner(self):
        rectangle = [(0, 0), (2, 0), (2, 1), (0, 1)]
        old = dataset({"A": rectangle, "B": rectangle,
                       "Far": [(5, 0), (6, 0), (6, 1), (5, 1)]})
        new = dataset({"A": [(0, 0), (1.5, 0), (1.5, 1), (0, 1)], "B": rectangle,
                       "Far": [(5, 0), (7, 0), (7, 1), (5, 1)]})
        with patch.object(border_comparison, "introduced_ambiguity", wraps=border_comparison.introduced_ambiguity) as measure:
            self.assertFalse(compare(new, old, .01)["regressions"])
        measure.assert_not_called()

    def test_changed_third_owner_keeps_established_overlap_multiplicity_check(self):
        rectangle = [(0, 0), (2, 0), (2, 1), (0, 1)]
        old = dataset({"A": rectangle, "B": rectangle,
                       "C": [(3, 0), (4, 0), (4, 1), (3, 1)]})
        new = dataset({"A": [(0, 0), (1.5, 0), (1.5, 1), (0, 1)], "B": rectangle,
                       "C": [(1, 0), (4, 0), (4, 1), (1, 1)]})
        report = compare(new, old, .01)
        self.assertIn(["A.poly", "B.poly"], [item["owners"] for item in report["introduced_overlap_parts"]])

    def test_multiplicity_guard_matches_unguarded_randomized_comparison(self):
        random = np.random.default_rng(9217)

        def rectangle():
            x, y = random.integers(0, 5, size=2)
            width, height = random.integers(1, 4, size=2)
            return [(x, y), (x + width, y), (x + width, y + height), (x, y + height)]

        for case in range(48):
            previous = {name: rectangle() for name in ("A", "B", "C", "D")}
            regions = dict(previous)
            regions["A"] = rectangle()
            if case % 3 == 0:
                regions["C"] = rectangle()
            if case % 4 == 0:
                regions["Added"] = rectangle()
            if case % 5 == 0:
                regions["Renamed"] = regions.pop("D")
            if case % 7 == 0:
                regions.pop("B")
            old, new = dataset(previous), dataset(regions)
            optimized = compare(new, old, .01)
            with patch.object(border_comparison, "changed_third_owner", return_value=True):
                reference = compare(new, old, .01)
            with self.subTest(case=case):
                self.assertEqual(optimized, reference)

    def test_established_owner_pair_cannot_replace_another_inherited_overlap(self):
        left = [(0, 0), (1, 0), (1, 1), (0, 1)]
        right = [(1, 0), (2, 0), (2, 1), (1, 1)]
        old = dataset({"A": left, "B": left, "C": right})
        new = dataset({"A": left, "B": right, "C": left})
        report = compare(new, old, .01)
        self.assertTrue(report["regressions"])
        self.assertEqual([item["owners"] for item in report["introduced_overlap_parts"]], [["A.poly", "C.poly"]])
