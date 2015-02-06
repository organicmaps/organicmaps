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

  TEST(integration::TestTurn(route, 0, {37.545981835916507, 67.530713137468041}, turns::TurnLeft), ());
  TEST(integration::TestTurn(route, 1, {37.546738218864334, 67.531659957257347}, turns::TurnLeft), ());
  TEST(integration::TestTurn(route, 2, {37.539925407915746, 67.537083383925875}, turns::TurnRight), ());
  TEST(!integration::TestTurn(route, 2, {37., 67.}, turns::TurnRight), ());

  TEST(integration::TestRouteLength(route, 2033.), ());
  TEST(!integration::TestRouteLength(route, 2533.), ());
  TEST(!integration::TestRouteLength(route, 1533.), ());
}

UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  RouteResultT const routeResult = integration::CalculateRoute(routerComponents, {37.395332276656617, 67.633925439079519},
                                            {0., 0.}, {37.392503720352721, 67.61975260731343});
  shared_ptr<Route> const route = routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());

  TEST(integration::TestTurn(route, 0, {37.388482521388539, 67.633382734905041}, turns::TurnSlightRight), ());
  TEST(integration::TestTurn(route, 1, {37.387117276989784, 67.633369323859881}, turns::TurnLeft), ());
  TEST(integration::TestTurn(route, 2, {37.387380133475205, 67.632781920081243}, turns::TurnLeft), ());
  TEST(integration::TestTurn(route, 3, {37.390526364673121, 67.633106467374461}, turns::TurnRight), ());

  TEST(integration::TestTurnCount(route, 4), ());

  TEST(integration::TestRouteLength(route, 1637.), ());

}

