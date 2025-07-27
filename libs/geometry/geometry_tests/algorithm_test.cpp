#include "testing/testing.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <vector>

namespace algorithm_test
{
using namespace std;

using m2::CalculateBoundingBox;
using m2::CalculatePointOnSurface;
using m2::CalculatePolyLineCenter;
using m2::PointD;
using m2::RectD;

PointD GetPolyLineCenter(vector<PointD> const & points)
{
  return m2::ApplyCalculator(points, m2::CalculatePolyLineCenter());
}

RectD GetBoundingBox(vector<PointD> const & points)
{
  return m2::ApplyCalculator(points, m2::CalculateBoundingBox());
}

PointD GetPointOnSurface(vector<PointD> const & points)
{
  ASSERT(!points.empty() && points.size() % 3 == 0, ("points.size() =", points.size()));
  auto const boundingBox = GetBoundingBox(points);
  return m2::ApplyCalculator(points, m2::CalculatePointOnSurface(boundingBox));
}

bool PointsAlmostEqual(PointD const & p1, PointD const & p2)
{
  return p1.EqualDxDy(p2, mercator::kPointEqualityEps);
}

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
}  // namespace algorithm_test
