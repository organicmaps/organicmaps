#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowSevTushinoParkPreferingBicycleWay)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetBicycleComponents(), MercatorBounds::FromLatLon(55.87445, 37.43711), {0., 0.},
      MercatorBounds::FromLatLon(55.87203, 37.44274), 460.0);
}

UNIT_TEST(RussiaMoscowNahimovskyLongRoute)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetBicycleComponents(), MercatorBounds::FromLatLon(55.66151, 37.63320), {0., 0.},
      MercatorBounds::FromLatLon(55.67695, 37.56220), 7570.0);
}

UNIT_TEST(RussiaDomodedovoSteps)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetBicycleComponents(), MercatorBounds::FromLatLon(55.44020, 37.77409), {0., 0.},
      MercatorBounds::FromLatLon(55.43972, 37.77254), 123.0);
}

UNIT_TEST(SwedenStockholmCyclewayPriority)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetBicycleComponents(), MercatorBounds::FromLatLon(59.33151, 18.09347), {0., 0.},
      MercatorBounds::FromLatLon(59.33052, 18.09391), 113.0);
}
