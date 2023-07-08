#include "testing/testing.hpp"

#include "routing/routing_callbacks.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"


namespace bicycle_route_test
{
using namespace integration;
using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowSevTushinoParkPreferingBicycleWay)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(55.87445, 37.43711), {0., 0.},
      mercator::FromLatLon(55.87203, 37.44274), 460.0);
}

UNIT_TEST(RussiaMoscowNahimovskyLongRoute)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(55.66151, 37.63320), {0., 0.},
      mercator::FromLatLon(55.67695, 37.56220), 5670.0);
}

UNIT_TEST(RussiaDomodedovoSteps)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(55.44010, 37.77416), {0., 0.},
      mercator::FromLatLon(55.43975, 37.77272), 100.0);
}

UNIT_TEST(SwedenStockholmCyclewayPriority)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(59.33151, 18.09347), {0., 0.},
      mercator::FromLatLon(59.33052, 18.09391), 113.0);
}

// Note. If the closest to start or finish road has "bicycle=no" tag the closest road where
// it's allowed to ride bicycle will be found.
UNIT_TEST(NetherlandsAmsterdamBicycleNo)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(52.32716, 5.05932), {0., 0.},
      mercator::FromLatLon(52.32587, 5.06121), 338.0);
}

UNIT_TEST(NetherlandsAmsterdamBicycleYes)
{
  TRouteResult const routeResult = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle),
                     mercator::FromLatLon(52.32872, 5.07527), {0.0, 0.0},
                     mercator::FromLatLon(52.33853, 5.08941));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());
  TestRouteTime(*routeResult.first, 296.487);
}

// This test on tag cycleway=opposite for a streets which have oneway=yes.
// It means bicycles may go in the both directions.
UNIT_TEST(NetherlandsAmsterdamSingelStCyclewayOpposite)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(52.37571, 4.88591), {0., 0.},
      mercator::FromLatLon(52.37736, 4.88744), 212.8);
}

UNIT_TEST(RussiaMoscowKashirskoe16ToCapLongRoute)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(55.66230, 37.63214), {0., 0.},
      mercator::FromLatLon(55.68927, 37.70356), 7075.0);
}

// Passing through living_street and service are allowed in Russia
UNIT_TEST(RussiaMoscowServicePassThrough1)
{
  TRouteResult route =
        integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Bicycle),
                                    mercator::FromLatLon(55.66230, 37.63214), {0., 0.},
                                    mercator::FromLatLon(55.68895, 37.70286));
  TEST_EQUAL(route.second, RouterResultCode::NoError, ());
}

UNIT_TEST(RussiaMoscowServicePassThrough2)
{
  TRouteResult route =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Bicycle),
                                  mercator::FromLatLon(55.69038, 37.70015), {0., 0.},
                                  mercator::FromLatLon(55.69123, 37.6953));
  TEST_EQUAL(route.second, RouterResultCode::NoError, ());
}

UNIT_TEST(RussiaMoscowServicePassThrough3)
{
  TRouteResult route =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Bicycle),
                                  mercator::FromLatLon(55.79649, 37.53738), {0., 0.},
                                  mercator::FromLatLon(55.79618, 37.54071));
  TEST_EQUAL(route.second, RouterResultCode::NoError, ());
}

UNIT_TEST(RussiaKerchStraitFerryRoute)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(45.4167, 36.7658), {0.0, 0.0},
      mercator::FromLatLon(45.3653, 36.6161), 17151.4);
}

// Test on building bicycle route past ferry.
UNIT_TEST(SwedenStockholmBicyclePastFerry)
{
  CalculateRouteAndTestRouteLength(
      GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(59.4725, 18.51355), {0.0, 0.0},
      mercator::FromLatLon(59.42533, 18.35991), 14338.0);
}

// Test on cross mwm bicycle routing.
UNIT_TEST(CrossMwmKaliningradRegionToLiepaja)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(54.63519, 21.80749), {0., 0.},
      mercator::FromLatLon(56.51119, 21.01847), 271237);

  // Valhalla makes the same route, but GraphHopper makes a detour (via unpaved):
  // https://www.openstreetmap.org/directions?engine=graphhopper_bicycle&route=55.340%2C21.459%3B55.715%2C21.135
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(55.3405073, 21.4595925), {0., 0.},
      mercator::FromLatLon(55.7140174, 21.1365445), 57180.3);
}

// Test on riding up from Adeje (sea level) to Vilaflor (altitude 1400 meters).
UNIT_TEST(SpainTenerifeAdejeVilaflor)
{
  TRouteResult const res = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle),
                              mercator::FromLatLon(28.11984, -16.72592), {0., 0.},
                              mercator::FromLatLon(28.15865, -16.63704));
  TEST_EQUAL(res.second, RouterResultCode::NoError, ());

  TestRouteLength(*res.first, 26997.4);
  TestRouteTime(*res.first, 11156.7);
}

