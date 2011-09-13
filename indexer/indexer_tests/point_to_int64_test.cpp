#include "../../testing/testing.hpp"

#include "test_polylines.hpp"

#include "../cell_id.hpp"

#include "../../std/cmath.hpp"


namespace
{
  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  uint32_t const coordBits = POINT_COORD_BITS;

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
    CheckEqualPoints(p, Int64ToPoint(PointToInt64(p, coordBits), coordBits));
  }
}

UNIT_TEST(PointToInt64_Grid)
{
  int const delta = 5;
  for (int ix = -180; ix <= 180; ix += delta)
    for (int iy = -180; iy <= 180; iy += delta)
    {
      CoordPointT const pt(ix, iy);
      int64_t const id = PointToInt64(pt, coordBits);
      CoordPointT const pt1 = Int64ToPoint(id, coordBits);

      CheckEqualPoints(pt, pt1);

      int64_t const id1 = PointToInt64(pt1, coordBits);
      TEST_EQUAL(id, id1, (pt, pt1));
    }
}

UNIT_TEST(PointToInt64_Bounds)
{
  double arrEps[] = { -1.0E-2, -1.0E-3, -1.0E-4, 0, 1.0E-4, 1.0E-3, 1.0E-2 };

  pod_point_t arrPt[] = { {0, 0},
    {-180, -180}, {-180, 180}, {180, 180}, {180, -180},
    {-90, -90}, {-90, 90}, {90, 90}, {90, -90}
  };

  for (size_t iP = 0; iP < ARRAY_SIZE(arrPt); ++iP)
    for (size_t iX = 0; iX < ARRAY_SIZE(arrEps); ++iX)
      for (size_t iY = 0; iY < ARRAY_SIZE(arrEps); ++iY)
      {
        CoordPointT const pt(arrPt[iP].x + arrEps[iX], arrPt[iP].y + arrEps[iY]);
        CoordPointT const pt1 = Int64ToPoint(PointToInt64(pt, coordBits), coordBits);

        TEST(fabs(pt.first - pt1.first) <= (fabs(arrEps[iX]) + eps) &&
             fabs(pt.second - pt1.second) <= (fabs(arrEps[iY]) + eps), (pt, pt1));
      }
}

UNIT_TEST(PointToInt64_DataSet1)
{
  for(size_t i = 0; i < ARRAY_SIZE(index_test::arr1); ++i)
  {
    CoordPointT const pt(index_test::arr1[i].x, index_test::arr1[i].y);
    int64_t const id = PointToInt64(pt, coordBits);
    CoordPointT const pt1 = Int64ToPoint(id, coordBits);

    CheckEqualPoints(pt, pt1);

    int64_t const id1 = PointToInt64(pt1, coordBits);
    TEST_EQUAL(id, id1, (pt, pt1));
  }
}
