#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

#include <vector>

using namespace routing;
using namespace routing::turns;

UNIT_TEST(Russia_Moscow_NagatinoUturn_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.67251, 37.63604), {0.01, -0.01},
                                  mercator::FromLatLon(55.67293, 37.63507));

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
UNIT_TEST(StPetersburg_SideRoadPenalty_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(59.85157, 30.28033), {0., 0.},
                                  mercator::FromLatLon(59.84268, 30.27589));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_Lenigradskiy39Uturn_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.79683, 37.5379), {0., 0.},
                                  mercator::FromLatLon(55.80212, 37.5389));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  integration::TestTurnCount(route, 4 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::UTurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);

  integration::TestRouteLength(route, 2050.);
}

UNIT_TEST(Russia_Moscow_SalameiNerisUturn_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.85182, 37.39533), {0., 0.},
                                  mercator::FromLatLon(55.84386, 37.39250));
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
UNIT_TEST(Russia_Moscow_TrikotagniAndPohodniRoundabout_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.83118, 37.40515), {0., 0.},
                                  mercator::FromLatLon(55.83384, 37.40521));
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

UNIT_TEST(Sweden_BarlangeRoundabout_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(60.48278, 15.42356), {0., 0.},
                                  mercator::FromLatLon(60.48462, 15.42120));
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

UNIT_TEST(Russia_Moscow_PlanetnayaOnlyStraight_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.80215, 37.54663), {0., 0.},
                                  mercator::FromLatLon(55.80186, 37.5496));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 4 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnRight);

  integration::TestRouteLength(route, 418.0);
}

UNIT_TEST(Russia_Moscow_NoTurnsOnMKAD_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.84656, 37.39163), {0., 0.},
                                  mercator::FromLatLon(55.56661, 37.69254));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestPoint({37.68276, 67.14062})
      .TestDirection(CarDirection::ExitHighwayToRight);

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(Russia_Moscow_TTKVarshavskoeShosseOut_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.70160, 37.60632), {0., 0.},
                                  mercator::FromLatLon(55.69349, 37.62122));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Russia_Moscow_TTKU_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.792848, 37.624424), {0., 0.},
                                  mercator::FromLatLon(55.792544, 37.624914));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 3 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::UTurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(Russia_Moscow_ParallelResidentalUTurnAvoiding_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.66192, 37.62852), {0., 0.},
                                  mercator::FromLatLon(55.66189, 37.63254));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  // Checking a turn in case going from a not-link to a link.
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(Russia_Moscow_PankratevskiPerBolshaySuharedskazPloschad_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.77177, 37.63556), {0., 0.},
                                  mercator::FromLatLon(55.77224, 37.63748));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  std::vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);

  integration::TestTurnCount(route, 3 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  /// @todo UTurnLeft
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnSharpLeft);
}

UNIT_TEST(Russia_Moscow_MKADPutilkovskeShosse_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.85305, 37.39414), {0., 0.},
                                  mercator::FromLatLon(55.85099, 37.39105));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Russia_Moscow_PetushkovaShodniaRever_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.84104, 37.40591), {0., 0.},
                                  mercator::FromLatLon(55.83929, 37.40855));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_HugeRoundabout_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.80141, 37.32581), {0., 0.},
                                  mercator::FromLatLon(55.80075, 37.32536));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  /// @todo Actualized exit num. But one exit is highway=unclassified, that (probably?) should not be counted?
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(CarDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(6);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(CarDirection::LeaveRoundAbout)
      .TestRoundAboutExitNum(6);
}

UNIT_TEST(Belarus_Misk_ProspNezavisimostiMKAD_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(53.93642, 27.65857), {0., 0.},
                                  mercator::FromLatLon(53.93933, 27.67046));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test case: turning form one street to another one with the same name.
