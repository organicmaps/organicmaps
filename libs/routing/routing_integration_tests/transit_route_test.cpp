#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

namespace transit_route_test
{
using namespace routing;

UNIT_TEST(Transit_Moscow_CenterToKotelniki_CrossMwm)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(55.75018, 37.60971), {0.0, 0.0},
                                                         mercator::FromLatLon(55.67245, 37.86130));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 22968.6);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_Moscow_DubrovkaToTrtykovskya)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(55.71813, 37.67756), {0.0, 0.0},
                                                         mercator::FromLatLon(55.74089, 37.62831));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 7622.19);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_Moscow_NoSubwayTest)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(55.73893, 37.62438), {0.0, 0.0},
                                                         mercator::FromLatLon(55.73470, 37.62617));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 604.86);

  CHECK(routeResult.first, ());
  integration::CheckSubwayAbsent(*routeResult.first);
}

UNIT_TEST(Transit_Piter_FrunzenskyaToPlochadVosstaniya)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(59.90511, 30.31425), {0.0, 0.0},
                                                         mercator::FromLatLon(59.93096, 30.35872));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  /// @todo Check https://github.com/organicmaps/organicmaps/issues/1669 for details.
  integration::TestRouteLength(*routeResult.first, 5837.21);

  TEST(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_Piter_TooLongPedestrian)
{
  TRouteResult const routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                               mercator::FromLatLon(59.90511, 30.31425), {0.0, 0.0},
                                                               mercator::FromLatLon(59.78014, 30.50036));

  /// @todo Returns valid route now with long pedestrian part in the end, I don't see problems here.
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  TEST(routeResult.first, ());
  auto const & route = *routeResult.first;

  integration::CheckSubwayExistence(route);
  integration::TestRouteLength(route, 23521.9);
  TEST_LESS(route.GetTotalTimeSec(), 3600 * 3, ());
}

UNIT_TEST(Transit_Vatikan_NotEnoughGraphDataAtThenEnd)
{
  TRouteResult const routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                               mercator::FromLatLon(41.89543, 12.41481), {0.0, 0.0},
                                                               mercator::FromLatLon(41.89203, 12.46263));

  // Returns valid route now with long pedestrian part in the end, I don't see a problem here.
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  TEST(routeResult.first, ());
  auto const & route = *routeResult.first;

  integration::CheckSubwayExistence(route);
  integration::TestRouteLength(route, 7564.21);
  TEST_LESS(route.GetTotalTimeSec(), 4000, ());
}

UNIT_TEST(Transit_Vatikan_CorneliaToOttaviano)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(41.90052, 12.42642), {0.0, 0.0},
                                                         mercator::FromLatLon(41.90414, 12.45640));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  // Use Ottaviano -> Cornelia metro.
  integration::TestRouteLength(*routeResult.first, 4267.9);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_London_PoplarToOval)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(51.50818, -0.01634), {0.0, 0.0},
                                                         mercator::FromLatLon(51.48041, -0.10843));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 9421.72);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_London_DeptfordBridgeToCyprus)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(51.47149, -0.030558), {0.0, 0.0},
                                                         mercator::FromLatLon(51.51242, 0.07101));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  // I don't see any bad routing sections here. Make actual value.
  integration::TestRouteLength(*routeResult.first, 12882.2);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_Washington_FoggyToShaw)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(38.89582, -77.04934), {0.0, 0.0},
                                                         mercator::FromLatLon(38.91516, -77.01513));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  // I don't see any bad routing sections here. Make actual value.
  integration::TestRouteLength(*routeResult.first, 5685.82);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Transit_NewYork_GrassmereToPleasantPlains)
{
  TRouteResult routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Transit),
                                                         mercator::FromLatLon(40.60536, -74.07736), {0.0, 0.0},
                                                         mercator::FromLatLon(40.53015, -74.21559));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  // I don't see any bad routing sections here. Make actual value.
  integration::TestRouteLength(*routeResult.first, 17223.2);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

// Buenos Aires: both endpoints sit exactly on stops of a direct bus route. The primary (active)
// route is subway + walking; the alternative is the less-walking / fewer-transfers variant (the
// direct bus) produced by IndexRouter::CalculateRoute for transit.
//
// This test also reproduces the paradox under investigation: the primary route wins on routing
// *weight* (the A* cost the router logs as "Result route weight") yet the alternative has the lower
// real *ETA*. The reason the two can disagree: the transfer/boarding penalty (interval / 2) is
// counted in the routing weight but NOT in the ETA (GetTotalTimeSec sums only the segment times).
// With no GTFS frequency these OSM bus routes get the default 900s interval -> a 450s boarding
// penalty that weighs the bus down in the search but is invisible in its displayed ETA.
// Run with logs to inspect both weights and compare against the per-route summary below.
UNIT_TEST(Transit_BuenosAires_SubwayVsBusAlternative)
{
  TRoutesResult const routesResult = integration::CalculateRoutes(
      integration::GetVehicleComponents(VehicleType::Transit),
      Checkpoints(mercator::FromLatLon(-34.5935185, -58.4051952), mercator::FromLatLon(-34.5601016, -58.4472671)));

  TEST_EQUAL(routesResult.second, RouterResultCode::NoError, ());

  auto const & routes = routesResult.first;
  // A primary route plus exactly one alternative.
  TEST_EQUAL(routes.size(), 2, ());

  auto const & primary = *routes[0];
  auto const & alt = *routes[1];

  for (size_t i = 0; i < routes.size(); ++i)
    LOG(LINFO, ("Transit route", i, "- length:", routes[i]->GetTotalDistanceMeters(), "m, ETA:",
                routes[i]->GetTotalTimeSec(), "s, pedestrian:", integration::GetWalkDistanceMeters(*routes[i]), "m"));

  // Primary is the subway route.
  integration::CheckSubwayExistence(primary);

  // The alternative walks less (endpoints are on its stops) — the bias it's built for.
  TEST_LESS(integration::GetWalkDistanceMeters(alt), integration::GetWalkDistanceMeters(primary), ());

  // The paradox: the alternative is actually faster in real ETA than the primary. ETA does not
  // include the boarding penalty that the routing weight charges, so the weight-minimal primary
  // can still be the slower route for the rider.
  TEST_LESS(alt.GetTotalTimeSec(), primary.GetTotalTimeSec(), ());

  // Reference lengths from a Buenos Aires run; loose tolerance — exact geometry is data-dependent.
  integration::TestRouteLength(primary, 7553.54, 0.1);
  integration::TestRouteLength(alt, 6020.02, 0.1);
}
}  // namespace transit_route_test
