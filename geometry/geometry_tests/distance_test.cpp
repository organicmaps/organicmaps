#include "testing/testing.hpp"
#include "geometry/distance.hpp"
#include "geometry/point2d.hpp"

template <class PointT>
void FloatingPointsTest()
{
  m2::DistanceToLineSquare<PointT> d;
  d.SetBounds(PointT(-1, 3), PointT(2, 1));

  TEST_ALMOST_EQUAL_ULPS(d(PointT(-1, 3)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(2, 1)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(-0.5, 0.5)), 3.25, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(3.5, 0.0)), 3.25, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(4.0, 4.0)), 13.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(0.5, 2.0)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(PointT(0.0, 1.25)), 0.5 * 0.5 + 0.75 * 0.75, ());
}

UNIT_TEST(DistanceToLineSquare2D_Floating)
{
  FloatingPointsTest<m2::PointD>();
  FloatingPointsTest<m2::PointF>();
}

UNIT_TEST(DistanceToLineSquare2D_Integer)
{
  m2::DistanceToLineSquare<m2::PointI> d;
  d.SetBounds(m2::PointI(-1, 3), m2::PointI(2, 1));

  TEST_ALMOST_EQUAL_ULPS(d(m2::PointI(-1, 3)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(m2::PointI(2, 1)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(m2::PointI(4, 4)), 13.0, ());

  double const sqSin = 4.0 / m2::PointI(-1, 3).SquaredLength(m2::PointI(2, 1));
  TEST_ALMOST_EQUAL_ULPS(d(m2::PointI(0, 1)), 4.0*sqSin, ());
  TEST_ALMOST_EQUAL_ULPS(d(m2::PointI(-1, 1)), 9.0*sqSin, ());
}

UNIT_TEST(DistanceToLineSquare2D_DegenerateSection)
{
  typedef m2::PointD P;
  m2::DistanceToLineSquare<P> d;
  d.SetBounds(P(5, 5), P(5, 5));

  TEST_ALMOST_EQUAL_ULPS(d(P(5, 5)), 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(P(6, 6)), 2.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(P(0, 0)), 50.0, ());
  TEST_ALMOST_EQUAL_ULPS(d(P(-1, -2)), 36.0 + 49.0, ());
}

UNIT_TEST(PointProjectionTests_Smoke)
{
  typedef m2::PointD P;
  m2::ProjectionToSection<P> p;

  P arr[][4] =
  {
    { P(3, 4), P(0, 0), P(10, 0), P(3, 0) },
    { P(3, 4), P(0, 0), P(0, 10), P(0, 4) },

    { P(3, 5), P(2, 2), P(5, 5), P(4, 4) },
    { P(5, 3), P(2, 2), P(5, 5), P(4, 4) },
    { P(2, 4), P(2, 2), P(5, 5), P(3, 3) },
    { P(4, 2), P(2, 2), P(5, 5), P(3, 3) },

    { P(5, 6), P(2, 2), P(5, 5), P(5, 5) },
    { P(1, 0), P(2, 2), P(5, 5), P(2, 2) }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    p.SetBounds(arr[i][1], arr[i][2]);
    TEST(m2::AlmostEqualULPs(p(arr[i][0]), arr[i][3]), (i));
  }
}