// An end user shall be informed about this manoeuvre.
UNIT_TEST(Russia_Moscow_PetushkovaPetushkova_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.83636, 37.40555), {0., 0.},
                                  mercator::FromLatLon(55.83707, 37.40489));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test case: a route goes straight along a unnamed big link road when joined a small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(Russia_Moscow_MKADLeningradka_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.87961, 37.43838), {0.0, 0.0},
                                  mercator::FromLatLon(55.87854, 37.44865));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Belarus_Vitebsk_Shosseinai_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.31541, 29.43123), {0., 0.},
                                  mercator::FromLatLon(55.31656, 29.42626));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// A route goes straight along a big road ignoring joined small roads.
UNIT_TEST(Thailand_Phuket_NearPrabarameeRoad_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(7.91797, 98.36937), {0., 0.},
                                  mercator::FromLatLon(7.90724, 98.3679));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // https://www.openstreetmap.org/directions?engine=fossgis_osrm_car&route=7.91797%2C98.36937%3B7.90724%2C98.36790
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_VarshavskoeShosseMKAD_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.58210, 37.59695), {0., 0.},
                                  mercator::FromLatLon(55.57514, 37.61020));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Russia_Moscow_BolshayaNikitskayaOkhotnyRyad_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.75509, 37.61067), {0., 0.},
                                  mercator::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(Russia_Moscow_TverskajaOkhotnyRyad_Test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.75765, 37.61355), {0., 0.},
                                  mercator::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(Russia_Moscow_BolshoyKislovskiyPerBolshayaNikitinskayaUl_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.75574, 37.60702), {0., 0.},
                                  mercator::FromLatLon(55.75586, 37.60819));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  /// @todo Problem with outgoingTurns from RoutingEngineResult::GetPossibleTurns at (turn_m_index == 4).
  /// For some reason it contains only one possible turn (+90), but it is expected that it will be two of them (-90 and +90).
  /// This is the reason why the RightTurn is discarded.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test case: a route goes in Moscow along Leningradskiy Prpt (towards city center).
UNIT_TEST(Russia_Moscow_LeningradskiyPrptToTheCenterUTurn_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.79368, 37.54833), {0., 0.},
                                  mercator::FromLatLon(55.79325, 37.54734));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::UTurnLeft);
}

UNIT_TEST(Switzerland_SamstagernBergstrasse_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(47.19307, 8.67594), {0., 0.},
                                  mercator::FromLatLon(47.19162, 8.67590));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_MikoiankNoUTurn_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.79041, 37.53770), {0., 0.},
                                  mercator::FromLatLon(55.79182, 37.53008));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_LeningradskiyPrptToTTK_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.78926, 37.55706), {0., 0.},
                                  mercator::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(Russia_Moscow_LeningradskiyPrptDublToTTK_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.79054, 37.55335), {0., 0.},
                                  mercator::FromLatLon(55.78925, 37.57110));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(Russia_Moscow_SvobodaSt_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.82484, 37.45151), {0., 0.},
                                  mercator::FromLatLon(55.81941, 37.45073));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(Russia_Tiinsk_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(54.37738, 49.63878), {0., 0.},
                                  mercator::FromLatLon(54.3967, 49.64924));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightLeft, CarDirection::TurnLeft});
}

UNIT_TEST(Netherlands_GorinchemBridge_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(51.84131, 4.94825), {0., 0.},
                                  mercator::FromLatLon(51.81518, 4.93773));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Voronezh_ProspTruda_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(51.67205, 39.16334), {0., 0.},
                                  mercator::FromLatLon(51.67193, 39.15636));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(Germany_FrankfurtAirport_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(50.07094, 8.61299), {0., 0.},
                                  mercator::FromLatLon(50.05807, 8.59542));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightRight);
}


UNIT_TEST(Germany_FrankfurtAirport2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(50.03249, 8.50814), {0., 0.},
                                  mercator::FromLatLon(50.02079, 8.49445));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}


// Test on absence of unnecessary turn which may appear between two turns in the test.
UNIT_TEST(Russia_Kubinka_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.58533, 36.83779), {0., 0.},
                                  mercator::FromLatLon(55.58365, 36.8333));

  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  Route const & route = *routeResult.first;
  integration::TestTurnCount(route, 5);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 4).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test on absence of unnecessary turn.
