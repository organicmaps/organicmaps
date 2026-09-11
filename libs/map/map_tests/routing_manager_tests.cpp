#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "coding/file_reader.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"
#include "base/task_loop.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace routing_manager_tests
{
namespace
{
class TestGuiThread final : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override { return PushImpl(std::move(task)); }
  PushResult Push(Task const & task) override { return PushImpl(task); }

  void RunNext()
  {
    Task task;
    {
      std::unique_lock lock(m_mutex);
      m_condition.wait(lock, [this] { return !m_tasks.empty(); });
      task = std::move(m_tasks.front());
      m_tasks.pop_front();
    }
    task();
  }

private:
  PushResult PushImpl(Task task)
  {
    {
      std::lock_guard lock(m_mutex);
      m_tasks.push_back(std::move(task));
    }
    m_condition.notify_one();
    return {true, kNoId};
  }

  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::deque<Task> m_tasks;
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
void WithTestRoutingManager(Fn && fn)
{
  auto & platform = GetPlatform();
  auto const testSettingsDir = base::JoinPath(platform.WritableDir(), "routing_manager_save_load_test");
  platform::tests_support::ScopedDirCleanup settingsDir(testSettingsDir);

  auto testGuiThread = std::make_unique<TestGuiThread>();
  auto * guiThread = testGuiThread.get();
  platform.SetGuiThread(std::move(testGuiThread));
  SCOPE_GUARD(restoreGuiThread, [&platform] { platform.SetGuiThread(std::make_unique<platform::GuiThread>()); });

  bool const optimizationEnabled = routing::RoutingOptions::LoadRouteOptimizationFromSettings();
  routing::RoutingOptions::SaveRouteOptimizationToSettings(false);
  SCOPE_GUARD(restoreOptimization,
              [optimizationEnabled] { routing::RoutingOptions::SaveRouteOptimizationToSettings(optimizationEnabled); });

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

size_t GetIntermediatePointsCount(std::vector<RouteMarkData> const & points)
{
  return std::count_if(points.begin(), points.end(),
                       [](RouteMarkData const & d) { return d.m_pointType == RouteMarkType::Intermediate; });
}

void TestSavedRouteOrder(bool myPositionStart, size_t passedStops, bool hasCurrentPosition,
                         size_t intermediateCount = 3)
{
  WithTestRoutingManager([=](RoutingManager & routingManager, BookmarkManager & bookmarks, TestGuiThread & guiThread)
  {
    auto start = MakeRoutePoint(RouteMarkType::Start, 0, 1.0);
    start.m_title = "Start";
    start.m_subTitle = "Start subtitle";
    start.m_isMyPosition = myPositionStart;
    bookmarks.MyPositionMark().SetUserPosition(start.m_position, true /* hasPosition */);
    routingManager.AddRoutePoint(std::move(start));
    for (size_t i = 0; i < intermediateCount; ++i)
    {
      // Descending coordinates ensure that loading does not optimize the saved order.
      auto const coordinate = static_cast<double>(intermediateCount - i + 1);
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, coordinate);
      point.m_title = "Stop " + std::to_string(i);
      point.m_subTitle = "Stop subtitle " + std::to_string(i);
      point.m_isPassed = i < passedStops;
      routingManager.AddRoutePoint(std::move(point));
    }
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 110.0);
    finish.m_title = "Finish";
    finish.m_subTitle = "Finish subtitle";
    routingManager.AddRoutePoint(std::move(finish));

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
  // Set up a manually ordered route without invoking automatic optimization.
  bool const enabled = routing::RoutingOptions::LoadRouteOptimizationFromSettings();
  routing::RoutingOptions::SaveRouteOptimizationToSettings(false);
  SCOPE_GUARD(restoreOptimization, [enabled] { routing::RoutingOptions::SaveRouteOptimizationToSettings(enabled); });
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, -1.0);
  start.m_position.y = 0.0;
  manager.AddRoutePoint(std::move(start));
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
  finish.m_position.y = 0.0;
  manager.AddRoutePoint(std::move(finish));
  for (size_t i = 0; i < stops.size(); ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, stops[i]);
    point.m_position.y = 0.0;
    manager.AddRoutePoint(std::move(point));
  }
}

std::vector<double> GetStopCoordinates(RoutingManager const & manager)
{
  std::vector<double> result;
  for (auto const & point : manager.GetRoutePoints())
    if (point.m_pointType == RouteMarkType::Intermediate)
      result.push_back(point.m_position.x);
  return result;
}
}  // namespace

