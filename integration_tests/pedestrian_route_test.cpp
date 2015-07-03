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
