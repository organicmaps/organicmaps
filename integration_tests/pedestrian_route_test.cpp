#include "testing/testing.hpp"

#include "integration_tests/routing_test_tools.hpp"

#include "../indexer/mercator.hpp"

using namespace routing;

UNIT_TEST(Zgrad424aTo1207)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9963, 37.2036), {0., 0.},
      MercatorBounds::FromLatLon(55.9955, 37.1948), 683.);
}

UNIT_TEST(Zgrad924aTo418)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9999, 37.2021), 2526.);
}

UNIT_TEST(Zgrad924aToFilaretovskyChurch)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9915, 37.1808), 1220.);
}

UNIT_TEST(Zgrad924aTo1145)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9924, 37.1853), 1400.);
}

UNIT_TEST(MoscowMuzeonToLebedinoeOzeroGorkyPark)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.7348, 37.606), {0., 0.},
      MercatorBounds::FromLatLon(55.724, 37.5956), 1617.);
}

// Uncomment tests below when PedestrianModel will be improved
/*

UNIT_TEST(Zgrad315parkingToMusicSchoolBus_BadRoute)
{
  // Bad route:
  // route goes through a highway-tertiary.

  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9996, 37.2174), {0., 0.},
      MercatorBounds::FromLatLon(55.9999, 37.2179), 161.);
}

UNIT_TEST(Zgrad924aToKrukovo_BadRoute)
{
  // Bad route:
  // route goes through a highway-primary.

  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9802, 37.1736), 0.);
}

UNIT_TEST(MoscowMailRuStarbucksToPetrovskoRazumovskyAlley_BadRoute)
{
  // Bad route:
  // route goes through a highway-tertiary although an available pedestrian road.

  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(55.7971, 37.5376), {0., 0.},
      MercatorBounds::FromLatLon(55.7953, 37.5597), 0.);
}

*/

UNIT_TEST(AustraliaMelburn_AvoidMotorway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(-37.7936, 144.985), {0., 0.},
      MercatorBounds::FromLatLon(-37.7896, 145.025), 5015.);
}

UNIT_TEST(AustriaWein_AvoidTrunk)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(48.233, 16.3562), {0., 0.},
      MercatorBounds::FromLatLon(48.2458, 16.3704), 2627.);
}

UNIT_TEST(FranceParis_AvoidBridleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(48.859, 2.25452), {0., 0.},
      MercatorBounds::FromLatLon(48.8634, 2.24315), 1307.);
}

UNIT_TEST(GermanyBerlin_AvoidCycleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(52.5459, 13.3952), {0., 0.},
      MercatorBounds::FromLatLon(52.5413, 13.3989), 1008.);
}

UNIT_TEST(HungaryBudapest_AvoidMotorway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(47.5535, 19.1321), {0., 0.},
      MercatorBounds::FromLatLon(47.595, 19.2235), 13265.);
}

UNIT_TEST(PolandWarshaw_AvoidCycleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetPedestrianComponents(),
      MercatorBounds::FromLatLon(52.2487, 21.0173), {0., 0.},
      MercatorBounds::FromLatLon(52.25, 21.0164), 372.);
}
