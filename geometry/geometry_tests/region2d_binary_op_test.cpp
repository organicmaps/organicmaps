#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../region2d/binary_operators.hpp"


namespace
{
  typedef m2::PointI P;
  typedef m2::RegionI R;
}

UNIT_TEST(RegionIntersect_Smoke)
{
  {
    P arr1[] = { P(-2, 1), P(2, 1), P(2, -1), P(-2, -1) };
    P arr2[] = { P(-1, 2), P(1, 2), P(1, -2), P(-1, -2) };

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::IntersectRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(res[0].GetRect(), m2::RectI(-1, -1, 1, 1), ());
  }

  {
    P arr1[] = { P(0, 0), P(1, 1), P(2, 0) };
    P arr2[] = { P(0, 0), P(1, -1), P(2, 0) };

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::IntersectRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 0, ());
  }
}
