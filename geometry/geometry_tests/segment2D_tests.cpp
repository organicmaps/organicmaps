#include "testing/testing.hpp"

#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

using namespace m2;

namespace
{
double const kEps = 1e-10;

void TestIntersectionResult(IntersectionResult const & result, IntersectionResult::Type expectedType,
    PointD const & expectedPoint)
{
  TEST_EQUAL(result.m_type, expectedType, ());
  TEST(m2::AlmostEqualAbs(result.m_point, expectedPoint, kEps), (result.m_point, expectedPoint, kEps));
}

UNIT_TEST(SegmentIntersection_NoIntersectionPoint)
{
  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {1.0, 0.0}), Segment2D({2.0, 0.0}, {4.0, 0.0}), kEps).m_type,
      IntersectionResult::Type::Zero, ());

  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {1.0, 1.0}), Segment2D({2.0, 0.0}, {4.0, 0.0}), kEps).m_type,
      IntersectionResult::Type::Zero, ());

  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {1.0, 1.0}), Segment2D({4.0, 4.0}, {2.0, 2.0}), kEps).m_type,
      IntersectionResult::Type::Zero, ());

  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {4.0, 4.0}), Segment2D({10.0, 0.0}, {6.0, 4.0}), kEps).m_type,
      IntersectionResult::Type::Zero, ());
}

UNIT_TEST(SegmentIntersection_OneIntersectionPoint)
{
  // Two intersected segments. The intersection point is at the middle of both of them.
  {
    auto const result =
        Intersect(Segment2D({-1.0, -1.0}, {1.0, 1.0}), Segment2D({-1.0, 0.0}, {1.0, 0.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {0.0, 0.0});
  }

  {
    auto const result =
        Intersect(Segment2D({0.0, 0.0}, {10.0, 10.0}), Segment2D({10.0, 0.0}, {0.0, 10.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {5.0, 5.0});
  }

  // Two intersected segments. The intersection point is at an end of both of them.
  {
    auto const result =
        Intersect(Segment2D({-1.0, -1.0}, {0.0, 0.0}), Segment2D({0.0, 0.0}, {11.0, 0.0}), kEps);
    TestIntersectionResult(result, IntersectionResult::Type::One, {0.0, 0.0});
  }
}

UNIT_TEST(SegmentIntersection_InfinityIntersectionPoints)
{
  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {2.0, 0.0}), Segment2D({1.0, 0.0}, {4.0, 0.0}), kEps).m_type,
      IntersectionResult::Type::Infinity, ());

  TEST_EQUAL(
      Intersect(Segment2D({0.0, 0.0}, {2.0, 4.0}), Segment2D({1.0, 2.0}, {2.0, 4.0}), kEps).m_type,
      IntersectionResult::Type::Infinity, ());
}
}  // namespace
