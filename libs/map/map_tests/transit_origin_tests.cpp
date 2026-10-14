#include "testing/testing.hpp"

#include "map/map_tests/transit_test_helpers.hpp"
#include "map/routing_manager.hpp"

#include "platform/platform.hpp"

#include <optional>

namespace transit_origin_tests
{
namespace
{
class TestRoute : public routing::RouteBase
{
public:
  TestRoute(std::vector<routing::RouteSegment> segments, std::vector<size_t> const & ends, size_t currentSubroute)
  {
    SetRouteSegments(std::move(segments));
    size_t begin = 0;
    for (auto const end : ends)
    {
      geometry::PointWithAltitude const start =
          begin == 0 ? geometry::PointWithAltitude({0, 0}, 0) : m_routeSegments[begin - 1].GetJunction();
      auto const finish = end == 0 ? start : m_routeSegments[end - 1].GetJunction();
      m_subrouteAttrs.emplace_back(start, finish, begin, end);
      begin = end;
    }
    SetCurrentSubrouteIdx(currentSubroute);
  }
};

TransitRouteInfo ProcessRemainingRoute(TransitRouteDisplay & display, TestRoute const & route)
{
  display.SetRouteOrigin(route);
  std::vector<routing::RouteSegment> segments;
  for (size_t i = route.GetCurrentSubrouteIdx(); i < route.GetSubrouteCount(); ++i)
  {
    route.GetSubrouteInfo(i, segments);
    df::Subroute subroute;
    auto const begin = route.GetSubrouteAttrs(i).GetBeginSegmentIdx();
    subroute.m_baseDistance = begin == 0 ? 0.0 : route.GetRouteSegments()[begin - 1].GetDistFromBeginningMerc();
    subroute.m_polyline.Add(route.GetSubrouteAttrs(i).GetStart().GetPoint());
    TEST(display.ProcessSubroute(segments, subroute,
                                 route.GetSubrouteAttrs(i).GetEndSegmentIdx() == route.GetRouteSegments().size()),
         (i));
  }
  return display.GetRouteInfo();
}
}  // namespace

UNIT_TEST(TransitDisplay_RoutingManagerInitializesRemainingOrigin)
{
  struct Delegate final : RoutingManager::Delegate
  {
    void OnRouteFollow(routing::RouterType) override {}
  } delegate;
  transit_tests::Fixture fixture;
  // Join the extrapolator's background tasks before destroying the manager.
  std::optional<RoutingManager> managerHolder;
  Platform::ThreadRunner const threadRunner;
  TestRoute const route({transit_tests::MakeSegment(0, 411.2), transit_tests::MakeSegment(1, 420.4),
                         transit_tests::MakeSegment(2, 430.6)},
                        {1, 2, 3}, 1);
  auto display = std::make_shared<TransitRouteDisplay>(fixture.MakeDisplay());

  auto & manager =
      managerHolder.emplace(RoutingManager::Callbacks(nullptr, nullptr, nullptr, nullptr, nullptr), delegate);
  manager.m_currentRouterType = routing::RouterType::Transit;
  RoutingManager::RoadWarningsCollection warnings;
  // A missing graphics engine skips submission while the production caller still builds route info.
  manager.InsertSingleRoute(route, true /* isActive */, 10.0 /* depthOffset */, display, warnings);

  auto const & info = display->GetRouteInfo();
  TEST(warnings.empty(), ());
  TEST_EQUAL(info.m_totalTimeInSec, 19, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 19, ());
  TEST_EQUAL(info.m_totalDistInMeters, 20, ());
  TEST_EQUAL(info.m_totalPedestrianDistInMeters, 20, ());
  TEST_EQUAL(info.m_steps.size(), 3, ());
  TEST_EQUAL(info.m_steps[0].m_timeInSec, 9, ());
  TEST_EQUAL(info.m_steps[0].m_distanceInMeters, 10, ());
  TEST_EQUAL(info.m_steps[1].m_type, TransitType::IntermediatePoint, ());
  TEST_EQUAL(info.m_steps[1].m_intermediateIndex, 1, ());
  TEST_EQUAL(info.m_steps[2].m_timeInSec, 10, ());
  TEST_EQUAL(info.m_steps[2].m_distanceInMeters, 10, ());

  TestRoute const emptyPassed({transit_tests::MakeSegment(0, 60), transit_tests::MakeSegment(1, 120)}, {0, 0, 1, 2}, 2);
  for (auto const * remainingRoute : {&route, &emptyPassed})
  {
    auto const index = remainingRoute->GetCurrentSubrouteIdx();
    auto const begin = remainingRoute->GetSubrouteAttrs(index).GetBeginSegmentIdx();
    auto const expectedBase =
        begin == 0 ? 0.0 : remainingRoute->GetRouteSegments()[begin - 1].GetDistFromBeginningMerc();
    std::vector<routing::RouteSegment> segments;
    remainingRoute->GetSubrouteInfo(index, segments);
    for (auto const type : {routing::RouterType::Vehicle, routing::RouterType::Pedestrian, routing::RouterType::Bicycle,
                            routing::RouterType::Transit, routing::RouterType::Ruler})
    {
      manager.m_currentRouterType = type;
      auto subroute = manager.MakeDrapeSubroute(*remainingRoute, index, segments, 10.0);
      TEST(subroute, (type));
      TEST_EQUAL(subroute->m_baseDistance, expectedBase, (type));
      if (type == routing::RouterType::Transit)
      {
        auto remainingDisplay = fixture.MakeDisplay();
        remainingDisplay.SetRouteOrigin(*remainingRoute);
        TEST(remainingDisplay.ProcessSubroute(segments, *subroute, true /* isLastSubroute */), ());
      }
      // The renderer subtracts the subroute base from route-global following progress.
      TEST_ALMOST_EQUAL_ABS(segments.back().GetDistFromBeginningMerc() - subroute->m_baseDistance,
                            subroute->m_polyline.GetLength(), 1e-12, (type));
    }
  }

  auto const segments = transit_tests::MakeMixedSegments({10, 20, 140, 180, 300, 310, 320}, false);
  TestRoute const rideRoute(segments, {2, 3, 4, 5, 7}, 1);
  std::vector<routing::RouteSegment> ride;
  rideRoute.GetSubrouteInfo(1, ride);
  manager.m_currentRouterType = routing::RouterType::Transit;
  auto subroute = manager.MakeDrapeSubroute(rideRoute, 1, ride, 10.0);
  auto remainingDisplay = fixture.MakeDisplay();
  remainingDisplay.SetRouteOrigin(rideRoute);
  TEST(remainingDisplay.ProcessSubroute(ride, *subroute, true /* isLastSubroute */), ());
  TEST_EQUAL(subroute->m_baseDistance, segments[1].GetDistFromBeginningMerc(), ());
  TEST_EQUAL(subroute->m_markers.size(), 2, ());
  TEST_EQUAL(subroute->m_markers.front().m_distance, 0.0, ());
  TEST_ALMOST_EQUAL_ABS(subroute->m_markers.back().m_distance,
                        segments[2].GetDistFromBeginningMerc() - segments[1].GetDistFromBeginningMerc(), 1e-12, ());
  auto const & rideInfo = remainingDisplay.GetRouteInfo();
  TEST_EQUAL(rideInfo.m_totalDistInMeters, 10, ());
  TEST_EQUAL(rideInfo.m_totalTimeInSec, 120, ());
}

UNIT_TEST(TransitDisplay_RemainingWalkExcludesPassedSubroutes)
{
  transit_tests::Fixture fixture;
  TestRoute const route({transit_tests::MakeSegment(0, 411), transit_tests::MakeSegment(1, 420)}, {1, 2}, 1);
  auto display = fixture.MakeDisplay();
  auto const info = ProcessRemainingRoute(display, route);
  TEST_EQUAL(info.m_steps.size(), 1, ());
  TEST_EQUAL(info.m_totalTimeInSec, 9, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 9, ());
  TEST_EQUAL(info.m_steps.front().m_timeInSec, 9, ());
  TEST_EQUAL(info.m_totalDistInMeters, 10, ());
  TEST_EQUAL(info.m_totalPedestrianDistInMeters, 10, ());
  TEST_EQUAL(info.m_steps.front().m_distanceInMeters, 10, ());
}

UNIT_TEST(TransitDisplay_FractionalRemainingOriginUsesWholeSecondTimestamps)
{
  transit_tests::Fixture fixture;
  TestRoute const route({transit_tests::MakeSegment(0, 411.2), transit_tests::MakeSegment(1, 420.4)}, {1, 2}, 1);
  auto display = fixture.MakeDisplay();
  auto const info = ProcessRemainingRoute(display, route);
  TEST_EQUAL(info.m_totalTimeInSec, 9, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 9, ());
  TEST_EQUAL(info.m_steps.front().m_timeInSec, 9, ());
  TEST_EQUAL(info.m_totalDistInMeters, 10, ());
}

UNIT_TEST(TransitDisplay_RemainingWaypointKeepsOriginalIndex)
{
  transit_tests::Fixture fixture;
  TestRoute const route(
      {transit_tests::MakeSegment(0, 120), transit_tests::MakeSegment(1, 180), transit_tests::MakeSegment(2, 240)},
      {1, 2, 3}, 1);
  auto display = fixture.MakeDisplay();
  auto const info = ProcessRemainingRoute(display, route);
  TEST_EQUAL(info.m_totalTimeInSec, 120, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 120, ());
  TEST_EQUAL(info.m_totalDistInMeters, 20, ());
  TEST_EQUAL(info.m_steps.size(), 3, ());
  TEST_EQUAL(info.m_steps[0].m_timeInSec, 60, ());
  TEST_EQUAL(info.m_steps[1].m_type, TransitType::IntermediatePoint, ());
  TEST_EQUAL(info.m_steps[1].m_intermediateIndex, 1, ());
  TEST_EQUAL(info.m_steps[2].m_timeInSec, 60, ());
}

UNIT_TEST(TransitDisplay_EmptyPassedSubroutesHaveZeroOrigin)
{
  transit_tests::Fixture fixture;
  TestRoute const route({transit_tests::MakeSegment(0, 60), transit_tests::MakeSegment(1, 120)}, {0, 0, 1, 2}, 2);
  auto display = fixture.MakeDisplay();
  auto const info = ProcessRemainingRoute(display, route);
  TEST_EQUAL(info.m_totalTimeInSec, 120, ());
  TEST_EQUAL(info.m_totalDistInMeters, 20, ());
  TEST_EQUAL(info.m_steps.size(), 3, ());
  TEST_EQUAL(info.m_steps[1].m_type, TransitType::IntermediatePoint, ());
  TEST_EQUAL(info.m_steps[1].m_intermediateIndex, 2, ());
}

UNIT_TEST(TransitDisplay_OriginPreservesMarkerDistances)
{
  transit_tests::Fixture fixture;
  auto const segments = transit_tests::MakeMixedSegments({10, 20, 140, 180, 300, 310, 320}, false);
  TestRoute const route(segments, {2, 3, 4, 5, 7}, 1);
  auto fullDisplay = fixture.MakeDisplay();
  df::Subroute prefix;
  prefix.m_polyline.Add({0, 0});
  TEST(fullDisplay.ProcessSubroute({segments.begin(), segments.begin() + 2}, prefix, false /* isLastSubroute */), ());
  df::Subroute full;
  full.m_baseDistance = segments[1].GetDistFromBeginningMerc();
  full.m_polyline.Add(route.GetSubrouteAttrs(1).GetStart().GetPoint());
  TEST(fullDisplay.ProcessSubroute({segments[2]}, full, true /* isLastSubroute */), ());

  auto remainingDisplay = fixture.MakeDisplay();
  remainingDisplay.SetRouteOrigin(route);
  df::Subroute remaining;
  remaining.m_baseDistance = segments[1].GetDistFromBeginningMerc();
  remaining.m_polyline.Add(route.GetSubrouteAttrs(1).GetStart().GetPoint());
  TEST(remainingDisplay.ProcessSubroute({segments[2]}, remaining, true /* isLastSubroute */), ());

  auto const & info = remainingDisplay.GetRouteInfo();
  TEST_EQUAL(info.m_totalTimeInSec, 120, ());
  TEST_EQUAL(info.m_totalDistInMeters, 10, ());
  TEST_EQUAL(info.m_steps.front().m_distanceInMeters, 10, ());
  TEST_EQUAL(remaining.m_polyline.GetPoints(), full.m_polyline.GetPoints(), ());
  TEST_EQUAL(remaining.m_markers.size(), full.m_markers.size(), ());
  TEST(!remaining.m_markers.empty(), ());
  TEST_EQUAL(remaining.m_markers.front().m_distance, 0.0, ());
  for (size_t i = 0; i < remaining.m_markers.size(); ++i)
    TEST_EQUAL(remaining.m_markers[i].m_distance, full.m_markers[i].m_distance, ());
}
}  // namespace transit_origin_tests
