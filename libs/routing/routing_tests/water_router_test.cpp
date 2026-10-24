#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/ruler_router.hpp"
#include "routing/water_router.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>

namespace water_router_test
{
using namespace routing;

water::Geometry River()
{
  water::Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{{0, 0}, 1}, {{0, 1}, 2}, {{1, 1}, 3}, {{1, 0}, 4}}};
  return geometry;
}

UNIT_TEST(WaterRouter_DistanceOnlyWithCheckpoints)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); });
  Checkpoints checkpoints(CheckpointsGeometry{{0, 0}, {0, 1}, {1, 0}});
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(checkpoints, {}, false, false, delegate, result) == RouterResultCode::NoError, ());
  TEST(result.IsValid(), ());
  auto const & route = result.GetActive();
  TEST_EQUAL(route.GetSubrouteCount(), 2, ());
  TEST_EQUAL(route.GetTotalTimeSec(), 0.0, ());
  TEST(route.GetWaterDistances().has_value(), ());
  auto const & distances = *route.GetWaterDistances();
  TEST_EQUAL(distances.m_landMeters, 0.0, ());
  TEST_ALMOST_EQUAL_ABS(distances.GetTotalMeters(), route.GetRouteSegments().back().GetDistFromBeginningMeters(), 1e-6,
                        ());
  for (auto const & segment : route.GetRouteSegments())
  {
    TEST(!segment.IsLandGap(), ());
    TEST(!IsNormalTurn(segment.GetTurn()) ||
             segment.GetTurn().m_pedestrianTurn == turns::PedestrianDirection::ReachedYourDestination,
         ());
  }
  Route active(route);
  turns::TurnItem turn;
  double distance = 0.0;
  active.GetNearestTurn(distance, turn);
  TEST(turn.m_pedestrianTurn == turns::PedestrianDirection::ReachedYourDestination, ());
  TEST_GREATER(distance, 0.0, ());
}

UNIT_TEST(WaterRouter_ProgressTracksWorkAndResets)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); });
  RouterDelegate delegate;
  std::vector<float> progress;
  delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
  for (size_t attempt = 0; attempt < 2; ++attempt)
  {
    progress.clear();
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints(CheckpointsGeometry{{0, 0}, {0, 1}, {1, 0}}), {}, false, false, delegate,
                               result) == RouterResultCode::NoError,
         ());
    TEST(result.IsValid(), ());
    TEST_GREATER(progress.size(), 4, ());
    TEST(progress.size() <= 101, ());
    TEST_EQUAL(progress.front(), 0, ());
    TEST_EQUAL(progress.back(), 100, ());
    TEST(std::is_sorted(progress.begin(), progress.end()), ());
    TEST(std::any_of(progress.begin(), progress.end(), [](float value) { return value > 30 && value < 98; }), ());
  }
}

UNIT_TEST(WaterRouter_FailedProgressNeverCompletes)
{
  for (bool missing : {false, true})
  {
    WaterRouter router([&](Checkpoints const &, RouterDelegate const &)
    { return missing ? water::Geometry{} : River(); });
    RouterDelegate delegate;
    std::vector<float> progress;
    delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints({0, 0}, {2, 0}), {}, false, false, delegate, result) !=
             RouterResultCode::NoError,
         ());
    TEST(!result.IsValid(), ());
    TEST(!progress.empty(), ());
    TEST_LESS(progress.back(), 100, ());
    TEST(std::is_sorted(progress.begin(), progress.end()), ());
  }
}

UNIT_TEST(WaterRouter_ProgressCancellation)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); });
  for (float threshold : {50.0f, 99.0f})
  {
    RouterDelegate delegate;
    std::vector<float> progress;
    delegate.SetProgressCallback([&](float value)
    {
      progress.push_back(value);
      if (value >= threshold)
        delegate.Cancel();
    });
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints({0, 0}, {1, 0}), {}, false, false, delegate, result) ==
             RouterResultCode::Cancelled,
         ());
    TEST(!result.IsValid(), ());
    TEST_LESS(progress.back(), 100, ());
    TEST(std::is_sorted(progress.begin(), progress.end()), ());
  }
}

