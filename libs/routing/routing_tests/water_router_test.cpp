#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/ruler_router.hpp"
#include "routing/water_router.hpp"

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
}  // namespace water_router_test
