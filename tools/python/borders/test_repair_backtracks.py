import unittest
from pathlib import Path
import tempfile

import numpy as np
from shapely import make_valid
from shapely.geometry import LineString, Point, Polygon

from maintain_borders import Border, Ring, project, read_poly
from repair_backtracks import areal_geometry, cancel_backtracks, repair_borders, write_retained_poly


def coordinates(points):
    return np.array(points, dtype=float)


def filled(points):
    polygon = Polygon(project(points))
    return areal_geometry(polygon if polygon.is_valid else make_valid(polygon))


class RepairBacktracksTests(unittest.TestCase):
    def check_cancellation(self, points):
        original = coordinates(points)
        repaired, operations = cancel_backtracks(original)
        self.assertTrue(operations)
        self.assertTrue(filled(original).equals(filled(repaired)))
        self.assertTrue(set(map(tuple, repaired)) <= set(map(tuple, original)))
        np.testing.assert_array_equal(repaired[0], repaired[-1])
        self.assertTrue(Polygon(project(repaired)).is_valid)
        return repaired, operations

    def test_complete_retrace_preserves_unrelated_duplicate_records(self):
        repaired, operations = self.check_cancellation([
            (0, 0), (0, 0), (2, 0), (2, 1), (3, 1), (3, 1), (2, 1), (2, 2), (0, 2), (0, 0),
        ])
        np.testing.assert_array_equal(repaired[:2], coordinates([(0, 0), (0, 0)]))
        self.assertNotIn((3, 1), map(tuple, repaired))
        self.assertEqual(operations[0]["removed_original_record_indices"], [4, 5])

    def test_horizontal_partial_retrace(self):
        repaired, operations = self.check_cancellation([
            (0, 1), (1, 0), (.5, 0), (2, 0), (2, 2), (0, 2), (0, 1),
        ])
        self.assertNotIn((.5, 0), map(tuple, repaired))
        self.assertEqual(operations[0]["type"], "axis-aligned partial retrace")

    def test_vertical_partial_retrace_at_high_latitude(self):
        repaired, _ = self.check_cancellation([(0, 50), (2, 60), (2, 55), (2, 80), (0, 80), (0, 50)])
        self.assertNotIn((2, 55), map(tuple, repaired))

    def test_complete_retrace_across_closing_vertex(self):
        repaired, operations = self.check_cancellation([
            (3, 1), (2, 1), (2, 2), (0, 2), (0, 0), (2, 0), (2, 1), (3, 1),
        ])
        self.assertNotIn((3, 1), map(tuple, repaired))
        self.assertEqual(operations[0]["removed_original_record_indices"], [0, 7])

    def test_partial_retrace_across_closing_vertex(self):
        repaired, _ = self.check_cancellation([
            (.5, 0), (2, 0), (2, 2), (0, 2), (0, 1), (1, 0), (.5, 0),
        ])
        self.assertNotIn((.5, 0), map(tuple, repaired))

    def test_sloped_lonlat_retrace_is_not_collinear_in_mercator(self):
        original = coordinates([(0, 60), (10, 80), (5, 70), (10, 84), (0, 84), (0, 60)])
        projected = project(original)
        self.assertGreater(LineString(projected[[0, 2]]).distance(Point(projected[1])), 0)
        repaired, operations = cancel_backtracks(original)
        self.assertEqual(operations, [])
        np.testing.assert_array_equal(repaired, original)

    def test_no_degenerate_ring_is_discarded(self):
        original = coordinates([(0, 0), (1, 0), (2, 0), (1, 0), (0, 0)])
        repaired, operations = cancel_backtracks(original)
        self.assertEqual(operations, [])
        np.testing.assert_array_equal(repaired, original)

    def test_reversed_neighbor_retraces_preserve_shared_boundary(self):
        a = coordinates([(0, 0), (1, 0), (1, .4), (1.1, .4), (1, .4), (1, 1), (0, 1), (0, 0)])
        b = coordinates([(1, 0), (2, 0), (2, 1), (1, 1), (1, .4), (1.1, .4), (1, .4), (1, 0)])
        borders = [Border("A.poly", "A title", [Ring("outer", a)]), Border("B.poly", "B title", [Ring("outer", b)])]
        output, report = repair_borders(borders)
        self.assertEqual(report["repaired_rings_now_valid"], 2)
        self.assertEqual(report["candidate_invalid_files"], [])
        after_a, after_b = filled(output["A.poly"][0]), filled(output["B.poly"][0])
        self.assertTrue(after_a.intersection(after_b).equals(LineString(project(coordinates([(1, 0), (1, 1)])))))
        self.assertTrue(filled(a).intersection(filled(b)).equals(after_a.intersection(after_b)))

    def test_valid_ring_and_hole_metadata_are_preserved(self):
        outer = coordinates([(0, 0), (0, 0), (3, 0), (3, 3), (0, 3), (0, 0)])
        hole = coordinates([(1, 1), (2, 1), (1, 2), (1, 1)])
        border = Border("A.poly", "A title", [Ring("outer name", outer), Ring("!hole name", hole)])
        output, report = repair_borders([border])
        self.assertEqual(report["changed_files"], [])
        self.assertEqual([ring.name for ring in border.rings], ["outer name", "!hole name"])
        np.testing.assert_array_equal(output[border.name][0], outer)
        np.testing.assert_array_equal(output[border.name][1], hole)

    def test_proper_crossing_is_left_for_separate_source_verification(self):
        original = coordinates([(0, 0), (2, 2), (2, 0), (0, 2), (0, 1), (-1, 1), (0, 1), (0, 0)])
        border = Border("A.poly", "A", [Ring("outer", original)])
        output, report = repair_borders([border])
        self.assertEqual(report["repaired_rings_now_valid"], 0)
        self.assertEqual(len(report["candidate_invalid_rings"]), 1)
        self.assertTrue(filled(original).equals(filled(output[border.name][0])))

    def test_writer_retains_coordinate_text_metadata_and_line_endings(self):
        original = (
            b"A title  \r\n\r\n  outer name \r\n"
            b" -0.0E+0\t+0.000\r\n\t2E0 0E0\r\n\t2.0 1e0\r\n"
            b" 3.000\t+1.000\r\n\t2.000 1.0\r\n 2.0 +2e0\r\n\t0E+0 2.0\r\n"
            b" -0.000E+00 0.0\r\n\r\nEND\r\n\r\n !hole name \r\n"
            b" .25 .25\r\n .5 .25\r\n .25 .5\r\n .25 .25\r\nEND\r\nEND"
        )
        with tempfile.TemporaryDirectory() as directory:
            source, output = Path(directory) / "A.poly", Path(directory) / "B.poly"
            source.write_bytes(original)
            border = read_poly(source)
            repaired, _ = cancel_backtracks(border.rings[0].points)
            write_retained_poly(output, border, [repaired, border.rings[1].points])
            self.assertEqual(output.read_bytes(), original.replace(b" 3.000\t+1.000\r\n", b""))
            parsed = read_poly(output)
            np.testing.assert_array_equal(parsed.rings[0].points, repaired)
            np.testing.assert_array_equal(parsed.rings[1].points, border.rings[1].points)

    def test_writer_copies_existing_text_for_new_closing_record(self):
        original = (
            b"A\n1\n .5 0\n\t2E0 0.000\n 2 2\n 0 2\n 0 1\n 1 0\n .5 0\nEND\nEND\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            source, output = Path(directory) / "A.poly", Path(directory) / "B.poly"
            source.write_bytes(original)
            border = read_poly(source)
            repaired, _ = cancel_backtracks(border.rings[0].points)
            write_retained_poly(output, border, [repaired])
            expected = b"A\n1\n\t2E0 0.000\n 2 2\n 0 2\n 0 1\n 1 0\n\t2E0 0.000\nEND\nEND\n"
            self.assertEqual(output.read_bytes(), expected)
            np.testing.assert_array_equal(read_poly(output).rings[0].points, repaired)


if __name__ == "__main__":
    unittest.main()
