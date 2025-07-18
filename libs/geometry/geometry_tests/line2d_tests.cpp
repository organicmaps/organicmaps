#include "testing/testing.hpp"

#include "geometry/line2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

namespace line2d_tests
{
using namespace m2;

double const kEps = 1e-12;

using Result = IntersectionResult;
using Type = Result::Type;

Result Intersect(Line2D const & lhs, Line2D const & rhs)
{
  return Intersect(lhs, rhs, kEps);
}

UNIT_TEST(LineIntersection_Smoke)
{
  {
    Line2D const line(Segment2D(PointD(0, 0), PointD(1, 0)));
    TEST_EQUAL(Intersect(line, line).m_type, Type::Infinity, ());
  }

  {
    Line2D const lhs(Segment2D(PointD(0, 0), PointD(1, 1)));
    Line2D const rhs(Segment2D(PointD(-10, -10), PointD(-100, -100)));
    TEST_EQUAL(Intersect(lhs, rhs).m_type, Type::Infinity, ());
  }

  {
    Line2D const lhs(Segment2D(PointD(0, 0), PointD(10, 10)));
    Line2D const rhs(Segment2D(PointD(10, 11), PointD(0, 1)));
    TEST_EQUAL(Intersect(lhs, rhs).m_type, Type::Zero, ());
  }

  {
    Line2D const lhs(Segment2D(PointD(10, 0), PointD(9, 10)));
    Line2D const rhs(Segment2D(PointD(-10, 0), PointD(-9, 10)));
    auto const result = Intersect(lhs, rhs);
    TEST_EQUAL(result.m_type, Type::One, ());
    TEST(AlmostEqualAbs(result.m_point, PointD(0, 100), kEps), (result.m_point));
  }
}
}  // namespace line2d_tests
