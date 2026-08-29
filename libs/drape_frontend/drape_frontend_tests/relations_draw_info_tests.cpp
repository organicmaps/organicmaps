#include "drape_frontend/relations_draw_info.hpp"

#include "testing/testing.hpp"

namespace relations_draw_info_tests
{
UNIT_TEST(GetMapStyleForRouteTest)
{
  df::ActiveHikingCyclingRoutes routes;
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleDefaultLight, routes), MapStyleDefaultLight, ());

  routes.m_hiking = true;
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleDefaultLight, routes), MapStyleOutdoorsLight, ());
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleDefaultDark, routes), MapStyleOutdoorsDark, ());
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleCyclingDark, routes), MapStyleOutdoorsDark, ());

  routes.m_hiking = false;
  routes.m_cycling = true;
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleCyclingLight, routes), MapStyleCyclingLight, ());
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleCyclingDark, routes), MapStyleCyclingDark, ());

  routes.m_hiking = true;
  TEST_EQUAL(df::GetMapStyleForRoute(MapStyleCyclingLight, routes), MapStyleCyclingLight, ());
}
}  // namespace relations_draw_info_tests