UNIT_TEST(WaterRouter_ExplicitStartDoesNotProject)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); });
  Checkpoints checkpoints({-1, 0}, {1, 0});
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(checkpoints, {}, false, false, delegate, result) == RouterResultCode::StartPointNotFound,
       ());
  TEST(!result.IsValid(), ());
  checkpoints.SetStartIsMyPosition(true);
  TEST(router.CalculateRoute(checkpoints, {}, false, false, delegate, result) == RouterResultCode::NoError, ());
  TEST_EQUAL(result.GetActive().GetSubrouteAttrs(0).GetStart().GetPoint(), m2::PointD(0, 0), ());
  TEST_EQUAL(result.GetActive().GetWaterDistances()->m_landMeters, 0.0, ());
}

UNIT_TEST(WaterRouter_ReportsOutsideDestinationAndIntermediate)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); });
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {2, 0}), {}, false, false, delegate, result) ==
           RouterResultCode::EndPointNotFound,
       ());
  TEST(!result.IsValid(), ());
  TEST(router.CalculateRoute(Checkpoints(CheckpointsGeometry{{0, 0}, {2, 0}, {1, 0}}), {}, false, false, delegate,
                             result) == RouterResultCode::IntermediatePointNotFound,
       ());
  TEST(!result.IsValid(), ());
}

UNIT_TEST(WaterRouter_MissingDataHasNoRulerFallback)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return water::Geometry{}; });
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {1, 1}), {}, false, false, delegate, result) ==
           RouterResultCode::RouteNotFound,
       ());
  TEST(!result.IsValid(), ());
}

UNIT_TEST(WaterRouter_RulerRemainsStraight)
{
  RulerRouter ruler;
  IRouter & router = ruler;
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {1, 0}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST(result.IsValid(), ());
  TEST(!result.GetActive().GetWaterDistances().has_value(), ());
  TEST_EQUAL(result.GetActive().GetTotalTimeSec(), 0.0, ());
  for (auto const & segment : result.GetActive().GetRouteSegments())
    TEST_EQUAL(segment.GetJunction().GetPoint().y, 0.0, ());
}
class WalkingStub final : public RulerRouter
{
public:
  explicit WalkingStub(size_t & calls, bool fail = false, size_t failFirst = 0)
    : m_calls(calls)
    , m_fail(fail)
    , m_failFirst(failFirst)
  {}
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                  RouterDelegate const & delegate, RoutesResult & result) override
  {
    ++m_calls;
    delegate.OnProgress(20);
    if (delegate.GetCancellable().IsCancelled())
      return RouterResultCode::Cancelled;
    if (m_fail || m_calls <= m_failFirst)
      return RouterResultCode::RouteNotFound;
    Checkpoints bent(CheckpointsGeometry{checkpoints.GetStart(), {-0.001, 0.001}, checkpoints.GetFinish()});
    RulerRouter ruler;
    IRouter & router = ruler;
    auto const code = router.CalculateRoute(bent, {}, false, false, delegate, result);
    delegate.OnProgress(100);
    return code;
  }

private:
  size_t & m_calls;
  bool m_fail;
  size_t m_failFirst;
};

