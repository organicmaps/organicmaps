#include "routing/routing_integration_tests/generated_map_test.hpp"
#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

#include <algorithm>

namespace guides_tests
{
using namespace routing;

// Test guide track is laid crosswise the OSM road graph. It doesn't match the OSM roads so we
// can test route length, time and points number and it is enough to guarantee that the route
// built during the test is the route through the guide which we expect.
GuidesTracks GetTestGuides()
{
  // Guide with single track.
  GuidesTracks guides;
  guides[10] = {{{mercator::FromLatLon(48.13999, 11.56873), 10},
                 {mercator::FromLatLon(48.14096, 11.57246), 10},
                 {mercator::FromLatLon(48.14487, 11.57259), 10}}};
  return guides;
}

void TestGuideRoute(Checkpoints const & checkpoints, double expectedDistM, double expectedTimeS,
                    size_t expectedPointsCount)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents(VehicleType::Pedestrian), checkpoints, GetTestGuides());

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, expectedDistM);
  integration::TestRouteTime(*routeResult.first, expectedTimeS);
  integration::TestRoutePointsNumber(*routeResult.first, expectedPointsCount);
}

void ReverseCheckpoints(Checkpoints const & checkpoints)
{
  auto points = checkpoints.GetPoints();
  std::reverse(points.begin(), points.end());
}

// Start and finish checkpoints are connected to the track via single fake edge.
UNIT_TEST(Guides_TwoPointsOnTrack)
{
  // Checkpoints lie on the track.
  Checkpoints const checkpoints{mercator::FromLatLon(48.14367, 11.57257), mercator::FromLatLon(48.13999, 11.56873)};

  double const expectedDistM = 600.8;
  double const expectedTimeS = 721.0;
  size_t const expectedPointsCount = 7;

  TestGuideRoute(checkpoints, expectedDistM, expectedTimeS, expectedPointsCount);
  ReverseCheckpoints(checkpoints);
  TestGuideRoute(checkpoints, expectedDistM, expectedTimeS, expectedPointsCount);
}

// One checkpoint is connected to the track projection via OSM,
// other checkpoint is connected to the track projection via single fake edge.
UNIT_TEST(Guides_TwoPointsOnTrackOneViaOsm)
{
  // Start is further from track then |kEqDistToTrackPointM|, but finish is closer.
  Checkpoints const checkpoints{mercator::FromLatLon(48.13998, 11.56982), mercator::FromLatLon(48.14448, 11.57259)};

  double const expectedDistM = 788.681;
  double const expectedTimeS = 903.3;
  size_t const expectedPointsCount = 13;

  TestGuideRoute(checkpoints, expectedDistM, expectedTimeS, expectedPointsCount);
  ReverseCheckpoints(checkpoints);
  TestGuideRoute(checkpoints, expectedDistM, expectedTimeS, expectedPointsCount);
}

// Start checkpoint is far away from the track, finish checkpoint lies in meters from
// the middle of the track. We build the first part of the route from start to the terminal
// track point, second part - from the terminal point to the finish.
UNIT_TEST(Guides_FinishPointOnTrack)
{
  Checkpoints const checkpoints{mercator::FromLatLon(48.1394659, 11.575924),
                                mercator::FromLatLon(48.1407632, 11.5716992)};

  TestGuideRoute(checkpoints, 845.416 /* expectedDistM */, 736.279 /* expectedTimeS */, 45 /* expectedPointsCount */);
}

// Start checkpoint is on the track, finish checkpoint is far away. We build the first part of the
// route through the track and the second part - through the OSM roads.
UNIT_TEST(Guides_StartPointOnTrack)
{
  Checkpoints const checkpoints{mercator::FromLatLon(48.14168, 11.57244), mercator::FromLatLon(48.13741, 11.56095)};

  TestGuideRoute(checkpoints, 1200.45 /* expectedDistM */, 1056.45 /* expectedTimeS */, 52 /* expectedPointsCount */);
}

