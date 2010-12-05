#include "../../testing/testing.hpp"
#include "../distance.hpp"
#include "../point2d.hpp"

UNIT_TEST(DistanceToLineSquare2D)
{
  mn::DistanceToLineSquare<m2::PointD> d(m2::PointD(-1, 3), m2::PointD(2, 1));
  TEST_ALMOST_EQUAL(d(m2::PointD(-1, 3)), 0.0, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(2, 1)), 0.0, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(-0.5, 0.5)), 3.25, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(-0.5, 0.5)), 3.25, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(3.5, 0.0)), 3.25, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(4.0, 4.0)), 13.0, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(0.5, 2.0)), 0.0, ());
  TEST_ALMOST_EQUAL(d(m2::PointD(0.0, 1.25)), 0.5 * 0.5 + 0.75 * 0.75, ());
}
