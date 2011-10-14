#include "../../testing/testing.hpp"

#include "test_polylines.hpp"

#include "../cell_id.hpp"

#include "../../base/logging.hpp"

#include "../../std/cmath.hpp"
#include "../../std/utility.hpp"


namespace
{
  double const g_eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  uint32_t const g_coordBits = POINT_COORD_BITS;

  void CheckEqualPoints(CoordPointT const & p1, CoordPointT const & p2)
  {
    TEST(fabs(p1.first - p2.first) < g_eps &&
         fabs(p1.second - p2.second) < g_eps,
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
    CheckEqualPoints(p, Int64ToPoint(PointToInt64(p, g_coordBits), g_coordBits));
  }
}

UNIT_TEST(PointToInt64_Grid)
{
  int const delta = 5;
  for (int ix = -180; ix <= 180; ix += delta)
    for (int iy = -180; iy <= 180; iy += delta)
    {
      CoordPointT const pt(ix, iy);
      int64_t const id = PointToInt64(pt, g_coordBits);
      CoordPointT const pt1 = Int64ToPoint(id, g_coordBits);

      CheckEqualPoints(pt, pt1);

      int64_t const id1 = PointToInt64(pt1, g_coordBits);
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
        CoordPointT const pt1 = Int64ToPoint(PointToInt64(pt, g_coordBits), g_coordBits);

        TEST(fabs(pt.first - pt1.first) <= (fabs(arrEps[iX]) + g_eps) &&
             fabs(pt.second - pt1.second) <= (fabs(arrEps[iY]) + g_eps), (pt, pt1));
      }
}

UNIT_TEST(PointToInt64_DataSet1)
{
  for (size_t i = 0; i < ARRAY_SIZE(index_test::arr1); ++i)
  {
    CoordPointT const pt(index_test::arr1[i].x, index_test::arr1[i].y);
    int64_t const id = PointToInt64(pt, g_coordBits);
    CoordPointT const pt1 = Int64ToPoint(id, g_coordBits);

    CheckEqualPoints(pt, pt1);

    int64_t const id1 = PointToInt64(pt1, g_coordBits);
    TEST_EQUAL(id, id1, (pt, pt1));
  }
}

UNIT_TEST(PointD2PointU_Epsilons)
{
  pod_point_t arrPt[] = { {-180, -180}, {-180, 180}, {180, 180}, {180, -180} };
  pod_point_t arrD[] = { {1, 1}, {1, -1}, {-1, -1}, {-1, 1} };
  size_t const count = ARRAY_SIZE(arrPt);

  /*
  double eps = 1.0;
  for (; true; eps = eps / 10.0)
  {
    size_t i = 0;
    for (; i < count; ++i)
    {
      m2::PointU p = PointD2PointU(arrPt[i].x, arrPt[i].y, g_coordBits);
      m2::PointU p1 = PointD2PointU(arrPt[i].x + arrD[i].x * eps,
                                    arrPt[i].y + arrD[i].y * eps,
                                    g_coordBits);

      if (p != p1) break;
    }
    if (i == count) break;
  }

  LOG(LINFO, ("Epsilon = ", eps));
  */

  for (size_t i = 0; i < count; ++i)
  {
    m2::PointU const p1 = PointD2PointU(arrPt[i].x, arrPt[i].y, g_coordBits);
    m2::PointU const p2(p1.x + arrD[i].x, p1.y + arrD[i].y);
    CoordPointT const p3 = PointU2PointD(p2, g_coordBits);

    LOG(LINFO, ("Dx = ", p3.first - arrPt[i].x, "Dy = ", p3.second - arrPt[i].y));
  }
}
