#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "geometry/mercator.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/settings.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace routing_manager_tests
{
namespace
{
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
}  // namespace

UNIT_TEST(RoutingManager_ContinueRouteToPointAtLimitKeepsFinish)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0));
  for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 1)),
                                 false /* reorderIntermediatePoints */);
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

// SetSettingsDir redirects route files, but settings:: retains its process-wide storage.
// Restore the router setting separately so the fixture does not change another test's routing mode.
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
  RoutingManagerTest() : m_framework(m_frameworkParams), m_manager(m_framework.GetRoutingManager()) {}

  ~RoutingManagerTest() { WaitForFileTasks(); }

protected:
  static void WaitForFileTasks()
  {
    std::promise<void> drained;
    GetPlatform().RunTask(Platform::Thread::File, [&] { drained.set_value(); });
    drained.get_future().wait();
  }

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

  static RouteMarkData MakePoint(RouteMarkType type, double lat, std::string title, size_t index = 0)
  {
    RouteMarkData point;
    point.m_pointType = type;
    point.m_intermediateIndex = index;
    point.m_position = mercator::FromLatLon(lat, 1);
    point.m_title = std::move(title);
    point.m_subTitle = "subtitle:" + point.m_title;
    return point;
  }

  void AddTestPoints()
  {
    m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1, "start"), false);
    m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.001, "A", 0), false);
    m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.002, "B", 1), false);
    m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.003, "C", 2), false);
    m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 1.004, "finish"), false);
  }

  void SavePoints()
  {
    m_manager.SaveRoutePoints();
    WaitForFileTasks();
  }

  RoutingTestSettings m_settings;
  FrameworkParams m_frameworkParams;
  Framework m_framework;
  RoutingManager & m_manager;
};

UNIT_CLASS_TEST(RoutingManagerTest, RestorePreservesOrderAndMetadata)
{
  AddTestPoints();
  auto const before = m_manager.GetRoutePoints();
  SavePoints();
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
    TEST_EQUAL(after[i].m_title, before[i].m_title, (i));
    TEST_EQUAL(after[i].m_subTitle, before[i].m_subTitle, (i));
    TEST_EQUAL(after[i].m_intermediateIndex, before[i].m_intermediateIndex, (i));
  }
}

UNIT_CLASS_TEST(RoutingManagerTest, LoadingSavedPointsDoesNotReplaceANewItinerary)
{
  AddTestPoints();
  SavePoints();
  m_manager.RemoveRoutePoints();
  bool done = false;
  m_manager.LoadRoutePoints([&](bool success)
  {
    TEST(!success, ());
    done = true;
  });
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 3, "new start"), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 4, "new finish"), false);
  WaitUntil([&] { return done; });
  TEST_EQUAL(m_manager.GetRoutePoints().back().m_title, "new finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, AppendingIntermediateOptimizesTheAddedPoint)
{
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1, "start"), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 1.004, "finish"), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.003, "C", 0));
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.001, "A", 1));
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.002, "B", 2));
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points[1].m_title, "A", ());
  TEST_EQUAL(points[2].m_title, "B", ());
  TEST_EQUAL(points[3].m_title, "C", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, ReplacingEndpointDoesNotReorderIntermediatePoints)
{
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1, "start"), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 1.004, "finish"), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.003, "C", 0), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.001, "A", 1), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.002, "B", 2), false);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 1.005, "new finish"));
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points[1].m_title, "C", ());
  TEST_EQUAL(points[2].m_title, "A", ());
  TEST_EQUAL(points[3].m_title, "B", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, BatchUsesArrayOrderAtThePointLimit)
{
  std::vector<RouteMarkData> points;
  for (size_t i = 0; i < RoutePointsLayout::kMaxRoutePointsCount; ++i)
    points.push_back(MakePoint(RouteMarkType::Start, 1 + i * 0.001, std::to_string(i), 99));
  points[1].m_isPassed = true;
  m_manager.ReplaceRoutePoints(points);
  auto const actual = m_manager.GetRoutePoints();
  TEST_EQUAL(actual.size(), points.size(), ());
  TEST(actual.front().m_pointType == RouteMarkType::Start, ());
  TEST(actual.back().m_pointType == RouteMarkType::Finish, ());
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST_EQUAL(actual[i].m_title, points[i].m_title, (i));
    TEST_EQUAL(actual[i].m_subTitle, points[i].m_subTitle, (i));
    TEST_EQUAL(actual[i].m_isPassed, points[i].m_isPassed, (i));
    if (i > 0 && i + 1 < actual.size())
    {
      TEST(actual[i].m_pointType == RouteMarkType::Intermediate, (i));
      TEST_EQUAL(actual[i].m_intermediateIndex, i - 1, (i));
    }
  }
  TEST(actual[1].m_isVisible, ());

  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 3, "rejected stop"));
  auto const afterRejectedInsert = m_manager.GetRoutePoints();
  TEST_EQUAL(afterRejectedInsert.size(), actual.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
    TEST_EQUAL(afterRejectedInsert[i].m_title, actual[i].m_title, (i));
}

