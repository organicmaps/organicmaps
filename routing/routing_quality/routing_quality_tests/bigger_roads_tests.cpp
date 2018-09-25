#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// Test on preferring better but longer roads should be grouped in this file.
namespace
{
UNIT_TEST(RoutingQuality_MoscowTushino)
{
  // Test in Tushino on routing along big routes.
  TEST(CheckCarRoute({55.84398, 37.45018} /* start */, {55.85489, 37.43784} /* finish */,
                     {{{55.84343, 37.43949}}} /* reference track */),
       ());
}
}  // namespace