UNIT_TEST(RoutingManager_OptimizationToggleRestoresUserOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    TEST(!routing::RoutingOptions::LoadRouteOptimizationFromSettings(), ());
    std::vector<double> const original = {5, 4, 3, 2, 1};
    AddTestRoute(manager, original);
    for (size_t i = 0; i < 2; ++i)
    {
      TEST(manager.SetRouteOptimizationEnabled(true), ());
      TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4, 5}), ());
      TEST(manager.SetRouteOptimizationEnabled(false), ());
      TEST_EQUAL(GetStopCoordinates(manager), original, ());
    }
    manager.MoveRoutePoint(1, 3);
    auto const manuallyReordered = GetStopCoordinates(manager);
    manager.SetRouteOptimizationEnabled(true);
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), manuallyReordered, ());
  });
}

UNIT_TEST(RoutingManager_ManualReorderKeepsSettingAndNextAddPreservesExistingOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    manager.MoveRoutePoint(1, 4);
    TEST(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), ());
    std::vector<double> const reordered = {2, 3, 4, 1, 5};
    TEST_EQUAL(GetStopCoordinates(manager), reordered, ());
    TEST(!manager.SetRouteOptimizationEnabled(true), ());
    TEST_EQUAL(GetStopCoordinates(manager), reordered, ());
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    manager.SetRouteOptimizationEnabled(true);
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), reordered, ());

    manager.SetRouteOptimizationEnabled(true);
    manager.MoveRoutePoint(1, 4);
    auto added = MakeRoutePoint(RouteMarkType::Intermediate, 0, 1.5);
    added.m_position.y = 0.0;
    manager.AddRoutePoint(std::move(added));
    auto const afterAdd = GetStopCoordinates(manager);
    auto existingStops = afterAdd;
    TEST_EQUAL(std::erase(existingStops, 1.5), 1, ());
    TEST_EQUAL(existingStops, reordered, ());
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    TEST_EQUAL(GetStopCoordinates(manager), afterAdd, ());
    manager.SetRouteOptimizationEnabled(true);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 1.5, 2, 3, 4, 5}), ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), afterAdd, ());
  });
}

UNIT_TEST(RoutingManager_SwapUsingMovesPreservesManualOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.SetRouteOptimizationEnabled(true);
    for (size_t from = 0; from < 7; ++from)
    {
      for (size_t to = 0; to < 7; ++to)
      {
        if (from == to)
          continue;
        manager.RemoveRoutePoints();
        AddTestRoute(manager, {5, 4, 3, 2, 1});
        auto expected = manager.GetRoutePoints();
        std::swap(expected[from], expected[to]);
        manager.MoveRoutePoint(from, to);
        if (from + 1 < to || to + 1 < from)
          manager.MoveRoutePoint(from < to ? to - 1 : to + 1, from);
        auto const actual = manager.GetRoutePoints();
        TEST_EQUAL(actual.size(), expected.size(), (from, to));
        for (size_t i = 0; i < actual.size(); ++i)
          TEST_EQUAL(actual[i].m_position, expected[i].m_position, (from, to, i));
        TEST(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), ());
      }
    }
  });
}

UNIT_TEST(RoutingManager_ReplacementKeepsUserOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    auto replacement = MakeRoutePoint(RouteMarkType::Intermediate, 2, 8.0);
    replacement.m_position.y = 0.0;
    manager.ReplaceRoutePoint(RouteMarkType::Intermediate, 2, std::move(replacement));
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 8, 4, 5}), ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 8, 4, 5}), ());
  });
}

UNIT_TEST(RoutingManager_SaveLoadKeepsVisibleOrderAndCurrentSetting)
{
  for (bool const enabled : {false, true})
  {
    WithTestRoutingManager([enabled](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
    {
      std::vector<double> const original = {5, 4, 3, 2, 1};
      AddTestRoute(manager, original);
      manager.SetRouteOptimizationEnabled(enabled);
      auto const displayed = GetStopCoordinates(manager);
      manager.SaveRoutePoints();
      WaitForFileThread();
      std::string saved;
      FileReader(GetPlatform().SettingsPathForFile("route_points.dat")).ReadAsString(saved);
      TEST(!saved.empty() && saved.front() == '[', (saved));
      TEST_EQUAL(saved.find("optimizationEnabled"), std::string::npos, (saved));
      manager.SetRouteOptimizationEnabled(!enabled);
      manager.RemoveRoutePoints();
      TEST(LoadRoutePoints(manager, gui), ());
      TEST_EQUAL(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), !enabled, ());
      TEST_EQUAL(GetStopCoordinates(manager), displayed, ());
      manager.SetRouteOptimizationEnabled(false);
      TEST_EQUAL(GetStopCoordinates(manager), displayed, ());
    });
  }
}