UNIT_TEST(Austria_Kitzbuhel_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(47.46894, 12.3841), {0., 0.},
                                  mercator::FromLatLon(47.46543, 12.38599));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn.
UNIT_TEST(Austria_Kitzbuhel2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(47.45119, 12.3841), {0., 0.},
                                  mercator::FromLatLon(47.45021, 12.382));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn in case of fake ingoing segment.
UNIT_TEST(Austria_Kitzbuhel3_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(47.45362, 12.38709), {0., 0.},
                                  mercator::FromLatLon(47.45255, 12.38498));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

// Test on absence of unnecessary turn.
UNIT_TEST(Austria_BrixentalStrasse_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(47.45091, 12.33453), {0., 0.},
                                  mercator::FromLatLon(47.45038, 12.32592));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_LeningradkaToMKAD_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.87192, 37.45772), {0., 0.},
                                  mercator::FromLatLon(55.87594, 37.45266));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Russia_Moscow_MKADToSvoboda_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.8801, 37.43862), {0., 0.},
                                  mercator::FromLatLon(55.87583, 37.43046));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test that there's no turns if to follow MKAD.
UNIT_TEST(Netherlands_Barneveld_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(52.15866, 5.56538), {0., 0.},
                                  mercator::FromLatLon(52.17042, 5.55834));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  /// @todo Reasonable solution from GraphHopper:
  // https://www.openstreetmap.org/directions?engine=graphhopper_car&route=52.15866%2C5.56538%3B52.17042%2C5.55834
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(Belarus_Minsk_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(53.90991, 27.57946), {0., 0.},
                                  mercator::FromLatLon(53.91552, 27.58211));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on building route from a point close to mwm border to a point close to a mwm border.
UNIT_TEST(England_London_ExitToLeft_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(51.603582, 0.266995), {0.0, 0.0},
                                  mercator::FromLatLon(51.606785, 0.264055));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  /// @note Important test since different mwms for one segment are used and this can cause extra GoStraight.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToLeft);
}

// Test on the route from Leninsky prospect to its frontage road and turns generated on the route.
UNIT_TEST(Russia_Moscow_LeninskyProsp_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.69035, 37.54948), {0., 0.},
                                  mercator::FromLatLon(55.69188, 37.55293));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
}

// Test on the route from TTK (primary) to a link.
UNIT_TEST(Russia_Moscow_TTKToLink_TurnTest)
{
TRouteResult const routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                mercator::FromLatLon(55.78594, 37.56656), {0., 0.},
                                mercator::FromLatLon(55.78598, 37.56737));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

// Test on the turn from TTK (primary) to a secondary road.
UNIT_TEST(Russia_Moscow_TTKToBegovayAlleya_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.77946, 37.55779), {0., 0.},
                                  mercator::FromLatLon(55.77956, 37.55891));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on the turn from TTK (primary) to a service road. The angle of the turn is not slight.
UNIT_TEST(Russia_Moscow_TTKToService_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.78856, 37.57017), {0., 0.},
                                  mercator::FromLatLon(55.78869, 37.57133));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on a turn from TTK (primary) to an unclassified road. The angle of the turn is slight.
UNIT_TEST(Russia_Moscow_TTKToNMaslovka_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.79057, 37.57292), {0., 0.},
                                  mercator::FromLatLon(55.79132, 37.57481));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::ExitHighwayToRight);
}

UNIT_TEST(Russia_Moscow_Complicated_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.68412, 37.60166), {0., 0.},
                                  mercator::FromLatLon(55.68426, 37.59854));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightLeft});
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(USA_Tampa_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(28.04875, -82.58292), {0., 0.},
                                  mercator::FromLatLon(28.04459, -82.58448));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {CarDirection::TurnSlightRight, CarDirection::TurnRight});
}

