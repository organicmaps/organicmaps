#include "../../testing/testing.hpp"

#include "../route.hpp"

using namespace routing;

UNIT_TEST(Routing_Smoke)
{
  TEST(true, ());
}

UNIT_TEST(Route_Distance)
{
  vector<m2::PointD> points =
  {
    {0, 0},
    {1, 0},
    {1, 0.5},
    {2, 0.5},
    {2, 1},
    {1.5, 1}
  };

  double const errorRadius = 0.1;

  {
    Route route("test", points.begin(), points.end());

    TEST_LESS(route.GetDistanceToTarget(m2::PointD(0.5, -0.11), errorRadius), 0, ());
    TEST_GREATER(route.GetDistanceToTarget(m2::PointD(0.5, -0.09), errorRadius), 0, ());

    // note that we are passing route, so we need to move forward by segments
    TEST_GREATER(route.GetDistanceToTarget(points.front(), errorRadius),
                 route.GetDistanceToTarget(m2::PointD(0.9, 0.1), errorRadius),
                 ());

    TEST_LESS(route.GetDistanceToTarget(m2::PointD(0.9, 0.1), errorRadius),
              route.GetDistanceToTarget(m2::PointD(0.9, 0.05), errorRadius),
              ());

    TEST_GREATER(route.GetDistanceToTarget(m2::PointD(0.9, 0.05), errorRadius),
                 route.GetDistanceToTarget(points.front(), errorRadius),
                 ());
  }

  {
    double const predictDist = 10000;

    Route route("test", points.begin(), points.end());

    TEST_GREATER(route.GetDistanceToTarget(points.front(), errorRadius, predictDist),
                 0, ());

    TEST_GREATER(route.GetDistanceToTarget(points.front(), errorRadius, predictDist),
                 route.GetDistanceToTarget(m2::PointD(0.5, 0), errorRadius, predictDist),
                 ());

    TEST_GREATER(route.GetDistanceToTarget(m2::PointD(1.99, 0.51), errorRadius, predictDist),
              route.GetDistanceToTarget(m2::PointD(1.99, 0.52), errorRadius, predictDist),
              ());

    TEST_LESS(route.GetDistanceToTarget(points.front(), errorRadius, predictDist),
              0,
              ());

    TEST_EQUAL(route.GetDistanceToTarget(points.back(), errorRadius, predictDist),
               0, ());

  }

}