UNIT_TEST(WaterRouter_WalkingApproachUsesPedestrianGeometry)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  std::vector<float> progress;
  delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
  RoutesResult result;
  m2::PointD const start(-0.001, 0), bend(-0.001, 0.001), entry(0, 0);
  TEST(router.CalculateRoute(Checkpoints(start, {0, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(calls, 1, ());
  Route const route(result.GetActive());
  auto const & points = route.GetPoly().GetPoints();
  TEST_EQUAL(points.front(), start, ());
  TEST(std::find(points.begin(), points.end(), bend) != points.end(), ());
  auto const & distances = *route.GetWaterDistances();
  TEST_ALMOST_EQUAL_ABS(distances.m_landMeters,
                        mercator::DistanceOnEarth(start, bend) + mercator::DistanceOnEarth(bend, entry), 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(distances.m_waterMeters, mercator::DistanceOnEarth(entry, {0, 0.01}), 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(distances.GetTotalMeters(), route.GetRouteSegments().back().GetDistFromBeginningMeters(), 1e-6,
                        ());
  TEST_EQUAL(route.GetTotalTimeSec(), 0, ());
  TEST(std::any_of(route.GetRouteSegments().begin(), route.GetRouteSegments().end(),
                   [](auto const & segment) { return segment.IsLandGap(); }),
       ());
  TEST(std::is_sorted(progress.begin(), progress.end()), ());
  TEST_EQUAL(progress.back(), 100, ());
}

UNIT_TEST(WaterRouter_WalkingApproachCannotFallBackToStraightLine)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls, true));
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({-0.001, 0}, {0, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::RouteNotFound,
       ());
  TEST_EQUAL(calls, 1, ());
  TEST(!result.IsValid(), ());
}

UNIT_TEST(WaterRouter_WalkingApproachCancellationAndLimits)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  delegate.SetProgressCallback([&](float value)
  {
    if (value >= 32)
      delegate.Cancel();
  });
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({-0.001, 0}, {0, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::Cancelled,
       ());
  TEST(!result.IsValid(), ());
  delegate.Reset();
  TEST(router.CalculateRoute(Checkpoints({-1, 0}, {0, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::StartPointNotFound,
       ());
  TEST_EQUAL(calls, 1, ());
}

UNIT_TEST(WaterRouter_WaterStartDoesNotCallPedestrianRouter)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {0, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(calls, 0, ());
}

UNIT_TEST(WaterRouter_NestedDelegateRetainsLocalAndParentCancellation)
{
  RouterDelegate parent;
  RouterDelegate child(parent.GetCancellable());
  child.Cancel();
  TEST(child.GetCancellable().IsCancelled(), ());
  TEST(!parent.IsCancelled(), ());
  child.Reset();
  parent.Cancel();
  TEST(child.IsCancelled(), ());
  TEST(child.GetCancellable().CancellationStatus() == base::Cancellable::Status::CancelCalled, ());
}
UNIT_TEST(WaterRouter_UnconfirmedShortestPathMetadata)
{
  WaterRouter router([](Checkpoints const &, RouterDelegate const &)
  {
    water::Geometry geometry;
    geometry.m_complete = true;
    geometry.m_bounds = m2::RectD(0, 0, 10, 10);
    geometry.m_lines = {{{{2, 2}, 1}, {{2, 6}, 2}, {{8, 6}, 3}, {{8, 2}, 4}}, {{{2, 2}, 1}, {{0, 2}, 5}}};
    return geometry;
  });
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({2, 2}, {8, 2}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST(result.IsValid(), ());
  TEST(!result.GetActive().GetWaterDistances()->m_shortestConfirmed, ());
  TEST_EQUAL(result.GetActive().GetWaterDistances()->m_landMeters, 0.0, ());
}
water::Geometry DisconnectedRivers()
{
  water::Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  geometry.m_lines = {{{{0, 0}}, {{0, 0.004}}}, {{{0.004, 0}}, {{0.004, 0.004}}}};
  return geometry;
}

UNIT_TEST(WaterRouter_InternalTransferUsesWalkingGeometry)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return DisconnectedRivers(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  std::vector<float> progress;
  delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
  Checkpoints checkpoints({0, 0.002}, {0.004, 0.002});
  water::Path candidate;
  TEST(water::FindPath(DisconnectedRivers(), checkpoints.GetStart(), checkpoints.GetFinish(), false,
                       [] { return false; }, candidate) == water::ResultCode::Success,
       ());
  auto const gap = std::find(candidate.m_landGaps.begin(), candidate.m_landGaps.end(), true);
  TEST(gap != candidate.m_landGaps.end(), ());
  size_t const index = std::distance(candidate.m_landGaps.begin(), gap);
  auto const expectedWalk = mercator::DistanceOnEarth(candidate.m_points[index], {-0.001, 0.001}) +
                            mercator::DistanceOnEarth({-0.001, 0.001}, candidate.m_points[index + 1]);
  RoutesResult result;
  TEST(router.CalculateRoute(checkpoints, {}, false, false, delegate, result) == RouterResultCode::NoError, ());
  TEST_EQUAL(calls, 1, ());
  auto const & route = result.GetActive();
  auto const distances = *route.GetWaterDistances();
  TEST_ALMOST_EQUAL_ABS(distances.m_landMeters, expectedWalk, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(distances.m_waterMeters, candidate.m_waterDistance, 1e-6, ());
  TEST_GREATER(distances.m_landMeters, candidate.m_landDistance, ());
  TEST(!distances.m_shortestConfirmed, ());
  TEST_ALMOST_EQUAL_ABS(distances.GetTotalMeters(), route.GetRouteSegments().back().GetDistFromBeginningMeters(), 1e-6,
                        ());
  TEST_EQUAL(std::count_if(route.GetRouteSegments().begin(), route.GetRouteSegments().end(),
                           [](auto const & segment) { return segment.IsLandGap(); }),
             2, ());
  TEST(std::is_sorted(progress.begin(), progress.end()), ());
  TEST_EQUAL(progress.back(), 100, ());
}

UNIT_TEST(WaterRouter_InternalTransferHasNoStraightFallback)
{
  for (bool const configured : {false, true})
  {
    size_t calls = 0;
    std::unique_ptr<IRouter> pedestrian;
    if (configured)
      pedestrian = std::make_unique<WalkingStub>(calls, true);
    WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return DisconnectedRivers(); },
                       std::move(pedestrian));
    RouterDelegate delegate;
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints({0, 0.002}, {0.004, 0.002}), {}, false, false, delegate, result) ==
             RouterResultCode::RouteNotFound,
         ());
    TEST(!result.IsValid(), ());
    if (configured)
    {
      TEST_GREATER(calls, 1, ());
      TEST_LESS_OR_EQUAL(calls, 32, ());
    }
    else
      TEST_EQUAL(calls, 0, ());
  }
}

UNIT_TEST(WaterRouter_InternalTransferCancellation)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return DisconnectedRivers(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  delegate.SetProgressCallback([&](float value)
  {
    if (value >= 89)
      delegate.Cancel();
  });
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0.002}, {0.004, 0.002}), {}, false, false, delegate, result) ==
           RouterResultCode::Cancelled,
       ());
  TEST_EQUAL(calls, 1, ());
  TEST(!result.IsValid(), ());
}
UNIT_TEST(WaterRouter_LandEndpointsPreferWater)
{
  for (bool landStart : {false, true})
  {
    size_t calls = 0;
    WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                       std::make_unique<WalkingStub>(calls));
    m2::PointD const start = landStart ? m2::PointD(-0.001, 0) : m2::PointD(0, 0);
    m2::PointD const finish(-0.001, 0.01), bend(-0.001, 0.001);
    RouterDelegate delegate;
    std::vector<float> progress;
    delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints(start, finish), {}, false, false, delegate, result) ==
             RouterResultCode::NoError,
         ());
    TEST_EQUAL(calls, landStart ? 2 : 1, ());
    Route const route(result.GetActive());
    auto const & points = route.GetPoly().GetPoints();
    TEST_EQUAL(points.front(), start, ());
    TEST_EQUAL(points.back(), finish, ());
    auto const distances = *route.GetWaterDistances();
    double const exitDistance = mercator::DistanceOnEarth({0, 0.01}, bend) + mercator::DistanceOnEarth(bend, finish);
    double const approachDistance =
        landStart ? mercator::DistanceOnEarth(start, bend) + mercator::DistanceOnEarth(bend, {0, 0}) : 0;
    TEST_ALMOST_EQUAL_ABS(distances.m_landMeters, approachDistance + exitDistance, 1e-6, ());
    TEST_ALMOST_EQUAL_ABS(distances.m_waterMeters, mercator::DistanceOnEarth({0, 0}, {0, 0.01}), 1e-6, ());
    TEST_ALMOST_EQUAL_ABS(distances.GetTotalMeters(), route.GetRouteSegments().back().GetDistFromBeginningMeters(),
                          1e-6, ());
    TEST(route.GetRouteSegments().back().IsLandGap(), ());
    TEST_EQUAL(route.GetSubrouteAttrs(0).GetFinish().GetPoint(), finish, ());
    TEST(std::is_sorted(progress.begin(), progress.end()), ());
    TEST_EQUAL(progress.back(), 100, ());
  }
}

