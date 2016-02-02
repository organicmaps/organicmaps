#include "testing/testing.hpp"

#include "geometry/robust_orientation.hpp"
#include "geometry/segment2d.hpp"
#include "geometry/triangle2d.hpp"


using namespace m2::robust;

namespace
{
using P = m2::PointD;

template <typename IterT>
void CheckSelfIntersections(IterT beg, IterT end, bool res)
{
  TEST_EQUAL(CheckPolygonSelfIntersections(beg, end), res, ());
  using ReverseIterT = reverse_iterator<IterT>;
  TEST_EQUAL(CheckPolygonSelfIntersections(ReverseIterT(end), ReverseIterT(beg)), res, ());
}

bool InsideSegment(P const & p, P const ps[])
{
  return IsPointInsideSegment(p, ps[0], ps[1]);
}

bool InsideTriangle(P const & p, P const ps[])
{
  return IsPointInsideTriangle(p, ps[0], ps[1], ps[2]);
}
}  // namespace

UNIT_TEST(OrientedS_Smoke)
{
  P arr[] = {{-1, -1}, {0, 0}, {1, -1}};
  TEST(OrientedS(arr[0], arr[2], arr[1]) > 0, ());
  TEST(OrientedS(arr[2], arr[0], arr[1]) < 0, ());
}

UNIT_TEST(Segment_Smoke)
{
  double constexpr eps = 1.0E-10;
  {
    P ps[] = {{0, 0}, {1, 0}};
    TEST(InsideSegment(ps[0], ps), ());
    TEST(InsideSegment(ps[1], ps), ());
    TEST(InsideSegment(P(0.5, 0), ps), ());

    TEST(InsideSegment(P(eps, 0), ps), ());
    TEST(InsideSegment(P(1.0 - eps, 0), ps), ());

    TEST(!InsideSegment(P(-eps, 0), ps), ());
    TEST(!InsideSegment(P(1.0 + eps, 0), ps), ());
    TEST(!InsideSegment(P(eps, eps), ps), ());
    TEST(!InsideSegment(P(eps, -eps), ps), ());
  }

  {
    P ps[] = {{10, 10}, {10, 10}};
    TEST(InsideSegment(ps[0], ps), ());
    TEST(InsideSegment(ps[1], ps), ());
    TEST(!InsideSegment(P(10 - eps, 10), ps), ());
    TEST(!InsideSegment(P(10 + eps, 10), ps), ());
    TEST(!InsideSegment(P(0, 0), ps), ());
  }
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

UNIT_TEST(Triangle_PointInsideSegment)
{
  double constexpr eps = 1.0E-10;

  P ps[] = {{0, 0}, {0, 1}, {0, 1}};
  TEST(InsideTriangle(ps[0], ps), ());
  TEST(InsideTriangle(ps[1], ps), ());
  TEST(InsideTriangle(ps[2], ps), ());
  TEST(InsideTriangle(P(0, eps), ps), ());
  TEST(InsideTriangle(P(0, 1.0 - eps), ps), ());

  TEST(!InsideTriangle(P(0, -eps), ps), ());
  TEST(!InsideTriangle(P(0, 1.0 + eps), ps), ());
  TEST(!InsideTriangle(P(-eps, eps), ps), ());
  TEST(!InsideTriangle(P(eps, eps), ps), ());
}

UNIT_TEST(Triangle_PointInsidePoint)
{
  double constexpr eps = 1.0E-10;

  P ps[] = {{0, 0}, {0, 0}, {0, 0}};
  TEST(InsideTriangle(ps[0], ps), ());
  TEST(InsideTriangle(ps[1], ps), ());
  TEST(InsideTriangle(ps[2], ps), ());

  TEST(!InsideTriangle(P(0, eps), ps), ());
  TEST(!InsideTriangle(P(0, -eps), ps), ());
  TEST(!InsideTriangle(P(-eps, eps), ps), ());
  TEST(!InsideTriangle(P(eps, eps), ps), ());
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
