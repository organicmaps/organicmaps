#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"
#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"

#include "storage/routing_helpers.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"
#include "base/thread_safe_queue.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <iomanip>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace routing_manager_tests
{
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
}  // namespace

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

// Only drain this fixture's GUI tasks: other Framework fixtures can leave callbacks on the OS queue.
class RoutingTestGuiThread : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    std::lock_guard lock(m_mutex);
    m_tasks.push(std::move(task));
    return {true, kNoId};
  }
  PushResult Push(Task const & task) override { return Push(Task(task)); }
  void Drain()
  {
    for (;;)
    {
      Task task;
      {
        std::lock_guard lock(m_mutex);
        if (m_tasks.empty())
          return;
        task = std::move(m_tasks.front());
        m_tasks.pop();
      }
      task();
    }
  }

private:
  std::mutex m_mutex;
  std::queue<Task> m_tasks;
};

class RoutingTestSettings
{
public:
  RoutingTestSettings() : m_settingsDir(GetPlatform().SettingsDir()), m_hadRouter(settings::Get("router", m_router))
  {
    GetPlatform().SetSettingsDir(m_dir);
    auto gui = std::make_unique<RoutingTestGuiThread>();
    m_gui = gui.get();
    GetPlatform().SetGuiThread(std::move(gui));
  }
  ~RoutingTestSettings()
  {
    if (m_hadRouter)
      settings::Set("router", m_router);
    else
      settings::Delete("router");
    GetPlatform().SetSettingsDir(m_settingsDir);
    GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>());
  }
  void DrainGui() { m_gui->Drain(); }

private:
  RoutingTestGuiThread * m_gui = nullptr;
  std::string const m_dir = GetPlatform().WritablePathForFile("routing_manager_test_settings");
  platform::tests_support::ScopedDirCleanup m_dirCleanup{m_dir};
  std::string m_settingsDir;
  std::string m_router;
  bool m_hadRouter;
};

class RoutingManagerTest
{
public:
  RoutingManagerTest() : m_framework(m_frameworkParams), m_manager(m_framework.GetRoutingManager())
  {
    m_manager.RemoveRoutePoints();
  }

  ~RoutingManagerTest()
  {
    // Drain asynchronous saves before the fixture removes its settings directory.
    std::promise<void> drained;
    GetPlatform().RunTask(Platform::Thread::File, [&drained]() { drained.set_value(); });
    drained.get_future().wait();
    Platform::RemoveFileIfExists(GetPlatform().SettingsPathForFile("route_points.dat"));
  }

protected:
  template <typename Predicate>
  void WaitUntil(Predicate && ready)
  {
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!ready() && std::chrono::steady_clock::now() < deadline)
    {
      m_settings.DrainGui();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    TEST(ready(), ("Asynchronous routing operation timed out"));
  }

  void BuildTestRoute(std::string const & callback = "app://finish")
  {
    m_manager.SetRouter(routing::RouterType::Ruler);
    m_manager.ReplaceRoutePoints({MakePoint(RouteMarkType::Start, 1, 1),
                                  MakePoint(RouteMarkType::Intermediate, 1.001, 1, "app://stop"),
                                  MakePoint(RouteMarkType::Finish, 1.002, 1, callback)});
    bool ready = false;
    m_manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
      ready = true;
    });
    m_manager.BuildRoute();
    WaitUntil([&] { return ready; });
    m_manager.SetRouteBuildingListener([](routing::RouterResultCode, storage::CountriesSet const &) {});
  }

  void PassFinish()
  {
    location::GpsInfo gps;
    gps.m_longitude = 1;
    gps.m_horizontalAccuracy = 1;
    for (int i = 0; i <= 100; ++i)
    {
      gps.m_latitude = 1 + i * 0.00002;
      m_manager.CheckLocationForRouting(gps);
    }
  }

  static RouteMarkData MakePoint(RouteMarkType type, double lat, double lon, std::string callback = {},
                                 size_t intermediateIndex = 0)
  {
    RouteMarkData data;
    data.m_pointType = type;
    data.m_intermediateIndex = intermediateIndex;
    data.m_position = mercator::FromLatLon(lat, lon);
    data.m_callback = std::move(callback);
    return data;
  }

  RoutingTestSettings m_settings;
  FrameworkParams m_frameworkParams;
  Framework m_framework;
  RoutingManager & m_manager;
};

