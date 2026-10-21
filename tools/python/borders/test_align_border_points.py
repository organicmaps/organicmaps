import contextlib
import io
from pathlib import Path
import tempfile
import unittest

import numpy as np
from shapely.geometry import LineString, Polygon

import align_border_points
from maintain_borders import Border, Ring


def border(name, coordinates):
    return Border(name + ".poly", name, [Ring("1", np.array(coordinates, dtype=float))])


def junction_sources(point=(1 + 5e-6, 1)):
    return [
        border("target", [(0, 0), (1, 0), (1, 2), (0, 2), (0, 0)]),
        border("donor1", [point, (2, .8), (2, 1.2), point]),
        border("donor2", [point, (3, .7), (3, 1.3), point]),
    ]


def single_donor(point=(1 + 5e-6, 1)):
    return border("donor", [(1, 0), point, (1, 2), (3, 2), (3, 0), (1, 0)])


def notched_target():
    return border("target", [
        (1, 0), (2, 0), (2, .29), (1 + 1e-6, .3),
        (2, .31), (2, 2), (1, 2), (1, 0),
    ])


def distant_invalid_target(notch=False):
    points = [(1, 0), (1, 2), (2, 3), (4, 2), (3, 0), (4, 0), (3, 2), (2, 2)]
    if notch:
        points.extend([(2, .31), (1 + 1e-6, .3), (2, .29)])
    points.extend([(2, 0), (1, 0)])
    return border("target", points)


