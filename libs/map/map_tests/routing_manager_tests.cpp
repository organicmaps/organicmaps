#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "routing/ruler_router.hpp"

#include "storage/routing_helpers.hpp"

#include "drape_frontend/visual_params.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"
#include "base/thread_safe_queue.hpp"

#include <algorithm>
#include <future>
#include <memory>
#include <utility>
#include <vector>

namespace routing_manager_tests
{
struct RoutingManagerAltMarksTest
{
  // Headless InsertRoute() skips rendering, so queue its mark work without a drape engine.
  static void QueueMarks(RoutingManager & manager, routing::RoutesResult const & result)
  {
    manager.CreateRouteAltMarks(result);
  }
};

namespace
{
// Pump only this fixture's tasks, not callbacks left by earlier Framework instances.
class TestGuiThread final : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    m_tasks.Push(std::move(task));
    return {true, kNoId};
  }
  PushResult Push(Task const & task) override { return Push(Task(task)); }

  void RunNext()
  {
    Task task;
    m_tasks.WaitAndPop(task);
    task();
  }

  void RunPending()
  {
    Task task;
    while (m_tasks.TryPop(task))
      task();
  }

private:
  threads::ThreadSafeQueue<Task> m_tasks;
};

void WaitForFileThread()
{
  std::promise<void> completed;
  auto future = completed.get_future();
  GetPlatform().RunTask(Platform::Thread::File, [&completed] { completed.set_value(); });
  future.wait();
}

bool LoadRoutePoints(RoutingManager & routingManager, TestGuiThread & guiThread)
{
  bool loaded = false;
  bool completed = false;
  // Failures may call back from the file thread, so always finish on the test GUI thread.
  routingManager.LoadRoutePoints([&](bool success)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [&, success]
    {
      loaded = success;
      completed = true;
    });
  });
  while (!completed)
    guiThread.RunNext();
  return loaded;
}

template <typename Fn>
void WithSavedRoute(Fn && fn)
{
  auto & platform = GetPlatform();
  auto const testSettingsDir = base::JoinPath(platform.WritableDir(), "routing_manager_save_load_test");
  platform::tests_support::ScopedDirCleanup settingsDir(testSettingsDir);

  auto testGuiThread = std::make_unique<TestGuiThread>();
  auto * guiThread = testGuiThread.get();
  platform.SetGuiThread(std::move(testGuiThread));
  SCOPE_GUARD(restoreGuiThread, [&platform] { platform.SetGuiThread(std::make_unique<platform::GuiThread>()); });

  // Initialize global settings before redirecting the saved route to its test directory.
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto const originalSettingsDir = platform.SettingsDir();
  platform.SetSettingsDir(testSettingsDir);
  SCOPE_GUARD(restoreSettingsDir, [&] { platform.SetSettingsDir(originalSettingsDir); });
  SCOPE_GUARD(drainFileThread, [] { WaitForFileThread(); });

  std::forward<Fn>(fn)(framework.GetRoutingManager(), framework.GetBookmarkManager(), *guiThread);
}

RouteMarkData MakeRoutePoint(RouteMarkType type, size_t intermediateIndex, double coord)
{
  RouteMarkData data;
  data.m_pointType = type;
  data.m_intermediateIndex = intermediateIndex;
  data.m_position = m2::PointD(coord, coord);
  return data;
}

RouteMarkData MakeRoutePointOnXAxis(RouteMarkType type, size_t intermediateIndex, double coord)
{
  auto point = MakeRoutePoint(type, intermediateIndex, coord);
  point.m_position.y = 0.0;
  return point;
}

size_t GetIntermediatePointsCount(std::vector<RouteMarkData> const & points)
{
  return std::count_if(points.begin(), points.end(),
                       [](RouteMarkData const & d) { return d.m_pointType == RouteMarkType::Intermediate; });
}

void FillRouteToLimit(RoutingManager & routingManager)
{
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0), false /* optimize */), ());
  for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
  {
    TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 1)),
                                      false /* optimize */),
         (i));
  }
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 101.0), false /* optimize */), ());
}

