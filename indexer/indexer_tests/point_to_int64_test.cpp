#include "../../testing/testing.hpp"
#include "../cell_id.hpp"
#include "../../std/cmath.hpp"

UNIT_TEST(PointToInt64_Simple)
{
  CoordPointT orig(1.25, 1.3);
  CoordPointT conv = Int64ToPoint(PointToInt64(orig.first, orig.second));
  TEST(fabs(orig.first  - conv.first ) < 0.000001 &&
       fabs(orig.second - conv.second) < 0.000001,
       (orig, conv));
}

UNIT_TEST(PointToInt64_Border)
{
  CoordPointT orig(180, 90);
  CoordPointT conv = Int64ToPoint(PointToInt64(orig.first, orig.second));
  TEST(fabs(orig.first  - conv.first ) < 0.000001 &&
       fabs(orig.second - conv.second) < 0.000001,
       (orig, conv));
}

UNIT_TEST(PointToInt64_908175295886057813)
{
  int64_t const id1 = 908175295886057813LL;
  CoordPointT const pt1 = Int64ToPoint(id1);
  int64_t const id2 = PointToInt64(pt1);
  TEST_EQUAL(id1, id2, (pt1));
}

UNIT_TEST(PointToInt64_MinMax)
{
  for (int ix = -180; ix <= 180; ix += 180)
  {
    for (int iy = -180; iy <= 180; iy += 180)
    {
      CoordPointT const pt(ix, iy);
      int64_t const id = PointToInt64(pt);
      CoordPointT const pt1 = Int64ToPoint(id);
      TEST_LESS(fabs(pt1.first  - pt.first ), 0.000001, (pt, pt1, id));
      TEST_LESS(fabs(pt1.second - pt.second), 0.000001, (pt, pt1, id));
      TEST_LESS(pt1.first,  180.000001, ());
      TEST_LESS(pt1.second, 180.000001, ());
      int64_t const id1 = PointToInt64(pt1);
      TEST_EQUAL(id, id1, (pt, pt1));
    }
  }
}