UNIT_TEST(RoutingManager_SaveLoadRestoresRemainingVisibleOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread & gui)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    {
      RoutePointsLayout layout(bookmarks);
      layout.PassRoutePoint(RouteMarkType::Intermediate, 0);
      layout.PassRoutePoint(RouteMarkType::Intermediate, 1);
    }
    manager.SaveRoutePoints();
    manager.RemoveRoutePoints();
    TEST(LoadRoutePoints(manager, gui), ());
    TEST_EQUAL(manager.GetRoutePoints().front().m_position.x, 2.0, ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 4, 5}), ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 4, 5}), ());
  });
}

UNIT_TEST(RoutingManager_CancelTransactionRestoresOrderAndOptimizationUndo)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    auto const transaction = manager.OpenRoutePointsTransaction();
    manager.MoveRoutePoint(1, 4);
    manager.CancelRoutePointsTransaction(transaction);
    TEST(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4, 5}), ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{5, 4, 3, 2, 1}), ());
  });
}

UNIT_TEST(RoutingManager_CancelTransactionKeepsCurrentOptimizationSetting)
{
  for (bool const enabled : {false, true})
  {
    WithTestRoutingManager([enabled](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
    {
      AddTestRoute(manager, {5, 4, 3, 2, 1});
      manager.SetRouteOptimizationEnabled(!enabled);
      auto const original = GetStopCoordinates(manager);
      auto const transaction = manager.OpenRoutePointsTransaction();
      manager.MoveRoutePoint(1, 4);
      manager.SetRouteOptimizationEnabled(enabled);
      manager.CancelRoutePointsTransaction(transaction);
      TEST_EQUAL(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), enabled, ());
      TEST_EQUAL(GetStopCoordinates(manager), original, ());
      TEST(!manager.SetRouteOptimizationEnabled(enabled), ());
      TEST_EQUAL(GetStopCoordinates(manager), original, ());
    });
  }
}

UNIT_TEST(RoutingManager_UserOrderAfterRemovingAndContinuing)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    manager.RemoveRoutePoint(RouteMarkType::Intermediate, 1);
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 20.0);
    finish.m_position.y = 0.0;
    TEST(manager.ContinueRouteToPoint(std::move(finish)), ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 3, 4, 5, 10}), ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 3, 4, 5, 10}), ());
    TEST_EQUAL(manager.GetRoutePoints().back().m_position.x, 20.0, ());
  });
}

UNIT_TEST(RoutingManager_UpdatingMyPositionKeepsAppendOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {4, 3, 2});
    auto start = MakeRoutePoint(RouteMarkType::Start, 0, -1.0);
    start.m_isMyPosition = true;
    manager.AddRoutePoint(std::move(start));
    auto moved = MakeRoutePoint(RouteMarkType::Intermediate, 0, 6.0);
    moved.m_position.y = 0.0;
    moved.m_isMyPosition = true;
    manager.AddRoutePoint(std::move(moved));
    TEST_EQUAL(manager.GetRoutePoints().front().m_position.x, 4.0, ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 6}), ());
    auto next = MakeRoutePoint(RouteMarkType::Intermediate, 0, 7.0);
    next.m_position.y = 0.0;
    manager.AddRoutePoint(std::move(next));
    manager.SetRouteOptimizationEnabled(true);
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 6, 7}), ());
  });
}

UNIT_TEST(RoutingManager_AddAppendsRegardlessOfSuppliedIndex)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {4, 2});
    for (size_t const index : {0, 1, 100})
      manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, index, index + 10.0));
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{4, 2, 10, 11, 110}), ());
    auto const points = manager.GetRoutePoints();
    for (size_t i = 1; i + 1 < points.size(); ++i)
      TEST_EQUAL(points[i].m_intermediateIndex, i - 1, ());
    TEST_EQUAL(points.back().m_position.x, 10.0, ());
  });
}

UNIT_TEST(RoutingManager_InitiallyEnabledOptimizationHasNoUndoOrder)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.SetRouteOptimizationEnabled(true);
    AddTestRoute(manager, {});
    for (double const coordinate : {4.0, 2.0, 3.0, 1.0})
    {
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, 0, coordinate);
      point.m_position.y = 0.0;
      manager.AddRoutePoint(std::move(point));
    }
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4}), ());
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4}), ());
  });
}