// Start and finish lie on the track; 3 intermediate points are far away from the track.
UNIT_TEST(Guides_MultipleIntermediatePoints)
{
  Checkpoints const checkpoints({mercator::FromLatLon(48.14403, 11.57259), mercator::FromLatLon(48.14439, 11.57480),
                                 mercator::FromLatLon(48.14192, 11.57548), mercator::FromLatLon(48.14106, 11.57279),
                                 mercator::FromLatLon(48.14044, 11.57061)});

  TestGuideRoute(checkpoints, 1231.91 /* expectedDistM */, 1042.65 /* expectedTimeS */, 67 /* expectedPointsCount */);
}

UNIT_TEST(Guides_SeparateTracksConnectThroughOsm)
{
  integration::GeneratedMapTest map("./data/test_data/osm/saved_tracks.osm", "Slovenia_East", VehicleType::Pedestrian);
  auto const start = mercator::FromLatLon(45.77, 14.36);
  auto const finish = mercator::FromLatLon(45.79, 14.36);
  auto const baseline = integration::CalculateRoute(map.GetComponents(), start, {}, finish);
  TEST_EQUAL(baseline.second, RouterResultCode::NoError, ());
  for (bool const reverse : {false, true})
  {
    GuidesTracks guides;
    guides[10] = {{{start, 0}, {mercator::FromLatLon(45.771, 14.36), 0}},
                  {{mercator::FromLatLon(45.789, 14.36), 0}, {finish, 0}}};
    auto const result = integration::CalculateRoute(
        map.GetComponents(), reverse ? Checkpoints(finish, start) : Checkpoints(start, finish), std::move(guides));
    TEST_EQUAL(result.second, RouterResultCode::NoError, (reverse));
    TEST(result.first, (reverse));
    integration::TestRouteLength(*result.first, 2226.5);
  }
}

UNIT_TEST(Guides_ContinuousTrackEta)
{
  integration::GeneratedMapTest map("./data/test_data/osm/saved_tracks.osm", "Slovenia_East", VehicleType::Pedestrian);
  auto const start = mercator::FromLatLon(45.77, 14.36);
  auto const finish = mercator::FromLatLon(45.79, 14.36);
  GuidesTracks guides;
  guides[10] = {{{start, 0},
                 {mercator::FromLatLon(45.775, 14.36), 0},
                 {mercator::FromLatLon(45.78, 14.36), 0},
                 {mercator::FromLatLon(45.785, 14.36), 0},
                 {finish, 0}}};
  auto const result = integration::CalculateRoute(map.GetComponents(), Checkpoints(start, finish), std::move(guides));
  TEST_EQUAL(result.second, RouterResultCode::NoError, ());
  TEST(result.first, ());
  integration::TestRouteLength(*result.first, 2226.5);
  // Each track segment contributes once, at the pedestrian off-road speed of 3 km/h.
  integration::TestRouteTime(*result.first, 2671.9);
}

UNIT_TEST(Guides_DenseTracksRebuildForWalkingAndCycling)
{
  auto const start = mercator::FromLatLon(45.77, 14.36);
  auto const finish = mercator::FromLatLon(45.79, 14.36);
  GuidesTracks guides;
  auto & track = guides[10].emplace_back();
  size_t constexpr kPointsCount = 10'001;
  for (size_t i = 0; i < kPointsCount; ++i)
    track.emplace_back(start + (finish - start) * (static_cast<double>(i) / (kPointsCount - 1)), 100);

  for (auto const vehicle : {VehicleType::Pedestrian, VehicleType::Bicycle})
  {
    integration::GeneratedMapTest map("./data/test_data/osm/saved_tracks.osm", "Slovenia_East", vehicle);
    auto & router = map.GetComponents().GetRouter();
    RouterDelegate delegate;
    for (bool const rebuild : {false, true})
    {
      auto const from = rebuild ? track[kPointsCount / 4].GetPoint() : start;
      router.SetGuides(GuidesTracks(guides));
      RoutesResult routes("dense tracks", 0);
      auto const result = router.CalculateRoute(Checkpoints(from, finish), {}, rebuild, false, delegate, routes);
      TEST_EQUAL(result, RouterResultCode::NoError, (vehicle, rebuild));
      TEST(routes.IsValid(), (vehicle, rebuild));
      Route const route(routes.GetActive());
      integration::TestRouteLength(route, rebuild ? 1669.8 : 2226.3);
      TEST_GREATER(route.GetPoly().GetSize(), 5000, ("The route must follow the dense track", vehicle, rebuild));
    }
  }
}
}  // namespace guides_tests
