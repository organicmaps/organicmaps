#include "testing/testing.hpp"

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_tests/tools.hpp"

#include "geometry/mercator.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace async_router_test
{
using namespace routing;
using namespace std;
using namespace std::chrono_literals;

Route MakeValidRoute(Checkpoints const & checkpoints)
{
  auto const & points = checkpoints.GetPoints();
  Route route;
  route.SetGeometry(points.cbegin(), points.cend());
  vector<RouteSegment> segments;
  double distanceMeters = 0.0;
  double distanceMercator = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    distanceMeters += mercator::DistanceOnEarth(points[i - 1], points[i]);
    distanceMercator += points[i - 1].Length(points[i]);
    turns::TurnItem const turn(
        i, i + 1 == points.size() ? turns::CarDirection::ReachedYourDestination : turns::CarDirection::None);
    segments.emplace_back(Segment(0, 0, i - 1, true), turn,
                          geometry::PointWithAltitude(points[i], geometry::kDefaultAltitudeMeters),
                          RouteSegment::RoadNameInfo{});
    segments.back().SetDistancesAndTime(distanceMeters, distanceMercator, 0.0);
  }
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(vector<Route::SubrouteAttrs>{Route::SubrouteAttrs(
      geometry::PointWithAltitude(points.front(), geometry::kDefaultAltitudeMeters),
      geometry::PointWithAltitude(points.back(), geometry::kDefaultAltitudeMeters), 0, points.size() - 1)});
  return route;
}

enum class Outcome
{
  Success,
  BlockedSuccess,
  Failure,
  Exception
};

class TestRouter final : public IRouter
{
public:
  explicit TestRouter(vector<Outcome> outcomes) : m_outcomes(std::move(outcomes)) {}
  string GetName() const override { return "async test"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks &&) override {}
  void SwapAltRouteToActive() override { ++m_swaps; }

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                  RouterDelegate const &, RoutesResult & result) override
  {
    CHECK_LESS(m_calculations, m_outcomes.size(), ());
    auto const outcome = m_outcomes[m_calculations++];
    if (outcome == Outcome::BlockedSuccess)
    {
      unique_lock lock(m_mutex);
      m_started = true;
      m_cv.notify_all();
      m_cv.wait(lock, [this] { return m_release; });
    }
    if (outcome == Outcome::Failure)
      return RouterResultCode::RouteNotFound;

    result.MakeFrom(GetName(), MakeValidRoute(checkpoints));
    auto alternative =
        MakeValidRoute(Checkpoints(CheckpointsGeometry{checkpoints.GetStart(), {2, 5}, checkpoints.GetFinish()}));
    alternative.SetDiffMidpoint({2, 5});
    result.m_routes.emplace_back(std::move(static_cast<RouteBase &>(alternative)));
    if (outcome == Outcome::Exception)
      MYTHROW(RootException, ("Test exception after constructing a valid result"));
    return RouterResultCode::NoError;
  }

  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    ++m_projections;
    return false;
  }

  bool WaitStarted()
  {
    unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, 30s, [this] { return m_started; });
  }

  void Release()
  {
    lock_guard lock(m_mutex);
    m_release = true;
    m_cv.notify_all();
  }

  bool WaitIdle(AsyncRouter & async)
  {
    auto const before = m_projections;
    auto const deadline = chrono::steady_clock::now() + 30s;
    while (chrono::steady_clock::now() < deadline)
    {
      EdgeProj proj;
      async.FindClosestProjectionToRoad({1, 2}, {0, 1}, 50, proj);
      if (m_projections > before)
        return true;
      this_thread::sleep_for(1ms);
    }
    return false;
  }

  size_t m_swaps = 0;
  size_t m_projections = 0;

private:
  vector<Outcome> const m_outcomes;
  size_t m_calculations = 0;
  mutex m_mutex;
  condition_variable m_cv;
  bool m_started = false;
  bool m_release = false;
};

