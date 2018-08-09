#include "testing/testing.hpp"

#include "routing/routing_callbacks.hpp"

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
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight});
  integration::GetNthTurn(route, 1).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightLeft});
  integration::GetNthTurn(route, 2).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightRight});
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 4).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
  integration::TestRouteLength(route, 753.0);
}

UNIT_TEST(RussiaMoscowGerPanfilovtsev22BicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85630, 37.41004), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85717, 37.41052));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowSalameiNerisPossibleTurnCorrectionBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85836, 37.36773), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85364, 37.37318));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 4 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(RussiaMoscowSalameiNerisNoUTurnBicycleWayTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.85839, 37.3677), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.85765, 37.36793));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowSevTushinoParkBicycleOnePointOnewayRoadTurnTest)
{
  m2::PointD const point = MercatorBounds::FromLatLon(55.8719, 37.4464);
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), point, {0.0, 0.0}, point);

  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());
}

UNIT_TEST(RussiaMoscowSevTushinoParkBicycleOnePointTwowayRoadTurnTest)
{
  m2::PointD const point = MercatorBounds::FromLatLon(55.87102, 37.44222);
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), point, {0.0, 0.0}, point);

  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());
}

UNIT_TEST(RussiaMoscowTatishchevaOnewayCarRoadTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Bicycle>(),
                                  MercatorBounds::FromLatLon(55.71566, 37.61569), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.71532, 37.61571));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

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
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 6 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 2).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightLeft, CarDirection::TurnLeft});
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 4).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 5).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 768.0);
}
