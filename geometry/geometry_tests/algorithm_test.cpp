#include "testing/testing.hpp"

#include "geometry/algorithm.hpp"

#include "std/vector.hpp"

using m2::PointD;
using m2::RectD;
using m2::CalculatePolyLineCenter;
using m2::CalculatePointOnSurface;
using m2::CalculateBoundingBox;

namespace
{
PointD GetPolyLineCenter(vector<PointD> const & points)
{
  CalculatePolyLineCenter doCalc;
  for (auto const & p : points)
    doCalc(p);
  return doCalc.GetCenter();
}

RectD GetBoundingBox(vector<PointD> const & points)
{
  CalculateBoundingBox doCalc;
  for (auto const p : points)
    doCalc(p);
  return doCalc.GetBoundingBox();
}

PointD GetPointOnSurface(vector<PointD> const & points)
{
  ASSERT(!points.empty() && points.size() % 3 == 0, ());
  CalculatePointOnSurface doCalc(GetBoundingBox(points));
  for (auto i = 0; i < points.size() - 3; i += 3)
    doCalc(points[i], points[i + 1], points[i + 2]);
  return doCalc.GetCenter();

}
}  // namespace

UNIT_TEST(CalculatePolyLineCenter)
{
  {
    vector<PointD> const points {
      {0, 0},
      {1, 1},
      {2, 2}
    };
    TEST_EQUAL(GetPolyLineCenter(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points {
      {0, 2},
      {1, 1},
      {2, 2}
    };
    TEST_EQUAL(GetPolyLineCenter(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points {
      {1, 1},
      {2, 2},
      {4, 4},
    };
    TEST_EQUAL(GetPolyLineCenter(points), PointD(2.5, 2.5), ());
  }
  {
    vector<PointD> const points {
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0}
    };
    // Also logs a warning message.
    TEST_EQUAL(GetPolyLineCenter(points), PointD(0, 0), ());
  }
  {
    vector<PointD> const points {
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0.000001},
      {0, 0.000001},
      {0, 0.000001},
      {0, 0.000001}
    };
    // Also logs a warning message.
    TEST(GetPolyLineCenter(points).EqualDxDy(PointD(0, .0000005), 1e-7), ());
  }
}

UNIT_TEST(CalculateCenter)
{
  {
    vector<PointD> const points {
      {2, 2}
    };
    TEST_EQUAL(GetBoundingBox(points), RectD({2, 2},  {2, 2}), ());
  }
  {
    vector<PointD> const points {
      {0, 0},
      {1, 1},
      {2, 2}
    };
    TEST_EQUAL(GetBoundingBox(points), RectD({0, 0}, {2, 2}), ());
  }
  {
    vector<PointD> const points {
      {0, 2},
      {1, 1},
      {2, 2},
      {1, 2},
      {2, 2},
      {2, 0}
    };
    TEST_EQUAL(GetBoundingBox(points), RectD({0, 0}, {2, 2}), ());
  }
}

UNIT_TEST(CalculatePointOnSurface)
{
  {
    vector<PointD> const points {
      {0, 0},
      {1, 1},
      {2, 2}
    };
    TEST_EQUAL(GetPointOnSurface(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points {
      {0, 2},
      {1, 1},
      {2, 2},
      {1, 2},
      {2, 2},
      {2, 0}
    };
    TEST_EQUAL(GetPointOnSurface(points), PointD(1, 1), ());
  }
  {
    vector<PointD> const points {
      {0, 0}, {1, 1}, {2, 0},
      {1, 1}, {2, 0}, {3, 1},
      {2, 0}, {3, 1}, {4, 0},

      {4, 0}, {3, 1}, {4, 2},
      {3, 1}, {4, 2}, {3, 3},
      {4, 2}, {3, 3}, {4, 4},

      {3, 3}, {4, 4}, {2, 4},
      {3, 3}, {2, 4}, {1, 3},
      {1, 3}, {2, 4}, {0, 4},

      {0, 4}, {1, 3}, {0, 2},
      {1, 3}, {0, 2}, {1, 1},  // Center of this triangle is used as a result.
      {0, 2}, {1, 1}, {0, 0},
    };
    TEST_EQUAL(GetPointOnSurface(points), PointD(2.0 / 3.0, 2), ());
  }
}
