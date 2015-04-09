#include "testing/testing.hpp"

#include "geometry/robust_orientation.hpp"


typedef m2::PointD P;

namespace
{
  template <typename IterT> void CheckSelfIntersections(IterT beg, IterT end, bool res)
  {
    TEST_EQUAL(m2::robust::CheckPolygonSelfIntersections(beg, end), res, ());
    typedef std::reverse_iterator<IterT> ReverseIterT;
    TEST_EQUAL(m2::robust::CheckPolygonSelfIntersections(ReverseIterT(end), ReverseIterT(beg)), res, ());
  }
}

UNIT_TEST(PolygonSelfIntersections_IntersectSmoke)
{
  {
    P arr[] = { P(0, 1), P(2, -1), P(2, 1), P(0, -1) };
    CheckSelfIntersections(&arr[0], arr + ARRAY_SIZE(arr), true);
  }
}

UNIT_TEST(PolygonSelfIntersections_TangentSmoke)
{
  {
    P arr[] = { P(0, 1), P(1, 0), P(2, 1), P(2, -1), P(1, 0), P(0, -1) };
    CheckSelfIntersections(&arr[0], arr + ARRAY_SIZE(arr), false);
  }

  {
    P arr[] = { P(0, 0), P(2, 0), P(2, 1), P(1, 0), P(0, 1) };
    CheckSelfIntersections(&arr[0], arr + ARRAY_SIZE(arr), false);
  }
}
