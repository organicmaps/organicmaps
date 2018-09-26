#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// Test on preferring better but longer roads should be grouped in this file.
namespace
{
UNIT_TEST(RoutingQuality_RussiaMoscowTushino)
{
  TEST(CheckCarRoute({55.84398, 37.45018} /* start */, {55.85489, 37.43784} /* finish */,
                     {{{55.84343, 37.43949}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_TurkeyIzmirArea)
{
  TEST(CheckCarRoute({38.80146, 26.97696} /* start */, {39.06835, 26.88686} /* finish */,
                     {{{39.08146, 27.11798}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_BosniaAndHerzegovina)
{
  TEST(CheckCarRoute({42.71401, 18.30412} /* start */, {42.95101, 18.08966} /* finish */,
                     {{{42.88222,17.9919}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_CzechiaPrague)
{
  TEST(CheckCarRoute({50.10159, 14.43324} /* start */, {50.20976, 14.43361} /* finish */,
                     {{{50.15078, 14.49205}}} /* reference track */),
       ());
}
}  // namespace