UNIT_TEST(RoutingManager_NewStopInsertionUsesRouteEndpointsAndPreservesPassedStops)
{
  for (bool const passed : {false, true})
  {
    WithTestRoutingManager([passed](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
    {
      manager.SetRouteOptimizationEnabled(true);
      AddTestRoute(manager, {2, 8});
      RoutePointsLayout layout(bookmarks);
      auto const * firstStop = layout.GetRoutePoint(RouteMarkType::Intermediate, 0);
      auto const firstStopId = firstStop->GetId();
      if (passed)
        layout.PassRoutePoint(RouteMarkType::Intermediate, 0);

      auto added = MakeRoutePoint(RouteMarkType::Intermediate, 0, 1.0);
      added.m_position.y = 0.0;
      manager.AddRoutePoint(std::move(added));
      auto const expected = passed ? std::vector<double>{2, 1, 8} : std::vector<double>{1, 2, 8};
      TEST_EQUAL(GetStopCoordinates(manager), expected, ());
      TEST_EQUAL(layout.GetRoutePoint(RouteMarkType::Intermediate, passed ? 0 : 1)->GetId(), firstStopId, ());
      TEST_EQUAL(firstStop->IsPassed(), passed, ());
      TEST_EQUAL(firstStop->IsVisible(), !passed, ());
      auto const points = manager.GetRoutePoints();
      for (size_t i = 1; i + 1 < points.size(); ++i)
        TEST_EQUAL(points[i].m_intermediateIndex, i - 1, ());
    });
  }
}

UNIT_TEST(RoutingManager_AddingEndpointDoesNotReorderStops)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.SetRouteOptimizationEnabled(true);
    auto start = MakeRoutePoint(RouteMarkType::Start, 0, -1.0);
    start.m_position.y = 0.0;
    manager.AddRoutePoint(std::move(start));
    for (double const coordinate : {3.0, 2.0, 1.0})
    {
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, 0, coordinate);
      point.m_position.y = 0.0;
      manager.AddRoutePoint(std::move(point));
    }
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 1}), ());
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
    finish.m_position.y = 0.0;
    manager.AddRoutePoint(std::move(finish));
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 2, 1}), ());
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    manager.SetRouteOptimizationEnabled(true);
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3}), ());
  });
}

UNIT_TEST(RoutingManager_RejectedAdditionKeepsOptimizationUndo)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    std::vector<double> stops;
    for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
      stops.push_back(9.0 - i * 0.05);
    AddTestRoute(manager, stops);
    manager.SetRouteOptimizationEnabled(true);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 0, 7.0));
    TEST_EQUAL(manager.GetRoutePointsCount(), RoutePointsLayout::kMaxRoutePointsCount, ());
    manager.SetRouteOptimizationEnabled(false);
    TEST_EQUAL(GetStopCoordinates(manager), stops, ());
  });
}

UNIT_TEST(RoutingManager_DeletionInvalidatesOptimizationUndo)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    manager.RemoveRoutePoint(RouteMarkType::Intermediate, 1);
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 3, 4, 5}), ());
  });
}

UNIT_TEST(RoutingManager_EndpointReplacementDoesNotOptimize)
{
  for (auto const type : {RouteMarkType::Start, RouteMarkType::Finish})
  {
    WithTestRoutingManager([type](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
    {
      AddTestRoute(manager, {5, 4, 3, 2, 1});
      manager.SetRouteOptimizationEnabled(true);
      auto replacement = MakeRoutePoint(type, 0, type == RouteMarkType::Start ? 20.0 : -10.0);
      replacement.m_position.y = 0.0;
      manager.AddRoutePoint(std::move(replacement));
      TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4, 5}), ());
      TEST(!manager.SetRouteOptimizationEnabled(false), ());
      TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{1, 2, 3, 4, 5}), ());
    });
  }
}

