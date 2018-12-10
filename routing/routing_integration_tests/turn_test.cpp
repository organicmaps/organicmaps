#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowNagatinoUturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.67251, 37.63604), {0.01, -0.01},
                                  MercatorBounds::FromLatLon(55.67293, 37.63507));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::UTurnLeft);

  integration::TestRouteLength(route, 248.0);
}

// Secondary should be preferred against residential.
UNIT_TEST(StPetersburgSideRoadPenaltyTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(59.85157, 30.28033), {0., 0.},
                                  MercatorBounds::FromLatLon(59.84268, 30.27589));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79683, 37.5379), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80212, 37.5389));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 4 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(CarDirection::UTurnLeft);
  integration::GetNthTurn(route, 2)
      .TestValid()
      .TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 2050.);
}

UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.85182, 37.39533), {0., 0.},
                                  MercatorBounds::FromLatLon(55.84386, 37.39250));
  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.38848, 67.63338}, 20.)
      .TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestPoint({37.38711, 67.63336}, 20.)
      .TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2)
      .TestValid()
      .TestPoint({37.38738, 67.63278}, 20.)
      .TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3)
      .TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 4)
      .TestValid()
      .TestPoint({37.39052, 67.63310}, 20.)
      .TestDirection(CarDirection::TurnRight);

  integration::TestRouteLength(route, 1637.);
}

// Fails because consider service roads are roundabout exits.
UNIT_TEST(RussiaMoscowTrikotagniAndPohodniRoundaboutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.83118, 37.40515), {0., 0.},
                                  MercatorBounds::FromLatLon(55.83384, 37.40521));
  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::LeaveRoundAbout);

  integration::TestRouteLength(route, 387.);
}

UNIT_TEST(SwedenBarlangeRoundaboutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(60.48278, 15.42356), {0., 0.},
                                  MercatorBounds::FromLatLon(60.48462, 15.42120));
  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::LeaveRoundAbout);

  integration::TestRouteLength(route, 255.);
}

UNIT_TEST(RussiaMoscowPlanetnayaOnlyStraightTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.80216, 37.54668), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80169, 37.54915));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 4).TestValid().TestDirection(CarDirection::TurnRight);

  integration::TestRouteLength(route, 454.);
}

UNIT_TEST(RussiaMoscowNoTurnsOnMKADTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.84656, 37.39163), {0., 0.},
                                  MercatorBounds::FromLatLon(55.56661, 37.69254));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.68276, 67.14062})
      .TestDirection(CarDirection::ExitHighwayToRight);

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(RussiaMoscowTTKVarshavskoeShosseOutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.70160, 37.60632), {0., 0.},
                                  MercatorBounds::FromLatLon(55.69349, 37.62122));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(RussiaMoscowTTKUTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.792848, 37.624424), {0., 0.},
                                  MercatorBounds::FromLatLon(55.792544, 37.624914));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 3 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::UTurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(RussiaMoscowParallelResidentalUTurnAvoiding)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.66192, 37.62852), {0., 0.},
                                  MercatorBounds::FromLatLon(55.66189, 37.63254));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  // Checking a turn in case going from a not-link to a link
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowPankratevskiPerBolshaySuharedskazPloschadTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.77177, 37.63556), {0., 0.},
                                  MercatorBounds::FromLatLon(55.77209, 37.63707));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  // It's not possible to get destination with less nubber of turns due to oneway roads.
  TEST_GREATER_OR_EQUAL(t.size(), 5, ());
}

UNIT_TEST(RussiaMoscowMKADPutilkovskeShosseTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.85305, 37.39414), {0., 0.},
                                  MercatorBounds::FromLatLon(55.85099, 37.39105));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(RussiaMoscowPetushkovaShodniaReverTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.84104, 37.40591), {0., 0.},
                                  MercatorBounds::FromLatLon(55.83929, 37.40855));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaHugeRoundaboutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.80141, 37.32581), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80075, 37.32536));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(5);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(CarDirection::LeaveRoundAbout)
      .TestRoundAboutExitNum(5);
}

UNIT_TEST(BelarusMiskProspNezavisimostiMKADTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(53.93642, 27.65857), {0., 0.},
                                  MercatorBounds::FromLatLon(53.93933, 27.67046));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test case: turning form one street to another one with the same name.
