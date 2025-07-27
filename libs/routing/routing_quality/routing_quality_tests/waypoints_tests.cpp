#include "testing/testing.hpp"

#include "routing/vehicle_mask.hpp"

#include "routing/routing_quality/waypoints.hpp"

#include <utility>
#include <vector>

namespace waypoins_tests
{
using namespace routing;
using namespace routing_quality;
using namespace std;

UNIT_TEST(RoutingQuality_CompareSmoke)
{
  // From office to Aseeva 6.
  TEST(CheckCarRoute({55.79723, 37.53777} /* start */, {55.80634, 37.52886} /* finish */,
                     {{{55.79676, 37.54138},
                       {55.79914, 37.53582},
                       {55.80353, 37.52478},
                       {55.80556, 37.52770}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_Sokol2Mayakovskaya)
{
  // From Sokol to Mayakovskaya through Leningradsky Avenue but not through its alternate.
  Params params(VehicleType::Car, {55.80432, 37.51603} /* start */, {55.77019, 37.59558} /* finish */);

  // All points lie on the alternate so the result should be 0.
  ReferenceRoutes waypoints = {{{55.79599, 37.54114}, {55.78142, 37.57364}, {55.77863, 37.57989}}};

  TEST_EQUAL(CheckWaypoints(params, std::move(waypoints)), 0.0, ());
}
}  // namespace waypoins_tests
