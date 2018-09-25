#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

namespace
{
UNIT_TEST(RoutingQuality_FinlandBridgeInsteadOfFerry)
{
  TEST(CheckCarRoute({56.11155, 12.81101} /* start */, {55.59857, 12.3069} /* finish */,
                     {{{55.56602, 12.88537}}} /* reference track */),
       ());
}
}  // namespace