void TestSavedRouteOrder(bool myPositionStart, size_t passedStops, bool hasCurrentPosition,
                         size_t intermediateCount = 3)
{
  WithSavedRoute([=](RoutingManager & routingManager, BookmarkManager & bookmarks, TestGuiThread & guiThread)
  {
    auto start = MakeRoutePoint(RouteMarkType::Start, 0, 1.0);
    start.m_title = "Start";
    start.m_subTitle = "Start subtitle";
    start.m_isMyPosition = myPositionStart;
    bookmarks.MyPositionMark().SetUserPosition(start.m_position, true /* hasPosition */);
    routingManager.AddRoutePoint(std::move(start), false /* optimize */);
    for (size_t i = 0; i < intermediateCount; ++i)
    {
      // Descending coordinates ensure that loading does not optimize the saved order.
      auto const coordinate = static_cast<double>(intermediateCount - i + 1);
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, coordinate);
      point.m_title = "Stop " + std::to_string(i);
      point.m_subTitle = "Stop subtitle " + std::to_string(i);
      point.m_isPassed = i < passedStops;
      routingManager.AddRoutePoint(std::move(point), false /* optimize */);
    }
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 110.0);
    finish.m_title = "Finish";
    finish.m_subTitle = "Finish subtitle";
    routingManager.AddRoutePoint(std::move(finish), false /* optimize */);

    auto expected = routingManager.GetRoutePoints();
    expected.erase(expected.begin(), expected.begin() + passedStops);
    expected.front().m_pointType = RouteMarkType::Start;
    expected.front().m_intermediateIndex = 0;
    for (size_t i = 1; i + 1 < expected.size(); ++i)
      expected[i].m_intermediateIndex = i - 1;

    routingManager.SaveRoutePoints();
    routingManager.RemoveRoutePoints();
    m2::PointD const currentPosition(20.0, 20.0);
    bookmarks.MyPositionMark().SetUserPosition(currentPosition, hasCurrentPosition);
    bool const restoreMyPosition = myPositionStart && passedStops == 0;
    if (restoreMyPosition && hasCurrentPosition)
    {
      expected.front().m_position = currentPosition;
      expected.front().m_title.clear();
      expected.front().m_subTitle.clear();
    }

    TEST(LoadRoutePoints(routingManager, guiThread), ());

    auto const actual = routingManager.GetRoutePoints();
    TEST_EQUAL(actual.size(), expected.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
    {
      TEST(actual[i].m_pointType == expected[i].m_pointType, (i));
      TEST_EQUAL(actual[i].m_intermediateIndex, expected[i].m_intermediateIndex, (i));
      TEST_EQUAL(actual[i].m_position, expected[i].m_position, (i));
      TEST_EQUAL(actual[i].m_title, expected[i].m_title, (i));
      TEST_EQUAL(actual[i].m_subTitle, expected[i].m_subTitle, (i));
      TEST(!actual[i].m_isPassed, (i));
    }
    TEST_EQUAL(actual.front().m_isMyPosition, restoreMyPosition && hasCurrentPosition, ());
    TEST_EQUAL(actual.front().m_replaceWithMyPositionAfterRestart, restoreMyPosition && !hasCurrentPosition, ());

    WaitForFileThread();
    TEST(!routingManager.HasSavedRoutePoints(), ());
  });
}

void AddTestRoute(RoutingManager & manager, std::vector<double> const & stops)
{
  auto start = MakeRoutePointOnXAxis(RouteMarkType::Start, 0, -1.0);
  manager.AddRoutePoint(std::move(start), false /* optimize */);
  for (size_t i = 0; i < stops.size(); ++i)
  {
    auto point = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, i, stops[i]);
    manager.AddRoutePoint(std::move(point), false /* optimize */);
  }
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 10.0), false /* optimize */);
}

std::vector<double> GetStopCoordinates(RoutingManager const & manager)
{
  std::vector<double> result;
  for (auto const & point : manager.GetRoutePoints())
    if (point.m_pointType == RouteMarkType::Intermediate)
      result.push_back(point.m_position.x);
  return result;
}

double GetRouteLengthMeters(RoutingManager const & manager)
{
  auto const points = manager.GetRoutePoints();
  double length = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
    length += mercator::DistanceOnEarth(points[i - 1].m_position, points[i].m_position);
  return length;
}

class AlternativeRouter final : public routing::IRouter
{
public:
  std::string GetName() const override { return "alternative-test-router"; }
  void ClearState() override {}
  void SetGuides(routing::GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, routing::EdgeProj &) override
  {
    return false;
  }

  routing::RouterResultCode CalculateRoute(routing::Checkpoints const & checkpoints, m2::PointD const & direction,
                                           bool adjust, bool needAlternatives, routing::RouterDelegate const & delegate,
                                           routing::RoutesResult & result) override
  {
    routing::RulerRouter ruler;
    auto const code = static_cast<routing::IRouter &>(ruler).CalculateRoute(checkpoints, direction, adjust,
                                                                            needAlternatives, delegate, result);
    auto alternative = result.GetActive();
    result.m_routes.push_back(std::move(alternative));
    return code;
  }
};

class RouteFollowDelegate final : public RoutingManager::Delegate
{
public:
  void OnRouteFollow(routing::RouterType type) override
  {
    TEST(type == routing::RouterType::Vehicle, ());
    ++m_followCalls;
  }

  size_t m_followCalls = 0;
};
}  // namespace

