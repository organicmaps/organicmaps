#include "testing/testing.hpp"

#include "routing/vehicle_mask.hpp"

#include "routing/routing_quality/waypoints.hpp"

#include <utility>
#include <vector>

namespace leaps_postprocessing_tests
{
using namespace routing;
using namespace routing_quality;
using namespace std;

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

UNIT_TEST(RoutingQuality_NoLoop_MacedoniaMontenegro_1)
{
  TEST(!CheckCarRoute({42.02901, 21.44826} /* start */, {42.77394, 18.94886} /* finish */,
                      {{{42.66290, 20.20949}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_MacedoniaMontenegro_2)
{
  TEST(!CheckCarRoute({41.14033, 22.50236} /* start */, {42.42424, 18.77128} /* finish */,
                      {{{42.66181, 20.28980}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_MacedoniaMontenegro_3)
{
  TEST(!CheckCarRoute({42.00375, 21.50582} /* start */, {42.14698, 19.04367} /* finish */,
                      {{{42.69257, 20.08659}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_AbkhaziaDonetsk)
{
  TEST(!CheckCarRoute({43.17286, 40.39015} /* start */, {48.01587, 37.80132} /* finish */,
                      {{{47.69821, 38.67685}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_RussiaKomiChuvashia)
{
  TEST(!CheckCarRoute({60.175920, 49.641070} /* start */, {56.077370, 47.875380} /* finish */,
                      {{{56.16673, 47.80223}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_SaratovMoscow)
{
  TEST(!CheckCarRoute({51.54151, 46.23666} /* start */, {56.12105, 37.61638} /* finish */,
                      {{{55.91385, 37.58531}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_BucharestMontenegro)
{
  TEST(!CheckCarRoute({44.41418, 26.11567} /* start */, {42.80962, 19.50849} /* finish */,
                      {{{42.66125, 20.26862}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_RyazanOblastGorshkovo)
{
  TEST(!CheckCarRoute({54.30282, 38.93610} /* start */, {56.37584, 37.40839} /* finish */,
                      {{{55.91318, 37.58136}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_BulgariaEastMontenegro)
{
  TEST(!CheckCarRoute({42.82058, 27.87946} /* start */, {42.42889, 18.70008} /* finish */,
                      {{{42.70045, 20.11199}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_NoLoop_SkopjeMontenegro)
{
  TEST(!CheckCarRoute({41.99137, 21.44921} /* start */, {42.25520, 18.89884} /* finish */,
                      {{{42.68296, 20.18158}}} /* reference point */),
       ());
}
}  // namespace leaps_postprocessing_tests
