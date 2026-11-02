#include "testing/testing.hpp"

#include "geometry/mercator.hpp"
#include "geometry/smoothing.hpp"

#include <vector>

namespace smoothing_test
{
using m2::PointD;
using std::vector;

double constexpr kMaxSegmentMeters = 1.5 * mercator::Bounds::kMetersInDegree;

UNIT_TEST(Smoothing_ShortSegmentsStaircase)
{
  // A diagonal contour traced along the lattice cells: the steps alternate around x - y = 0.5.
  vector<PointD> path;
  for (int i = 0; i < 8; ++i)
    path.insert(path.end(), {{double(i), double(i)}, {i + 1.0, double(i)}});
  path.emplace_back(8, 8);

  auto smoothed = path;
  m2::SmoothShortSegments(smoothed, kMaxSegmentMeters, 2 /* passes */);
  TEST_EQUAL(smoothed.size(), path.size(), ());
  TEST_EQUAL(smoothed.front(), path.front(), ());
  TEST_EQUAL(smoothed.back(), path.back(), ());
  // The first pass straightens the steps, the second one blends in the fixed ends; the vertices
  // keep their even spacing along the line.
  for (size_t i = 1; i + 1 < smoothed.size(); ++i)
  {
    double const offset = (i == 1 || i + 2 == smoothed.size()) ? 0.375 : 0.5;
    TEST_ALMOST_EQUAL_ABS(smoothed[i].x - smoothed[i].y, offset, 1e-12, (i));
    TEST_ALMOST_EQUAL_ABS(smoothed[i].x + smoothed[i].y, double(i), 1e-12, (i));
  }
}

UNIT_TEST(Smoothing_ShortSegmentsKeepLongCorners)
{
  // The vertices joined to a long segment stay, the other short-segment ones move.
  vector<PointD> const path = {{0, 0}, {1, 0}, {1, 1}, {11, 1}, {12, 1}, {12, 2}};
  auto smoothed = path;
  m2::SmoothShortSegments(smoothed, kMaxSegmentMeters, 3 /* passes */);
  TEST_EQUAL(smoothed[0], path[0], ());
  TEST_NOT_EQUAL(smoothed[1], path[1], ());
  TEST_EQUAL(smoothed[2], path[2], ());
  TEST_EQUAL(smoothed[3], path[3], ());
  TEST_NOT_EQUAL(smoothed[4], path[4], ());
  TEST_EQUAL(smoothed[5], path[5], ());

  // No short segments - nothing changes.
  vector<PointD> const corner = {{0, 0}, {10, 0}, {10, 10}};
  smoothed = corner;
  m2::SmoothShortSegments(smoothed, kMaxSegmentMeters, 3 /* passes */);
  TEST_EQUAL(smoothed, corner, ());
}

UNIT_TEST(Smoothing_ShortSegmentsClosedRing)
{
  // The ring wraps: every corner of the square moves symmetrically, the ring stays closed.
  vector<PointD> smoothed = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}};
  m2::SmoothShortSegments(smoothed, kMaxSegmentMeters, 1 /* passes */);
  vector<PointD> const expected = {{0.25, 0.25}, {0.75, 0.25}, {0.75, 0.75}, {0.25, 0.75}, {0.25, 0.25}};
  TEST_EQUAL(smoothed.size(), expected.size(), ());
  for (size_t i = 0; i < expected.size(); ++i)
    TEST(smoothed[i].EqualDxDy(expected[i], 1e-12), (i, smoothed[i]));
}

UNIT_TEST(Smoothing_ShortSegmentsFollowLatitude)
{
  // A mercator degree at 60 degrees is half as long on the ground, whichever tile reads the path.
  double const y = mercator::LatToY(60.0);
  vector<PointD> const path = {{0, y}, {0.75, y}, {1.25, y + 0.5}};
  auto smoothed = path;
  m2::SmoothShortSegments(smoothed, mercator::Bounds::kMetersInDegree / 2, 1 /* passes */);
  TEST_NOT_EQUAL(smoothed[1], path[1], ());

  vector<PointD> const equator = {{0, 0}, {0.75, 0}, {1.25, 0.5}};
  smoothed = equator;
  m2::SmoothShortSegments(smoothed, mercator::Bounds::kMetersInDegree / 2, 1 /* passes */);
  TEST_EQUAL(smoothed, equator, ());
}

UNIT_TEST(Smoothing_ShortSegmentsDegenerate)
{
  for (vector<PointD> const & path : vector<vector<PointD>>{{}, {{0, 0}}, {{0, 0}, {1, 1}}, {{0, 0}, {1, 0}, {0, 0}}})
  {
    auto smoothed = path;
    m2::SmoothShortSegments(smoothed, kMaxSegmentMeters, 2 /* passes */);
    TEST_EQUAL(smoothed, path, ());
  }
}
}  // namespace smoothing_test