UNIT_CLASS_TEST(RoutingManagerTest, FlushesLatestPendingRoutePointCallback)
{
  BuildTestRoute();
  TEST(m_manager.RoutingSession().EnableFollowMode(), ());
  PassFinish();
  TEST(m_manager.IsRouteFinished(), ());
  m_manager.CloseRouting(true);

  // Only the most recent stop callback is kept while no platform callback is
  // attached, and it is delivered exactly once.
  std::vector<std::string> callbacks;
  m_manager.SetRoutePointCallback([&callbacks](std::string const & callback) { callbacks.push_back(callback); });

  TEST_EQUAL(callbacks.size(), 1, ());
  TEST_EQUAL(callbacks[0], "app://finish", ());

  callbacks.clear();
  m_manager.SetRoutePointCallback([&callbacks](std::string const & callback) { callbacks.push_back(callback); });
  TEST(callbacks.empty(), ());
}

UNIT_CLASS_TEST(RoutingManagerTest, PreviewDoesNotOpenCallbacks)
{
  BuildTestRoute();
  size_t callbacks = 0;
  m_manager.SetRoutePointCallback([&](std::string const &) { ++callbacks; });
  PassFinish();
  m_settings.DrainGui();
  TEST_EQUAL(callbacks, 0, ());
}

UNIT_CLASS_TEST(RoutingManagerTest, CheckpointBelongsToTheRouteThatPassedIt)
{
  BuildTestRoute();
  TEST(m_manager.RoutingSession().EnableFollowMode(), ());
  std::vector<std::string> callbacks;
  m_manager.SetRoutePointCallback([&](std::string const & callback) { callbacks.push_back(callback); });
  PassFinish();
  TEST_EQUAL(callbacks.size(), 2, ());
  TEST_EQUAL(callbacks.back(), "app://finish", ());
  m_manager.ReplaceRoutePoints(
      {MakePoint(RouteMarkType::Start, 3, 3), MakePoint(RouteMarkType::Finish, 4, 4, "app://new")});
  m_settings.DrainGui();
  TEST_EQUAL(callbacks.size(), 2, ());
  TEST(!m_manager.GetRoutePoints().back().m_isPassed, ());
}

UNIT_CLASS_TEST(RoutingManagerTest, ReplacementDiscardsPendingCallbacks)
{
  BuildTestRoute();
  TEST(m_manager.RoutingSession().EnableFollowMode(), ());
  PassFinish();
  m_manager.ReplaceRoutePoints({MakePoint(RouteMarkType::Start, 3, 3), MakePoint(RouteMarkType::Finish, 4, 4)});
  m_manager.SetRoutePointCallback([](std::string const &) { TEST(false, ("Callback belongs to a replaced route")); });
}

UNIT_CLASS_TEST(RoutingManagerTest, RestorePreservesOrderAndCallbacks)
{
  m_manager.ReplaceRoutePoints(
      {MakePoint(RouteMarkType::Start, 1, 1), MakePoint(RouteMarkType::Intermediate, 1.001, 1, "A"),
       MakePoint(RouteMarkType::Intermediate, 1.002, 1, "B"), MakePoint(RouteMarkType::Intermediate, 1.003, 1, "C"),
       MakePoint(RouteMarkType::Finish, 1.004, 1, "finish")});
  auto const before = m_manager.GetRoutePoints();
  m_manager.SaveRoutePoints();
  std::promise<void> saved;
  GetPlatform().RunTask(Platform::Thread::File, [&] { saved.set_value(); });
  saved.get_future().wait();
  m_manager.RemoveRoutePoints();
  bool loaded = false;
  m_manager.LoadRoutePoints([&](bool success)
  {
    TEST(success, ());
    loaded = true;
  });
  WaitUntil([&] { return loaded; });
  auto const after = m_manager.GetRoutePoints();
  TEST_EQUAL(after.size(), before.size(), ());
  for (size_t i = 0; i < after.size(); ++i)
  {
    TEST_EQUAL(after[i].m_position, before[i].m_position, (i));
    TEST_EQUAL(after[i].m_callback, before[i].m_callback, (i));
    TEST_EQUAL(after[i].m_intermediateIndex, before[i].m_intermediateIndex, (i));
  }
}