UNIT_TEST(RoutingManager_ContinueRoutePreservesStopOrder)
{
  for (bool const enabled : {false, true})
  {
    WithTestRoutingManager([enabled](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
    {
      manager.SetRouteOptimizationEnabled(enabled);
      AddTestRoute(manager, {5, 4, 3, 2, 1});
      auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 20.0);
      finish.m_position.y = 0.0;
      TEST(manager.ContinueRouteToPoint(std::move(finish)), ());
      std::vector<double> const expected = {5, 4, 3, 2, 1, 10};
      TEST_EQUAL(GetStopCoordinates(manager), expected, ());
      TEST_EQUAL(manager.GetRoutePoints().back().m_position.x, 20.0, ());
      TEST(!manager.SetRouteOptimizationEnabled(false), ());
      TEST_EQUAL(GetStopCoordinates(manager), expected, ());
    });
  }
}

UNIT_TEST(RoutingManager_SaveLoadPreservesManualOrderWithOptimizationEnabled)
{
  WithTestRoutingManager([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    AddTestRoute(manager, {5, 4, 3, 2, 1});
    manager.SetRouteOptimizationEnabled(true);
    manager.MoveRoutePoint(1, 4);
    auto const approved = GetStopCoordinates(manager);
    manager.SaveRoutePoints();
    manager.RemoveRoutePoints();
    TEST(LoadRoutePoints(manager, gui), ());
    TEST(routing::RoutingOptions::LoadRouteOptimizationFromSettings(), ());
    TEST_EQUAL(GetStopCoordinates(manager), approved, ());
    TEST(!manager.SetRouteOptimizationEnabled(false), ());
    TEST_EQUAL(GetStopCoordinates(manager), approved, ());
  });
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

UNIT_TEST(RoutingManager_SaveRoutePointsRemovesUnusableSavedRoute)
{
  WithTestRoutingManager([](RoutingManager & routingManager, BookmarkManager &, TestGuiThread & guiThread)
  {
    TEST(!LoadRoutePoints(routingManager, guiThread), ());

    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1.0));
    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2.0));
    routingManager.SaveRoutePoints();
    WaitForFileThread();
    TEST(routingManager.HasSavedRoutePoints(), ());

    routingManager.RemoveRoutePoint(RouteMarkType::Finish);
    routingManager.SaveRoutePoints();
    WaitForFileThread();
    TEST(!routingManager.HasSavedRoutePoints(), ());

    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2.0));
    routingManager.SaveRoutePoints();
    WaitForFileThread();
    TEST(routingManager.HasSavedRoutePoints(), ());

    routingManager.OnRoutePointPassed(RouteMarkType::Finish, 0);
    WaitForFileThread();
    TEST(!routingManager.HasSavedRoutePoints(), ());
  });
}

UNIT_TEST(RoutingManager_ContinueRouteToPointAtLimitKeepsFinish)
{
  WithTestRoutingManager([](RoutingManager & routingManager, BookmarkManager &, TestGuiThread &)
  {
    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0));
    for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
      routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 1)));
    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 101.0));

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
  });
}

// If route marks are wiped between IsRoutingActive() and ContinueRouteToPoint(),
// the latter must return false without mutating the (empty) layout.
UNIT_TEST(RoutingManager_ContinueRouteToPointWithoutFinishFailsCleanly)
{
  WithTestRoutingManager([](RoutingManager & routingManager, BookmarkManager &, TestGuiThread &)
  {
    TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());

    auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 1.0);
    TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());
    TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());
  });
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsUsesRouteMarks)
{
  WithTestRoutingManager([](RoutingManager & routingManager, BookmarkManager &, TestGuiThread &)
  {
    std::vector<double> const intermediateCoordinates = {3.0, 1.0, 2.0};
    std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0};

    auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
    start.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(start));
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
    finish.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(finish));
    for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
    {
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
      point.m_position.y = 0.0;
      routingManager.AddRoutePoint(std::move(point));
    }

    routingManager.OptimizeRoutePoints();
    auto const actualPoints = routingManager.GetRoutePoints();

    TEST_EQUAL(actualPoints.size(), expectedCoordinates.size() + 2, ());
    for (size_t i = 0; i < expectedCoordinates.size(); ++i)
      TEST_EQUAL(actualPoints[i + 1].m_position, m2::PointD(expectedCoordinates[i], 0.0), (i));
  });
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsPreservesPassedStops)
{
  WithTestRoutingManager([](RoutingManager & routingManager, BookmarkManager & bookmarks, TestGuiThread &)
  {
    std::vector<double> const intermediateCoordinates = {1.0, 4.0, 2.0, 3.0};
    std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0, 4.0};

    auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
    start.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(start));
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
    finish.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(finish));
    for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
    {
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
      point.m_position.y = 0.0;
      routingManager.AddRoutePoint(std::move(point));
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
  });
}
}  // namespace routing_manager_tests
