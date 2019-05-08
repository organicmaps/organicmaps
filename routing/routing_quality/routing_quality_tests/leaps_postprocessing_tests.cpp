#include "testing/testing.hpp"

#include "routing/vehicle_mask.hpp"

#include "routing/routing_quality/utils.hpp"
#include "routing/routing_quality/waypoints.hpp"

#include <utility>
#include <vector>

using namespace routing;
using namespace routing_quality;
using namespace std;

namespace
{
UNIT_TEST(RoutingQuality_NoLoop_MoscowToKazan)
{
  TEST(!CheckCarRoute({55.63113, 37.63054} /* start */, {55.68213, 52.37379} /* finish */,
                      {{{55.80643, 37.83981}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_Canada)
{
  TEST(!CheckCarRoute({53.53540, -113.50798} /* start */, {69.44402, -133.03189} /* finish */,
                      {{{61.74073, -121.21379}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_ZhitomirTver)
{
  TEST(!CheckCarRoute({50.94928, 28.64163} /* start */, {54.50750, 30.47854} /* finish */,
                      {{{51.62925, 29.08458}}} /* reference point */),
       ());
}
}  // namespace