UNIT_CLASS_TEST(RoutingManagerTest, BatchInvalidatesPreviousRouteTransactions)
{
  AddTestPoints();
  auto const transaction = m_manager.OpenRoutePointsTransaction();
  m_manager.ReplaceRoutePoints(
      {MakePoint(RouteMarkType::Start, 3, "new start"), MakePoint(RouteMarkType::Finish, 4, "new finish")});
  m_manager.CancelRoutePointsTransaction(transaction);
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points.size(), 2, ());
  TEST_EQUAL(points.front().m_title, "new start", ());
  TEST_EQUAL(points.back().m_title, "new finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, MyPositionTitleUsesCoreStrings)
{
  auto start = MakePoint(RouteMarkType::Start, 1, "");
  start.m_isMyPosition = true;
  m_manager.ReplaceRoutePoints({start, MakePoint(RouteMarkType::Finish, 2, "finish")});
  TEST_EQUAL(m_manager.GetRoutePoints().front().m_title, "My Position", ());
  m_framework.AddString("core_my_position", "Localized position");
  m_manager.ReplaceRoutePoints({start, MakePoint(RouteMarkType::Finish, 2, "finish")});
  TEST_EQUAL(m_manager.GetRoutePoints().front().m_title, "Localized position", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, RestoredMyPositionReplacesSavedLabels)
{
  auto start = MakePoint(RouteMarkType::Start, 1, "saved label");
  start.m_replaceWithMyPositionAfterRestart = true;
  m_manager.ReplaceRoutePoints({start, MakePoint(RouteMarkType::Finish, 2, "finish")});
  SavePoints();
  m_manager.RemoveRoutePoints();
  auto const currentPosition = mercator::FromLatLon(1.1, 1);
  m_framework.GetBookmarkManager().MyPositionMark().SetUserPosition(currentPosition, true);
  bool loaded = false;
  m_manager.LoadRoutePoints([&](bool success)
  {
    TEST(success, ());
    loaded = true;
  });
  WaitUntil([&] { return loaded; });
  auto const restored = m_manager.GetRoutePoints().front();
  TEST(restored.m_isMyPosition, ());
  TEST_EQUAL(restored.m_position, currentPosition, ());
  TEST_EQUAL(restored.m_title, "My Position", ());
  TEST(restored.m_subTitle.empty(), ());
}

UNIT_CLASS_TEST(RoutingManagerTest, UnknownMyPositionKeepsIntermediateOrder)
{
  auto start = MakePoint(RouteMarkType::Start, 1, "my position");
  start.m_isMyPosition = true;
  start.m_position = m2::PointD::Zero();
  m_manager.ReplaceRoutePoints({start, MakePoint(RouteMarkType::Finish, 1.004, "finish")});
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.003, "C", 0));
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.001, "A", 1));
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points[1].m_title, "C", ());
  TEST_EQUAL(points[2].m_title, "A", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, RebuildingBatchRemovesPassedIntermediates)
{
  auto start = MakePoint(RouteMarkType::Start, 1, "my position");
  start.m_isMyPosition = true;
  auto passed = MakePoint(RouteMarkType::Intermediate, 1.001, "passed stop");
  passed.m_isPassed = true;
  m_manager.ReplaceRoutePoints({start, passed, MakePoint(RouteMarkType::Finish, 1.004, "finish")});
  m_manager.SetRouteBuildingListener([](routing::RouterResultCode code, storage::CountriesSet const &)
  { TEST_EQUAL(code, routing::RouterResultCode::NoCurrentPosition, ()); });
  // Passed-point cleanup happens before the synchronous missing-position result.
  m_manager.BuildRoute();
  auto const points = m_manager.GetRoutePoints();
  TEST_EQUAL(points.size(), 2, ());
  TEST_EQUAL(points.back().m_title, "finish", ());
}
}  // namespace routing_manager_tests
