#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowSevTushinoParkPreferingBicycleWay)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(),
      MercatorBounds::FromLatLon(55.87445, 37.43711), {0., 0.},
      MercatorBounds::FromLatLon(55.87203, 37.44274), 460.0);
}

UNIT_TEST(RussiaMoscowNahimovskyLongRoute)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(55.66151, 37.63320), {0., 0.},
      MercatorBounds::FromLatLon(55.67695, 37.56220), 5670.0);
}

UNIT_TEST(RussiaDomodedovoSteps)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(55.44010, 37.77416), {0., 0.},
      MercatorBounds::FromLatLon(55.43975, 37.77272), 123.0);
}

UNIT_TEST(SwedenStockholmCyclewayPriority)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(59.33151, 18.09347), {0., 0.},
      MercatorBounds::FromLatLon(59.33052, 18.09391), 113.0);
}

// Note. If the closest to start or finish road has "bicycle=no" tag the closest road where
// it's allowed to ride bicycle will be found.
UNIT_TEST(NetherlandsAmsterdamBicycleNo)
{
  integration::CalculateRouteAndTestRouteLength(
    integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(52.32716, 5.05932), {0., 0.},
    MercatorBounds::FromLatLon(52.32587, 5.06121), 363.4);
}

UNIT_TEST(NetherlandsAmsterdamBicycleYes)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(52.32872, 5.07527),
      {0.0, 0.0}, MercatorBounds::FromLatLon(52.33853, 5.08941));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());
  TEST(my::AlmostEqualAbs(route.GetTotalTimeSec(), 356.0, 1.0), ());
}

UNIT_TEST(NetherlandsAmsterdamSingelStOnewayBicycleNo)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(52.3785, 4.89407), {0., 0.},
      MercatorBounds::FromLatLon(52.37462, 4.88983), 519.0);
}

UNIT_TEST(RussiaMoscowKashirskoe16ToCapLongRoute)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(55.66230, 37.63214), {0., 0.},
      MercatorBounds::FromLatLon(55.68895, 37.70286), 7057.0);
}

UNIT_TEST(RussiaKerchStraitFerryRoute)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Bicycle>(), MercatorBounds::FromLatLon(45.4167, 36.7658), {0.0, 0.0},
      MercatorBounds::FromLatLon(45.3653, 36.6161), 18000.0);
}
