#include "../../testing/testing.hpp"
#include "../distance.hpp"
#include "../point2d.hpp"

template <class PointT>
void FloatingPointsTest()
{
  mn::DistanceToLineSquare<PointT> d(PointT(-1, 3), PointT(2, 1));

  TEST_ALMOST_EQUAL(d(PointT(-1, 3)), 0.0, ());
  TEST_ALMOST_EQUAL(d(PointT(2, 1)), 0.0, ());
  TEST_ALMOST_EQUAL(d(PointT(-0.5, 0.5)), 3.25, ());
  TEST_ALMOST_EQUAL(d(PointT(-0.5, 0.5)), 3.25, ());
  TEST_ALMOST_EQUAL(d(PointT(3.5, 0.0)), 3.25, ());
  TEST_ALMOST_EQUAL(d(PointT(4.0, 4.0)), 13.0, ());
  TEST_ALMOST_EQUAL(d(PointT(0.5, 2.0)), 0.0, ());
  TEST_ALMOST_EQUAL(d(PointT(0.0, 1.25)), 0.5 * 0.5 + 0.75 * 0.75, ());
}

UNIT_TEST(DistanceToLineSquare2D_Floating)
{
  FloatingPointsTest<m2::PointD>();
  FloatingPointsTest<m2::PointF>();
}

UNIT_TEST(DistanceToLineSquare2D_Integer)
{
  mn::DistanceToLineSquare<m2::PointI> dI(m2::PointI(-1, 3), m2::PointI(2, 1));

  TEST_ALMOST_EQUAL(dI(m2::PointI(-1, 3)), 0.0, ());
  TEST_ALMOST_EQUAL(dI(m2::PointI(2, 1)), 0.0, ());
  TEST_ALMOST_EQUAL(dI(m2::PointI(4, 4)), 13.0, ());
}

UNIT_TEST(DistanceToLineSquare2D_DegenerateSection)
{
  typedef m2::PointD P;
  mn::DistanceToLineSquare<P> d(P(5, 5), P(5, 5));

  TEST_ALMOST_EQUAL(d(P(5, 5)), 0.0, ());
  TEST_ALMOST_EQUAL(d(P(6, 6)), 2.0, ());
  TEST_ALMOST_EQUAL(d(P(0, 0)), 50.0, ());
  TEST_ALMOST_EQUAL(d(P(-1, -2)), 36.0 + 49.0, ());
}
