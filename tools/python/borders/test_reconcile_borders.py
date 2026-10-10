import contextlib
import io
from pathlib import Path
import tempfile
import unittest

import numpy as np
from shapely.geometry import Polygon

import maintain_borders
import reconcile_borders


def border(name, points):
    return maintain_borders.Border(
        name + ".poly", name, [maintain_borders.Ring("outer", np.array(points, dtype=float))]
    )


def edges(points):
    if np.array_equal(points[0], points[-1]):
        points = points[:-1]
    return {
        tuple(sorted((tuple(first), tuple(last))))
        for first, last in zip(points, np.roll(points, -1, axis=0))
    }


def three_owners(offset=1e-6):
    a = border("A", [(0, 0), (1, 0), (1 + offset, .5), (1, 1), (0, 1), (0, 0)])
    b = border("B", [(1, 0), (2, 0), (2, 1), (1, 1), (1, 0)])
    c = border("C", [(1, 1), (3, 1), (3, 0), (1, 0)])
    return [a, b, c]


class ReconcileBordersTest(unittest.TestCase):
    def assert_unchanged(self, sources, result):
        for source in sources:
            for ring, points in zip(source.rings, result[source.name]):
                np.testing.assert_array_equal(ring.points, points)

    def test_continuous_bound_for_differently_segmented_collinear_arcs(self):
        first = np.array([(0., 0.), (2., 0.), (4., 0.)])
        second = np.array([(0., 0.), (1., 0.), (3., 0.), (4., 0.)])
        self.assertEqual(reconcile_borders.arc_distance_upper_bound(first, second), 0)

    def test_continuous_bound_for_near_parallel_arcs(self):
        epsilon = maintain_borders.DEFAULT_EPSILON
        first = np.array([(0., 0.), (2., 0.), (4., 0.)])
        second = np.array([(0., .9 * epsilon), (1., .9 * epsilon), (4., .9 * epsilon)])
        bound = reconcile_borders.arc_distance_upper_bound(first, second)
        self.assertAlmostEqual(bound, .9 * epsilon)
        self.assertLess(bound, epsilon)

    def test_dense_arc_propagates_to_three_owners(self):
        sources = three_owners()
        result, report = reconcile_borders.normalize(sources)
        self.assertEqual(report["normalized_endpoint_groups"], 1)
        self.assertEqual(report["changed_files"], ["B.poly", "C.poly"])
        shared = {
            ((1, 0), (1 + 1e-6, .5)),
            ((1, 1), (1 + 1e-6, .5)),
        }
        for a, b in ((sources[0], sources[1]), (sources[0], sources[2])):
            self.assertEqual(edges(result[a.name][0]) & edges(result[b.name][0]), shared)
        for source in sources:
            self.assertTrue(Polygon(result[source.name][0]).is_valid)
            self.assertEqual(
                np.array_equal(source.rings[0].points[0], source.rings[0].points[-1]),
                np.array_equal(result[source.name][0][0], result[source.name][0][-1]),
            )
        # The complementary outside arcs have the same endpoint pair, but
        # must not suppress or get merged with this shared arc.
        np.testing.assert_array_equal(result["A.poly"][0], sources[0].rings[0].points)

    def test_larger_discrepancy_is_preserved(self):
        sources = three_owners(offset=.001)
        result, report = reconcile_borders.normalize(sources)
        self.assertEqual(report["normalized_endpoint_groups"], 0)
        self.assertFalse(report["changed_files"])
        self.assert_unchanged(sources, result)

    def test_existing_third_owner_junction_is_not_skipped(self):
        sources = three_owners()
        # A and C already share the middle vertex, while B skips it. It is a
        # fixed anchor, so A's two partial arcs cannot match B's full arc.
        sources[2].rings[0].points = np.array([
            (1, 0), (1 + 1e-6, .5), (1.2, .25), (1, 0),
        ])
        result, report = reconcile_borders.normalize(sources)
        self.assertEqual(report["normalized_endpoint_groups"], 0)
        self.assert_unchanged(sources, result)

    def test_reconciliation_is_idempotent_after_shared_replacement(self):
        sources = three_owners()
        result, _ = reconcile_borders.normalize(sources)
        normalized = [
            maintain_borders.Border(source.name, source.title, [
                maintain_borders.Ring(ring.name, points)
                for ring, points in zip(source.rings, result[source.name])
            ])
            for source in sources
        ]
        repeated, report = reconcile_borders.normalize(normalized)
        self.assertEqual(report["normalized_endpoint_groups"], 0)
        self.assertFalse(report["changed_files"])
        self.assert_unchanged(normalized, repeated)

    def test_invalid_source_owner_locks_every_owner(self):
        sources = three_owners()
        sources[0].rings[0].points = np.array(
            [(0, 0), (1, 0), (1 + 1e-6, .5), (1, 1), (0, 1), (.5, -.5), (0, 0)]
        )
        result, report = reconcile_borders.normalize(sources)
        self.assertEqual(report["invalid_source_files"], ["A.poly"])
        self.assertEqual(report["groups_touching_invalid_files"], 1)
        self.assert_unchanged(sources, result)

    def test_preserved_owner_locks_every_owner(self):
        sources = three_owners()
        result, report = reconcile_borders.normalize(sources, preserve_files=["A"])
        self.assertEqual(report["preserved_files"], ["A.poly"])
        self.assertEqual(report["groups_touching_preserved_files"], 1)
        self.assert_unchanged(sources, result)
        with self.assertRaisesRegex(ValueError, "Unknown preserved files"):
            reconcile_borders.normalize(sources, preserve_files=["missing"])

    def test_new_self_intersection_rolls_back_in_every_owner(self):
        sources = three_owners()
        # A narrow notch is valid beside the sparse straight arc. Replacing
        # that arc with A's outward bend crosses the notch boundary.
        sources[1].rings[0].points = np.array([
            (1, 0), (2, 0), (2, .49), (1 + .5e-6, .5),
            (2, .51), (2, 1), (1, 1), (1, 0),
        ])
        self.assertTrue(Polygon(sources[1].rings[0].points).is_valid)
        result, report = reconcile_borders.normalize(sources)
        self.assertEqual(report["blocked_groups"], 1)
        self.assertEqual(report["validation_passes"], 2)
        self.assertEqual(report["reverted_files"], ["B.poly"])
        self.assertEqual(report["normalized_endpoint_groups"], 0)
        self.assert_unchanged(sources, result)

    def test_written_metadata_and_holes_survive_replacement(self):
        sources = three_owners()
        sources[1].title = "A custom title"
        sources[1].rings[0].name = "A named outer"
        sources[1].rings.append(maintain_borders.Ring(
            "!A named hole", np.array([(1.2, .1), (1.3, .1), (1.2, .2), (1.2, .1)])
        ))
        result, report = reconcile_borders.normalize(sources)
        self.assertIn("B.poly", report["changed_files"])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "B.poly"
            maintain_borders.write_poly(path, sources[1], result["B.poly"])
            parsed = maintain_borders.read_poly(path)
        self.assertEqual(parsed.title, sources[1].title)
        self.assertEqual([ring.name for ring in parsed.rings], ["A named outer", "!A named hole"])
        np.testing.assert_array_equal(parsed.rings[1].points, sources[1].rings[1].points)

    def test_input_validation(self):
        for epsilon in (0, -1, float("inf"), float("nan")):
            with self.subTest(epsilon=epsilon), self.assertRaisesRegex(ValueError, "finite and positive"):
                reconcile_borders.normalize(three_owners(), epsilon)
        with self.assertRaisesRegex(ValueError, "No borders provided"):
            reconcile_borders.normalize([])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "input"
            source.mkdir()
            arguments = [
                [str(source), str(source)],
                [str(source), str(source / "nested")],
                [str(source), str(root)],
                [str(source), str(root / "output"), "--epsilon", "nan"],
                [str(source), str(root / "output")],
            ]
            for argv in arguments:
                with self.subTest(argv=argv), contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as error:
                        reconcile_borders.main(argv)
                    self.assertEqual(error.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
