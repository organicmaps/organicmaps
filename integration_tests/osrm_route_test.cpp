#include "../../testing/testing.hpp"

#include "osrm_test_tools.hpp"

using namespace routing;

namespace
{
  UNIT_TEST(RussiaMoscowLenigradskiy39GerPanfilovtsev22RouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    TEST(integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.53758809983519, 67.536162466434234},
                                                      {0., 0.}, {37.40993977728661, 67.644784047393685}, 14296.), ());
  }

  UNIT_TEST(RussiaMoscowGerPanfilovtsev22SolodshaPravdiRouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    TEST(integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.409929478750627, 67.644798619710073},
                                                      {0., 0.}, {39.836562407458047, 65.774372510437971}, 253275.), ());
  }
}
