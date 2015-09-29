#include "testing/testing.hpp"

#include "integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"


using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest) // FAILED!
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.796936768447288557, 37.537544383032567907), {0., 0.},
      MercatorBounds::FromLatLon(55.802121583039429709, 37.538908531885972764));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  integration::TestTurnCount(route, 3);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestOneOfDirections({TurnDirection::TurnSlightLeft, TurnDirection::TurnLeft});
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(TurnDirection::TurnRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 2033.);
}

UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.851822802555439296, 37.39533227665661741), {0., 0.},
      MercatorBounds::FromLatLon(55.843866280669693936, 37.392503720352721075));
  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 4);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.388482521388539, 67.633382734905041}, 20.)
      .TestDirection(TurnDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestPoint({37.387117276989784, 67.633369323859881}, 20.)
      .TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 2)
      .TestValid()
      .TestPoint({37.387380133475205, 67.632781920081243}, 20.)
      .TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 3)
      .TestValid()
      .TestPoint({37.390526364673121, 67.633106467374461}, 20.)
      .TestDirection(TurnDirection::TurnRight);

  integration::TestRouteLength(route, 1637.);
}

UNIT_TEST(RussiaMoscowTrikotagniAndPohodniRoundaboutTurnTest) //Failed
{
  TRouteResult const routeResult = integration::CalculateRoute(
    integration::GetOsrmComponents(),
    MercatorBounds::FromLatLon(55.831185109716038539, 37.405153751040685961), {0., 0.},
    MercatorBounds::FromLatLon(55.833843764465711956, 37.405210716570380214));
  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(TurnDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(TurnDirection::LeaveRoundAbout);

  integration::TestRouteLength(route, 387.);
}

UNIT_TEST(RussiaMoscowPlanetnaiTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.802161062035722239, 37.546683164991776493), {0., 0.},
      MercatorBounds::FromLatLon(55.801690565608659256, 37.549153861529006804));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 214.);
}

UNIT_TEST(RussiaMoscowNoTurnsOnMKADTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.846564134248033895, 37.391635636579785285), {0., 0.},
      MercatorBounds::FromLatLon(55.566611639650901111, 37.692547253527685314));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.682761085650043, 67.140620702062705})
      .TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(RussiaMoscowTTKKashirskoeShosseOutTurnTest) // Failed
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.70160163595921432, 37.606320023648997619), {0., 0.},
      MercatorBounds::FromLatLon(55.693494620969019593, 37.621220025471167503));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  // Checking a turn in case going from a not-link to a link
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPankratevskiPerBolshaySuharedskazPloschadTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.771770051239521138, 37.635563528539393019), {0., 0.},
      MercatorBounds::FromLatLon(55.772033898671161012, 37.637054339197831609));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnRight);
}

UNIT_TEST(RussiaMoscowMKADPutilkovskeShosseTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.853059336007213176, 37.394141645624102921), {0., 0.},
      MercatorBounds::FromLatLon(55.850996974780500182, 37.391050708989460816));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPetushkovaShodniaReverTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.841047134659639539, 37.405917692164507571), {0., 0.},
      MercatorBounds::FromLatLon(55.839290964451890886, 37.408550782937481927));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0);
}

UNIT_TEST(RussiaHugeRoundaboutTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.801410375094782523, 37.325810690728495445), {0., 0.},
      MercatorBounds::FromLatLon(55.800757342904596214, 37.325360456262153264));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(TurnDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(4);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(TurnDirection::LeaveRoundAbout)
      .TestRoundAboutExitNum(0);
}

UNIT_TEST(BelarusMiskProspNezavisimostiMKADTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(53.936425941857663702, 27.658572046123946819), {0., 0.},
      MercatorBounds::FromLatLon(53.939337747485986085, 27.670461944729382253));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

// Test case: turning form one street to another one with the same name.
// An end user shall be informed about this manoeuvre.
UNIT_TEST(RussiaMoscowPetushkovaPetushkovaTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.836368736509733424, 37.405549999999998079), {0., 0.},
      MercatorBounds::FromLatLon(55.837076293494696699, 37.404890000000001749));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);
}

// Test case: a route goes straight along a unnamed big link road when joined a small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(RussiaMoscowMKADLeningradkaTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(), MercatorBounds::FromLatLon(55.87992, 37.43940),
      {0., 0.}, MercatorBounds::FromLatLon(55.87854, 37.44865));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
}

UNIT_TEST(BelarusMKADShosseinai)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(55.315418609956843454, 29.431229999999999336), {0., 0.},
      MercatorBounds::FromLatLon(55.316562400560414403, 29.426259999999999195));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::GoStraight, TurnDirection::TurnSlightRight});
}

// Test case: a route goes straight along a unnamed big road when joined small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(ThailandPhuketNearPrabarameeRoad)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(),
      MercatorBounds::FromLatLon(7.9179763567919980716, 98.369370000000003529), {0., 0.},
      MercatorBounds::FromLatLon(7.9072494672603861332, 98.367850000000004229));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0);
}

// Test case: a route goes in Moscow from Varshavskoe shosse (from the city center direction)
// to MKAD (the outer side). A turn instruction (to leave Varshavskoe shosse)
// shall be generated.
UNIT_TEST(RussiaMoscowVarshavskoeShosseMKAD)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(), MercatorBounds::FromLatLon(55.58210, 37.59695), {0., 0.},
      MercatorBounds::FromLatLon(55.57514, 37.61020));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

// Test case: a route goes in Moscow from Tverskaya street (towards city center)
// to Mokhovaya street. A turn instruction (turn left) shell be generated.
UNIT_TEST(RussiaMoscowTverskajaOkhotnyRyadTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(), MercatorBounds::FromLatLon(55.75765, 37.61355), {0., 0.},
      MercatorBounds::FromLatLon(55.75737, 37.61601));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);
}

UNIT_TEST(RussiaMoscowBolshoyKislovskiyPerBolshayaNikitinskayaUlTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(), MercatorBounds::FromLatLon(55.75574, 37.60702), {0., 0.},
      MercatorBounds::FromLatLon(55.75586, 37.60819));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnRight);
}

// Test case: a route goes in Moscow along Leningradskiy Prpt (towards city center)
// and makes u-turn. A only one turn instruction (turn left) shell be generated.
UNIT_TEST(RussiaMoscowLeningradskiyPrptToTheCenterUTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetOsrmComponents(), MercatorBounds::FromLatLon(55.79231, 37.54951), {0., 0.},
      MercatorBounds::FromLatLon(55.79280, 37.55028));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 2);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);
}
