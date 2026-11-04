#include "testing/testing.hpp"

#include "routing/routing_session.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

namespace routing_session_checkpoint_test
{
using namespace routing;
using namespace std::chrono_literals;

class TestGuiThread final : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    std::lock_guard lock(m_mutex);
    m_tasks.push_back(std::move(task));
    m_cv.notify_one();
    return {true};
  }

  PushResult Push(Task const & task) override { return Push(Task(task)); }

  bool WaitForTask()
  {
    std::unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, 30s, [this] { return !m_tasks.empty(); });
  }

  bool RunNext()
  {
    if (!WaitForTask())
      return false;
    Task task;
    {
      std::lock_guard lock(m_mutex);
      task = std::move(m_tasks.front());
      m_tasks.pop_front();
    }
    task();
    return true;
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<Task> m_tasks;
};

Route MakeRoute(Checkpoints const & checkpoints, bool detour = false)
{
  auto const & points = checkpoints.GetPoints();
  std::vector<m2::PointD> path{points[checkpoints.GetPassedIdx()]};
  Route route;
  std::vector<RouteSegment> segments;
  std::vector<Route::SubrouteAttrs> subroutes;
  double meters = 0.0;
  double merc = 0.0;
  auto const addSegment = [&](m2::PointD const & to, bool destination)
  {
    auto const & from = path.back();
    meters += mercator::DistanceOnEarth(from, to);
    merc += (to - from).Length();
    auto const turn = turns::TurnItem(
        segments.size() + 1, destination ? turns::CarDirection::ReachedYourDestination : turns::CarDirection::None);
    segments.emplace_back(Segment(0, 0, segments.size(), true), turn,
                          geometry::PointWithAltitude(to, geometry::kDefaultAltitudeMeters),
                          RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime(meters, merc, meters / 10.0);
    path.push_back(to);
  };
  for (size_t i = 1; i < points.size(); ++i)
  {
    auto const start = geometry::PointWithAltitude(points[i - 1], geometry::kDefaultAltitudeMeters);
    auto const finish = geometry::PointWithAltitude(points[i], geometry::kDefaultAltitudeMeters);
    auto const begin = segments.size();
    if (i > checkpoints.GetPassedIdx())
    {
      if (detour && i == 2)
        addSegment({.003, .0045}, false);
      addSegment(points[i], i + 1 == points.size());
    }
    subroutes.emplace_back(start, finish, begin, segments.size());
  }
  route.SetGeometry(path.begin(), path.end());
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::move(subroutes), checkpoints.GetPassedIdx());
  return route;
}

DECLARE_EXCEPTION(RebuildException, RootException);

class CheckpointRouter final : public IRouter
{
public:
  enum class RebuildResult
  {
    Success,
    Detour,
    Failure,
    Exception
  };

  explicit CheckpointRouter(RebuildResult result) : m_result(result) {}

  std::string GetName() const override { return "checkpoint router"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    return false;
  }

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                  RouterDelegate const &, RoutesResult & result) override
  {
    if (++m_buildCount == 2)
    {
      std::unique_lock lock(m_mutex);
      m_blocked = true;
      m_cv.notify_one();
      m_cv.wait(lock, [this] { return m_released; });
      if (m_result == RebuildResult::Failure)
        return RouterResultCode::RouteNotFound;
      if (m_result == RebuildResult::Exception)
        MYTHROW(RebuildException, ("Rebuild failed."));
    }

    result.MakeFrom(GetName(), MakeRoute(checkpoints, m_buildCount == 2 && m_result == RebuildResult::Detour));
    result.m_routes.push_back(result.GetActive());
    return RouterResultCode::NoError;
  }

  bool WaitBlocked()
  {
    std::unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, 30s, [this] { return m_blocked; });
  }

  void Release()
  {
    std::lock_guard lock(m_mutex);
    m_released = true;
    m_cv.notify_one();
  }

private:
  RebuildResult m_result;
  size_t m_buildCount = 0;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  bool m_blocked = false;
  bool m_released = false;
};