// An end user shall be informed about this manoeuvre.
UNIT_TEST(RussiaMoscowPetushkovaPetushkovaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.83636, 37.40555), {0., 0.},
                                  MercatorBounds::FromLatLon(55.83707, 37.40489));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test case: a route goes straight along a unnamed big link road when joined a small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(RussiaMoscowMKADLeningradkaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.87961, 37.43838), {0.0, 0.0},
                                  MercatorBounds::FromLatLon(55.87854, 37.44865));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(BelarusMKADShosseinai)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.31541, 29.43123), {0., 0.},
                                  MercatorBounds::FromLatLon(55.31656, 29.42626));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test case: a route goes straight along a big road when joined small road.
// An end user shall not be informed about such manoeuvres.
// But at the end of the route an end user shall be informed about junction of two big roads.
UNIT_TEST(ThailandPhuketNearPrabarameeRoad)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(7.91797, 98.36937), {0., 0.},
                                  MercatorBounds::FromLatLon(7.90724, 98.3679));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToLeft);
}

// Test case: a route goes in Moscow from Varshavskoe shosse (from the city center direction)
// to MKAD (the outer side). A turn instruction (to leave Varshavskoe shosse)
// shall be generated.
UNIT_TEST(RussiaMoscowVarshavskoeShosseMKAD)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.58210, 37.59695), {0., 0.},
                                  MercatorBounds::FromLatLon(55.57514, 37.61020));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(RussiaMoscowBolshayaNikitskayaOkhotnyRyadTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75509, 37.61067), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(RussiaMoscowTverskajaOkhotnyRyadTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75765, 37.61355), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowBolshoyKislovskiyPerBolshayaNikitinskayaUlTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75574, 37.60702), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75586, 37.60819));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test case: a route goes in Moscow along Leningradskiy Prpt (towards city center).
UNIT_TEST(RussiaMoscowLeningradskiyPrptToTheCenterUTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79231, 37.54951), {0., 0.},
                                  MercatorBounds::FromLatLon(55.79280, 37.55028));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::UTurnLeft);
}

UNIT_TEST(SwitzerlandSamstagernBergstrasseTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.19307, 8.67594), {0., 0.},
                                  MercatorBounds::FromLatLon(47.19162, 8.67590));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(RussiaMoscowMikoiankNoUTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79041, 37.53770), {0., 0.},
                                  MercatorBounds::FromLatLon(55.79182, 37.53008));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLeningradskiyPrptToTTKTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.78926, 37.55706), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  // @TODO(bykoianko) It's a case when two possible ways go slight left.
  // A special processing should be implemented for such cases.
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(RussiaMoscowLeningradskiyPrptDublToTTKTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79059, 37.55345), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(RussiaMoscowSvobodaStTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.82484, 37.45151), {0., 0.},
                                  MercatorBounds::FromLatLon(55.81941, 37.45073));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(RussiaTiinskTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(54.37738, 49.63878), {0., 0.},
                                  MercatorBounds::FromLatLon(54.3967, 49.64924));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
  integration::GetNthTurn(route, 1).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightLeft, CarDirection::TurnLeft});
}

UNIT_TEST(NetherlandsGorinchemBridgeTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(51.84131, 4.94825), {0., 0.},
                                  MercatorBounds::FromLatLon(51.81518, 4.93773));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaVoronezhProspTrudaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(51.67205, 39.16334), {0., 0.},
                                  MercatorBounds::FromLatLon(51.67193, 39.15636));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

UNIT_TEST(GermanyFrankfurtAirportTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(50.07094, 8.61299), {0., 0.},
                                  MercatorBounds::FromLatLon(50.05807, 8.59542));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}


UNIT_TEST(GermanyFrankfurtAirport2Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(50.03249, 8.50814), {0., 0.},
                                  MercatorBounds::FromLatLon(50.02079, 8.49445));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}


// Test on absence of unnecessary turn which may appear between two turns in the test.
UNIT_TEST(RussiaKubinkaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.58533, 36.83779), {0., 0.},
                                  MercatorBounds::FromLatLon(55.58365, 36.8333));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  integration::GetNthTurn(route, 1).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightLeft, CarDirection::TurnLeft});
}

// Test on absence of unnecessary turn.
UNIT_TEST(AustriaKitzbuhelTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.46894, 12.3841), {0., 0.},
                                  MercatorBounds::FromLatLon(47.46543, 12.38599));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn.
UNIT_TEST(AustriaKitzbuhel2Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.45119, 12.3841), {0., 0.},
                                  MercatorBounds::FromLatLon(47.45021, 12.382));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn in case of fake ingoing segment.
UNIT_TEST(AustriaKitzbuhel3Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.45362, 12.38709), {0., 0.},
                                  MercatorBounds::FromLatLon(47.45255, 12.38498));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn.
