#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// Tests on availability to build route.
namespace
{
// Test on building route from and to Bilbau Airport.
UNIT_TEST(RoutingQuality_BilbauAirport)
{
  // From Bilbau Airport.
  TEST(CheckCarRoute({43.3017, -2.9151} /* start */, {43.2805, -2.87853} /* finish */,
                     {{{43.282, -2.88333}}} /* reference track */),
       ());

  // To Bilbau Airport.
  TEST(CheckCarRoute({43.28069, -2.87835} /* start */, {43.2805, -2.87853} /* finish */,
                     {{{43.28242, -2.88414}}} /* reference track */),
       ());
}

// Test on building route from and to Fairbanks International Airport.
UNIT_TEST(RoutingQuality_FairbanksAirport)
{
  // From Fairbanks International Airport.
  TEST(CheckCarRoute({64.81398, -147.85897} /* start */, {64.85873, -147.69372} /* finish */,
                     {{{64.85886, -147.70319}}} /* reference track */),
       ());

  // To Fairbanks International Airport.
  TEST(CheckCarRoute({64.85893, -147.69438} /* start */, {64.81398, -147.85897} /* finish */,
                     {{{64.85825, -147.71106}}} /* reference track */),
       ());
}
}  // namespace