class SessionTest
{
  Platform::ThreadRunner m_runner;

public:
  explicit SessionTest(CheckpointRouter::RebuildResult result = CheckpointRouter::RebuildResult::Success)
  {
    auto gui = std::make_unique<TestGuiThread>();
    m_gui = gui.get();
    GetPlatform().SetGuiThread(std::move(gui));
    m_session = std::make_unique<RoutingSession>();
    m_session->Init(nullptr);
    auto router = std::make_unique<CheckpointRouter>(result);
    m_router = router.get();
    m_session->SetRouter(std::move(router), nullptr);
    auto const ready = [this](RoutesResult const &, RouterResultCode) { ++m_readyCount; };
    m_session->SetRoutingCallbacks(ready, ready, nullptr, [this](RouterResultCode) { ++m_removeCount; });
    m_session->SetCheckpointCallback([this](size_t passed) { m_passed.push_back(passed); });
  }

  ~SessionTest()
  {
    m_router->Release();
    m_session.reset();
    GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>());
  }

  void Build()
  {
    m_session->BuildRoute(Checkpoints(CheckpointsGeometry{{0, .001}, {0, .003}, {0, .006}, {0, .009}}),
                          RouterDelegate::kNoTimeout);
    TEST(m_gui->RunNext(), ());
    TEST_EQUAL(m_readyCount, 1, ());
    TEST(m_session->EnableFollowMode(), ());
    MoveTo(.001);
  }

  void Rebuild(double start = .001)
  {
    m_session->RebuildRoute({0, start}, [this](RoutesResult const &, RouterResultCode) { ++m_readyCount; }, nullptr,
                            [this](RouterResultCode) { ++m_removeCount; }, RouterDelegate::kNoTimeout,
                            SessionState::RouteRebuilding, true);
    TEST(m_router->WaitBlocked(), ());
  }

  void MoveTo(double y)
  {
    location::GpsInfo gps;
    gps.m_latitude = mercator::YToLat(y);
    gps.m_longitude = 0;
    gps.m_horizontalAccuracy = 1;
    m_session->OnLocationPositionChanged(gps);
  }

  void Finish()
  {
    MoveTo(.003);
    MoveTo(.006);
    MoveTo(.009);
    TEST(m_session->IsFinished(), ());
    TEST_EQUAL(m_passed, (std::vector<size_t>{1, 2, 3}), ());
  }

  void CompleteRebuild()
  {
    m_router->Release();
    TEST(m_gui->RunNext(), ());
  }

  std::unique_ptr<RoutingSession> m_session;
  TestGuiThread * m_gui;
  CheckpointRouter * m_router;
  std::vector<size_t> m_passed;
  size_t m_readyCount = 0;
  size_t m_removeCount = 0;
};

void TestProgressDuringRebuild(size_t passed, double currentY)
{
  SessionTest test;
  test.Build();
  test.Rebuild();
  test.MoveTo(.003);
  if (passed == 2)
    test.MoveTo(.006);
  test.MoveTo(currentY);
  auto const completion = test.m_session->GetCompletionPercent();
  auto const remaining = test.m_session->GetRoute()->GetCurrentDistanceToEndMeters();
  test.CompleteRebuild();

  TEST_EQUAL(test.m_readyCount, 2, ());
  auto const & route = *test.m_session->GetRoute();
  TEST_EQUAL(route.GetCurrentSubrouteIdx(), passed, ());
  TEST_EQUAL(route.GetCurrentIter().m_ind, passed, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, currentY, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentDistanceToEndMeters(), remaining, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(test.m_session->GetCompletionPercent(), completion, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentTimeToEndSec(), remaining / 10.0, 1e-6, ());
  test.m_session->RouteCall([passed](RoutesResult const & result)
  {
    for (auto const & variant : result.m_routes)
      TEST_EQUAL(variant.GetCurrentSubrouteIdx(), passed, ());
  });

  // A GPS fix behind the last passed checkpoint must not rematch its passed leg.
  test.MoveTo(.002);
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, currentY, 1e-12, ());
  TEST_EQUAL(test.m_passed.size(), passed, ());

  // A subsequent rebuild must start at the active leg, rather than restore the whole route's origin.
  test.m_session->OnTrafficInfoClear();
  TEST(test.m_gui->RunNext(), ());
  TEST_EQUAL(test.m_session->GetRoute()->GetPoly().Front(), (m2::PointD{0, passed == 1 ? .003 : .006}), ());
}