UNIT_TEST(RoutingManager_QueuedAlternativeMarksRespectFollowing)
{
  auto & platform = GetPlatform();
  auto testGuiThread = std::make_unique<TestGuiThread>();
  auto * guiThread = testGuiThread.get();
  platform.SetGuiThread(std::move(testGuiThread));
  SCOPE_GUARD(restoreGuiThread, [&platform] { platform.SetGuiThread(std::make_unique<platform::GuiThread>()); });

  Framework framework(FrameworkParams(false /* m_enableDiffs */), false /* loadMaps */);
  auto & bookmarks = framework.GetBookmarkManager();
  auto callbacks = framework.GetRoutingManager().GetCallbacksForTests();
  RouteFollowDelegate delegate;
  RoutingManager manager(std::move(callbacks), delegate);
  manager.SetBookmarkManager(&bookmarks);
  manager.SetTransitManager(&framework.GetTransitManager());
  manager.Init(routing::CreateNumMwmIds(framework.GetStorage()));
  auto const originalRouter = manager.GetLastUsedRouter();
  SCOPE_GUARD(restoreRouter, [&] { manager.SetLastUsedRouter(originalRouter); });
  manager.SetRouter(routing::RouterType::Vehicle);
  df::VisualParams::Init(1.0, 256);

  auto & session = manager.RoutingSession();
  session.SetRouter(std::make_unique<AlternativeRouter>(), nullptr /* finder */);
  session.SetRoutingCallbacks([&](routing::RoutesResult const & result, routing::RouterResultCode code)
  {
    RoutingManagerAltMarksTest::QueueMarks(manager, result);
    manager.OnBuildRouteReady(result, code);
  }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);

  size_t builds = 0;
  bool resumeNavigation = false;
  manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
  {
    TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
    TEST(!manager.IsRoutingFollowing(), ());
    if (resumeNavigation)
    {
      manager.FollowRoute();
      TEST(manager.IsRoutingFollowing(), ());
      TEST(bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).empty(), ());
    }
    ++builds;
  });
  auto const build = [&]
  {
    auto const expected = builds + 1;
    manager.BuildRoute();
    while (builds != expected)
      guiThread->RunNext();
  };
  auto const marksCount = [&] { return bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).size(); };
  AddTestRoute(manager, {});

  build();
  guiThread->RunPending();
  TEST_EQUAL(marksCount(), 2, ("Planning must show both route choices"));
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(marksCount(), 0, ("Starting navigation must clear existing balloons"));
  guiThread->RunPending();
  TEST_EQUAL(marksCount(), 0, ());

  // A full build disables following; the platform listener resumes the existing navigation session.
  resumeNavigation = true;
  build();
  guiThread->RunPending();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(marksCount(), 0, ("Queued balloons must stay hidden after navigation resumes"));
  TEST_EQUAL(delegate.m_followCalls, 2, ());
  manager.CloseRouting(true /* removeRoutePoints */);
  guiThread->RunPending();
}

UNIT_TEST(RoutingManager_ReverseRoutePointsRequiresBothEndpoints)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  TEST(!manager.ReverseRoutePoints(), ());

  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1.0), false /* optimize */);
  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 0, 2.0), false /* optimize */);
  auto const startAndStop = manager.GetRoutePoints();
  TEST(!manager.ReverseRoutePoints(), ());
  auto const afterMissingFinish = manager.GetRoutePoints();
  TEST_EQUAL(afterMissingFinish.size(), startAndStop.size(), ());
  for (size_t i = 0; i < startAndStop.size(); ++i)
  {
    TEST(afterMissingFinish[i].m_pointType == startAndStop[i].m_pointType, (i));
    TEST_EQUAL(afterMissingFinish[i].m_position, startAndStop[i].m_position, (i));
  }

  manager.RemoveRoutePoints();
  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 0, 2.0), false /* optimize */);
  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 3.0), false /* optimize */);
  auto const stopAndFinish = manager.GetRoutePoints();
  TEST(!manager.ReverseRoutePoints(), ());
  auto const afterMissingStart = manager.GetRoutePoints();
  TEST_EQUAL(afterMissingStart.size(), stopAndFinish.size(), ());
  for (size_t i = 0; i < stopAndFinish.size(); ++i)
  {
    TEST(afterMissingStart[i].m_pointType == stopAndFinish[i].m_pointType, (i));
    TEST_EQUAL(afterMissingStart[i].m_position, stopAndFinish[i].m_position, (i));
  }
}

