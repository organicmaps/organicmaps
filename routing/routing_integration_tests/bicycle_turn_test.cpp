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

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
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

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Fails to generate direction.
UNIT_TEST(RussiaMoscowSalameiNerisPossibleTurnCorrectionBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85777, 37.3679), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85579, 37.36867));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid()
                                   .TestOneOfDirections({CarDirection::GoStraight,
                                                         CarDirection::TurnSlightRight,
                                                         CarDirection::TurnRight});
}

// Fails to build one-point route.
UNIT_TEST(RussiaMoscowSevTushinoParkBicycleOnePointTurnTest)
{
  m2::PointD const point = MercatorBounds::FromLatLon(55.8719, 37.4464);
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), point, {0.0, 0.0}, point);

  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::IRouter::NoError, ());
}

UNIT_TEST(RussiaMoscowTatishchevaOnewayCarRoadTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.71566, 37.61569), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.71532, 37.61571));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 4 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 675.0);
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

  integration::TestTurnCount(route, 3 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 768.0);
}