// Test on riding down from Vilaflor (altitude 1400 meters) to Adeje (sea level).
UNIT_TEST(SpainTenerifeVilaflorAdeje)
{
  TRouteResult const res = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle),
                              mercator::FromLatLon(28.15865, -16.63704), {0., 0.},
                              mercator::FromLatLon(28.11984, -16.72592));
  TEST_EQUAL(res.second, RouterResultCode::NoError, ());

  TestRouteLength(*res.first, 25413.6);
  TestRouteTime(*res.first, 5105.25);
}

// Two tests on not building route against traffic on road with oneway:bicycle=yes.
UNIT_TEST(Munich_OnewayBicycle1)
{
  /// @todo Should combine TurnSlightLeft, TurnLeft, TurnLeft into UTurnLeft?
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(48.1601673, 11.5630245), {0.0, 0.0},
      mercator::FromLatLon(48.1606349, 11.5631699), 279.515 /* expectedRouteMeters */);
}

UNIT_TEST(Munich_OnewayBicycle2)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(48.17819, 11.57286), {0.0, 0.0},
      mercator::FromLatLon(48.17867, 11.57303), 201.532 /* expectedRouteMeters */);
}

// https://github.com/organicmaps/organicmaps/issues/1603
UNIT_TEST(London_GreenwichTunnel)
{
  // Avoiding barrier=gate https://www.openstreetmap.org/node/3881243716
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(51.4817397, -0.0100070258), {0.0, 0.0},
      mercator::FromLatLon(51.4883739, -0.00809729298), 1332.8 /* expectedRouteMeters */);
}

UNIT_TEST(Batumi_AvoidServiceDetour)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(41.6380014, 41.6269446), {0.0, 0.0},
      mercator::FromLatLon(41.6392113, 41.6260084), 156.465 /* expectedRouteMeters */);
}

UNIT_TEST(Gdansk_AvoidLongCyclewayDetour)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(54.2632738, 18.6771661), {0.0, 0.0},
      mercator::FromLatLon(54.2698882, 18.6765837), 753.837 /* expectedRouteMeters */);
}

// https://github.com/organicmaps/organicmaps/issues/4145
UNIT_TEST(Belarus_StraightFootway)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(53.8670285, 30.3162749), {0.0, 0.0},
      mercator::FromLatLon(53.876436, 30.3348084), 1613.34 /* expectedRouteMeters */);
}

UNIT_TEST(Spain_Madrid_ElevationsDetour)
{
  TRouteResult const r1 = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle),
                              mercator::FromLatLon(40.459616, -3.690031), {0., 0.},
                              mercator::FromLatLon(40.4408171, -3.69261893));
  TEST_EQUAL(r1.second, RouterResultCode::NoError, ());

  TRouteResult const r2 = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle),
                              mercator::FromLatLon(40.459616, -3.690031), {0., 0.},
                              mercator::FromLatLon(40.4403523, -3.69267444));
  TEST_EQUAL(r2.second, RouterResultCode::NoError, ());

  TEST(r1.first && r2.first, ());

  // The first route is shorter, but the second one is better by ETA because of elevation.
  // Can't say for sure is it ok or not, so this test may fail in future.
  TEST_LESS(r1.first->GetTotalDistanceMeters(), r2.first->GetTotalDistanceMeters(), ());
  TEST_GREATER(r1.first->GetTotalTimeSec(), r2.first->GetTotalTimeSec(), ());
}

UNIT_TEST(Spain_Zaragoza_Fancy_NoBicycle_Crossings)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(41.6523561, -0.881151311), {0.0, 0.0},
      mercator::FromLatLon(41.6476614, -0.885694674), 649.855 /* expectedRouteMeters */);
}

// https://github.com/organicmaps/organicmaps/issues/1201#issuecomment-946042937
UNIT_TEST(Netherlands_Use_Bicycle_Track)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(48.420723, 9.90350146), {0.0, 0.0},
      mercator::FromLatLon(48.4080367, 9.86597073), 3778.41 /* expectedRouteMeters */);
}

// https://github.com/orgs/organicmaps/discussions/5158#discussioncomment-5938807
UNIT_TEST(Finland_Use_Tertiary_LowTraffic)
{
  CalculateRouteAndTestRouteLength(GetVehicleComponents(VehicleType::Bicycle),
      mercator::FromLatLon(61.5445696, 23.9394003), {0.0, 0.0},
      mercator::FromLatLon(61.6153965, 23.876755), 9876.65 /* expectedRouteMeters */);
}

} // namespace bicycle_route_test
