#include "testing/testing.hpp"

#include "geometry/robust_orientation.hpp"
#include "geometry/triangle2d.hpp"


using namespace m2::robust;

namespace
{
  using P = m2::PointD;

  template <typename IterT> void CheckSelfIntersections(IterT beg, IterT end, bool res)
  {
    TEST_EQUAL(CheckPolygonSelfIntersections(beg, end), res, ());
    using ReverseIterT = reverse_iterator<IterT>;
    TEST_EQUAL(CheckPolygonSelfIntersections(ReverseIterT(end), ReverseIterT(beg)), res, ());
  }
}

UNIT_TEST(OrientedS_Smoke)
{
  P arr[] = {{-1, -1}, {0, 0}, {1, -1}};
  TEST(OrientedS(arr[0], arr[2], arr[1]) > 0, ());
  TEST(OrientedS(arr[2], arr[0], arr[1]) < 0, ());
}

UNIT_TEST(Triangle_Smoke)
{
  P arr[] = {{0, 0}, {0, 3}, {3, 0}};

  TEST(IsPointInsideTriangle(arr[0], arr[0], arr[1], arr[2]), ());
  TEST(IsPointInsideTriangle(arr[1], arr[0], arr[1], arr[2]), ());
  TEST(IsPointInsideTriangle(arr[2], arr[0], arr[1], arr[2]), ());
  TEST(IsPointInsideTriangle({1, 1}, arr[0], arr[1], arr[2]), ());
  TEST(IsPointInsideTriangle({1, 2}, arr[0], arr[1], arr[2]), ());
  TEST(IsPointInsideTriangle({2, 1}, arr[0], arr[1], arr[2]), ());

  double constexpr eps = 1.0E-10;
  TEST(!IsPointInsideTriangle({-eps, -eps}, arr[0], arr[1], arr[2]), ());
  TEST(!IsPointInsideTriangle({1 + eps, 2}, arr[0], arr[1], arr[2]), ());
  TEST(!IsPointInsideTriangle({2, 1 + eps}, arr[0], arr[1], arr[2]), ());
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
