#include "../../testing/testing.hpp"

#include "osrm_test_tools.hpp"

#include "../routing/route.hpp"


using namespace routing;

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();

  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.537544383032568, 67.536216737893028},
                                                               {0., 0.}, {37.538908531885973, 67.54544090660923});

  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, OsrmRouter::NoError, ());

  integration::GetNthTurn(route, 0).TestValid().TestPoint({37.545981835916507, 67.530713137468041})
      .TestOneOfDirections({turns::TurnSlightLeft , turns::TurnLeft});
  integration::GetNthTurn(route, 1).TestValid().TestPoint({37.546738218864334, 67.531659957257347}).TestDirection(turns::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestPoint({37.539925407915746, 67.537083383925875}).TestDirection(turns::TurnRight);

  integration::GetTurnByPoint(route, {37., 67.}).TestNotValid();
  integration::GetTurnByPoint(route, {37.546738218864334, 67.531659957257347}).TestValid().TestDirection(turns::TurnLeft);

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
  integration::GetNthTurn(route, 0).TestValid().TestPoint({37.388482521388539, 67.633382734905041}, 5.)
      .TestOneOfDirections({turns::GoStraight, turns::TurnSlightRight});
  integration::GetNthTurn(route, 1).TestValid().TestPoint({37.387117276989784, 67.633369323859881}).TestDirection(turns::TurnLeft);
  integration::GetNthTurn(route, 2).TestValid().TestPoint({37.387380133475205, 67.632781920081243}).TestDirection(turns::TurnLeft);
  integration::GetNthTurn(route, 3).TestValid().TestPoint({37.390526364673121, 67.633106467374461}).TestDirection(turns::TurnRight);

  integration::TestTurnCount(route, 4);
  integration::TestRouteLength(route, 1637.);
}