class AlignBorderPointsTest(unittest.TestCase):
    def assert_unchanged(self, sources, output):
        for source in sources:
            for ring, points in zip(source.rings, output[source.name]):
                np.testing.assert_array_equal(ring.points, points)

    def test_existing_junction_becomes_exact_in_all_three_owners(self):
        point = (1 + 5e-6, 1)
        sources = junction_sources(point)
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 1)
        self.assertEqual(report["changed_files"], ["target.poly"])
        for source in sources:
            self.assertIn(point, map(tuple, output[source.name][0]))
        source_coordinates = {tuple(p) for source in sources for ring in source.rings for p in ring.points}
        for rings in output.values():
            for points in rings:
                self.assertTrue(set(map(tuple, points)) <= source_coordinates)
        np.testing.assert_array_equal(output["donor1.poly"][0], sources[1].rings[0].points)
        np.testing.assert_array_equal(output["donor2.poly"][0], sources[2].rings[0].points)
        self.assertLessEqual(report["max_inserted_distance_mercator"], 1e-5)

    def test_replacement_pairs_include_crossings_touches_and_overlaps_once(self):
        replacements = {
            30: LineString([(5, 5), (6, 6)]),
            20: LineString([(1, -1), (1, 1)]),
            10: LineString([(0, 0), (2, 0)]),
            50: LineString([(.5, 0), (1.5, 0)]),
            40: LineString([(2, 0), (3, 1)]),
        }
        self.assertEqual(list(align_border_points.intersecting_replacement_pairs(replacements)),
                         [(10, 20), (10, 40), (10, 50), (20, 50)])
        self.assertEqual(list(align_border_points.intersecting_replacement_pairs({})), [])

    def test_multiple_insertions_preserve_unchanged_vertices_and_closure(self):
        side_point = (1 + 5e-6, 1)
        closing_point = (.5, -5e-6)
        for closed in (False, True):
            with self.subTest(closed=closed):
                coordinates = [(1, 0), (1, 2)] + [(lon, 2) for lon in np.linspace(.9, 0, 10)] + [(0, 0)]
                sources = junction_sources(side_point)
                sources[0] = border("target", coordinates + ([(1, 0)] if closed else []))
                sources.extend([
                    border("closing_donor1", [closing_point, (.4, -1), (.6, -1), closing_point]),
                    border("closing_donor2", [closing_point, (.3, -2), (.7, -2), closing_point]),
                ])
                output, report = align_border_points.align_points(sources)
                expected = coordinates[:1] + [side_point] + coordinates[1:] + [closing_point]
                if closed:
                    expected.append(coordinates[0])
                np.testing.assert_array_equal(output["target.poly"][0], expected)
                self.assertEqual(report["inserted_vertices"], 2)
                self.assertEqual(report["changed_files"], ["target.poly"])
                for source in sources[1:]:
                    np.testing.assert_array_equal(output[source.name][0], source.rings[0].points)

    def test_single_owner_requires_both_endpoints(self):
        sources = junction_sources()[:2]
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 0)
        self.assert_unchanged(sources, output)
        sources[1] = single_donor()
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 1)
        self.assertEqual(report["inserted_single_owner_vertices"], 1)

    def test_single_owner_evidence_must_come_from_one_ring(self):
        sources = junction_sources()[:1]
        donor = border("donor", [(1, 0), (1 + 5e-6, 1), (3, .5), (1, 0)])
        donor.rings.append(Ring("other", np.array([(1, 2), (3, 2), (3, 1.5), (1, 2)])))
        sources.append(donor)
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 0)
        self.assert_unchanged(sources, output)

    def test_ambiguous_target_edges_are_rejected(self):
        sources = junction_sources()
        sources[0] = border("target", [(1, 0), (1 + 1e-5, 0), (1 + 1e-5, 2), (1, 2), (1, 0)])
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 0)
        self.assertGreater(report["query_counts"]["ambiguous_target_matches"], 0)
        self.assert_unchanged(sources, output)

    def test_preserved_donors_and_targets_are_untouched(self):
        for preserved in ("donor1", "target"):
            with self.subTest(preserved=preserved):
                sources = junction_sources()
                output, report = align_border_points.align_points(sources, preserve_files=[preserved])
                self.assertEqual(report["inserted_vertices"], 0)
                self.assert_unchanged(sources, output)

    def test_near_endpoints_are_not_merged(self):
        sources = junction_sources((1 + 2e-6, 2e-6))
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["inserted_vertices"], 0)
        self.assertGreater(report["query_counts"]["near_endpoint_matches"], 0)
        self.assert_unchanged(sources, output)

    def test_exact_edge_exclusion_preserves_other_alignment(self):
        sources = junction_sources()
        # A reversed endpoint pair identifies the same original edge. An
        # unrelated target receives its own valid existing junction point.
        point = (11 + 5e-6, 1)
        sources.extend([
            border("other", [(10, 0), (11, 0), (11, 2), (10, 2), (10, 0)]),
            border("other_donor1", [point, (12, .8), (12, 1.2), point]),
            border("other_donor2", [point, (13, .7), (13, 1.3), point]),
        ])
        excluded = [{"file": "target", "endpoints": [[1, 2], [1, 0]], "reason": "Preserve audited edge"}]
        output, report = align_border_points.align_points(sources, excluded_edges=excluded)
        np.testing.assert_array_equal(output["target.poly"][0], sources[0].rings[0].points)
        self.assertIn(point, map(tuple, output["other.poly"][0]))
        self.assertEqual(report["inserted_vertices"], 1)
        self.assertEqual(report["query_counts"]["excluded_edge_matches"], 1)
        self.assertEqual(report["matched_excluded_edges"], [{
            "file": "target.poly", "endpoints": [[1., 0.], [1., 2.]], "reason": "Preserve audited edge",
        }])

    def test_excluding_one_edge_does_not_resolve_an_ambiguous_match(self):
        sources = junction_sources()
        sources[0] = border("target", [(1, 0), (1 + 1e-5, 0), (1 + 1e-5, 2), (1, 2), (1, 0)])
        excluded = [{"file": "target", "endpoints": [[1, 0], [1, 2]]}]
        output, report = align_border_points.align_points(sources, excluded_edges=excluded)
        self.assertEqual(report["inserted_vertices"], 0)
        self.assertGreater(report["query_counts"]["ambiguous_target_matches"], 0)
        self.assert_unchanged(sources, output)

    def test_historical_exclusions_are_reported_as_unmatched(self):
        sources = junction_sources()
        excluded = [
            {"file": "target", "endpoints": [[100, 10], [101, 11]], "reason": "Absent endpoint"},
            {"file": "target", "endpoints": [[0, 0], [1, 2]], "reason": "Former edge"},
        ]
        output, report = align_border_points.align_points(sources, excluded_edges=excluded)
        self.assertEqual(report["inserted_vertices"], 1)
        self.assertFalse(report["matched_excluded_edges"])
        self.assertEqual(len(report["unmatched_excluded_edges"]), 2)
        self.assertIn((1 + 5e-6, 1), map(tuple, output["target.poly"][0]))
        with self.assertRaisesRegex(ValueError, "Unknown border files"):
            align_border_points.align_points(sources, excluded_edges=[{
                "file": "missing", "endpoints": [[0, 0], [1, 2]],
            }])

    def test_new_invalid_geometry_rolls_back_insertion(self):
        sources = [notched_target(), single_donor()]
        self.assertTrue(Polygon(sources[0].rings[0].points).is_valid)
        output, report = align_border_points.align_points(sources)
        self.assertEqual(report["blocked_edge_groups"], 1)
        self.assertEqual(report["reverted_files"], ["target.poly"])
        self.assert_unchanged(sources, output)

    def test_allowed_invalid_target_rejects_new_local_intersections(self):
        donor = border("donor", [(1, 0), (1 + 5e-6, 1), (1, 2), (5, 3), (5, -1), (1, 0)])
        sources = [distant_invalid_target(notch=True), donor]
        output, report = align_border_points.align_points(sources, allow_invalid_files=["target"])
        self.assertTrue(report["local_intersection_rejections"])
        self.assertEqual(report["inserted_vertices"], 0)
        self.assert_unchanged(sources, output)

    def test_allowed_invalid_target_keeps_distant_defect(self):
        donor = border("donor", [(1, 0), (1 + 5e-6, 1), (1, 2), (5, 3), (5, -1), (1, 0)])
        sources = [distant_invalid_target(), donor]
        output, report = align_border_points.align_points(sources, allow_invalid_files=["target"])
        self.assertEqual(report["inserted_vertices"], 1)
        self.assertFalse(report["local_intersection_rejections"])
        self.assertIn("target.poly", report["invalid_source_reasons"])

    def test_explicit_invalid_permission_rejects_multiple_rings(self):
        sources = junction_sources()
        sources[0].rings.append(Ring("distant", np.array([(10, 0), (11, 1), (11, 0), (10, 1), (10, 0)])))
        with self.assertRaisesRegex(ValueError, "requires one ring"):
            align_border_points.align_points(sources, allow_invalid_files=["target"])

    def test_input_validation(self):
        for epsilon in (0, -1, float("inf"), float("nan")):
            with self.subTest(epsilon=epsilon), self.assertRaisesRegex(ValueError, "finite and positive"):
                align_border_points.align_points(junction_sources(), epsilon)
        with self.assertRaisesRegex(ValueError, "No borders provided"):
            align_border_points.align_points([])
        with self.assertRaisesRegex(ValueError, "Unknown border files"):
            align_border_points.align_points(junction_sources(), allow_invalid_files=["missing"])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "input"
            source.mkdir()
            for argv in ([str(source), str(source)], [str(source), str(source / "nested")],
                         [str(source), str(root)], [str(source), str(root / "output"), "--epsilon", "nan"],
                         [str(source), str(root / "output")]):
                with self.subTest(argv=argv), contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as error:
                        align_border_points.main(argv)
                    self.assertEqual(error.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