UNIT_TEST(WaterRouter_LandIntermediateRetainsCheckpoint)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls));
  m2::PointD const stop(-0.001, 0.005);
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints(CheckpointsGeometry{{0, 0}, stop, {0, 0.01}}), {}, false, false, delegate,
                             result) == RouterResultCode::NoError,
       ());
  TEST_EQUAL(calls, 2, ());
  auto const & route = result.GetActive();
  TEST_EQUAL(route.GetSubrouteCount(), 2, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetFinish().GetPoint(), stop, ());
  TEST_EQUAL(route.GetSubrouteAttrs(1).GetStart().GetPoint(), stop, ());
  TEST_ALMOST_EQUAL_ABS(route.GetWaterDistances()->m_waterMeters, mercator::DistanceOnEarth({0, 0}, {0, 0.01}), 1e-6,
                        ());
}

UNIT_TEST(WaterRouter_WalkingExitFailureAndCancellation)
{
  for (bool fail : {false, true})
  {
    size_t calls = 0;
    WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                       std::make_unique<WalkingStub>(calls, fail));
    RouterDelegate delegate;
    std::vector<float> progress;
    delegate.SetProgressCallback([&](float value)
    {
      progress.push_back(value);
      if (!fail && value >= 36)
        delegate.Cancel();
    });
    RoutesResult result;
    auto const code = router.CalculateRoute(Checkpoints({0, 0}, {-0.001, 0.01}), {}, false, false, delegate, result);
    TEST(code == (fail ? RouterResultCode::RouteNotFound : RouterResultCode::Cancelled), ());
    TEST_EQUAL(calls, 1, ());
    TEST(!result.IsValid(), ());
    TEST_LESS(progress.back(), 100, ());
  }
}

