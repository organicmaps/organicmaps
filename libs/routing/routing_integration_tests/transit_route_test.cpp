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

// Buenos Aires: transit routing returns a primary route plus a "less-walking / fewer-transfers"
// alternative produced by IndexRouter::CalculateRoute (the alt is computed with a 2x walking +
// 2x transfer penalty). The alternative walks noticeably less than the primary, trading the saved
// walking for more in-transit time.
UNIT_TEST(Transit_BuenosAires_SubwayVsBusAlternative)
{
  TRoutesResult const routesResult = integration::CalculateRoutes(
      integration::GetVehicleComponents(VehicleType::Transit),
      Checkpoints(mercator::FromLatLon(-34.5934571, -58.4051285), mercator::FromLatLon(-34.5592725, -58.4483561)));

  TEST_EQUAL(routesResult.second, RouterResultCode::NoError, ());

  auto const & routes = routesResult.first;
  // A primary route plus exactly one alternative.
  TEST_EQUAL(routes.size(), 2, ());

  auto const & primary = *routes[0];
  auto const & alt = *routes[1];

  for (size_t i = 0; i < routes.size(); ++i)
    LOG(LINFO, ("Transit route", i, "- length:", routes[i]->GetTotalDistanceMeters(), "m, ETA:",
                routes[i]->GetTotalTimeSec(), "s, pedestrian:", integration::GetWalkDistanceMeters(*routes[i]), "m"));

  integration::CheckSubwayExistence(primary);

  // The alternative is the less-walking variant — it walks noticeably less than the primary.
  TEST_LESS(integration::GetWalkDistanceMeters(alt), integration::GetWalkDistanceMeters(primary), ());

  // Reference lengths from a Buenos Aires run; loose tolerance — exact geometry is data-dependent.
  integration::TestRouteLength(primary, 7435.59, 0.1);
  integration::TestRouteLength(alt, 6076.72, 0.1);
}

// Belarus, Minsk: checks the pedestrian (walking) lengths of a transit route, and documents how the
// boarding gate's road attachment shapes the first walking leg.
//
// The transit and pedestrian routers use the SAME pedestrian model and estimator, so footway
// preference is identical. The difference in the first subroute is not the model but the target: the
// walk must reach the bus through the gate's road attachment (its |bestPedestrianSegment|), which the
// generator's CalculateBestPedestrianSegments picks as the gate's geometrically NEAREST routable
// segment by distance, ignoring highway class. Here that nearest segment is a highway=secondary, so
// the first leg walks ~100 m of that secondary to board, shorter than (but less footway-friendly
// than) the route a standalone pedestrian search to the stop point would take along the parallel
// footway. This is a known artifact of distance-based gate attachment, not a routing-model bug.
UNIT_TEST(Transit_Minsk_PedestrianLegToGate)
{
  TRoutesResult const routesResult = integration::CalculateRoutes(
      integration::GetVehicleComponents(VehicleType::Transit),
      Checkpoints(mercator::FromLatLon(53.8880136, 27.4282779), mercator::FromLatLon(53.906727, 27.4542114)));

  TEST_EQUAL(routesResult.second, RouterResultCode::NoError, ());
  TEST(!routesResult.first.empty(), ());

  auto const & route = *routesResult.first[0];
  integration::CheckSubwayExistence(route);
  integration::TestRouteLength(route, 3496.24, 0.1);

  // First subroute: the walk from the start to the first boarding runs along the highway=secondary
  // the gate attaches to (see the note above), so it is ~100 m rather than the longer footway path.
  auto const & segs = route.GetRouteSegments();
  size_t firstTransit = segs.size();
  for (size_t i = 0; i < segs.size(); ++i)
  {
    if (segs[i].HasTransitInfo())
    {
      firstTransit = i;
      break;
    }
  }
  TEST_LESS(firstTransit, segs.size(), ("Route doesn't use transit."));
  double const startWalk = segs[firstTransit - 1].GetDistFromBeginningMeters();
  TEST_ALMOST_EQUAL_ABS(startWalk, 99.5, 10.0, ());

  // Total walking length of the transit route (first leg + the short hop off the bus at the end).
  TEST_ALMOST_EQUAL_ABS(integration::GetWalkDistanceMeters(route), 228.5, 20.0, ());
}

namespace
{
// Checks the walking legs at the very start and end of a transit route. The pedestrian leg to the
// first boarding stop (and from the last alighting stop) should stay close to the straight-line
// distance — it must not detour to a far feature junction and back. See the SPb snapping bug.
void TestTransitStartEndWalk(Route const & route, m2::PointD const & start, m2::PointD const & finish, double maxFactor)
{
  auto const & segs = route.GetRouteSegments();
  size_t firstTransit = segs.size(), lastTransit = segs.size();
  for (size_t i = 0; i < segs.size(); ++i)
  {
    if (segs[i].HasTransitInfo())
    {
      if (firstTransit == segs.size())
        firstTransit = i;
      lastTransit = i;
    }
  }
  TEST_LESS(firstTransit, segs.size(), ("Route doesn't use transit at all."));
  TEST_GREATER(firstTransit, 0, ());

  // Boarding point = start junction of the first transit segment = end junction of the previous one.
  auto const boardPt = segs[firstTransit - 1].GetJunction().GetPoint();
  double const walkToBoard = segs[firstTransit - 1].GetDistFromBeginningMeters();
  double const directToBoard = mercator::DistanceOnEarth(start, boardPt);

  // Alighting point = end junction of the last transit segment.
  auto const alightPt = segs[lastTransit].GetJunction().GetPoint();
  double const walkFromAlight = route.GetTotalDistanceMeters() - segs[lastTransit].GetDistFromBeginningMeters();
  double const directFromAlight = mercator::DistanceOnEarth(alightPt, finish);

  LOG(LINFO, ("Transit start walk:", walkToBoard, "m (direct", directToBoard, "m), end walk:", walkFromAlight,
              "m (direct", directFromAlight, "m)"));

  TEST_LESS(walkToBoard, maxFactor * directToBoard, ());
  TEST_LESS(walkFromAlight, maxFactor * directFromAlight, ());
}
}  // namespace

// St. Petersburg: a bus route whose start/finish stops are right next to the checkpoints. The
// pedestrian legs onto/off the bus must not detour to a far highway junction and back.
UNIT_TEST(Transit_SPb_StartEndSnapping)
{
  Checkpoints const cp(mercator::FromLatLon(59.933578, 30.437052), mercator::FromLatLon(59.935438, 30.498636));

  auto const res = integration::CalculateRoutes(integration::GetVehicleComponents(VehicleType::Transit), cp);
  TEST_EQUAL(res.second, RouterResultCode::NoError, ());
  TEST(!res.first.empty(), ());

  TestTransitStartEndWalk(*res.first[0], cp.GetStart(), cp.GetFinish(), 2.0 /* maxFactor */);
}
}  // namespace transit_route_test
