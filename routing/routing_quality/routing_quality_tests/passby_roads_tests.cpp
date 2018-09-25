#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// In most cases a passby road should be preferred in case of going pass a city.
// Test on such cases should be grouped in this file.
namespace
{
UNIT_TEST(RoutingQuality_Zelegonrad2Domodedovo)
{
  // From Zelenograd to Domodedovo. MKAD should be preferred.
  TEST(CheckCarRoute({55.98301, 37.21141} /* start */, {55.42081, 37.89361} /* finish */,
                     {{{55.99751, 37.23804}, // Through M-11 and MKAD.
                       {56.00719, 37.28533},
                       {55.88759, 37.48068},
                       {55.83513, 37.39569},
                       {55.57103, 37.69203}},
                      {{55.99775, 37.24941}, // Through M-10 and MKAD.
                       {55.88627, 37.43915},
                       {55.86882, 37.40784},
                       {55.58645, 37.71672},
                       {55.57855, 37.75468}}} /* reference tracks */),
       ());
}

UNIT_TEST(RoutingQuality_BelarusKobrin)
{
  // Test on using a passby road around Kobirn.
  TEST(CheckCarRoute({52.18429, 24.20225} /* start */, {52.24404, 24.45842} /* finish */,
                     {{{52.18694, 24.39903}}} /* reference track */),
       ());
}
}  // namespace
