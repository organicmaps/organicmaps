#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowSevTushinoParkBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.87467, 37.43658), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.8719, 37.4464));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::TestRouteLength(route, 1054.0);
}

UNIT_TEST(RussiaMoscowGerPanfilovtsev22BicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85630, 37.41004), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85717, 37.41052));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 2);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowSalameiNerisPossibleTurnCorrectionBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85159, 37.38903), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85157, 37.38813));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 1);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::GoStraight);
}

UNIT_TEST(RussiaMoscowSevTushinoParkBicycleOnePointTurnTest)
{
  m2::PointD const point = MercatorBounds::FromLatLon(55.8719, 37.4464);
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), point, {0.0, 0.0}, point);

  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::IRouter::NoError, ());
}

UNIT_TEST(RussiaMoscowPlanernaiOnewayCarRoadTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.87012, 37.44028), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.87153, 37.43928));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 4);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 420.0);
}

UNIT_TEST(RussiaMoscowSvobodiOnewayBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.87277, 37.44002), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.87362, 37.43853));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 3);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 768.0);
}
