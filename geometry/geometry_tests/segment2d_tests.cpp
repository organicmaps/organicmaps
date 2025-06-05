#include "testing/testing.hpp"

#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

namespace segment2d_tests
{
using namespace m2;

double const kEps = 1e-10;

void TestIntersectionResult(IntersectionResult const & result, IntersectionResult::Type expectedType,
                            PointD const & expectedPoint)
{
  TEST_EQUAL(result.m_type, expectedType, ());
  TEST(AlmostEqualAbs(result.m_point, expectedPoint, kEps), (result.m_point, expectedPoint, kEps));
}

bool TestSegmentsIntersect(PointD a, PointD b, PointD c, PointD d)
{
  bool const res = SegmentsIntersect(a, b, c, d);
  TEST_EQUAL(res, SegmentsIntersect(a, b, d, c), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(b, a, c, d), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(b, a, d, c), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(c, d, a, b), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(c, d, b, a), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(d, c, a, b), (a, b, c, d));
  TEST_EQUAL(res, SegmentsIntersect(d, c, b, a), (a, b, c, d));
  return res;
}

UNIT_TEST(SegmentIntersection_Collinear)
{
  TEST(!TestSegmentsIntersect({0.0, 0.0}, {1.0, 1.0}, {2.0, 3.0}, {3.0, 3.0}), ("Far away"));
  TEST(TestSegmentsIntersect({0.0, 0.0}, {1.0, 1.0}, {1.0, 1.0}, {3.0, 3.0}), ("Border intersection"));
  TEST(TestSegmentsIntersect({0.0, 0.0}, {2.0, 2.0}, {1.0, 1.0}, {3.0, 3.0}), ("Normal intersection"));
  TEST(TestSegmentsIntersect({0.0, 0.0}, {2.0, 2.0}, {0.0, 0.0}, {3.0, 3.0}), ("Border inclusion"));
  TEST(TestSegmentsIntersect({1.0, 1.0}, {2.0, 2.0}, {0.0, 0.0}, {3.0, 3.0}), ("Normal inclusion"));
}

UNIT_TEST(SegmentIntersection_NoIntersection)
{
  TEST(!TestSegmentsIntersect({0.0, 1.0}, {2.0, 5.0}, {7.0, 7.0}, {15.0, 7.0}), ("Far away"));
  TEST(!TestSegmentsIntersect({0.0, 0.0}, {2.0, 4.0}, {1.0, 1.0}, {4.0, 3.0}), ("Rect intersect, segments don't"));
  TEST(!TestSegmentsIntersect({0.0, 0.0}, {8.0, 8.0}, {1.0, 2.0}, {4.0, 8.0}), ("Only one cross product is 0"));
}

UNIT_TEST(SegmentIntersection_Intersect)
{
  TEST(TestSegmentsIntersect({0.0, 0.0}, {2.0, 4.0}, {0.0, 3.0}, {4.0, 0.0}), ("Normal"));
  TEST(TestSegmentsIntersect({1.0, 2.0}, {3.0, 4.0}, {1.0, 2.0}, {5.0, 1.0}), ("Fan"));
  TEST(TestSegmentsIntersect({1.0, 2.0}, {3.0, 4.0}, {1.0, 2.0}, {3.0, -2.0}), ("Fan"));
  TEST(TestSegmentsIntersect({0.0, 0.0}, {2.0, 2.0}, {0.0, 4.0}, {4.0, 0.0}), ("Border"));
}

UNIT_TEST(SegmentIntersection_NoIntersectionPoint)
{
  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {1.0, 0.0}), Segment2D({2.0, 0.0}, {4.0, 0.0}), kEps).m_type,
             IntersectionResult::Type::Zero, ());

  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {1.0, 1.0}), Segment2D({2.0, 0.0}, {4.0, 0.0}), kEps).m_type,
             IntersectionResult::Type::Zero, ());

  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {1.0, 1.0}), Segment2D({4.0, 4.0}, {2.0, 2.0}), kEps).m_type,
             IntersectionResult::Type::Zero, ());

  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {4.0, 4.0}), Segment2D({10.0, 0.0}, {6.0, 4.0}), kEps).m_type,
             IntersectionResult::Type::Zero, ());
}

UNIT_TEST(SegmentIntersection_OneIntersectionPoint)
{
  // Two intersected segments. The intersection point is in the middle of both of them.
  {
    auto const result = Intersect(Segment2D({-1.0, -1.0}, {1.0, 1.0}), Segment2D({-1.0, 0.0}, {1.0, 0.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {0.0, 0.0});
  }

  {
    auto const result = Intersect(Segment2D({0.0, 0.0}, {10.0, 10.0}), Segment2D({10.0, 0.0}, {0.0, 10.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {5.0, 5.0});
  }

  // Two intersected segments. The intersection point is in an end of both of them.
  {
    auto const result = Intersect(Segment2D({-1.0, -1.0}, {0.0, 0.0}), Segment2D({0.0, 0.0}, {11.0, 0.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {0.0, 0.0});
  }
}

UNIT_TEST(SegmentIntersection_InfinityIntersectionPoints)
{
  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {2.0, 0.0}), Segment2D({1.0, 0.0}, {4.0, 0.0}), kEps).m_type,
             IntersectionResult::Type::Infinity, ());

  TEST_EQUAL(Intersect(Segment2D({0.0, 0.0}, {2.0, 4.0}), Segment2D({1.0, 2.0}, {2.0, 4.0}), kEps).m_type,
             IntersectionResult::Type::Infinity, ());
}
}  // namespace segment2d_tests
