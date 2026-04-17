#include "testing/testing.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include <vector>

namespace algorithm_test
{
using namespace std;

using m2::CalculateBoundingBox;
using m2::CalculatePointOnSurface;
using m2::CalculatePolyLineCenter;
using m2::PointD;
using m2::RectD;

namespace
{
double constexpr kEqPointsEps = 1.0E-8;

PointD GetPolyLineCenter(vector<PointD> const & points)
{
  return m2::ApplyCalculatorPoly(points, m2::CalculatePolyLineCenter());
}

RectD GetBoundingBox(vector<PointD> const & points)
{
  return m2::ApplyCalculatorPoly(points, m2::CalculateBoundingBox());
}

PointD GetPointOnSurface(vector<PointD> const & points)
{
  auto const boundingBox = GetBoundingBox(points);
  return m2::ApplyCalculatorTrg(points, m2::CalculatePointOnSurface(boundingBox));
}

bool PointsAlmostEqual(PointD const & p1, PointD const & p2)
{
  return p1.EqualDxDy(p2, mercator::kPointEqualityEps);
}
}  // namespace

UNIT_TEST(CalculatePolyLineCenter)
{
  {
    vector<PointD> const points{{0, 0}, {1, 1}, {2, 2}};
    TEST_EQUAL(GetPolyLineCenter(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points{{0, 2}, {1, 1}, {2, 2}};
    TEST_EQUAL(GetPolyLineCenter(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points{
        {1, 1},
        {2, 2},
        {4, 4},
    };
    TEST_EQUAL(GetPolyLineCenter(points), PointD(2.5, 2.5), ());
  }
  {
    vector<PointD> const points{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    // Also logs a warning message.
    TEST_EQUAL(GetPolyLineCenter(points), PointD(0, 0), ());
  }
  {
    vector<PointD> const points{{0, 0}, {0, 0}, {0, 0}, {0, 0.000001}, {0, 0.000001}, {0, 0.000001}, {0, 0.000001}};
    // Also logs a warning message.
    TEST(GetPolyLineCenter(points).EqualDxDy(PointD(0, .0000005), 1e-7), ());
  }
}

UNIT_TEST(CalculateCenter)
{
  {
    vector<PointD> const points{{2, 2}};
    TEST_EQUAL(GetBoundingBox(points), RectD({2, 2}, {2, 2}), ());
  }
  {
    vector<PointD> const points{{0, 0}, {1, 1}, {2, 2}};
    TEST_EQUAL(GetBoundingBox(points), RectD({0, 0}, {2, 2}), ());
  }
  {
    vector<PointD> const points{{0, 2}, {1, 1}, {2, 2}, {1, 2}, {2, 2}, {2, 0}};
    TEST_EQUAL(GetBoundingBox(points), RectD({0, 0}, {2, 2}), ());
  }
}

UNIT_TEST(CalculatePointOnSurface)
{
  {
    vector<PointD> const points{{0, 0}, {1, 1}, {2, 2}};
    TEST_EQUAL(GetPointOnSurface(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points{{0, 2}, {1, 1}, {2, 2}, {1, 2}, {2, 2}, {2, 0}};
    TEST_EQUAL(GetPointOnSurface(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points{
        {0, 0}, {1, 1}, {2, 0}, {1, 1}, {2, 0}, {3, 1},  // Center of this triangle is used as a result.
        {2, 0}, {3, 1}, {4, 0},

        {4, 0}, {3, 1}, {4, 2}, {3, 1}, {4, 2}, {3, 3},  // Or this.
        {4, 2}, {3, 3}, {4, 4},

        {3, 3}, {4, 4}, {2, 4}, {3, 3}, {2, 4}, {1, 3},  // Or this.
        {1, 3}, {2, 4}, {0, 4},

        {0, 4}, {1, 3}, {0, 2}, {1, 3}, {0, 2}, {1, 1},  // Or this
        {0, 2}, {1, 1}, {0, 0},
    };
    auto const result = GetPointOnSurface(points);
    TEST(PointsAlmostEqual(result, {10.0 / 3.0, 2}) || PointsAlmostEqual(result, {2, 2.0 / 3.0}) ||
             PointsAlmostEqual(result, {2, 10.0 / 3.0}) || PointsAlmostEqual(result, {2.0 / 3.0, 2}),
         ("result = ", result));
  }
}

UNIT_TEST(InterpolatePointAtDistance_NegativeDistance)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, -5.0), points.front(), kEqPointsEps, ());
}

UNIT_TEST(InterpolatePointAtDistance_BeyondEnd)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 100.0), points.back(), kEqPointsEps, ());
}

UNIT_TEST(InterpolatePointAtDistance_Midpoint)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.5), PointD(0.5, 0), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 1.5), PointD(1.5, 0), kEqPointsEps, ());
}

