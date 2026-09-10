#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

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
    routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */);
    for (size_t i = 0; i < intermediateCount; ++i)
    {
      // Descending coordinates ensure that loading does not optimize the saved order.
      auto const coordinate = static_cast<double>(intermediateCount - i + 1);
      auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, coordinate);
      point.m_title = "Stop " + std::to_string(i);
      point.m_subTitle = "Stop subtitle " + std::to_string(i);
      point.m_isPassed = i < passedStops;
      routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */);
    }
    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 110.0);
    finish.m_title = "Finish";
    finish.m_subTitle = "Finish subtitle";
    routingManager.AddRoutePoint(std::move(finish), false /* reorderIntermediatePoints */);

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
}  // namespace

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

    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1.0), false /* reorderIntermediatePoints */);
    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2.0), false /* reorderIntermediatePoints */);
    routingManager.SaveRoutePoints();
    WaitForFileThread();
    TEST(routingManager.HasSavedRoutePoints(), ());

    routingManager.RemoveRoutePoint(RouteMarkType::Finish);
    routingManager.SaveRoutePoints();
    WaitForFileThread();
    TEST(!routingManager.HasSavedRoutePoints(), ());

    routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2.0), false /* reorderIntermediatePoints */);
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

UNIT_TEST(RoutingManager_OptimizeRoutePointsUsesRouteMarks)
{
  std::vector<double> const intermediateCoordinates = {3.0, 1.0, 2.0};
  std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0};

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
  start.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */);
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
  finish.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(finish), false /* reorderIntermediatePoints */);
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
    point.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */);
  }

  routingManager.OptimizeRoutePoints();
  auto const actualPoints = routingManager.GetRoutePoints();

  TEST_EQUAL(actualPoints.size(), expectedCoordinates.size() + 2, ());
  for (size_t i = 0; i < expectedCoordinates.size(); ++i)
    TEST_EQUAL(actualPoints[i + 1].m_position, m2::PointD(expectedCoordinates[i], 0.0), (i));
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsPreservesPassedStops)
{
  std::vector<double> const intermediateCoordinates = {1.0, 4.0, 2.0, 3.0};
  std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0, 4.0};

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
  start.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */);
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
  finish.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(finish), false /* reorderIntermediatePoints */);
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
    point.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */);
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
