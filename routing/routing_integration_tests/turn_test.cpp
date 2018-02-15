#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"


using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowNagatinoUturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.67251, 37.63604), {0.01, -0.01},
                                  MercatorBounds::FromLatLon(55.67293, 37.63507));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::UTurnLeft);

  integration::TestRouteLength(route, 248.0);
}

UNIT_TEST(StPetersburgSideRoadPenaltyTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(59.85157, 30.28033), {0., 0.},
                                  MercatorBounds::FromLatLon(59.84268, 30.27589));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79683, 37.5379), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80212, 37.5389));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

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

// Fails: generates "GoStraight" description instead of TurnSlightRight.
UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.85182, 37.39533), {0., 0.},
                                  MercatorBounds::FromLatLon(55.84386, 37.39250));
  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 4 /* expectedTurnCount */);
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::GoStraight);
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.68276, 67.14062})
      .TestOneOfDirections({CarDirection::TurnSlightRight, CarDirection::TurnRight});

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(RussiaMoscowTTKKashirskoeShosseOutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.70160, 37.60632), {0., 0.},
                                  MercatorBounds::FromLatLon(55.69349, 37.62122));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  // Checking a turn in case going from a not-link to a link
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowTTKUTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.792848, 37.624424), {0., 0.},
                                  MercatorBounds::FromLatLon(55.792544, 37.624914));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  // Checking a turn in case going from a not-link to a link
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});;
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::UTurnLeft);
}

UNIT_TEST(RussiaMoscowParallelResidentalUTurnAvoiding)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.66192, 37.62852), {0., 0.},
                                  MercatorBounds::FromLatLon(55.66189, 37.63254));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
                                  MercatorBounds::FromLatLon(55.77203, 37.63705));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());

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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

// Fails due to uneeded "GoStraight".
UNIT_TEST(RussiaMoscowPetushkovaShodniaReverTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.84104, 37.40591), {0., 0.},
                                  MercatorBounds::FromLatLon(55.83929, 37.40855));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Fails because consider service roads are roundabout exits.
UNIT_TEST(RussiaHugeRoundaboutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.80141, 37.32581), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80075, 37.32536));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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

// Fails: generates "GoStraight" description instead of TurnSlightRight/TurnRight.
UNIT_TEST(BelarusMiskProspNezavisimostiMKADTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(53.93642, 27.65857), {0., 0.},
                                  MercatorBounds::FromLatLon(53.93933, 27.67046));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

UNIT_TEST(BelarusMKADShosseinai)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.31541, 29.43123), {0., 0.},
                                  MercatorBounds::FromLatLon(55.31656, 29.42626));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test case: a route goes straight along a unnamed big road when joined small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(ThailandPhuketNearPrabarameeRoad)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(7.91797, 98.36937), {0., 0.},
                                  MercatorBounds::FromLatLon(7.90724, 98.3679));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

// Test case: a route goes in Moscow from Bolshaya Nikitskaya street
// (towards city center) to Mokhovaya street. A turn instruction (turn left)
// shall be generated.
UNIT_TEST(RussiaMoscowBolshayaNikitskayaOkhotnyRyadTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75509, 37.61067), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test case: a route goes in Moscow from Tverskaya street (towards city center)
// to Mokhovaya street. Road goes left but there are no other turn option.
// Turn instruction is not needed in this case.
UNIT_TEST(RussiaMoscowTverskajaOkhotnyRyadTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75765, 37.61355), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowBolshoyKislovskiyPerBolshayaNikitinskayaUlTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.75574, 37.60702), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75586, 37.60819));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::UTurnLeft);
}

// Fails: generates unnecessary turn.
// Test case: checking that no unnecessary turn on a serpentine road.
// This test was written after reducing factors kMaxPointsCount and kMinDistMeters.
UNIT_TEST(SwitzerlandSamstagernBergstrasseTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(47.19300, 8.67568), {0., 0.},
                                  MercatorBounds::FromLatLon(47.19162, 8.67590));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowMikoiankNoUTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79041, 37.53770), {0., 0.},
                                  MercatorBounds::FromLatLon(55.79182, 37.53008));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLeningradskiyPrptToTTKTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.78926, 37.55706), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::GoStraight);
}

UNIT_TEST(RussiaMoscowLeningradskiyPrptDublToTTKTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.79059, 37.55345), {0., 0.},
                                  MercatorBounds::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::GoStraight);
}

UNIT_TEST(RussiaMoscowSvobodaStTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.82484, 37.45151), {0., 0.},
                                  MercatorBounds::FromLatLon(55.81941, 37.45073));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaVoronigProspTrudaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(51.67205, 39.16334), {0., 0.},
                                  MercatorBounds::FromLatLon(51.67193, 39.15636));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}


UNIT_TEST(GermanyFrankfurtAirport2Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(50.03249, 8.50814), {0., 0.},
                                  MercatorBounds::FromLatLon(50.02079, 8.49445));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
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
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(RussiaMoscowLeningradkaToMKADTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.87192, 37.45772), {0., 0.},
                                  MercatorBounds::FromLatLon(55.87594, 37.45266));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowMKADToSvobodaTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.8801, 37.43862), {0., 0.},
                                  MercatorBounds::FromLatLon(55.87583, 37.43046));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

// Test that there's no turns if to follow MKAD.
UNIT_TEST(RussiaMoscowMKADTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                  MercatorBounds::FromLatLon(55.90394, 37.61034), {0., 0.},
                                  MercatorBounds::FromLatLon(55.77431, 37.36945));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}