UNIT_TEST(RoutingManager_ReverseRoutePointsReversesEntireRoute)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 1.0);
  start.m_title = "Start";
  start.m_isMyPosition = true;
  manager.AddRoutePoint(std::move(start), false /* optimize */);
  for (size_t i = 0; i < 3; ++i)
  {
    auto stop = MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 2));
    stop.m_title = "Stop " + std::to_string(i);
    manager.AddRoutePoint(std::move(stop), false /* optimize */);
  }
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 5.0);
  finish.m_title = "Finish";
  manager.AddRoutePoint(std::move(finish), false /* optimize */);
  {
    RoutePointsLayout layout(framework.GetBookmarkManager());
    layout.PassRoutePoint(RouteMarkType::Intermediate, 0);
  }

  TEST(manager.ReverseRoutePoints(), ());
  auto const reversed = manager.GetRoutePoints();
  TEST_EQUAL(reversed.size(), 5, ());
  for (size_t i = 0; i < reversed.size(); ++i)
  {
    TEST_EQUAL(reversed[i].m_position, m2::PointD(5.0 - i, 5.0 - i), (i));
    TEST(!reversed[i].m_isPassed, (i));
    TEST_EQUAL(reversed[i].m_isVisible, i != reversed.size() - 1, (i));
    if (i > 0 && i + 1 < reversed.size())
    {
      TEST(reversed[i].m_pointType == RouteMarkType::Intermediate, (i));
      TEST_EQUAL(reversed[i].m_intermediateIndex, i - 1, (i));
      TEST_EQUAL(reversed[i].m_title, "Stop " + std::to_string(3 - i), (i));
    }
  }
  TEST(reversed.front().m_pointType == RouteMarkType::Start, ());
  TEST_EQUAL(reversed.front().m_title, "Finish", ());
  TEST(reversed.back().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(reversed.back().m_title, "Start", ());
  TEST(reversed.back().m_isMyPosition, ());

  TEST(manager.ReverseRoutePoints(), ());
  auto const originalOrder = manager.GetRoutePoints();
  for (size_t i = 0; i < originalOrder.size(); ++i)
    TEST_EQUAL(originalOrder[i].m_position, m2::PointD(i + 1.0, i + 1.0), (i));
}

UNIT_TEST(RoutingManager_ReverseRoutePointsWithNoStops)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();
  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1.0), false /* optimize */);
  manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2.0), false /* optimize */);

  TEST(manager.ReverseRoutePoints(), ());
  auto const points = manager.GetRoutePoints();
  TEST_EQUAL(points.size(), 2, ());
  TEST(points[0].m_pointType == RouteMarkType::Start, ());
  TEST_EQUAL(points[0].m_position, m2::PointD(2.0, 2.0), ());
  TEST(points[1].m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(points[1].m_position, m2::PointD(1.0, 1.0), ());
}

UNIT_TEST(RoutingManager_MoveSingleRoutePointBetweenEndpoints)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();
  auto point = MakeRoutePoint(RouteMarkType::Start, 0, 1.0);
  point.m_title = "Only point";
  manager.AddRoutePoint(std::move(point), false /* optimize */);
  auto const getMarkId = [&framework]
  {
    RoutePointsLayout layout(framework.GetBookmarkManager());
    return layout.GetRoutePoints().front()->GetId();
  };
  auto const markId = getMarkId();

  manager.MoveRoutePoint(0, 1);
  auto const finish = manager.GetRoutePoints();
  TEST_EQUAL(finish.size(), 1, ());
  TEST(finish.front().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(finish.front().m_position, m2::PointD(1.0, 1.0), ());
  TEST_EQUAL(finish.front().m_title, "Only point", ());
  TEST_EQUAL(getMarkId(), markId, ());

  manager.MoveRoutePoint(1, 0);
  auto const start = manager.GetRoutePoints();
  TEST_EQUAL(start.size(), 1, ());
  TEST(start.front().m_pointType == RouteMarkType::Start, ());
  TEST_EQUAL(start.front().m_position, m2::PointD(1.0, 1.0), ());
  TEST_EQUAL(getMarkId(), markId, ());

  // Empty endpoint cells can also be dragged onto the existing point.
  manager.MoveRoutePoint(1, 0);
  auto const finishFromEmptyStart = manager.GetRoutePoints();
  TEST(finishFromEmptyStart.front().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(getMarkId(), markId, ());

  manager.MoveRoutePoint(0, 1);
  auto const startFromEmptyFinish = manager.GetRoutePoints();
  TEST(startFromEmptyFinish.front().m_pointType == RouteMarkType::Start, ());
  TEST_EQUAL(getMarkId(), markId, ());
}

UNIT_TEST(RoutingManager_OptimizationWithIncompleteRoute)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  TEST(!manager.OptimizeRoutePoints(), ());
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Start, 0, -1.0), false /* optimize */);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 2.0), false /* optimize */);
  TEST(!manager.OptimizeRoutePoints(), ());
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 10.0), false /* optimize */);
  TEST(!manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{2}), ());
}

UNIT_TEST(RoutingManager_OptimizationCannotChangeWhileFollowing)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {3, 1, 2});
  manager.RoutingSession().SetState(routing::SessionState::RouteNotStarted);
  TEST(manager.RoutingSession().EnableFollowMode(), ());
  TEST(manager.IsRoutingFollowing(), ());
  TEST(!manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 1, 2}), ());
}

