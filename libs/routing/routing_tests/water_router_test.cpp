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
  explicit WalkingStub(size_t & calls, bool fail = false) : m_calls(calls), m_fail(fail) {}
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                  RouterDelegate const & delegate, RoutesResult & result) override
  {
    ++m_calls;
    delegate.OnProgress(20);
    if (delegate.GetCancellable().IsCancelled())
      return RouterResultCode::Cancelled;
    if (m_fail)
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
}  // namespace water_router_test
