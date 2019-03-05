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
// TODO: This test doesn't pass because routing::RouteWeight::operator<
// prefer roads with less number of barriers. It will be more useful to consider
// barriers only with access=no/private/etc tag.
//UNIT_TEST(RoutingQuality_RussiaToCrimeaFerry)
//{
//  // From Russia to Crimea
//  TEST(CheckCarRoute({45.34123, 36.67679} /* start */, {45.36479, 36.62194} /* finish */,
//                     {{{45.3532, 36.64912}}} /* reference track */),
//       ());
//}

UNIT_TEST(RoutingQuality_RussiaFromCrimeaFerry)
{
  TEST(CheckCarRoute({45.36479, 36.62194} /* start */, {45.34123, 36.67679} /* finish */,
                     {{{45.3532, 36.64912}}} /* reference track */),
       ());
}
}  // namespace
