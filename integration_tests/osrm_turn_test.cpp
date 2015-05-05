#include "testing/testing.hpp"

#include "integration_tests/osrm_test_tools.hpp"

#include "routing/route.hpp"


using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();

  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.537544383032568, 67.536216737893028},
                                                               {0., 0.}, {37.538908531885973, 67.54544090660923});

  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, OsrmRouter::NoError, ());

  integration::GetNthTurn(route, 0).TestValid().TestPoint({37.545981835916507, 67.530713137468041})
      .TestOneOfDirections({TurnDirection::TurnSlightLeft , TurnDirection::TurnLeft});
  integration::GetNthTurn(route, 1).TestValid().TestPoint({37.546738218864334, 67.531659957257347}).TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestPoint({37.539925407915746, 67.537083383925875}).TestDirection(TurnDirection::TurnRight);

  integration::GetTurnByPoint(route, {37., 67.}).TestNotValid();
  integration::GetTurnByPoint(route, {37.546738218864334, 67.531659957257347}).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 2033.);
}

UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.395332276656617, 67.633925439079519},
                                                               {0., 0.}, {37.392503720352721, 67.61975260731343});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 4);
  integration::GetNthTurn(route, 0).TestValid().TestPoint({37.388482521388539, 67.633382734905041}, 5.)
      .TestDirection(TurnDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1).TestValid().TestPoint({37.387117276989784, 67.633369323859881}).TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestPoint({37.387380133475205, 67.632781920081243}).TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestPoint({37.390526364673121, 67.633106467374461}).TestDirection(TurnDirection::TurnRight);

  integration::TestRouteLength(route, 1637.);
}

UNIT_TEST(RussiaMoscowTrikotagniAndPohodniRoundaboutTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.405153751040686, 67.5971698246356},
                                                               {0., 0.}, {37.40521071657038, 67.601903779043795});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 2);

  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::EnterRoundAbout).TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(TurnDirection::LeaveRoundAbout);

  integration::TestRouteLength(route, 387.);
}

UNIT_TEST(RussiaMoscowPlanetnaiTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.546683164991776, 67.545511147376089},
                                                               {0., 0.}, {37.549153861529007, 67.54467404790482});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 214.);
}

UNIT_TEST(RussiaMoscowNoTurnsOnMKADTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.391635636579785, 67.62455792789649},
                                                               {0., 0.}, {37.692547253527685, 67.127684414191762});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestPoint({37.682761085650043, 67.140620702062705})
      .TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(RussiaMoscowTTKKashirskoeShosseOutTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.606320023648998, 67.36682695403141},
                                                               {0., 0.}, {37.621220025471168, 67.352441627022912});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 2);
  /// Checking turn in case going from not link to link
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
  /// Checking turn in case of ingoing edge(s)
  integration::GetNthTurn(route, 1).TestValid().TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPankratevskiPerBolshaySuharedskazPloschadTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.635563528539393, 67.491460725721268},
                                                               {0., 0.}, {37.637054339197832, 67.491929797067371});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnRight);
}

UNIT_TEST(RussiaMoscowMKADPutilkovskeShosseTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.394141645624103, 67.63612831787222},
                                                               {0., 0.}, {37.391050708989461, 67.632454269643091});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPetushkovaShodniaReverTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.405917692164508, 67.614731601278493},
                                                               {0., 0.}, {37.408550782937482, 67.61160397953185});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 0);
}

UNIT_TEST(RussiaHugeRoundaboutTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.325810690728495, 67.544175542376436},
                                                               {0., 0.}, {37.325360456262153, 67.543013703414516});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 2);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::EnterRoundAbout).TestRoundAboutExitNum(5);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(TurnDirection::LeaveRoundAbout).TestRoundAboutExitNum(0);
}

UNIT_TEST(BelarusMiskProspNezavisimostiMKADTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {27.658572046123947, 64.302533720228126},
                                                               {0., 0.}, {27.670461944729382, 64.307480201489582});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}