UNIT_TEST(RoutingManager_RulerPreservesMeasurementOrder)
{
  for (bool const enabled : {false, true})
  {
    Framework framework(FrameworkParams(false /* m_enableDiffs */));
    auto & manager = framework.GetRoutingManager();
    manager.Init(routing::CreateNumMwmIds(framework.GetStorage()));

    auto const originalRouter = manager.GetLastUsedRouter();
    SCOPE_GUARD(restoreRouter, [&] { manager.SetLastUsedRouter(originalRouter); });
    manager.SetRouter(routing::RouterType::Pedestrian);
    AddTestRoute(manager, {3, 1, 2});
    auto const before = GetStopCoordinates(manager);
    manager.SetRouter(routing::RouterType::Ruler);
    TEST(manager.GetRouter() == routing::RouterType::Ruler, ());
    TEST_EQUAL(GetStopCoordinates(manager), before, ());
    TEST(!manager.OptimizeRoutePoints(), ());
    TEST_EQUAL(GetStopCoordinates(manager), before, ());
    manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 0.5), enabled /* optimize */);
    auto expected = before;
    expected.push_back(0.5);
    TEST_EQUAL(GetStopCoordinates(manager), expected, ());
  }
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsUsesRouteMarks)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {5, 4, 3, 2, 1});
  std::vector<double> const optimized = {1, 2, 3, 4, 5};
  TEST(manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), optimized, ());

  manager.MoveRoutePoint(1, 4);
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{2, 3, 4, 1, 5}), ());
  TEST(manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), optimized, ());
  TEST(!manager.OptimizeRoutePoints(), ());
}

UNIT_TEST(RoutingManager_OptimizationNeverIncreasesRouteLength)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  RouteMarkData start;
  start.m_pointType = RouteMarkType::Start;
  start.m_position = {0.0, 0.0};
  manager.AddRoutePoint(std::move(start), false /* optimize */);

  std::vector<m2::PointD> const stops = {
      {9.226, 1.094}, {-0.569, -2.191}, {-1.687, 1.795}, {-1.871, 4.575}, {7.611, 5.628}};
  for (size_t i = 0; i < stops.size(); ++i)
  {
    RouteMarkData stop;
    stop.m_pointType = RouteMarkType::Intermediate;
    stop.m_intermediateIndex = i;
    stop.m_position = stops[i];
    manager.AddRoutePoint(std::move(stop), false /* optimize */);
  }

  RouteMarkData finish;
  finish.m_pointType = RouteMarkType::Finish;
  finish.m_position = {10.0, 0.0};
  manager.AddRoutePoint(std::move(finish), false /* optimize */);

  double previousLength = GetRouteLengthMeters(manager);
  bool stabilized = false;
  for (size_t i = 0; i < stops.size(); ++i)
  {
    bool const changed = manager.OptimizeRoutePoints();
    double const currentLength = GetRouteLengthMeters(manager);
    TEST_LESS_OR_EQUAL(currentLength, previousLength, (i, currentLength, previousLength));
    if (!changed)
    {
      stabilized = true;
      break;
    }
    previousLength = currentLength;
  }
  TEST(stabilized, ());
}

UNIT_TEST(RoutingManager_ManualEditsAndNextAddPreserveExistingOrder)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {5, 4, 3, 2, 1});
  manager.OptimizeRoutePoints();
  manager.MoveRoutePoint(1, 4);
  manager.ReplaceRoutePoint(RouteMarkType::Intermediate, 1, MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 8.0));
  manager.RemoveRoutePoint(RouteMarkType::Intermediate, 2);
  std::vector<double> const edited = {2, 8, 1, 5};
  TEST_EQUAL(GetStopCoordinates(manager), edited, ());

  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 1.5), true /* optimize */);
  auto existingStops = GetStopCoordinates(manager);
  TEST_EQUAL(std::erase(existingStops, 1.5), 1, ());
  TEST_EQUAL(existingStops, edited, ());
}

UNIT_TEST(RoutingManager_SaveLoadKeepsVisibleOrder)
{
  for (bool const enabled : {false, true})
  {
    WithSavedRoute([enabled](RoutingManager & manager, BookmarkManager &, TestGuiThread & guiThread)
    {
      std::vector<double> const original = {5, 4, 3, 2, 1};
      AddTestRoute(manager, original);
      if (enabled)
        manager.OptimizeRoutePoints();
      manager.MoveRoutePoint(1, 4);
      auto const displayed = GetStopCoordinates(manager);
      manager.SaveRoutePoints();
      WaitForFileThread();
      manager.RemoveRoutePoints();
      TEST(LoadRoutePoints(manager, guiThread), ());
      TEST_EQUAL(GetStopCoordinates(manager), displayed, ());
    });
  }
}