UNIT_TEST(WaterRouter_LandCheckpointOutsideAccessLimit)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return River(); },
                     std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {-1, 0.01}), {}, false, false, delegate, result) ==
           RouterResultCode::EndPointNotFound,
       ());
  TEST(router.CalculateRoute(Checkpoints(CheckpointsGeometry{{0, 0}, {-1, 0.005}, {0, 0.01}}), {}, false, false,
                             delegate, result) == RouterResultCode::IntermediatePointNotFound,
       ());
  TEST_EQUAL(calls, 0, ());
  TEST(!result.IsValid(), ());
}
water::Geometry WindowDetour(bool expanded)
{
  water::Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(-0.01, -0.01, 0.02, expanded ? 0.05 : 0.02);
  if (expanded)
    geometry.m_lines = {{{{0, 0}, 1}, {{0, 0.04}, 2}, {{0.01, 0.04}, 3}, {{0.01, 0}, 4}}};
  else
    geometry.m_lines = {{{{0, 0}, 1}, {{0, 0.02}, 0, false}}, {{{0.01, 0.02}, 0, false}, {{0.01, 0}, 4}}};
  return geometry;
}

UNIT_TEST(WaterRouter_RetriesUnavailableWaterInWiderWindow)
{
  size_t expandedCalls = 0;
  size_t walkingCalls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return WindowDetour(false); },
                     std::make_unique<WalkingStub>(walkingCalls), [&](Checkpoints const &, RouterDelegate const &)
  {
    ++expandedCalls;
    return WindowDetour(true);
  });
  RouterDelegate delegate;
  std::vector<float> progress;
  delegate.SetProgressCallback([&](float value) { progress.push_back(value); });
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {0.01, 0}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(expandedCalls, 1, ());
  TEST_EQUAL(walkingCalls, 0, ());
  TEST_GREATER(result.GetActive().GetWaterDistances()->m_waterMeters, 8000.0, ());
  TEST_EQUAL(result.GetActive().GetWaterDistances()->m_landMeters, 0.0, ());
  TEST(std::is_sorted(progress.begin(), progress.end()), ());
  TEST_EQUAL(progress.back(), 100, ());
}