UNIT_TEST(RoutingSession_RebuildPreservesCheckpointProgress)
{
  TestProgressDuringRebuild(1, .004);
}

UNIT_TEST(RoutingSession_RebuildPreservesMultipleCheckpoints)
{
  TestProgressDuringRebuild(2, .007);
}

UNIT_TEST(RoutingSession_AlternativePromotionPreservesCheckpointProgress)
{
  SessionTest test;
  test.Build();
  test.MoveTo(.003);
  test.MoveTo(.004);
  TEST(test.m_session->SwapActiveAlternative(1), ());
  auto const & route = *test.m_session->GetRoute();
  TEST_EQUAL(route.GetCurrentSubrouteIdx(), 1, ());
  TEST_EQUAL(route.GetCurrentIter().m_ind, 1, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, .003, 1e-12, ());
  test.MoveTo(.002);
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, .003, 1e-12, ());
  test.MoveTo(.004);
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, .004, 1e-12, ());
}

UNIT_TEST(RoutingSession_RebuildWithPassedCheckpoint)
{
  SessionTest test;
  test.Build();
  test.MoveTo(.003);
  test.Rebuild(.004);
  test.CompleteRebuild();
  auto const & route = *test.m_session->GetRoute();
  TEST_EQUAL(route.GetCurrentSubrouteIdx(), 1, ());
  TEST_EQUAL(route.GetCurrentIter().m_ind, 0, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentIter().m_pt.y, .004, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentDistanceToEndMeters(), mercator::DistanceOnEarth({0, .004}, {0, .009}), 1e-6,
                        ());
  test.MoveTo(.006);
  TEST_EQUAL(test.m_passed, (std::vector<size_t>{1, 2}), ());
}