UNIT_TEST(RoutingManager_UpdatingMyPositionKeepsAppendOrder)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {4, 3, 2});
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, -1.0);
  start.m_isMyPosition = true;
  manager.AddRoutePoint(std::move(start), false /* optimize */);
  auto moved = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 6.0);
  moved.m_isMyPosition = true;
  manager.AddRoutePoint(std::move(moved), false /* optimize */);
  TEST_EQUAL(manager.GetRoutePoints().front().m_position.x, 4.0, ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 6}), ());
  auto next = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 7.0);
  manager.AddRoutePoint(std::move(next), false /* optimize */);
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 6, 7}), ());
}

UNIT_TEST(RoutingManager_ReplaceStopWithMyPositionKeepsSlot)
{
  // Only one My Position mark may exist, and dropping the stale one can re-type or renumber the replaced slot.
  struct Case
  {
    RouteMarkType m_myPositionType;
    size_t m_replacedIndex;
    std::vector<double> m_stops;
    double m_start;
    double m_finish;
  };
  for (auto const & c : {Case{RouteMarkType::Start, 1, {5, 3}, 1, 10}, Case{RouteMarkType::Start, 0, {2, 3}, 5, 10},
                         Case{RouteMarkType::Finish, 2, {1, 2}, -1, 5}})
  {
    Framework framework(FrameworkParams(false /* m_enableDiffs */));
    auto & manager = framework.GetRoutingManager();

    AddTestRoute(manager, {1, 2, 3});
    bool const isStart = c.m_myPositionType == RouteMarkType::Start;
    auto endpoint = MakeRoutePointOnXAxis(c.m_myPositionType, 0, isStart ? -1.0 : 10.0);
    endpoint.m_isMyPosition = true;
    manager.AddRoutePoint(std::move(endpoint), false /* optimize */);

    auto replacement = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 5.0);
    replacement.m_isMyPosition = true;
    manager.ReplaceRoutePoint(RouteMarkType::Intermediate, c.m_replacedIndex, std::move(replacement));

    TEST_EQUAL(GetStopCoordinates(manager), c.m_stops, (c.m_replacedIndex));
    auto const points = manager.GetRoutePoints();
    TEST_EQUAL(points.front().m_position.x, c.m_start, (c.m_replacedIndex));
    TEST_EQUAL(points.back().m_position.x, c.m_finish, (c.m_replacedIndex));
    for (size_t i = 1; i + 1 < points.size(); ++i)
      TEST_EQUAL(points[i].m_intermediateIndex, i - 1, (i));
  }
}

UNIT_TEST(RoutingManager_AddAppendsRegardlessOfSuppliedIndex)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {4, 2});
  for (size_t const index : {0, 1, 100})
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, index, index + 10.0), false /* optimize */);
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{4, 2, 10, 11, 110}), ());
  auto const points = manager.GetRoutePoints();
  for (size_t i = 1; i + 1 < points.size(); ++i)
    TEST_EQUAL(points[i].m_intermediateIndex, i - 1, ());
  TEST_EQUAL(points.back().m_position.x, 10.0, ());
}

UNIT_TEST(RoutingManager_NewStopInsertionUsesRouteEndpointsAndPreservesPassedStops)
{
  for (bool const passed : {false, true})
  {
    Framework framework(FrameworkParams(false /* m_enableDiffs */));
    auto & manager = framework.GetRoutingManager();
    auto & bookmarks = framework.GetBookmarkManager();

    AddTestRoute(manager, {2, 8});
    RoutePointsLayout layout(bookmarks);
    auto const * firstStop = layout.GetRoutePoint(RouteMarkType::Intermediate, 0);
    auto const firstStopId = firstStop->GetId();
    if (passed)
      layout.PassRoutePoint(RouteMarkType::Intermediate, 0);

    auto added = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 1.0);
    manager.AddRoutePoint(std::move(added), true /* optimize */);
    auto const expected = passed ? std::vector<double>{2, 1, 8} : std::vector<double>{1, 2, 8};
    TEST_EQUAL(GetStopCoordinates(manager), expected, ());
    TEST_EQUAL(layout.GetRoutePoint(RouteMarkType::Intermediate, passed ? 0 : 1)->GetId(), firstStopId, ());
    TEST_EQUAL(firstStop->IsPassed(), passed, ());
    TEST_EQUAL(firstStop->IsVisible(), !passed, ());
    auto const points = manager.GetRoutePoints();
    for (size_t i = 1; i + 1 < points.size(); ++i)
      TEST_EQUAL(points[i].m_intermediateIndex, i - 1, ());
  }
}

