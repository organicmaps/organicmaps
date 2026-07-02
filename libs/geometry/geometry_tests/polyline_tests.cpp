#include "testing/testing.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/rect2d.hpp"

#include <vector>

namespace polyline_tests
{
double constexpr kEps = 1e-5;

void TestClosest(std::vector<m2::PointD> const & points, m2::PointD const & point, double expectedSquaredDist,
                 uint32_t expectedIndex)
{
  auto const closestByPoints = m2::CalcMinSquaredDistance(points.begin(), points.end(), point);
  TEST_ALMOST_EQUAL_ABS(closestByPoints.first, expectedSquaredDist, kEps, ());
  TEST_EQUAL(closestByPoints.second, expectedIndex, ());

  m2::PolylineD const poly(points);
  auto const closestByPoly = poly.CalcMinSquaredDistance(m2::PointD(point));
  TEST_ALMOST_EQUAL_ABS(closestByPoly.first, expectedSquaredDist, kEps, ());
  TEST_EQUAL(closestByPoly.second, expectedIndex, ());
}

UNIT_TEST(Rect_PolylineSmokeTest)
{
  m2::PolylineD poly = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
  TEST_EQUAL(poly.GetSize(), 3, ());
  TEST_ALMOST_EQUAL_ABS(poly.GetLength(), 2.0, kEps, ());

  auto const limitRect = poly.GetLimitRect();
  TEST_ALMOST_EQUAL_ABS(limitRect.LeftBottom(), m2::PointD(0.0, 0.0), kEps, ());
  TEST(AlmostEqualAbs(limitRect.RightTop(), m2::PointD(1.0, 1.0), kEps), ());

  poly.PopBack();
  TEST_EQUAL(poly.GetSize(), 2, ());
}

// Naive reference: rescan the polyline from the start for every query. The scanner must match it.
m2::PointD PointByDistanceRef(m2::PolylineD const & poly, double distance)
{
  m2::PolylineDistanceScanner<double> scan(poly);
  return scan.GetPointByDistance(distance);
}

UNIT_TEST(Polyline_DistanceScanner_Ascending)
{
  // Segment lengths: 2, 3, 3 -> total length 8.
  m2::PolylineD const poly = {{0.0, 0.0}, {2.0, 0.0}, {2.0, 3.0}, {5.0, 3.0}};
  TEST_ALMOST_EQUAL_ABS(poly.GetLength(), 8.0, kEps, ());

  TEST_ALMOST_EQUAL_ABS(PointByDistanceRef(poly, 0), poly.GetPoint(0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(PointByDistanceRef(poly, 2), poly.GetPoint(1), kEps, ());
  TEST_ALMOST_EQUAL_ABS(PointByDistanceRef(poly, 5), poly.GetPoint(2), kEps, ());
  TEST_ALMOST_EQUAL_ABS(PointByDistanceRef(poly, 8), poly.GetPoint(3), kEps, ());

  m2::PolylineDistanceScanner<double> scan(poly);
  // Ascending queries, including out-of-range on both ends, exercise the fast forward path.
  for (double d = -1.0; d <= 9.0; d += 0.1)
    TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(d), PointByDistanceRef(poly, d), kEps, (d));
}

UNIT_TEST(Polyline_DistanceScanner_ArbitraryOrder)
{
  m2::PolylineD const poly = {{0.0, 0.0}, {2.0, 0.0}, {2.0, 3.0}, {5.0, 3.0}};
  m2::PolylineDistanceScanner<double> scan(poly);

  // A shared cursor must stay correct even when queries step backwards (the restart path).
  std::vector<double> const queries = {0.0, 5.0, 1.0, 8.0, 3.5, 2.0, 7.9, -0.5, 4.0, 2.0};
  for (double const d : queries)
    TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(d), PointByDistanceRef(poly, d), kEps, (d));
}

UNIT_TEST(Polyline_DistanceScanner_Edges)
{
  m2::PolylineD const poly = {{0.0, 0.0}, {10.0, 0.0}};
  m2::PolylineDistanceScanner<double> scan(poly);
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(-1.0), m2::PointD(0.0, 0.0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(0.0), m2::PointD(0.0, 0.0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(2.5), m2::PointD(2.5, 0.0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(10.0), m2::PointD(10.0, 0.0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(100.0), m2::PointD(10.0, 0.0), kEps, ());
}

UNIT_TEST(Polyline_DistanceScanner_ZeroLengthSegment)
{
  // A duplicate point makes a zero-length segment; the query at that distance must not divide by zero.
  m2::PolylineD const poly = {{0.0, 0.0}, {2.0, 0.0}, {2.0, 0.0}, {4.0, 0.0}};
  m2::PolylineDistanceScanner<double> scan(poly);
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(2.0), m2::PointD(2.0, 0.0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(scan.GetPointByDistance(3.0), m2::PointD(3.0, 0.0), kEps, ());
}

UNIT_TEST(Rect_PolylineMinDistanceTest)
{
  // 1 |              |
  //   |              |
  //   0----1----2----3
  std::vector<m2::PointD> const poly = {{0.0, 1.0}, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {3.0, 1.0}};

  TestClosest(poly, m2::PointD(0.0, 1.0), 0.0 /* expectedSquaredDist */, 0 /* expectedIndex */);
  TestClosest(poly, m2::PointD(0.0, 0.0), 0.0 /* expectedSquaredDist */, 0 /* expectedIndex */);
  TestClosest(poly, m2::PointD(0.1, 0.0), 0.0 /* expectedSquaredDist */, 1 /* expectedIndex */);
  TestClosest(poly, m2::PointD(0.5, 0.2), 0.2 * 0.2 /* expectedSquaredDist */, 1 /* expectedIndex */);
  TestClosest(poly, m2::PointD(1.5, 1.0), 1.0 /* expectedSquaredDist */, 2 /* expectedIndex */);
  TestClosest(poly, m2::PointD(1.5, -5.0), 5.0 * 5.0 /* expectedSquaredDist */, 2 /* expectedIndex */);
  TestClosest(poly, m2::PointD(1.5, 5.0), 4.0 * 4.0 + 1.5 * 1.5 /* expectedSquaredDist */, 0 /* expectedIndex */);
  TestClosest(poly, m2::PointD(3.0, 1.0), 0.0 /* expectedSquaredDist */, 4 /* expectedIndex */);
}
}  // namespace polyline_tests
