#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowSevTushinoParkBicycleWayTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetBicycleComponents(), MercatorBounds::FromLatLon(55.87445, 37.43711),
      {0.0, 0.0}, MercatorBounds::FromLatLon(55.8719, 37.4464));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 3);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(TurnDirection::TurnRight);

  integration::TestRouteLength(route, 711.);
}
