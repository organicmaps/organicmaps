#include "../../testing/testing.hpp"
#include "../cell_id.hpp"
#include "../../std/cmath.hpp"


namespace
{
  double const eps = 0.000001;

  void CheckEqualPoints(CoordPointT const & p1, CoordPointT const & p2)
  {
    TEST(fabs(p1.first - p2.first) < eps &&
         fabs(p1.second - p2.second) < eps,
        (p1, p2));

    TEST_GREATER_OR_EQUAL(p1.first, -180.0, ());
    TEST_GREATER_OR_EQUAL(p1.second, -180.0, ());
    TEST_LESS_OR_EQUAL(p1.first, 180.0, ());
    TEST_LESS_OR_EQUAL(p1.second, 180.0, ());

    TEST_GREATER_OR_EQUAL(p2.first, -180.0, ());
    TEST_GREATER_OR_EQUAL(p2.second, -180.0, ());
    TEST_LESS_OR_EQUAL(p2.first, 180.0, ());
    TEST_LESS_OR_EQUAL(p2.second, 180.0, ());
  }

  struct pod_point_t
  {
    double x, y;
  };
}

UNIT_TEST(PointToInt64_Smoke)
{
  pod_point_t arr[] = { {1.25, 1.3}, {180, 90}, {-180, -90}, {0, 0} };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    CoordPointT p(arr[i].x, arr[i].y);
    CheckEqualPoints(p, Int64ToPoint(PointToInt64(p)));
  }
}

UNIT_TEST(PointToInt64_908175295886057813)
{
  int64_t const id1 = 908175295886057813LL;
  CoordPointT const pt1 = Int64ToPoint(id1);
  int64_t const id2 = PointToInt64(pt1);
  TEST_EQUAL(id1, id2, (pt1));
}

UNIT_TEST(PointToInt64_Grid)
{
  int delta = 5;
  for (int ix = -180; ix <= 180; ix += delta)
  {
    for (int iy = -180; iy <= 180; iy += delta)
    {
      CoordPointT const pt(ix, iy);
      int64_t const id = PointToInt64(pt);
      CoordPointT const pt1 = Int64ToPoint(id);

      CheckEqualPoints(pt, pt1);

      int64_t const id1 = PointToInt64(pt1);
      TEST_EQUAL(id, id1, (pt, pt1));
    }
  }
}

UNIT_TEST(PointToInt64_Bounds)
{
  double arrEps[] = { -1.0E-2, -1.0E-3, -1.0E-4, 0, 1.0E-4, 1.0E-3, 1.0E-2 };

  pod_point_t arrPt[] = { {-180, -180}, {-180, 180}, {180, 180}, {180, -180} };

  for (size_t iP = 0; iP < ARRAY_SIZE(arrPt); ++iP)
  {
    for (size_t iE = 0; iE < ARRAY_SIZE(arrEps); ++iE)
    {
      CoordPointT const pt(arrPt[iP].x, arrPt[iP].y);
      CheckEqualPoints(pt, Int64ToPoint(PointToInt64(pt)));
    }
  }
}