UNIT_TEST(WaterRouter_WiderWindowFailureDoesNotInventRoute)
{
  for (bool complete : {false, true})
  {
    size_t expandedCalls = 0;
    WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return WindowDetour(false); }, {},
                       [&](Checkpoints const &, RouterDelegate const &)
    {
      ++expandedCalls;
      auto geometry = WindowDetour(false);
      geometry.m_complete = complete;
      return geometry;
    });
    RouterDelegate delegate;
    RoutesResult result;
    TEST(router.CalculateRoute(Checkpoints({0, 0}, {0.01, 0}), {}, false, false, delegate, result) ==
             RouterResultCode::RouteNotFound,
         ());
    TEST_EQUAL(expandedCalls, 1, ());
    TEST(!result.IsValid(), ());
  }
}

UNIT_TEST(WaterRouter_WiderWindowCancellation)
{
  RouterDelegate delegate;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return WindowDetour(false); }, {},
                     [&](Checkpoints const &, RouterDelegate const &)
  {
    delegate.Cancel();
    return WindowDetour(true);
  });
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {0.01, 0}), {}, false, false, delegate, result) ==
           RouterResultCode::Cancelled,
       ());
  TEST(!result.IsValid(), ());
}
UNIT_TEST(WaterRouter_TwoWalkingAccessesUsePedestrianGeometryOnceEach)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &)
  {
    water::Geometry geometry;
    geometry.m_complete = true;
    geometry.m_hasSourceTopology = false;
    geometry.m_lines = {{{{0.002, 0}}, {{0.018, 0}}}};
    for (double x : {0.0, 0.02})
    {
      geometry.m_triangles.push_back({m2::PointD(x - 0.001, -0.001), {x + 0.001, -0.001}, {x + 0.001, 0.001}});
      geometry.m_triangles.push_back({m2::PointD(x - 0.001, -0.001), {x + 0.001, 0.001}, {x - 0.001, 0.001}});
    }
    return geometry;
  }, std::make_unique<WalkingStub>(calls));
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0}, {0.02, 0}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(calls, 2, ());
  Route const route(result.GetActive());
  TEST_GREATER(route.GetWaterDistances()->m_waterMeters, 1500.0, ());
  TEST_GREATER(route.GetWaterDistances()->m_landMeters, 400.0, ());
  TEST_EQUAL(
      std::count(route.GetPoly().GetPoints().begin(), route.GetPoly().GetPoints().end(), m2::PointD(-0.001, 0.001)), 2,
      ());
  TEST(!route.GetWaterDistances()->m_shortestConfirmed, ());
}
UNIT_TEST(WaterRouter_RetriesAnotherPedestrianTransfer)
{
  size_t calls = 0;
  WaterRouter router([](Checkpoints const &, RouterDelegate const &) { return DisconnectedRivers(); },
                     std::make_unique<WalkingStub>(calls, false, 1));
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0.002}, {0.004, 0.002}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(calls, 2, ());
  Route const route(result.GetActive());
  auto const & points = route.GetPoly().GetPoints();
  TEST(std::find(points.begin(), points.end(), m2::PointD(-0.001, 0.001)) != points.end(), ());
}
UNIT_TEST(WaterRouter_WiderWaterRoutePrecedesLocalWalkingProposal)
{
  size_t calls = 0, expanded = 0;
  WaterRouter router(
      [](Checkpoints const &, RouterDelegate const &)
  {
    auto geometry = DisconnectedRivers();
    geometry.m_bounds = m2::RectD(-0.01, -0.01, 0.02, 0.02);
    return geometry;
  }, std::make_unique<WalkingStub>(calls), [&](Checkpoints const &, RouterDelegate const &)
  {
    ++expanded;
    water::Geometry geometry;
    geometry.m_complete = true;
    geometry.m_bounds = m2::RectD(-0.01, -0.01, 0.02, 0.03);
    geometry.m_lines = {{{{0, 0}}, {{0, 0.015}}, {{0.004, 0.015}}, {{0.004, 0}}}};
    return geometry;
  });
  RouterDelegate delegate;
  RoutesResult result;
  TEST(router.CalculateRoute(Checkpoints({0, 0.002}, {0.004, 0.002}), {}, false, false, delegate, result) ==
           RouterResultCode::NoError,
       ());
  TEST_EQUAL(expanded, 1, ());
  TEST_EQUAL(calls, 0, ());
  TEST_EQUAL(result.GetActive().GetWaterDistances()->m_landMeters, 0.0, ());
}
}  // namespace water_router_test