// Test on go straight direction if it's possible to go through a roundabout.
UNIT_TEST(Russia_Moscow_Minskia1_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.7355, 37.48717), {0., 0.},
                                  mercator::FromLatLon(55.73694, 37.48587));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_Moscow_Minskia2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.74244, 37.4808), {0., 0.},
                                  mercator::FromLatLon(55.74336, 37.48124));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// This test on getting correct (far enough) outgoing turn point (result of method GetPointForTurn())
// despite the fact that a small road adjoins immediately after the turn point.
UNIT_TEST(Russia_Moscow_Barikadnai_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.75979, 37.58502), {0., 0.},
                                  mercator::FromLatLon(55.75936, 37.58286));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// This test on getting correct (far enough) outgoing turn point (result of method GetPointForTurn()).
UNIT_TEST(Russia_Moscow_Komsomolsky_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.73442, 37.59391), {0., 0.},
                                  mercator::FromLatLon(55.73485, 37.59543));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

// Test on no go straight direction in case of a route along a big road and pass smaller ones.
UNIT_TEST(Russia_Moscow_TTKNoGoStraight_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.78949, 37.5711), {0., 0.},
                                  mercator::FromLatLon(55.78673, 37.56726));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Russia_MoscowLeninskyProsp2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.80376, 37.52048), {0., 0.},
                                  mercator::FromLatLon(55.80442, 37.51802));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightRight);
}

UNIT_TEST(Germany_ShuttleTrain_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(54.78152, 8.83952), {0., 0.},
                                  mercator::FromLatLon(54.79281, 8.83466));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // No turns on shutte train road.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 3 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 1).TestValid().TestOneOfDirections(
      {CarDirection::GoStraight, CarDirection::TurnSlightRight});
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(Germany_ShuttleTrain2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(54.90181, 8.32472), {0., 0.},
                                  mercator::FromLatLon(54.91681, 8.31346));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // No turns on shutte train road.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSharpRight);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::TurnRight);
  integration::GetNthTurn(route, 4).TestValid().TestDirection(CarDirection::TurnLeft);
}

UNIT_TEST(Cyprus_Nicosia_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.12459, 33.34449), {0., 0.},
                                  mercator::FromLatLon(35.13832, 33.34741));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // No SlightTurns at not straight junctions. Issue #2262.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestRouteLength(route, 1941.);

  integration::TestTurnCount(route, 0 /* expectedTurnCount */);
}

UNIT_TEST(Cyprus_Nicosia2_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.14528, 33.34014), {0., 0.},
                                  mercator::FromLatLon(35.13930, 33.34171));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // No SlightTurns at not straight junctions. Issue #2262.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(Cyprus_NicosiaPresidentialPark_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.15992, 33.34625), {0., 0.},
                                  mercator::FromLatLon(35.15837, 33.35058));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2438.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnRight);
}

UNIT_TEST(Cyprus_NicosiaSchoolParking_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.15395, 33.35000), {0., 0.},
                                  mercator::FromLatLon(35.15159, 33.34961));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2438.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnSlightLeft);
}

UNIT_TEST(Cyprus_NicosiaStartRoundabout_TurnTest)
{
  // Start movement at roundabout.
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.12788, 33.36568), {0., 0.},
                                  mercator::FromLatLon(35.12302, 33.37632));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2531.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 3 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::LeaveRoundAbout);
  integration::GetNthTurn(route, 0).TestValid().TestRoundAboutExitNum(3);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::EnterRoundAbout);
  integration::GetNthTurn(route, 1).TestValid().TestRoundAboutExitNum(3);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::LeaveRoundAbout);
  integration::GetNthTurn(route, 2).TestValid().TestRoundAboutExitNum(3);
}

UNIT_TEST(Cyprus_NicosiaSmallRoundabout_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(35.13103, 33.37222), {0., 0.},
                                  mercator::FromLatLon(35.13099, 33.37089));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2570.
  // Don't ignore exit to parking for this small roundabout.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::EnterRoundAbout);
  integration::GetNthTurn(route, 0).TestValid().TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::LeaveRoundAbout);
  integration::GetNthTurn(route, 1).TestValid().TestRoundAboutExitNum(2);
}

