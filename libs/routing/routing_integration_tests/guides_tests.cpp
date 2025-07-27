#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "routing/route.hpp"
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

  TestGuideRoute(checkpoints, 840.1 /* expectedDistM */, 736.279 /* expectedTimeS */, 37 /* expectedPointsCount */);
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
}  // namespace guides_tests