// Duplicated points don't cause issues because the algorithm interpolates
// by cumulative distances, not by pairwise point gaps.
UNIT_TEST(InterpolatePointAtDistance_DuplicatedPoints)
{
  vector<PointD> const points = {{0, 0}, {0, 0}, {1, 0}};
  vector<double> const distances = {0.0, 1.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.5), PointD(0.5, 0), kEqPointsEps, ());
}

UNIT_TEST(InterpolatePointAtDistance_ZeroDistance)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.0), points.front(), kEqPointsEps, ());
}

UNIT_TEST(InterpolatePointAtDistance_ExactlyAtTotal)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 2.0), points.back(), kEqPointsEps, ());
}

// Hitting an interior vertex exactly: upper_bound lands on that distance, returning the vertex.
UNIT_TEST(InterpolatePointAtDistance_ExactlyAtInteriorVertex)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {2, 0}};
  vector<double> const distances = {1.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 1.0), PointD(1, 0), kEqPointsEps, ());
}

// Minimal polyline (single segment).
UNIT_TEST(InterpolatePointAtDistance_TwoPoints)
{
  vector<PointD> const points = {{0, 0}, {10, 0}};
  vector<double> const distances = {10.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, -1.0), points.front(), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.0), points.front(), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 2.5), PointD(2.5, 0), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 10.0), points.back(), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 20.0), points.back(), kEqPointsEps, ());
}

// Non-axis-aligned polyline: linear interpolation stays on the segment.
UNIT_TEST(InterpolatePointAtDistance_Diagonal)
{
  vector<PointD> const points = {{0, 0}, {3, 4}};  // length = 5
  vector<double> const distances = {5.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 2.5), PointD(1.5, 2.0), kEqPointsEps, ());
}

// Segments with different lengths: interpolation is proportional within each segment.
UNIT_TEST(InterpolatePointAtDistance_UnequalSegments)
{
  vector<PointD> const points = {{0, 0}, {1, 0}, {1, 10}};
  vector<double> const distances = {1.0, 11.0};  // segments of length 1, 10

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.25), PointD(0.25, 0), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 6.0), PointD(1, 5), kEqPointsEps, ());
}

// Change of direction between segments: ensure we pick the right segment at the join.
UNIT_TEST(InterpolatePointAtDistance_LShape)
{
  vector<PointD> const points = {{0, 0}, {10, 0}, {10, 10}};
  vector<double> const distances = {10.0, 20.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 5.0), PointD(5, 0), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 15.0), PointD(10, 5), kEqPointsEps, ());
}

// Leading zero distance (e.g. the first segment is degenerate) must not break upper_bound.
UNIT_TEST(InterpolatePointAtDistance_LeadingZeroDistance)
{
  vector<PointD> const points = {{0, 0}, {0, 0}, {2, 0}};
  vector<double> const distances = {0.0, 2.0};

  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 0.0), points.front(), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 1.0), PointD(1, 0), kEqPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(m2::InterpolatePointAtDistance(distances, points, 2.0), points.back(), kEqPointsEps, ());
}
}  // namespace algorithm_test
