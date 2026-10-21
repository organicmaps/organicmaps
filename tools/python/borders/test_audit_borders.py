from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import numpy as np
from shapely.geometry import Point

from audit_borders import audit, load, ring_coverage
from border_comparison import compare, source_dataset
from maintain_borders import project


class AuditTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.baseline = Path(self.temporary.name) / "before"
        self.candidate = Path(self.temporary.name) / "after"
        self.baseline.mkdir()
        self.candidate.mkdir()

    def write(self, directory, name, points, title=None):
        self.write_rings(directory, name, [("1", points)], title)

    def write_rings(self, directory, name, rings, title=None):
        lines = [title or name]
        for label, points in rings:
            lines.extend((label, *(f"{x} {y}" for x, y in points + points[:1]), "END"))
        (directory / f"{name}.poly").write_text("\n".join(lines + ["END", ""]))

    def test_new_overlap_is_reported(self):
        for directory in (self.baseline, self.candidate):
            self.write(directory, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
        self.write(self.baseline, "B", [(1, 0), (2, 0), (2, 1), (1, 1)])
        self.write(self.candidate, "B", [(0.999, 0), (2, 0), (2, 1), (0.999, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(len(result["introduced_overlap_parts"]), 1)
        self.assertEqual(result["new_invalid_rings"], [])

    def test_existing_overlap_is_not_a_regression(self):
        for directory in (self.baseline, self.candidate):
            self.write(directory, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
            self.write(directory, "B", [(0.9, 0), (2, 0), (2, 1), (0.9, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(len(result["overlaps"]), 1)
        self.assertEqual(result["introduced_overlap_parts"], [])

    def test_exterior_coverage_loss_is_reported_without_groups(self):
        self.write(self.baseline, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
        self.write(self.candidate, "A", [(0, 0), (.9, 0), (.9, 1), (0, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["interior_holes"], [])
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertEqual(len(result["introduced_uncovered_parts"]), 1)
        lost = result["introduced_uncovered_parts"][0]
        self.assertEqual(lost["file"], "A.poly")
        self.assertGreater(lost["approx_m2"], 1)
        self.assertAlmostEqual(lost["bounds"][0], .9)
        self.assertAlmostEqual(lost["bounds"][2], 1)
        self.assertIn("POINT", lost["point"])

        report = Path(self.temporary.name) / "report.json"
        completed = subprocess.run([sys.executable, str(Path(__file__).with_name("audit_borders.py")),
                                    str(self.candidate), "--baseline", str(self.baseline), "--report", str(report)],
                                   capture_output=True, text=True)
        self.assertEqual(completed.returncode, 1, completed.stderr)
        self.assertTrue(report.exists())

    def test_coverage_transferred_to_neighbor_is_retained(self):
        self.write(self.baseline, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
        self.write(self.baseline, "B", [(1, 0), (2, 0), (2, 1), (1, 1)])
        self.write(self.candidate, "A", [(0, 0), (.9, 0), (.9, 1), (0, 1)])
        self.write(self.candidate, "B", [(.9, 0), (2, 0), (2, 1), (.9, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["introduced_uncovered_parts"], [])
        self.assertEqual(result["introduced_overlap_parts"], [])

    def test_moving_an_internal_outer_partition_keeps_coverage(self):
        self.write_rings(self.baseline, "A", [
            ("1", [(0, 0), (1, 0), (1, 1), (0, 1)]),
            ("2", [(1, 0), (2, 0), (2, 1), (1, 1)]),
        ])
        self.write_rings(self.candidate, "A", [
            ("1", [(0, 0), (1.5, 0), (1.5, .5), (1.5, 1), (0, 1)]),
            ("2", [(1.5, 1), (2, 1), (2, 0), (1.5, 0)]),
        ])
        old, new = source_dataset(self.baseline), source_dataset(self.candidate)
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        self.assertEqual(old.invalid_files, ["A.poly"])
        self.assertEqual(new.invalid_files, old.invalid_files)
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["new_invalid_file_contacts"], [])
        self.assertFalse(result["regressions"])

    def test_moving_an_outer_partition_preserves_an_enclosed_hole(self):
        self.write_rings(self.baseline, "A", [
            ("1", [(0, 0), (2, 0), (2, 1), (1, 1), (1, 3), (2, 3), (2, 4), (0, 4)]),
            ("2", [(2, 0), (4, 0), (4, 4), (2, 4), (2, 3), (3, 3), (3, 1), (2, 1)]),
        ])
        self.write_rings(self.candidate, "A", [
            ("1", [(0, 0), (4, 0), (4, 2), (3, 2), (3, 1), (1, 1), (1, 2), (0, 2)]),
            ("2", [(0, 2), (1, 2), (1, 3), (3, 3), (3, 2), (4, 2), (4, 4), (0, 4)]),
        ])
        old, new = source_dataset(self.baseline), source_dataset(self.candidate)
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        self.assertEqual(len(new.shapes[0].interiors), 1)
        self.assertFalse(new.shapes[0].covers(Point(project(np.array([[2., 2.]]))[0])))
        result = audit(self.candidate, self.baseline, groups=["*"])
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertEqual(result["new_invalid_file_contacts"], [])
        self.assertFalse(result["regressions"])

    def test_neighbor_retains_only_part_of_an_exterior_loss(self):
        self.write(self.baseline, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
        self.write(self.candidate, "A", [(0, 0), (.9, 0), (.9, 1), (0, 1)])
        self.write(self.baseline, "B", [(1, 0), (2, 0), (2, .5), (1, .5)])
        self.write(self.candidate, "B", [(.9, 0), (2, 0), (2, .5), (.9, .5)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(len(result["introduced_uncovered_parts"]), 1)
        bounds = result["introduced_uncovered_parts"][0]["bounds"]
        for actual, expected in zip(bounds, (.9, .5, 1, 1)):
            self.assertAlmostEqual(actual, expected)

    def test_new_enclosed_gap_and_occupied_enclave(self):
        outer = [(0, 0), (2, 0), (2, 2), (0, 2)]
        self.write(self.baseline, "Group_A", outer)
        # A second outer ring is unnecessary: .poly holes express the gap directly.
        self.write(self.candidate, "Group_A", outer)
        path = self.candidate / "Group_A.poly"
        path.write_text(path.read_text()[:-4] + "!hole\n0.9 0.9\n1.1 0.9\n1.1 1.1\n0.9 1.1\n0.9 0.9\nEND\nEND\n")
        result = audit(self.candidate, self.baseline, groups=["Group_"])
        self.assertEqual(len(result["introduced_interior_holes"]), 1)
        self.assertEqual(len(result["introduced_uncovered_parts"]), 1)
        enclave = [(0.9, 0.9), (1.1, 0.9), (1.1, 1.1), (0.9, 1.1)]
        for directory in (self.baseline, self.candidate):
            self.write(directory, "Enclave", enclave)
        result = audit(self.candidate, self.baseline, groups=["Group_"])
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])

    def test_new_invalid_ring_and_metadata(self):
        self.write(self.baseline, "A", [(0, 0), (1, 0), (1, 1), (0, 1)])
        self.write(self.candidate, "A", [(0, 0), (1, 1), (1, 0), (0, 1)], title="Renamed")
        result = audit(self.candidate, self.baseline)
        self.assertEqual(len(result["new_invalid_rings"]), 1)
        self.assertEqual(result["changed_metadata"], ["A.poly"])

    def test_zero_area_crossing_in_inherited_invalid_ring_fails_manual_cli(self):
        square = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0)]
        retrace = [(1, 0), (0, 0)]
        loop = [(-1, -1), (-3, -3), (-1, -3), (-3, -1), (-1, -1)]
        self.write(self.baseline, "A", square + retrace)
        self.write(self.candidate, "A", square + retrace + loop + loop[1:] + [(0, 0)])
        old, new = source_dataset(self.baseline), source_dataset(self.candidate)
        self.assertTrue(old.shapes[0].equals(new.shapes[0]))
        strict = compare(new, old, 1)
        result = audit(self.candidate, self.baseline, groups=["*"])
        self.assertEqual(result["new_invalid_rings"], [])
        self.assertEqual(result["new_invalid_files"], [])
        self.assertEqual(result["introduced_overlap_parts"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertTrue(result["regressions"])
        self.assertEqual(result["new_invalid_ring_contacts"], strict["new_invalid_ring_contacts"])

        report = Path(self.temporary.name) / "crossing-report.json"
        completed = subprocess.run([sys.executable, str(Path(__file__).with_name("audit_borders.py")),
                                    str(self.candidate), "--baseline", str(self.baseline), "--report", str(report)],
                                   capture_output=True, text=True)
        self.assertEqual(completed.returncode, 1, completed.stderr)
        self.assertTrue(report.exists())

    def test_unchanged_inherited_invalid_contacts_pass_manual_cli(self):
        points = [(0, 0), (4, 0), (4, 4), (0, 4), (0, 0), (1, 0), (0, 0)]
        for directory in (self.baseline, self.candidate):
            self.write(directory, "A", points)
        result = audit(self.candidate, self.baseline)
        self.assertTrue(result["invalid_rings"])
        self.assertEqual(result["new_invalid_ring_contacts"], [])
        self.assertFalse(result["regressions"])

        report = Path(self.temporary.name) / "inherited-report.json"
        completed = subprocess.run([sys.executable, str(Path(__file__).with_name("audit_borders.py")),
                                    str(self.candidate), "--baseline", str(self.baseline), "--report", str(report)],
                                   capture_output=True, text=True)
        self.assertEqual(completed.returncode, 0, completed.stderr)

    def test_inherited_invalid_file_cannot_gain_hidden_outer_overlap(self):
        left = [(0, 0), (2, 0), (2, 2), (0, 2)]
        old_right = [(2, 0), (4, 0), (4, 2), (2, 2)]
        new_right = [(1, 0), (4, 0), (4, 2), (1, 2)]
        self.write_rings(self.baseline, "A", [("1", left), ("2", old_right)])
        self.write_rings(self.candidate, "A", [("1", left), ("2", new_right)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["new_invalid_rings"], [])
        self.assertEqual(result["new_invalid_files"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])
        self.assertTrue(result["new_invalid_file_contacts"])
        self.assertTrue(result["regressions"])

    def test_island_inside_another_union_parts_hole_is_covered(self):
        outer = [(0, 0), (4, 0), (4, 4), (3, 4), (3, 1), (1, 1), (1, 4), (0, 4)]
        island = [(1.5, 1.5), (2.5, 1.5), (2.5, 2.5), (1.5, 2.5)]
        for directory in (self.baseline, self.candidate):
            self.write(directory, "Group_A", outer)
            self.write(directory, "Group_island", island)
        self.write(self.baseline, "Group_top", [(0, 3), (1, 3), (1, 4), (0, 4)])
        self.write(self.candidate, "Group_top", [(0, 3), (4, 3), (4, 4), (0, 4)])
        result = audit(self.candidate, self.baseline, groups=["Group_"])
        self.assertEqual(len(result["interior_holes"]), 1)
        self.assertEqual(result["introduced_interior_holes"], [])

    def test_long_northern_segments_use_generator_projection(self):
        # This midpoint lies on the Mercator segment, above the lon/lat segment.
        midpoint = (5, 72.59014795112851)
        self.write(self.baseline, "Group_A", [(0, 50), (10, 50), (10, 80), midpoint, (0, 60)])
        self.write(self.candidate, "Group_A", [(0, 50), (10, 50), (10, 80), (0, 60)])
        for directory in (self.baseline, self.candidate):
            self.write(directory, "Group_B", [(0, 60), midpoint, (10, 80), (10, 84), (0, 84)])
            self.write(directory, "Group_left", [(-1, 50), (.25, 50), (.25, 84), (-1, 84)])
            self.write(directory, "Group_right", [(9.75, 50), (11, 50), (11, 84), (9.75, 84)])
        result = audit(self.candidate, self.baseline, groups=["Group_"])
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertEqual(result["introduced_overlap_parts"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])

    def test_enclosing_an_existing_exterior_void_is_not_lost_coverage(self):
        outer = [(0, 0), (4, 0), (4, 4), (3, 4), (3, 1), (1, 1), (1, 4), (0, 4)]
        for directory in (self.baseline, self.candidate):
            self.write(directory, "Group_A", outer)
        self.write(self.baseline, "Group_B", [(0, 3), (1, 3), (1, 4), (0, 4)])
        self.write(self.candidate, "Group_B", [(0, 3), (4, 3), (4, 4), (0, 4)])
        result = audit(self.candidate, self.baseline, groups=["Group_"])
        self.assertEqual(len(result["interior_holes"]), 1)
        self.assertEqual(result["introduced_interior_holes"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])

    def test_repeated_ring_fill_change_introduces_overlap_and_fails_cli(self):
        square = [(0, 0), (2, 0), (2, 2), (0, 2)]
        self.write(self.baseline, "A", square * 2)
        self.write(self.candidate, "A", square)
        for directory in (self.baseline, self.candidate):
            self.write(directory, "B", [(.5, .5), (1, .5), (1, 1), (.5, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(len(result["introduced_overlap_parts"]), 1)
        self.assertEqual(result["introduced_uncovered_parts"], [])

        report = Path(self.temporary.name) / "parity-report.json"
        completed = subprocess.run([sys.executable, str(Path(__file__).with_name("audit_borders.py")),
                                    str(self.candidate), "--baseline", str(self.baseline), "--report", str(report)],
                                   capture_output=True, text=True)
        self.assertEqual(completed.returncode, 1, completed.stderr)
        self.assertTrue(report.exists())

    def test_repeated_ring_can_remove_all_coverage(self):
        square = [(0, 0), (2, 0), (2, 2), (0, 2)]
        self.write(self.baseline, "A", square)
        self.write(self.candidate, "A", square * 2)
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["overlaps"], [])
        self.assertEqual(len(result["introduced_uncovered_parts"]), 1)
        self.assertEqual(result["introduced_uncovered_parts"][0]["file"], "A.poly")
        _, shapes, _ = load(self.candidate)
        self.assertTrue(shapes[0].is_empty)

    def test_odd_repeated_ring_retains_coverage_in_either_direction(self):
        square = [(0, 0), (2, 0), (2, 2), (0, 2)]
        self.write(self.baseline, "A", square)
        for points in (square * 3, square[::-1] * 3):
            with self.subTest(points=points):
                self.write(self.candidate, "A", points)
                result = audit(self.candidate, self.baseline)
                self.assertEqual(result["introduced_uncovered_parts"], [])
                _, shapes, _ = load(self.candidate)
                self.assertTrue(shapes[0].covers(Point(1, 1)))

    def test_invalid_hole_uses_its_own_traversal_parity(self):
        outer = [(0, 0), (3, 0), (3, 3), (0, 3)]
        hole = [(.5, .5), (2, .5), (2, 2), (.5, 2)]
        self.write_rings(self.baseline, "A", [("1", outer), ("!hole", hole * 3)])
        self.write_rings(self.candidate, "A", [("1", outer), ("!hole", hole * 2)])
        for directory in (self.baseline, self.candidate):
            self.write(directory, "B", [(.75, .75), (1, .75), (1, 1), (.75, 1)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["new_invalid_rings"], [])
        self.assertEqual(len(result["introduced_overlap_parts"]), 1)
        self.assertEqual(result["introduced_uncovered_parts"], [])

    def test_later_hole_subtracts_from_overlapping_outers(self):
        outer = [(0, 0), (4, 0), (4, 4), (0, 4)]
        inner = [(1, 1), (3, 1), (3, 3), (1, 3)]
        hole = [(1.5, 1.5), (2.5, 1.5), (2.5, 2.5), (1.5, 2.5)]
        self.write_rings(self.candidate, "A", [("1", outer), ("2", inner), ("!hole", hole)])
        borders, shapes, _ = load(self.candidate)
        self.assertEqual(borders[0].hole_owners, {2: 1})
        self.assertTrue(shapes[0].covers(Point(1.25, 1.25)))
        # A later hole subtracts from all coverage accumulated before it.
        self.assertFalse(shapes[0].covers(Point(2, 2)))

    def test_island_after_hole_restores_only_its_own_coverage(self):
        outer = [(0, 0), (4, 0), (4, 4), (0, 4)]
        hole = [(1, 1), (3, 1), (3, 3), (1, 3)]
        island = [(1.5, 1.5), (2.5, 1.5), (2.5, 2.5), (1.5, 2.5)]
        self.write_rings(self.candidate, "A", [("outer", outer), ("!hole", hole), ("island", island)])
        borders, shapes, report = load(self.candidate)
        self.assertEqual(borders[0].hole_owners, {1: 0})
        self.assertEqual(report["invalid_rings"], [])
        self.assertEqual(report["invalid_files"], [])
        gap, filled = project(np.array([(1.25, 2), (2, 2)], dtype=float))
        self.assertFalse(shapes[0].covers(Point(gap)))
        self.assertTrue(shapes[0].covers(Point(filled)))

    def test_later_hole_removes_earlier_island_despite_valid_geometry(self):
        island = [(1, 1), (1.5, 1), (1.5, 1.5), (1, 1.5)]
        outer = [(0, 0), (4, 0), (4, 4), (0, 4)]
        hole = [(.5, .5), (3.5, .5), (3.5, 3.5), (.5, 3.5)]
        self.write_rings(self.candidate, "A", [("island", island), ("outer", outer), ("!hole", hole)])
        borders, shapes, report = load(self.candidate)
        self.assertEqual(borders[0].hole_owners, {2: 1})
        self.assertTrue(borders[0].valid)
        self.assertEqual(report["invalid_files"], [])
        removed, retained = project(np.array([(1.25, 1.25), (.25, .25)], dtype=float))
        self.assertFalse(shapes[0].covers(Point(removed)))
        self.assertTrue(shapes[0].covers(Point(retained)))

    def test_hole_before_outer_subtracts_from_empty_coverage(self):
        outer = [(0, 0), (4, 0), (4, 4), (0, 4)]
        hole = [(1, 1), (3, 1), (3, 3), (1, 3)]
        self.write_rings(self.candidate, "A", [("!hole", hole), ("outer", outer)])
        borders, shapes, report = load(self.candidate)
        center = project(np.array([(2, 2)], dtype=float))[0]
        self.assertTrue(shapes[0].covers(Point(center)))
        self.assertEqual(borders[0].hole_owners, {})
        self.assertEqual(report["invalid_rings"], [])
        self.assertEqual(report["invalid_files"], ["A.poly"])

    def test_shrinking_later_hole_exposes_enclave_in_overlapping_outers(self):
        outer = [(0, 0), (4, 0), (4, 4), (0, 4)]
        inner = [(1, 1), (3, 1), (3, 3), (1, 3)]
        hole = [(1.5, 1.5), (2.5, 1.5), (2.5, 2.5), (1.5, 2.5)]
        smaller_hole = [(1.75, 1.75), (2.25, 1.75), (2.25, 2.25), (1.75, 2.25)]
        self.write_rings(self.baseline, "A", [("1", outer), ("2", inner), ("!hole", hole)])
        self.write_rings(self.candidate, "A", [("1", outer), ("2", inner), ("!hole", smaller_hole)])
        for directory in (self.baseline, self.candidate):
            self.write(directory, "Enclave", [(1.55, 1.55), (1.7, 1.55), (1.7, 1.7), (1.55, 1.7)])
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["invalid_rings"], [])
        self.assertEqual(result["invalid_files"], ["A.poly"])
        self.assertEqual(result["baseline"]["invalid_files"], ["A.poly"])
        self.assertEqual(result["new_invalid_rings"], [])
        self.assertEqual(result["new_invalid_files"], [])
        self.assertEqual(result["introduced_uncovered_parts"], [])
        self.assertEqual(len(result["introduced_overlap_parts"]), 1)
        self.assertEqual(result["introduced_overlap_parts"][0]["owners"], ["A.poly", "Enclave.poly"])
        self.assertTrue(result["regressions"])

    def test_invalid_rings_close_implicitly_and_ignore_collapsed_segments(self):
        square = [(0, 0), (2, 0), (2, 2), (0, 2)]
        self.write(self.baseline, "A", square)
        points = [point for point in square * 3 for _ in range(2)]
        coordinates = "\n".join(f"{x} {y}" for x, y in points)
        (self.candidate / "A.poly").write_text(f"A\n1\n{coordinates}\nEND\nEND\n")
        result = audit(self.candidate, self.baseline)
        self.assertEqual(result["introduced_uncovered_parts"], [])
        self.assertEqual(result["unclosed_rings"], [{"file": "A.poly", "ring": "1"}])
        self.write(self.candidate, "A", [(0, 0), (0, 0), (0, 0)])
        _, shapes, _ = load(self.candidate)
        self.assertTrue(shapes[0].is_empty)

    def test_face_parity_is_not_changed_by_narrow_neck_tolerances(self):
        narrow = 1e-10
        points = np.array([(-1, -2), (1, -2), (1, -narrow), (narrow, -narrow),
                           (narrow, narrow), (1, narrow), (1, 2), (-1, 2),
                           (-1, narrow), (-narrow, narrow), (-narrow, -narrow), (-1, -narrow)])
        # The representative point is near a vertex and has tiny crossing
        # products, but the face on either side of the neck has substantial area.
        self.assertGreater(ring_coverage(np.tile(points, (3, 1)), False).area, 7.9)
        self.assertTrue(ring_coverage(np.tile(points, (2, 1)), False).is_empty)


if __name__ == "__main__":
    unittest.main()