using ReadyResult = pair<uint64_t, RouterResultCode>;

struct ReadyRequest
{
  // Keep the promise alive when cancellation drops its callback, so an undelivered future stays pending.
  shared_ptr<promise<ReadyResult>> m_ready = make_shared<promise<ReadyResult>>();
  future<ReadyResult> m_future = m_ready->get_future();
};

ReadyRequest Calculate(AsyncRouter & async, RemoveRouteCallback const & removed = {})
{
  // GUI callbacks can outlive the test body, including after an assertion throws.
  ReadyRequest request;
  async.CalculateRoute(Checkpoints({1, 2}, {5, 6}), {3, 4}, true /* adjustToPrevRoute */, true /* needAlternatives */,
                       [ready = request.m_ready](shared_ptr<RoutesResult> result, RouterResultCode code)
  { ready->set_value({result->m_routesId, code}); }, nullptr, removed, nullptr);
  return request;
}

uint64_t WaitReady(ReadyRequest & ready)
{
  TEST(ready.m_future.wait_for(30s) == future_status::ready, ("Route was not built."));
  auto const [id, code] = ready.m_future.get();
  TEST_EQUAL(code, RouterResultCode::NoError, ());
  return id;
}

template <typename Fn>
auto RunOnGui(Fn && fn)
{
  // Return GUI-thread assertion failures to the test thread, keeping the task alive on timeout.
  auto task = make_shared<packaged_task<invoke_result_t<Fn>()>>(std::forward<Fn>(fn));
  auto done = task->get_future();
  GetPlatform().RunTask(Platform::Thread::Gui, [task] { (*task)(); });
  TEST(done.wait_for(30s) == future_status::ready, ("GUI task did not finish."));
  return done.get();
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, RouterCallsWhileCalculating)
{
  auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::BlockedSuccess});
  auto * raw = router.get();
  AsyncRouter async(nullptr);
  async.SetRouter(std::move(router), nullptr);
  // Release before ~AsyncRouter joins its worker, even when a TEST throws.
  SCOPE_GUARD(releaseRouter, [raw] { raw->Release(); });

  auto first = Calculate(async);
  auto const oldId = WaitReady(first);
  TEST(async.SwapAltRouteToActive(oldId), ("Warm the cache with a promotable result."));
  auto second = Calculate(async);
  TEST(raw->WaitStarted(), ("Rebuild did not start."));

  TEST(!async.SwapAltRouteToActive(oldId), ("Rebuilding must invalidate an existing cache generation."));
  TEST_EQUAL(raw->m_swaps, 1, ());
  EdgeProj proj;
  TEST(!async.FindClosestProjectionToRoad({1, 2}, {0, 1}, 50, proj), ());
  TEST_EQUAL(raw->m_projections, 0, ("The busy router must not be queried."));

  raw->Release();
  auto const newId = WaitReady(second);
  TEST(!async.SwapAltRouteToActive(oldId), ("The previously delivered result is stale."));
  TEST(async.SwapAltRouteToActive(newId), ());
  TEST_EQUAL(raw->m_swaps, 2, ());
  async.FindClosestProjectionToRoad({1, 2}, {0, 1}, 50, proj);
  TEST_EQUAL(raw->m_projections, 1, ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, SwapAfterFailedCalculation)
{
  auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::Failure, Outcome::Success});
  AsyncRouter async(nullptr);
  async.SetRouter(std::move(router), nullptr);
  auto first = Calculate(async);
  auto const oldId = WaitReady(first);

  auto failed = make_shared<promise<RouterResultCode>>();
  auto failure = failed->get_future();
  Calculate(async, [failed](RouterResultCode code) { failed->set_value(code); });
  TEST(failure.wait_for(30s) == future_status::ready, ());
  auto const code = failure.get();
  TEST_EQUAL(code, RouterResultCode::RouteNotFound, ());
  TEST(!async.SwapAltRouteToActive(oldId), ());

  auto recovered = Calculate(async);
  TEST(async.SwapAltRouteToActive(WaitReady(recovered)), ("A successful build restores promotion."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, SwapAfterExceptionalCalculation)
{
  base::ScopedLogAbortLevelChanger allowExpectedError(base::LCRITICAL);
  auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::Exception, Outcome::Success});
  auto * raw = router.get();
  AsyncRouter async(nullptr);
  async.SetRouter(std::move(router), nullptr);
  auto first = Calculate(async);
  auto const oldId = WaitReady(first);
  auto exception = Calculate(async);
  TEST(exception.m_future.wait_for(30s) == future_status::ready, ());
  auto const [failedId, code] = exception.m_future.get();
  TEST_EQUAL(code, RouterResultCode::InternalError, ());
  TEST(!async.SwapAltRouteToActive(oldId), ());
  TEST(!async.SwapAltRouteToActive(failedId), ("A partially constructed result cannot validate caches."));
  TEST(raw->WaitIdle(async), ());
  auto recovered = Calculate(async);
  TEST(async.SwapAltRouteToActive(WaitReady(recovered)), ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, SwapAfterClearState)
{
  auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::BlockedSuccess});
  auto * raw = router.get();
  AsyncRouter async(nullptr);
  async.SetRouter(std::move(router), nullptr);
  SCOPE_GUARD(releaseRouter, [raw] { raw->Release(); });
  auto first = Calculate(async);
  auto const oldId = WaitReady(first);
  auto cancelled = Calculate(async);
  TEST(raw->WaitStarted(), ());
  async.ClearState();
  raw->Release();
  TEST(raw->WaitIdle(async), ());
  TEST(!async.SwapAltRouteToActive(oldId), ());
  TEST(cancelled.m_future.wait_for(0s) != future_status::ready, ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, SwapAfterSupersededCalculation)
{
  auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::BlockedSuccess, Outcome::Success});
  auto * raw = router.get();
  AsyncRouter async(nullptr);
  async.SetRouter(std::move(router), nullptr);
  SCOPE_GUARD(releaseRouter, [raw] { raw->Release(); });
  auto first = Calculate(async);
  auto const oldId = WaitReady(first);
  auto superseded = Calculate(async);
  TEST(raw->WaitStarted(), ());
  auto replacement = Calculate(async);
  raw->Release();
  auto const newId = WaitReady(replacement);
  TEST(superseded.m_future.wait_for(0s) != future_status::ready, ());
  TEST(!async.SwapAltRouteToActive(oldId), ());
  TEST(async.SwapAltRouteToActive(newId), ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, AlternativesDiscardedDuringRebuild)
{
  auto built = make_shared<promise<bool>>();
  auto builtFuture = built->get_future();
  RunOnGui([this, built]
  {
    InitRoutingSession();
    m_session->SetRouter(make_unique<TestRouter>(vector{Outcome::Success, Outcome::Failure, Outcome::Success}),
                         nullptr);
    m_session->SetRoutingCallbacks([this, built](RoutesResult const &, RouterResultCode)
    { built->set_value(m_session->SwapActiveAlternative(1)); }, nullptr, nullptr, nullptr);
    m_session->BuildRoute(Checkpoints({1, 2}, {5, 6}), RouterDelegate::kNoTimeout);
  });
  TEST(builtFuture.wait_for(30s) == future_status::ready, ());
  TEST(builtFuture.get(), ());

  auto failed = make_shared<promise<void>>();
  auto failedFuture = failed->get_future();
  RunOnGui([this, failed]
  {
    m_session->RebuildRoute({1, 2}, [](RoutesResult const &, RouterResultCode) {}, nullptr, [failed](RouterResultCode)
    { failed->set_value(); }, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true);
    m_session->RouteCall([](RoutesResult const & result)
    {
      TEST_EQUAL(result.m_routes.size(), 1, ());
      TEST_EQUAL(result.GetActive().GetRouteSegments().size(), 2,
                 ("Only the selected three-point route remains available while rebuilding."));
    });
  });
  TEST(failedFuture.wait_for(30s) == future_status::ready, ());

  auto recovered = make_shared<promise<bool>>();
  auto recoveredFuture = recovered->get_future();
  RunOnGui([this, recovered]
  {
    m_session->RouteCall([](RoutesResult const & result)
    {
      TEST_EQUAL(result.m_routes.size(), 1, ());
      TEST_EQUAL(result.GetActive().GetRouteSegments().size(), 2,
                 ("Failure retains the selected route without choices."));
    });
    m_session->RebuildRoute({1, 2}, [this, recovered](RoutesResult const &, RouterResultCode) {
      recovered->set_value(m_session->SwapActiveAlternative(1));
    }, nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true);
  });
  TEST(recoveredFuture.wait_for(30s) == future_status::ready, ());
  TEST(recoveredFuture.get(), ("A successful result restores alternatives."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, RoadProjectionAfterReturningToRouteDuringRebuild)
{
  auto built = make_shared<promise<void>>();
  auto builtFuture = built->get_future();
  auto * raw = RunOnGui([this, built]
  {
    InitRoutingSession();
    auto router = make_unique<TestRouter>(vector{Outcome::Success, Outcome::BlockedSuccess});
    auto * raw = router.get();
    m_session->SetRouter(std::move(router), nullptr);
    m_session->SetRoutingCallbacks([built](RoutesResult const &, RouterResultCode) { built->set_value(); }, nullptr,
                                   nullptr, nullptr);
    m_session->BuildRoute(Checkpoints({1, 2}, {5, 6}), RouterDelegate::kNoTimeout);
    return raw;
  });
  SCOPE_GUARD(releaseRouter, [raw] { raw->Release(); });
  TEST(builtFuture.wait_for(30s) == future_status::ready, ());

  auto rebuilt = make_shared<promise<void>>();
  auto rebuiltFuture = rebuilt->get_future();
  RunOnGui([this, rebuilt]
  {
    TEST(m_session->EnableFollowMode(), ());
    m_session->RebuildRoute({1, 2}, [rebuilt](RoutesResult const &, RouterResultCode) { rebuilt->set_value(); },
                            nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true);
    TEST(m_session->IsRebuildingOnly(), ());
  });
  TEST(raw->WaitStarted(), ("Rebuild did not start."));

  RunOnGui([this, raw]
  {
    location::GpsInfo info;
    info.m_longitude = 1.002;
    info.m_latitude = mercator::YToLat(2);
    // Iterator movement accepts the accuracy radius; route snapping uses a smaller threshold.
    info.m_horizontalAccuracy = 500;
    TEST_EQUAL(m_session->OnLocationPositionChanged(info), SessionState::OnRoute, ());
    location::RouteMatchingInfo matchingInfo;
    TEST(!m_session->MatchLocationToRoute(info, matchingInfo), ("The caller must fall back to the road graph."));

    // Ensure road projection has a direction, so it reaches the async-router guard.
    m_session->PushPositionAccumulator({1, 2});
    m_session->PushPositionAccumulator({1, 2.0002});
    m_session->MatchLocationToRoadGraph(info);
    TEST_EQUAL(raw->m_projections, 0, ("OnRoute must not expose the busy road graph."));
  });
  TEST(rebuiltFuture.wait_for(0s) != future_status::ready, ("The worker must still be blocked."));

  raw->Release();
  TEST(rebuiltFuture.wait_for(30s) == future_status::ready, ());
  RunOnGui([this, raw]
  {
    location::GpsInfo info;
    m_session->MatchLocationToRoadGraph(info);
    TEST_EQUAL(raw->m_projections, 1, ("Road projection resumes after the worker finishes."));
  });
}
}  // namespace async_router_test
