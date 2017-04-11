#include "testing/testing.hpp"

#include "coding/point_to_integer.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include "std/cmath.hpp"
#include "std/utility.hpp"

namespace
{
double const g_eps = MercatorBounds::GetCellID2PointAbsEpsilon();
uint32_t const g_coordBits = POINT_COORD_BITS;

void CheckEqualPoints(m2::PointD const & p1, m2::PointD const & p2)
{
  TEST(p1.EqualDxDy(p2, g_eps), (p1, p2));

  TEST_GREATER_OR_EQUAL(p1.x, -180.0, ());
  TEST_GREATER_OR_EQUAL(p1.y, -180.0, ());
  TEST_LESS_OR_EQUAL(p1.x, 180.0, ());
  TEST_LESS_OR_EQUAL(p1.y, 180.0, ());

  TEST_GREATER_OR_EQUAL(p2.x, -180.0, ());
  TEST_GREATER_OR_EQUAL(p2.y, -180.0, ());
  TEST_LESS_OR_EQUAL(p2.x, 180.0, ());
  TEST_LESS_OR_EQUAL(p2.y, 180.0, ());
}
}

UNIT_TEST(PointToInt64_Smoke)
{
  m2::PointD const arr[] = {{1.25, 1.3}, {180, 90}, {-180, -90}, {0, 0}};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    CheckEqualPoints(arr[i], Int64ToPoint(PointToInt64(arr[i], g_coordBits), g_coordBits));
}

UNIT_TEST(PointToInt64_Grid)
{
  int const delta = 5;
  for (int ix = -180; ix <= 180; ix += delta)
    for (int iy = -180; iy <= 180; iy += delta)
    {
      m2::PointD const pt(ix, iy);
      int64_t const id = PointToInt64(pt, g_coordBits);
      m2::PointD const pt1 = Int64ToPoint(id, g_coordBits);

      CheckEqualPoints(pt, pt1);

      int64_t const id1 = PointToInt64(pt1, g_coordBits);
      TEST_EQUAL(id, id1, (pt, pt1));
    }
}

UNIT_TEST(PointToInt64_Bounds)
{
  double const arrEps[] = {-1.0E-2, -1.0E-3, -1.0E-4, 0, 1.0E-4, 1.0E-3, 1.0E-2};

  m2::PointD const arrPt[] = {{0, 0},     {-180, -180}, {-180, 180}, {180, 180}, {180, -180},
                              {-90, -90}, {-90, 90},    {90, 90},    {90, -90}};

  for (size_t iP = 0; iP < ARRAY_SIZE(arrPt); ++iP)
    for (size_t iX = 0; iX < ARRAY_SIZE(arrEps); ++iX)
      for (size_t iY = 0; iY < ARRAY_SIZE(arrEps); ++iY)
      {
        m2::PointD const pt(arrPt[iP].x + arrEps[iX], arrPt[iP].y + arrEps[iY]);
        m2::PointD const pt1 = Int64ToPoint(PointToInt64(pt, g_coordBits), g_coordBits);

        TEST(fabs(pt.x - pt1.x) <= (fabs(arrEps[iX]) + g_eps) &&
                 fabs(pt.y - pt1.y) <= (fabs(arrEps[iY]) + g_eps),
             (pt, pt1));
      }
}

UNIT_TEST(PointD2PointU_Epsilons)
{
  m2::PointD const arrPt[] = {{-180, -180}, {-180, 180}, {180, 180}, {180, -180}};
  m2::PointD const arrD[] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
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
    m2::PointD const p3 = PointU2PointD(p2, g_coordBits);

    LOG(LINFO, ("Dx = ", p3.x - arrPt[i].x, "Dy = ", p3.y - arrPt[i].y));
  }
}