UNIT_TEST(RoutingSession_RebuildProgressOutsideNewRoute)
{
  SessionTest test(CheckpointRouter::RebuildResult::Detour);
  test.Build();
  test.Rebuild();
  test.MoveTo(.003);
  test.MoveTo(.004);
  test.CompleteRebuild();
  auto const & route = *test.m_session->GetRoute();
  TEST_EQUAL(route.GetCurrentSubrouteIdx(), 1, ());
  TEST_EQUAL(route.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(route.GetCurrentIter().m_pt, (m2::PointD{0, .003}), ());
  test.MoveTo(.002);
  TEST_EQUAL(route.GetCurrentIter().m_pt, (m2::PointD{0, .003}), ());
}

UNIT_TEST(Route_PromotionSkipsRepeatedPassedCheckpoints)
{
  auto source = MakeRoute(Checkpoints(CheckpointsGeometry{{0, .001}, {0, .003}, {0, .003}, {0, .006}}));
  source.SetCurrentSubrouteIdx(2);
  Route route(static_cast<RouteBase const &>(source));
  TEST_EQUAL(route.GetCurrentIter().m_ind, 2, ());
  TEST_EQUAL(route.GetCurrentIter().m_pt, (m2::PointD{0, .003}), ());
  location::GpsInfo gps;
  gps.m_latitude = mercator::YToLat(.002);
  gps.m_longitude = 0;
  gps.m_horizontalAccuracy = 1;
  TEST(!route.MoveIterator(gps), ());
}

UNIT_TEST(Route_PromotionWithEmptyTerminalSubroute)
{
  auto source = MakeRoute(Checkpoints({0, .001}, {0, .003}));
  auto subroutes = source.GetSubroutes();
  auto const finish = subroutes.back().GetFinish();
  auto const end = source.GetRouteSegments().size();
  subroutes.emplace_back(finish, finish, end, end);
  source.SetSubroutes(std::move(subroutes), 1);
  Route route(static_cast<RouteBase const &>(source));
  TEST(route.IsValid(), ());
  TEST_EQUAL(route.GetCurrentIter().m_ind, end - 1, ());
  TEST_EQUAL(route.GetCurrentIter().m_pt, finish.GetPoint(), ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentDistanceToEndMeters(), 0.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetCurrentTimeToEndSec(), 0.0, 1e-6, ());
  TEST(route.IsSubroutePassed(1), ());
}

UNIT_TEST(Route_PromotionWithEmptyIntermediateSubroute)
{
  auto source = MakeRoute(Checkpoints(CheckpointsGeometry{{0, .001}, {0, .003}, {0, .006}}));
  auto subroutes = source.GetSubroutes();
  auto const checkpoint = subroutes.front().GetFinish();
  subroutes.insert(subroutes.begin() + 1, Route::SubrouteAttrs(checkpoint, checkpoint, 1, 1));
  source.SetSubroutes(std::move(subroutes), 1);
  Route route(static_cast<RouteBase const &>(source));
  TEST_EQUAL(route.GetCurrentIter().m_ind, 1, ());
  location::GpsInfo gps;
  gps.m_latitude = mercator::YToLat(.003);
  gps.m_longitude = 0;
  gps.m_horizontalAccuracy = 1;
  TEST(route.MoveIterator(gps), ());
  TEST(route.IsSubroutePassed(1), ());
  route.PassNextSubroute();
  gps.m_latitude = mercator::YToLat(.006);
  TEST(route.MoveIterator(gps), ());
  TEST(route.IsSubroutePassed(2), ());
}

void TestFinishDuringRebuild(CheckpointRouter::RebuildResult result, bool queued)
{
  SessionTest test(result);
  test.Build();
  test.Rebuild();
  if (queued)
  {
    test.m_router->Release();
    TEST(test.m_gui->WaitForTask(), ());
  }
  test.Finish();
  TEST(!test.m_session->SwapActiveAlternative(1), ());
  auto const * finishedRoute = test.m_session->GetRoute();
  test.CompleteRebuild();
  TEST_EQUAL(test.m_readyCount, 1, ());
  TEST_EQUAL(test.m_removeCount, 0, ());
  TEST(test.m_session->IsFinished(), ());
  TEST(test.m_session->IsFollowing(), ());
  TEST_EQUAL(test.m_session->GetRoute(), finishedRoute, ());
  TEST_ALMOST_EQUAL_ABS(test.m_session->GetRoute()->GetCurrentDistanceToEndMeters(), 0.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(test.m_session->GetCompletionPercent(), 100.0, 1e-6, ());
  test.MoveTo(.004);
  TEST(test.m_session->IsFinished(), ());
  TEST_EQUAL(test.m_passed, (std::vector<size_t>{1, 2, 3}), ());
}

UNIT_TEST(RoutingSession_FinishedRouteCancelsBlockedRebuild)
{
  TestFinishDuringRebuild(CheckpointRouter::RebuildResult::Success, false /* queued */);
}

UNIT_TEST(RoutingSession_FinishedRouteCancelsQueuedRebuild)
{
  TestFinishDuringRebuild(CheckpointRouter::RebuildResult::Success, true /* queued */);
}

UNIT_TEST(RoutingSession_FinishedRouteCancelsRebuildFailure)
{
  TestFinishDuringRebuild(CheckpointRouter::RebuildResult::Failure, false /* queued */);
}

UNIT_TEST(RoutingSession_FinishedRouteCancelsRebuildException)
{
  base::ScopedLogAbortLevelChanger allowExpectedError;
  TestFinishDuringRebuild(CheckpointRouter::RebuildResult::Exception, false /* queued */);
}
}  // namespace routing_session_checkpoint_test