UNIT_TEST(RoutingManager_AddingEndpointDoesNotReorderStops)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  auto start = MakeRoutePointOnXAxis(RouteMarkType::Start, 0, -1.0);
  manager.AddRoutePoint(std::move(start), true /* optimize */);
  for (double const coordinate : {3.0, 2.0, 1.0})
  {
    auto point = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, coordinate);
    manager.AddRoutePoint(std::move(point), true /* optimize */);
  }
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 1}), ());
  auto finish = MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 10.0);
  manager.AddRoutePoint(std::move(finish), true /* optimize */);
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 1}), ());
  TEST(manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3}), ());
}

UNIT_TEST(RoutingManager_ReplacementAtCapacityKeepsSlot)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();

  std::vector<double> stops;
  for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
    stops.push_back(9.0 - i * 0.05);
  AddTestRoute(manager, stops);
  manager.OptimizeRoutePoints();
  auto expected = GetStopCoordinates(manager);
  size_t const index = expected.size() / 2;
  kml::MarkId replacedId = kml::kInvalidMarkId;
  {
    RoutePointsLayout layout(bookmarks);
    auto const * replaced = layout.GetRoutePoint(RouteMarkType::Intermediate, index);
    TEST(replaced != nullptr, ());
    replacedId = replaced->GetId();
  }
  expected[index] = 0.5;
  manager.ReplaceRoutePoint(RouteMarkType::Intermediate, index,
                            MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 0.5));
  TEST_EQUAL(manager.GetRoutePointsCount(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST_EQUAL(GetStopCoordinates(manager), expected, ());
  {
    RoutePointsLayout layout(bookmarks);
    auto const * replaced = layout.GetRoutePoint(RouteMarkType::Intermediate, index);
    TEST(replaced != nullptr, ());
    TEST_EQUAL(replaced->GetId(), replacedId, ());
    TEST_EQUAL(replaced->GetPivot(), (m2::PointD{0.5, 0.0}), ());
  }
}

UNIT_TEST(RoutingManager_ReplacingPassedStopMakesItPending)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();

  AddTestRoute(manager, {1, 2, 3});
  {
    RoutePointsLayout layout(bookmarks);
    layout.PassRoutePoint(RouteMarkType::Intermediate, 0);
    layout.PassRoutePoint(RouteMarkType::Intermediate, 1);
  }

  manager.ReplaceRoutePoint(RouteMarkType::Intermediate, 0,
                            MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 50.0));

  auto const points = manager.GetRoutePoints();
  TEST(!points[1].m_isPassed, ());
  TEST(points[1].m_isVisible, ());
  TEST(points[2].m_isPassed, ());

  // BuildRoute() starts with this cleanup. The new stop must remain to be visited.
  manager.RemovePassedRoutePoints();
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{50, 3}), ());
}

UNIT_TEST(RoutingManager_AddingOptimizedStopAfterPassedStopReplacement)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();

  AddTestRoute(manager, {1, 2, 3});
  {
    RoutePointsLayout layout(bookmarks);
    layout.PassRoutePoint(RouteMarkType::Intermediate, 0);
    layout.PassRoutePoint(RouteMarkType::Intermediate, 1);
  }
  manager.ReplaceRoutePoint(RouteMarkType::Intermediate, 0,
                            MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 50.0));

  TEST(manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 4.0), true /* optimize */), ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{50, 2, 3, 4}), ());
  TEST(manager.GetRoutePoints()[2].m_isPassed, ());

  manager.RemovePassedRoutePoints();
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{50, 3, 4}), ());
}

UNIT_TEST(RoutingManager_ReplaceMissingStopAddsNormally)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {1, 2});
  manager.ReplaceRoutePoint(RouteMarkType::Intermediate, 10,
                            MakeRoutePointOnXAxis(RouteMarkType::Intermediate, 0, 3.0));
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3}), ());
}

UNIT_TEST(RoutingManager_EndpointReplacementDoesNotOptimize)
{
  for (auto const type : {RouteMarkType::Start, RouteMarkType::Finish})
  {
    Framework framework(FrameworkParams(false /* m_enableDiffs */));
    auto & manager = framework.GetRoutingManager();

    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.OptimizeRoutePoints();
    auto replacement = MakeRoutePointOnXAxis(type, 0, type == RouteMarkType::Start ? 20.0 : -10.0);
    manager.AddRoutePoint(std::move(replacement), true /* optimize */);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4, 5}), ());
  }
}

