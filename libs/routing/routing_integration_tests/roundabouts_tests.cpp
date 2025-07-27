#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

#include "geometry/latlon.hpp"

#include <vector>

using namespace routing;

namespace
{
struct RouteData
{
  ms::LatLon m_start;
  ms::LatLon m_finish;
  double m_routeLengthM;
};

UNIT_TEST(MiniRoundabout_CalculateRoute)
{
  std::vector<RouteData> const routesWithMiniRoundabouts{{{51.45609, 0.05974}, {51.45562, 0.06005}, 61.3},
                                                         {{51.49746, -0.40027}, {51.49746, -0.40111}, 65.64},
                                                         {{55.98700, -3.38256}, {55.98665, -3.38260}, 43.6},
                                                         {{52.22163, 21.09296}, {52.22189, 21.09286}, 41.5}};

  for (auto const & route : routesWithMiniRoundabouts)
  {
    TRouteResult const routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                                                 mercator::FromLatLon(route.m_start), {0.0, 0.0},
                                                                 mercator::FromLatLon(route.m_finish));

    TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

    integration::TestRouteLength(*routeResult.first, route.m_routeLengthM);
  }
}
}  // namespace
