import tempfile
from pathlib import Path
import unittest

import numpy as np
from shapely.geometry import Polygon

import maintain_borders as borders
import reconcile_borders


def border(name, points):
    return borders.Border(name + ".poly", name, [borders.Ring("1", np.array(points, dtype=float))])


def edges(points):
    if np.array_equal(points[0], points[-1]):
        points = points[:-1]
    return {
        tuple(sorted((tuple(first), tuple(last))))
        for first, last in zip(points, np.roll(points, -1, axis=0))
    }


class MaintainBordersTest(unittest.TestCase):
    def test_validity_uses_straight_mercator_segments_at_high_latitude(self):
        planar_valid = [(0, 0), (10, 80), (0, 80), (5, 50), (0, 0)]
        mercator_valid = [(0, 80), (10, 0), (0, 0), (5, 50), (0, 80)]
        self.assertTrue(Polygon(planar_valid).is_valid)
        self.assertFalse(Polygon(borders.project(np.array(planar_valid, dtype=float))).is_valid)
        self.assertFalse(Polygon(mercator_valid).is_valid)
        self.assertTrue(Polygon(borders.project(np.array(mercator_valid, dtype=float))).is_valid)
        a, b = border("A", planar_valid), border("B", mercator_valid)
        invalid_rings, invalid_files = borders.prepare_validity([a, b])
        self.assertEqual([entry["file"] for entry in invalid_rings], ["A.poly"])
        self.assertEqual(invalid_files, ["A.poly"])
        self.assertFalse(a.rings[0].valid)
        self.assertTrue(b.rings[0].valid)

    def test_hole_ownership_uses_projected_outer_segments(self):
        outer = np.array([(0, 0), (10, 80), (0, 80), (0, 0)], dtype=float)
        hole = np.array([(4.9, 49.9), (5.1, 49.9), (5, 50.1), (4.9, 49.9)], dtype=float)
        self.assertTrue(Polygon(outer).covers(Polygon(hole).representative_point()))
        a = borders.Border("A.poly", "A", [borders.Ring("outer", outer), borders.Ring("!hole", hole)])
        _, invalid_files = borders.prepare_validity([a])
        self.assertEqual(invalid_files, ["A.poly"])
        self.assertFalse(a.hole_owners)

    def test_reversed_shared_arcs(self):
        a = border("A", [(0, 0), (1, 0), (1.001, .2), (1, .4), (1.001, .6), (1, 1), (0, 1), (0, 0)])
        b = border("B", [(1, 0), (2, 0), (2, 1), (1, 1), (1.001, .6), (1, .4), (1.001, .2), (1, 0)])
        result, report = borders.simplify_borders([a, b], .005)
        self.assertGreater(report["removed_vertices"], 0)
        shared = edges(result[a.name][0]) & edges(result[b.name][0])
        self.assertEqual(shared, {((1, 0), (1, 1))})
        for points in (result[a.name][0], result[b.name][0]):
            self.assertTrue(Polygon(points).is_valid)
            np.testing.assert_array_equal(points[0], points[-1])

    def test_ownership_changes_lock_triple_junction(self):
        a = border("A", [(0, 0), (1, 0), (1.001, .5), (1, 1), (1.001, 1.5), (1, 2), (0, 2), (0, 0)])
        b = border("B", [(1, 1), (2, 1), (2, 2), (1, 2), (1.001, 1.5), (1, 1)])
        c = border("C", [(1, 0), (2, 0), (2, 1), (1, 1), (1.001, .5), (1, 0)])
        result, _ = borders.simplify_borders([a, b, c], .005)
        for name in (a.name, b.name, c.name):
            self.assertIn((1, 1), map(tuple, result[name][0]))
        self.assertEqual(edges(result[a.name][0]) & edges(result[b.name][0]), {((1, 1), (1, 2))})
        self.assertEqual(edges(result[a.name][0]) & edges(result[c.name][0]), {((1, 0), (1, 1))})

    def test_closed_ring_rotation_direction_and_minimum_vertices(self):
        points = [(0, 0), (1, 0), (2, 0), (2, 1), (2, 2), (1, 2), (0, 2), (0, 1)]
        rotated = points[3:] + points[:3]
        a = border("A", points + [points[0]])
        b = border("B", rotated[::-1] + [rotated[-1]])
        result, _ = borders.simplify_borders([a, b], .01)
        self.assertEqual(edges(result[a.name][0]), edges(result[b.name][0]))
        self.assertEqual(len(result[a.name][0]), 5)
        result, _ = borders.simplify_borders([a, b], 100)
        self.assertGreaterEqual(len(np.unique(result[a.name][0], axis=0)), 3)

    def test_single_junction_closed_arc_preserves_its_endpoint(self):
        a = border("A", [(0, 0), (1, 0), (2, 0), (2, 1), (2, 2), (1, 2), (0, 2), (0, 1), (0, 0)])
        b = border("B", [(1, 0), (.9, -.2), (1.1, -.2), (1, 0)])
        result, _ = borders.simplify_borders([a, b], .01)
        self.assertIn((1, 0), map(tuple, result[a.name][0]))
        np.testing.assert_array_equal(result[a.name][0], a.rings[0].points)
        # Direction normalization of a closed junction arc must not rotate it.
        arc = np.array([4, 8, 1, 3, 4], dtype=np.uint32)
        _, canonical, _ = borders.canonical_arc(arc)
        self.assertEqual(canonical[0], arc[0])
        self.assertEqual(canonical[-1], arc[-1])

    def test_unmatched_arcs_are_preserved_then_shared_arcs_can_simplify(self):
        a = border("A", [(0, 0), (1, 0), (1.000005, .5), (1, 1), (0, 1), (0, 0)])
        b = border("B", [(1, 0), (2, 0), (2, 1), (1, 1), (1, 0)])
        result, _ = borders.simplify_borders([a, b], .01)
        np.testing.assert_array_equal(result[a.name][0], a.rings[0].points)
        np.testing.assert_array_equal(result[b.name][0], b.rings[0].points)
        matched, _ = reconcile_borders.normalize([a, b])
        sources = [
            borders.Border(source.name, source.title, [borders.Ring("1", matched[source.name][0])])
            for source in (a, b)
        ]
        simplified, report = borders.simplify_borders(sources, .01)
        self.assertEqual(report["removed_vertices"], 2)
        self.assertEqual(edges(simplified[a.name][0]) & edges(simplified[b.name][0]), {((1, 0), (1, 1))})

    def test_invalid_ring_is_preserved_without_freezing_other_rings(self):
        invalid = np.array([(0, 0), (2, 2), (2, 0), (0, 2), (0, 0)], dtype=float)
        valid = np.array([(10, 0), (11, 0), (12, 0), (12, 2), (10, 2), (10, 0)], dtype=float)
        a = borders.Border("A.poly", "A", [borders.Ring("bad", invalid), borders.Ring("good", valid)])
        result, report = borders.simplify_borders([a], .01)
        np.testing.assert_array_equal(result[a.name][0], invalid)
        self.assertLess(len(result[a.name][1]), len(valid))
        self.assertEqual(report["invalid_source_rings"][0]["ring"], "bad")

    def test_explicit_preservation_locks_neighboring_arcs(self):
        a = border("A", [(0, 0), (1, 0), (1.001, .5), (1, 1), (0, 1), (0, 0)])
        b = border("B", [(1, 0), (2, 0), (2, 1), (1, 1), (1.001, .5), (1, 0)])
        result, _ = borders.simplify_borders([a, b], .005, preserve_files=["A"])
        np.testing.assert_array_equal(result[a.name][0], a.rings[0].points)
        self.assertIn((1.001, .5), map(tuple, result[b.name][0]))

    def test_new_self_intersection_reverts_the_shared_arc(self):
        points = [
            (.117, .027), (.378, .100), (.066, .022), (.643, .281), (.171, .130), (.008, .309),
            (-.153, .370), (-.386, .173), (-.011, .003), (-.121, -.010), (-.860, -.087), (.011, -.003),
        ]
        self.assertTrue(Polygon(points).is_valid)
        a = border("A", points + [points[0]])
        b = border("B", points[::-1] + [points[-1]])
        result, report = borders.simplify_borders([a, b], .2)
        self.assertEqual(report["validation_passes"], 2)
        self.assertEqual(report["reverted_files"], ["A.poly", "B.poly"])
        np.testing.assert_array_equal(result[a.name][0], a.rings[0].points)
        np.testing.assert_array_equal(result[b.name][0], b.rings[0].points)

    def test_two_junction_ring_collapse_reverts_both_neighboring_arcs(self):
        a = border("A", [(0, 0), (1, 0), (1, 1), (0, 1), (0, 0)])
        b = border("B", [(0, 0), (0, -1), (2, -1), (2, 2), (1, 1), (1, 0), (0, 0)])
        c = border("C", [(0, 0), (0, 1), (1, 1), (1, 2), (-1, 2), (-1, -1), (0, 0)])
        sources = [a, b, c]
        for source in sources:
            source.rings[0].points *= borders.DEFAULT_EPSILON / 2
            self.assertTrue(Polygon(borders.project(source.rings[0].points)).is_valid)
        result, report = borders.simplify_borders(sources)
        self.assertEqual(len(a.rings[0].junctions[:-1]), 2)
        self.assertEqual(report["validation_passes"], 2)
        self.assertEqual(report["reverted_rings"], [{"file": "A.poly", "ring": "1"}])
        self.assertEqual(report["reverted_files"], ["A.poly"])
        for source in sources:
            np.testing.assert_array_equal(result[source.name][0], source.rings[0].points)
        for neighbor in (b, c):
            self.assertEqual(
                edges(result[a.name][0]) & edges(result[neighbor.name][0]),
                edges(a.rings[0].points) & edges(neighbor.rings[0].points),
            )

    def test_invalid_repeated_loop_locks_each_shared_edge(self):
        points = np.array([(0, 0), (1, 0), (2, 0), (2, 1), (2, 2), (1, 2), (0, 2), (0, 1)], dtype=float)
        a = borders.Border("A.poly", "A", [borders.Ring("twice", np.concatenate((points, points, points[:1])))])
        b = border("B", points.tolist() + [points[0].tolist()])
        result, report = borders.simplify_borders([a, b], .01)
        self.assertFalse(a.rings[0].valid)
        self.assertEqual(report["protected_vertices"], len(points))
        np.testing.assert_array_equal(result[a.name][0], a.rings[0].points)
        np.testing.assert_array_equal(result[b.name][0], b.rings[0].points)

    def test_parser_writer_preserve_title_ring_names_and_holes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "A.poly"
            path.write_text("A title\nouter name\n0 0\n3 0\n3 3\n0 3\n0 0\nEND\n!hole\n1 1\n2 1\n1 2\n1 1\nEND\nEND\n")
            a = borders.read_poly(path)
            result, report = borders.simplify_borders([a], .01)
            output = Path(directory) / "B.poly"
            borders.write_poly(output, a, result[a.name])
            parsed = borders.read_poly(output)
            self.assertEqual(parsed.title, "A title")
            self.assertEqual([ring.name for ring in parsed.rings], ["outer name", "!hole"])
            self.assertTrue(parsed.rings[1].hole)
            self.assertFalse(report["invalid_source_files"])

    def test_parser_rejects_malformed_and_nonfinite_numeric_tokens(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "A.poly"
            for row in ("oops 2", "0 1junk", "0 1#junk", "0 1e", "0 nan", "0 inf", "0 -inf"):
                with self.subTest(row=row):
                    path.write_text(f"A title\nouter\n0 0\n1 0\n0 1\n{row}\nEND\nEND\n")
                    with self.assertRaisesRegex(ValueError, "invalid coordinates in ring outer"):
                        borders.read_poly(path)

    def test_parser_rejects_coordinate_rows_without_lon_lat_pairs(self):
        rows = (
            "0\n1\n2\n0",
            "0 0 0\n1 0 0\n0 1 0\n0 0 0",
            "0 0\n1 0\n0 1\n0",
            "0 0\n1 0\n0 1\n0 0 0",
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "A.poly"
            for coordinates in rows:
                with self.subTest(coordinates=coordinates):
                    path.write_text(f"A title\nouter\n{coordinates}\nEND\nEND\n")
                    with self.assertRaisesRegex(ValueError, "invalid coordinates in ring outer"):
                        borders.read_poly(path)

    def test_parser_accepts_scientific_notation_and_whitespace(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "A.poly"
            path.write_text("A title\nouter\n\t0.0  -1e-1\n1E+0\t0\n 0  +1.0e0 \nEND\nEND\n")
            parsed = borders.read_poly(path)
            np.testing.assert_array_equal(parsed.rings[0].points, [[0, -.1], [1, 0], [0, 1]])


if __name__ == "__main__":
    unittest.main()
