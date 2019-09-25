#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// TODO: Uncomment after upcoming tiles seeding
// There is a category of barriers through which no road must be built:
// ice roads (highway = ice_road), fords (highway = ford) etc.
// Tests on this kind of cases are grouped in this file.
namespace
{
  //UNIT_TEST(RoutingQuality_NoRoute_Broad_Node_Jamaica)
  //{
  //  TEST(!CheckCarRoute({17.947273, -76.254299} /* start */, {17.944998, -76.254598} /* finish */,
  //                     {{{17.9451506, -76.254426}}} /* reference point */),
  //      ());
  //}

  //UNIT_TEST(RoutingQuality_NoRoute_Broad_Way_Jamaica)
  //{
  //  TEST(!CheckCarRoute({18.102603, -76.983744} /* start */, {18.100315, -76.983744} /* finish */,
  //                     {{{18.1007818, -76.9841238}}} /* reference point */),
  //      ());
  //}

  //UNIT_TEST(RoutingQuality_NoRoute_Broad_Node_Norway)
  //{
  //  TEST(!CheckCarRoute({56.985110, 9.770313} /* start */, {56.983581, 9.778153} /* finish */,
  //                      {{{56.9854269, 9.7744898}}} /* reference point */),
  //       ());
  //}

  //UNIT_TEST(RoutingQuality_NoRoute_Broad_Node_Norway_2)
  //{
  //  TEST(!CheckCarRoute({56.202478, 8.775192} /* start */, {56.197321, 8.791904} /* finish */,
  //                      {{{56.2017246, 8.7787964}}} /* reference point */),
  //       ());
  //}
}  // namespace