UNIT_TEST(RoutingManager_SaveLoadRoutePointsPreservesOrder)
{
  TestSavedRouteOrder(false /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */,
                      0 /* intermediateCount */);
  TestSavedRouteOrder(false /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(false /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */,
                      RoutePointsLayout::kMaxIntermediatePointsCount);
}

UNIT_TEST(RoutingManager_SaveLoadRoutePointsPreservesRemainingStops)
{
  TestSavedRouteOrder(false /* myPositionStart */, 1 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 1 /* passedStops */, true /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 1 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 3 /* passedStops */, true /* hasCurrentPosition */);
}

UNIT_TEST(RoutingManager_SaveLoadRoutePointsRestoresMyPosition)
{
  TestSavedRouteOrder(true /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 0 /* passedStops */, true /* hasCurrentPosition */);
}

UNIT_TEST(RoutingManager_ContinueRouteToPointAtLimitKeepsFinish)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  FillRouteToLimit(routingManager);

  auto const pointsBefore = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsBefore.size(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsBefore), RoutePointsLayout::kMaxIntermediatePointsCount, ());
  TEST(pointsBefore.back().m_pointType == RouteMarkType::Finish, ());

  auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 102.0);
  TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());

  auto const pointsAfter = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsAfter.size(), pointsBefore.size(), ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsAfter), RoutePointsLayout::kMaxIntermediatePointsCount, ());
  TEST(pointsAfter.back().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(pointsAfter.back().m_position.x, pointsBefore.back().m_position.x, ());
  TEST_EQUAL(pointsAfter.back().m_position.y, pointsBefore.back().m_position.y, ());
}

UNIT_TEST(RoutingManager_AddRoutePointAtLimitReportsFailure)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  FillRouteToLimit(routingManager);

  auto const pointsBefore = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsBefore.size(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST(!routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 100, 102.0), false /* optimize */),
       ());
  auto const pointsAfter = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsAfter.size(), pointsBefore.size(), ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsAfter), GetIntermediatePointsCount(pointsBefore), ());
  TEST_EQUAL(pointsAfter.front().m_position, pointsBefore.front().m_position, ());
  TEST_EQUAL(pointsAfter.back().m_position, pointsBefore.back().m_position, ());
}

// Callers ignore AddRoutePoint()'s result for start and finish because the layout drops the point
// standing in the slot before adding. Pin that: at the limit only a new intermediate point can be rejected.
UNIT_TEST(RoutingManager_ReplaceStartAndFinishAtLimitSucceeds)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  FillRouteToLimit(routingManager);

  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 200.0), false /* optimize */), ());
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 201.0), false /* optimize */), ());

  auto const points = routingManager.GetRoutePoints();
  TEST_EQUAL(points.size(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST_EQUAL(GetIntermediatePointsCount(points), RoutePointsLayout::kMaxIntermediatePointsCount, ());
  TEST(points.front().m_pointType == RouteMarkType::Start, ());
  TEST(points.back().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(points.front().m_position, m2::PointD(200.0, 200.0), ());
  TEST_EQUAL(points.back().m_position, m2::PointD(201.0, 201.0), ());
}

// If route marks are wiped between IsRoutingActive() and ContinueRouteToPoint(),
// the latter must return false without mutating the (empty) layout.
UNIT_TEST(RoutingManager_ContinueRouteToPointWithoutFinishFailsCleanly)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());

  auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 1.0);
  TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());
  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsPreservesPassedStops)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();

  std::vector<double> const intermediateCoordinates = {1.0, 4.0, 2.0, 3.0};
  std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0, 4.0};

  auto start = MakeRoutePointOnXAxis(RouteMarkType::Start, 0, 0.0);
  routingManager.AddRoutePoint(std::move(start), false /* optimize */);
  auto finish = MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 10.0);
  routingManager.AddRoutePoint(std::move(finish), false /* optimize */);
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto point = MakeRoutePointOnXAxis(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
    routingManager.AddRoutePoint(std::move(point), false /* optimize */);
  }

  kml::MarkId passedId;
  {
    RoutePointsLayout layout(bookmarks);
    auto const * passedPoint = layout.GetRoutePoint(RouteMarkType::Intermediate, 0);
    TEST(passedPoint != nullptr, ());
    passedId = passedPoint->GetId();
    layout.PassRoutePoint(RouteMarkType::Intermediate, 0);
  }

  auto const markIds = bookmarks.GetUserMarkIds(UserMark::Type::ROUTING);
  routingManager.OptimizeRoutePoints();

  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::ROUTING).size(), markIds.size(), ());
  for (auto const markId : markIds)
    TEST(bookmarks.GetMark<RouteMarkPoint>(markId) != nullptr, (markId));

  RoutePointsLayout layout(bookmarks);
  auto const points = layout.GetRoutePoints();
  TEST_EQUAL(points.size(), expectedCoordinates.size() + 2, ());
  for (size_t i = 0; i < expectedCoordinates.size(); ++i)
    TEST_EQUAL(points[i + 1]->GetPivot(), m2::PointD(expectedCoordinates[i], 0.0), (i));

  auto const * passedPoint = layout.GetRoutePoint(RouteMarkType::Intermediate, 0);
  TEST(passedPoint != nullptr, ());
  TEST_EQUAL(passedPoint->GetId(), passedId, ());
  TEST(passedPoint->IsPassed(), ());
  TEST(!passedPoint->IsVisible(), ());
}
}  // namespace routing_manager_tests