UNIT_TEST(Cyprus_A1AlphaMega_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(34.81834, 33.35914), {0., 0.},
                                  mercator::FromLatLon(34.81881, 33.36561));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2536.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 1 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  // No extra GoStraight caused by possible turn to parking.
}

UNIT_TEST(Crimea_Roundabout_test)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(45.20895, 33.32677), {0., 0.},
                                  mercator::FromLatLon(45.20899, 33.32840));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  // Issue #2536.
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 2 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::EnterRoundAbout);
  integration::GetNthTurn(route, 0).TestValid().TestRoundAboutExitNum(3);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::LeaveRoundAbout);
  integration::GetNthTurn(route, 1).TestValid().TestRoundAboutExitNum(3);
}


UNIT_TEST(Russia_Moscow_OnlyUTurnTest1_TurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.90382, 37.40219), {0.0, 0.0},
                                  mercator::FromLatLon(55.90278, 37.40354));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::TestTurnCount(route, 5 /* expectedTurnCount */);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(CarDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestDirection(CarDirection::EnterRoundAbout);
  integration::GetNthTurn(route, 4).TestValid().TestDirection(CarDirection::LeaveRoundAbout);
}
/*
UNIT_TEST(Russia_Moscow_OnlyUTurnTest1WithDirection_TurnTest)
{
  auto const startDir = mercator::FromLatLon(55.90423, 37.40176);
  auto const endDir = mercator::FromLatLon(55.90218, 37.40433);
  auto const direction = endDir - startDir;

  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(55.90382, 37.40219), direction,
                                  mercator::FromLatLon(55.90278, 37.40354));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  integration::GetNthTurn(route, 0).TestValid().TestDirection(CarDirection::UTurnLeft);
}
*/

// Slight turn to the long link which is followed by another link.
UNIT_TEST(USA_California_Cupertino_TurnTestNextRoad)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(37.5031583, -122.3317724), {0., 0.},
                                  mercator::FromLatLon(37.5110368, -122.3317238));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  double d;
  TurnItem turn;
  route.GetNearestTurn(d, turn);
  TEST_EQUAL(turn.m_turn, CarDirection::TurnSlightRight, ());
  RouteSegment::RoadNameInfo ri;
  route.GetNextTurnStreetName(ri);
  TEST_EQUAL(ri.m_destination, "San Mateo; Hayward; Belmont", ());
  TEST_EQUAL(ri.m_destination_ref, "CA 92 East", ());
}

// Take destination from link and destination_ref from the next road.
UNIT_TEST(Cyprus_Governors_Beach_TurnTestNextRoad)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(34.7247792, 33.2719482), {0., 0.},
                                  mercator::FromLatLon(34.7230031, 33.2727327));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  double d;
  TurnItem turn;
  route.GetNearestTurn(d, turn);
  TEST_EQUAL(turn.m_turn, CarDirection::ExitHighwayToLeft, ());
  RouteSegment::RoadNameInfo ri;
  route.GetNextTurnStreetName(ri);
  TEST_EQUAL(ri.m_destination, "Governer's Beach; Pentakomo", ());
  TEST_EQUAL(ri.m_destination_ref, "B1", ());
}

// Exit which is marked as non-link, but has link tags m_destination_ref and m_destination.
UNIT_TEST(Cyprus_A1_A5_TurnTestNextRoad)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                  mercator::FromLatLon(34.83254, 33.3835), {0., 0.},
                                  mercator::FromLatLon(34.83793, 33.3926));

  Route const & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;

  TEST_EQUAL(result, RouterResultCode::NoError, ());

  double d;
  TurnItem turn;
  route.GetNearestTurn(d, turn);
  TEST_EQUAL(turn.m_turn, CarDirection::TurnSlightLeft, ());
  RouteSegment::RoadNameInfo ri;
  route.GetNextTurnStreetName(ri);
  TEST_EQUAL(ri.m_destination, "Larnaka; Kefinou; Airport", ());
  TEST_EQUAL(ri.m_destination_ref, "A5", ());
}