UNIT_TEST(AustriaBrixentalStrasseTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.45091, 12.33453), {0., 0.},
                                  MercatorBounds::FromLatLon(47.45038, 12.32592));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(RussiaMoscowLeningradkaToMKADTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.87192, 37.45772), {0., 0.},
                                  MercatorBounds::FromLatLon(55.87594, 37.45266));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(RussiaMoscowMKADToSvobodaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.8801, 37.43862), {0., 0.},
                                  MercatorBounds::FromLatLon(55.87583, 37.43046));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test that there's no turns if to follow MKAD.
UNIT_TEST(RussiaMoscowMKADTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(52.15866, 5.56538), {0., 0.},
                                  MercatorBounds::FromLatLon(52.16668, 5.55665));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
}

UNIT_TEST(NetherlandsBarneveldTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(52.15866, 5.56538), {0., 0.},
                                  MercatorBounds::FromLatLon(52.16667, 5.55663));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(GermanyRaunheimAirportTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(50.03322, 8.50995), {0., 0.},
                                  MercatorBounds::FromLatLon(50.02089, 8.49441));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(BelorussiaMinskTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(53.90991, 27.57946), {0., 0.},
                                  MercatorBounds::FromLatLon(53.91552, 27.58211));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  // @TODO(bykoianko) In this case it's better to get GoStraight direction or not get
  // direction at all. But the turn generator generates TurnSlightRight based on road geometry.
  // It's so because the turn generator does not take into account the significant number of lanes
  // of the roads at the crossing. While turn generation number of lanes should be taken into account.
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightRight});
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(EnglandLondonExitToLeftTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(51.603582, 0.266995), {0., 0.},
                                  MercatorBounds::FromLatLon(51.606785, 0.264055));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToLeft);
}

// Test on the route from Leninsky prospect to its frontage road and turns generated on the route.
UNIT_TEST(RussiaMoscowLeninskyProspTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.69035, 37.54948), {0., 0.},
                                  MercatorBounds::FromLatLon(55.69188, 37.55293));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test on the route from TTK (primary) to a link.
UNIT_TEST(RussiaMoscowTTKToLinkTest)
{
TRouteResult const routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                MercatorBounds::FromLatLon(55.78594, 37.56656), {0., 0.},
                                MercatorBounds::FromLatLon(55.78598, 37.56737));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test on the turn from TTK (primary) to a secondary road.
UNIT_TEST(RussiaMoscowTTKToBegovayAlleyaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.77946, 37.55779), {0., 0.},
                                  MercatorBounds::FromLatLon(55.77956, 37.55891));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on the turn from TTK (primary) to a service road. The angle of the turn is not slight.
UNIT_TEST(RussiaMoscowTTKToServiceTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.78874, 37.5704), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78881, 37.57106));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on a turn from TTK (primary) to an unclassified road. The angle of the turn is slight.
UNIT_TEST(RussiaMoscowTTKToNMaslovkaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79057, 37.57292), {0., 0.},
                                  MercatorBounds::FromLatLon(55.79132, 37.57481));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(RussiaMoscowComplicatedTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.68412, 37.60166), {0., 0.},
                                  MercatorBounds::FromLatLon(55.68426, 37.59854));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightLeft});
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(USATampaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(28.04875, -82.58292), {0., 0.},
                                  MercatorBounds::FromLatLon(28.04459, -82.58448));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

// Test on go straight direction if it's possible to go through a roundabout.
UNIT_TEST(RussiaMoscowMinskia1TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.7355, 37.48717), {0., 0.},
                                  MercatorBounds::FromLatLon(55.73694, 37.48587));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowMinskia2TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.74244, 37.4808), {0., 0.},
                                  MercatorBounds::FromLatLon(55.74336, 37.48124));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// This test on getting correct (far enough) outgoing turn point (result of method GetPointForTurn())
// despite the fact that a small road adjoins immediately after the turn point.
UNIT_TEST(RussiaMoscowBarikadnaiTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75979, 37.58502), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75936, 37.58286));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// This test on getting correct (far enough) outgoing turn point (result of method GetPointForTurn()).
UNIT_TEST(RussiaMoscowKomsomolskyTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.73442, 37.59391), {0., 0.},
                                  MercatorBounds::FromLatLon(55.73485, 37.59543));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on no go straight direction in case of a route along a big road and pass smaller ones.
UNIT_TEST(RussiaMoscowTTKNoGoStraightTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.78949, 37.5711), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78673, 37.56726));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLeninskyProsp2Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.80376, 37.52048), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80442, 37.51802));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}