UNIT_CLASS_TEST(RoutingManagerTest, OptimizedBatchKeepsPointMetadataTogether)
{
  m_manager.ReplaceRoutePoints(
      {MakePoint(RouteMarkType::Start, 1, 1), MakePoint(RouteMarkType::Intermediate, 1.003, 1, "C"),
       MakePoint(RouteMarkType::Intermediate, 1.001, 1, "A"), MakePoint(RouteMarkType::Intermediate, 1.002, 1, "B"),
       MakePoint(RouteMarkType::Finish, 1.004, 1, "finish")},
      true);
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points[1].m_callback, "A", ());
  TEST_EQUAL(points[2].m_callback, "B", ());
  TEST_EQUAL(points[3].m_callback, "C", ());
  TEST_EQUAL(points.back().m_callback, "finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, LoadingSavedPointsDoesNotReplaceANewItinerary)
{
  m_manager.ReplaceRoutePoints({MakePoint(RouteMarkType::Start, 1, 1), MakePoint(RouteMarkType::Finish, 2, 2)});
  m_manager.SaveRoutePoints();
  std::promise<void> saved;
  GetPlatform().RunTask(Platform::Thread::File, [&] { saved.set_value(); });
  saved.get_future().wait();
  m_manager.RemoveRoutePoints();

  bool done = false;
  m_manager.LoadRoutePoints([&](bool success)
  {
    TEST(!success, ());
    done = true;
  });
  m_manager.ReplaceRoutePoints({MakePoint(RouteMarkType::Start, 3, 3), MakePoint(RouteMarkType::Finish, 4, 4)});
  WaitUntil([&] { return done; });
  TEST_EQUAL(m_manager.GetRoutePoints().back().m_position, mercator::FromLatLon(4, 4), ());
}

UNIT_CLASS_TEST(RoutingManagerTest, PendingCallbackCanDetachItsListener)
{
  BuildTestRoute();
  TEST(m_manager.RoutingSession().EnableFollowMode(), ());
  PassFinish();
  size_t calls = 0;
  m_manager.SetRoutePointCallback([&](std::string const & callback)
  {
    m_manager.SetRoutePointCallback({});
    TEST_EQUAL(callback, "app://finish", ());
    ++calls;
  });
  TEST_EQUAL(calls, 1, ());
  m_manager.SetRoutePointCallback([&](std::string const &) { ++calls; });
  TEST_EQUAL(calls, 1, ());
}

UNIT_CLASS_TEST(RoutingManagerTest, ExecutesApiRouteRequest)
{
  url_scheme::ParsedMapApi const api(
      "om://v2/dir?destination=3,3&destination_name=Finish&destination_callback=app%3A%2F%2Ffinish"
      "&waypoints=1.5,1.5|2,2&waypoint_names=A|B&waypoint_callbacks=app%3A%2F%2F1|app%3A%2F%2F2&mode=walk");
  TEST_EQUAL(api.GetRequestType(), url_scheme::ParsedMapApi::UrlType::Route, ());

  std::vector<routing::RouterResultCode> buildResults;
  m_manager.SetRouteBuildingListener([&buildResults](routing::RouterResultCode code, storage::CountriesSet const &)
  { buildResults.push_back(code); });

  api.ExecuteRouteApiRequest(m_framework);

  // The itinerary is materialized in URL order and the build starts right away; with no
  // known position for the implicit my-position start it fails with NoCurrentPosition.
  TEST_EQUAL(buildResults.size(), 1, ());
  TEST_EQUAL(static_cast<int>(buildResults[0]), static_cast<int>(routing::RouterResultCode::NoCurrentPosition), ());

  auto const routePoints = m_manager.GetRoutePoints();
  TEST_EQUAL(routePoints.size(), 4, ());
  TEST_EQUAL(static_cast<int>(routePoints[0].m_pointType), static_cast<int>(RouteMarkType::Start), ());
  TEST(routePoints[0].m_isMyPosition, ());
  // The desktop test platform's GetLocalizedString() returns the key itself.
  TEST_EQUAL(routePoints[0].m_title, "core_my_position", ());
  TEST_EQUAL(routePoints[1].m_title, "A", ());
  TEST_EQUAL(routePoints[1].m_callback, "app://1", ());
  TEST_EQUAL(routePoints[1].m_intermediateIndex, 0, ());
  TEST_EQUAL(routePoints[2].m_title, "B", ());
  TEST_EQUAL(routePoints[2].m_callback, "app://2", ());
  TEST_EQUAL(routePoints[2].m_intermediateIndex, 1, ());
  TEST_EQUAL(static_cast<int>(routePoints[3].m_pointType), static_cast<int>(RouteMarkType::Finish), ());
  TEST_EQUAL(routePoints[3].m_title, "Finish", ());
  TEST_EQUAL(routePoints[3].m_callback, "app://finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, DoesNotMaterializeOriginCallbackOnStartMark)
{
  // origin_callback is reserved: the start is departed from, never "passed", so it must
  // never become an executable callback on the Start mark, even though the parser keeps it
  // on the origin RoutePoint.
  url_scheme::ParsedMapApi const api(
      "om://v2/dir?origin=currentLocation&origin_callback=app%3A%2F%2Forigin"
      "&destination=2,2&destination_callback=app%3A%2F%2Ffinish");
  TEST_EQUAL(api.GetRequestType(), url_scheme::ParsedMapApi::UrlType::Route, ());
  TEST_EQUAL(api.GetRoutePoints().front().m_callback, "app://origin", ());

  // ExecuteRouteApiRequest starts a build, which reports completion through this listener.
  m_manager.SetRouteBuildingListener([](routing::RouterResultCode, storage::CountriesSet const &) {});
  api.ExecuteRouteApiRequest(m_framework);

  auto const routePoints = m_manager.GetRoutePoints();
  TEST_EQUAL(routePoints.size(), 2, ());
  TEST_EQUAL(static_cast<int>(routePoints[0].m_pointType), static_cast<int>(RouteMarkType::Start), ());
  TEST_EQUAL(routePoints[0].m_callback, "", ());
  TEST_EQUAL(routePoints[1].m_callback, "app://finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, LoadsRoutePointsWithoutCallbackField)
{
  auto const start = mercator::FromLatLon(1.0, 1.0);
  auto const finish = mercator::FromLatLon(2.0, 2.0);

  std::ostringstream data;
  data << std::setprecision(17);
  data << "["
       << "{\"type\":0,\"title\":\"Start\",\"subtitle\":\"\",\"x\":" << start.x << ",\"y\":" << start.y
       << ",\"replaceWithMyPosition\":false},"
       << "{\"type\":2,\"title\":\"Finish\",\"subtitle\":\"\",\"x\":" << finish.x << ",\"y\":" << finish.y
       << ",\"replaceWithMyPosition\":false}"
       << "]";

  auto const points = RoutingManager::DeserializeRoutePointsForTesting(data.str());
  TEST_EQUAL(points.size(), 2, ());
  TEST_EQUAL(points[0].m_title, "Start", ());
  TEST_EQUAL(points[0].m_callback, "", ());
  TEST_EQUAL(points[0].m_position, start, ());
  TEST_EQUAL(points[1].m_title, "Finish", ());
  TEST_EQUAL(points[1].m_callback, "", ());
  TEST_EQUAL(points[1].m_position, finish, ());
}

UNIT_CLASS_TEST(RoutingManagerTest, DoesNotSaveCallbackForPassedRoutePoint)
{
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1.0, 1.0), false /* optimize */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.5, 1.5, "app://passed", 0), false /* optimize */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.7, 1.7, "app://next", 1), false /* optimize */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 2.0, 2.0, "app://finish"), false /* optimize */);

  m_manager.SetRoutePointCallback([](std::string const &) {});
  m_manager.OnRoutePointPassed(RouteMarkType::Intermediate, 0);

  auto const points = m_manager.GetRoutePointsToSaveForTesting();
  TEST_EQUAL(points.size(), 3, ());
  TEST_EQUAL(static_cast<int>(points[0].m_pointType), static_cast<int>(RouteMarkType::Start), ());
  TEST_EQUAL(points[0].m_callback, "", ());
  TEST_EQUAL(static_cast<int>(points[1].m_pointType), static_cast<int>(RouteMarkType::Intermediate), ());
  TEST_EQUAL(points[1].m_callback, "app://next", ());
  TEST_EQUAL(static_cast<int>(points[2].m_pointType), static_cast<int>(RouteMarkType::Finish), ());
  TEST_EQUAL(points[2].m_callback, "app://finish", ());
}
}  // namespace routing_manager_tests
