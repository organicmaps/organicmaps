#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// There is a category of barriers through which no road must be built:
// ice roads (highway = ice_road).
// And there is a category of barriers through which the road should be built:
// fords (highway = ford).
// Tests on this kind of cases are grouped in this file.
namespace
{
UNIT_TEST(RoutingQuality_Broad_Node_Jamaica)
{
  TEST(CheckCarRoute({17.94727, -76.25429} /* start */, {17.94499, -76.25459} /* finish */,
                     {{{17.945150, -76.25442}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_Broad_Way_Jamaica)
{
  TEST(CheckCarRoute({18.10260, -76.98374} /* start */, {18.10031, -76.98374} /* finish */,
                     {{{18.10078, -76.98412}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_Broad_Node_Spain)
{
  TEST(CheckCarRoute({41.95027, -0.54596} /* start */, {56.98358, 9.77815} /* finish */,
                     {{{41.95026, -0.54562}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_Broad_Node_Norway)
{
  TEST(CheckCarRoute({56.20247, 8.77519} /* start */, {56.19732, 8.79190} /* finish */,
                     {{{56.20172, 8.77879}}} /* reference point */),
       ());
}
}  // namespace
