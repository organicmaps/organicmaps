import unittest

from shapely import STRtree, prepare
from shapely.geometry import GeometryCollection, LineString, MultiPoint, Point, Polygon

from audit_borders import area_m2, uncovered_loss
from border_geometry import measurement_difference, union_coverage


class MeasurementTests(unittest.TestCase):
    def test_coverage_union_handles_nearly_collinear_overlay_remnants(self):
        # Reduced Northwest Territories North/Nunavut South measurement results.
        # Mixed union fails in GEOS 3.11 and discards polygon area in GEOS 3.14.
        first = GeometryCollection([
            LineString([(-120.6828, 93.8477210655052), (-120.683, 98.15365076910341)]),
            Polygon([(-120.66860161897527, 98.15461540439692), (-120.67176041022279, 98.15440377772049),
                     (-120.66702653206724, 98.15472092906802), (-120.66702653206882, 98.1547209290679)]),
            Polygon([(-120.65022085003486, 98.15584684412696), (-120.65435392565752, 98.15556994420261),
                     (-120.65435392565817, 98.15556994420257), (-120.65108273692378, 98.15578910107163)]),
        ])
        second = GeometryCollection([
            Polygon([(-120.66702653206643, 98.15472092906806), (-120.66860161897527, 98.15461540439692),
                     (-120.66702653206592, 98.1547209290681)]),
            Polygon([(-120.65022085003486, 98.15584684412696), (-120.65435392565912, 98.1555699442025),
                     (-120.6543539256601, 98.15556994420244), (-120.65022085003339, 98.15584684412705)]),
            Polygon([(-120.63929828590689, 98.15657861326373), (-120.64927629764748, 98.1559101254478),
                     (-120.64927629764868, 98.15591012544773), (-120.63929828590669, 98.15657861326375)]),
        ])
        self.assertTrue(first.is_valid)
        self.assertTrue(second.is_valid)
        merged = union_coverage([first, second])
        self.assertTrue(merged.is_valid)
        self.assertGreater(merged.area, 0)

    def test_coverage_union_keeps_small_areas_without_double_counting(self):
        first = Polygon([(0, 0), (2e-9, 0), (2e-9, 1), (0, 1)])
        second = Polygon([(1e-9, 0), (3e-9, 0), (3e-9, 1), (1e-9, 1)])
        remnants = [LineString([(0, 0), (10, 10)]), Point(20, 20)]
        merged = union_coverage([GeometryCollection([first, *remnants]),
                                 GeometryCollection([second, *remnants])])
        self.assertTrue(merged.is_valid)
        self.assertGreater(area_m2(merged), .01)
        self.assertAlmostEqual(merged.area, 3e-9, delta=1e-16)
        self.assertEqual(merged.bounds, (0, 0, 3e-9, 1))

    def test_point_on_line_in_mixed_collection_is_inherited(self):
        point = Point(13.58948, 56.31274395355062)
        old = GeometryCollection([LineString([(point.x, point.y), (13.58949, point.y)]), Point(13, 56)])
        self.assertTrue(old.covers(point))
        self.assertTrue(measurement_difference(point, old).is_empty)
        new = GeometryCollection([point, Point(12, 55)])
        self.assertTrue(measurement_difference(new, old).equals(Point(12, 55)))

    def test_mixed_collection_preserves_new_points_and_line_parts(self):
        old = GeometryCollection([LineString([(0, 0), (2, 0)]), Point(5, 0)])
        new = GeometryCollection([LineString([(1, 0), (3, 0)]), MultiPoint([(1, 0), (5, 0), (6, 0)])])
        difference = measurement_difference(new, old)
        self.assertTrue(difference.equals(GeometryCollection([LineString([(2, 0), (3, 0)]), Point(6, 0)])))

    def test_near_collinear_source_area_is_retained_by_exact_shared_neighbor(self):
        # Exact shared A-C segments retain the source coverage; prepared predicates
        # can disagree with overlays on near-collinear boundaries.
        a = (31.21306, -14.84761133688672)
        b = (31.21999, -14.846587911840558)
        c = (31.23056, -14.845026939127916)
        below = [(c[0], c[1] - .01), (a[0], a[1] - .01)]
        old = Polygon([a, b, c, *below])
        current = Polygon([a, c, *below])
        neighbor = Polygon([c, a, (a[0], a[1] + .01), (c[0], c[1] + .01)])
        occupants = [current, neighbor]
        prepare(occupants)
        lost = uncovered_loss(old, current, occupants, STRtree(occupants))
        self.assertLess(area_m2(lost), .01)

    def test_real_submillimetre_gap_is_not_rounded_away(self):
        old = Polygon([(0, 0), (1, 0), (1, 1), (0, 1)])
        current = Polygon([(0, 0), (.5, 0), (.5, 1), (0, 1)])
        neighbor = Polygon([(.5 + 1e-9, 0), (1, 0), (1, 1), (.5 + 1e-9, 1)])
        occupants = [current, neighbor]
        lost = uncovered_loss(old, current, occupants, STRtree(occupants))
        self.assertGreater(area_m2(lost), .01)
        self.assertAlmostEqual(lost.area, 1e-9, delta=1e-16)
