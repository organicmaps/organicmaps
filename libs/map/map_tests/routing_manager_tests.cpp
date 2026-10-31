#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"
#include "map/track.hpp"

#include "routing/track_router.hpp"

#include "drape_frontend/visual_params.hpp"

#include "routing/router.hpp"
#include "routing/ruler_router.hpp"

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
#include <cmath>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace routing_manager_tests
{
namespace
{
// Framework reads the saved router at initialization; optimization cases need a road router.
class ScopedVehicleRouter
{
public:
  ScopedVehicleRouter() : m_hasOriginal(settings::Get("router", m_original))
  {
    settings::Set("router", routing::ToString(routing::RouterType::Vehicle));
  }

  ~ScopedVehicleRouter()
  {
    if (m_hasOriginal)
      settings::Set("router", m_original);
    else
      settings::Delete("router");
  }

private:
  std::string m_original;
  bool const m_hasOriginal;
};

class TestFramework
{
public:
  RoutingManager & GetRoutingManager() { return m_framework.GetRoutingManager(); }
  BookmarkManager & GetBookmarkManager() { return m_framework.GetBookmarkManager(); }
  storage::Storage & GetStorage() { return m_framework.GetStorage(); }

private:
  // Select the router before Framework reads it, and restore it after Framework is destroyed.
  ScopedVehicleRouter const m_routerSetting;
  Framework m_framework{FrameworkParams(false /* m_enableDiffs */)};
};

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
  TestFramework framework;
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

auto constexpr kAsyncBuildTimeout = std::chrono::seconds(30);

// A point near the equator built from meter offsets (one mercator unit is one degree there).
m2::PointD MeterPoint(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

// The AsyncRouter delivers route build results via GetPlatform().RunTask(Thread::Gui, ...).
// Async tests replace the GUI task loop with this manually pumped queue: the tasks execute on
// the test's main thread (satisfying the thread checkers bound at Framework construction)
// exactly when the test asks for them, and stale tasks queued by earlier tests in this binary
// through the default GUI loop stay untouched. Tasks still queued when the loop is replaced
// back are dropped, so nothing capturing the test's framework can run after its destruction.
class ManualGuiLoop : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override { return Add(std::move(task)); }
  PushResult Push(Task const & task) override { return Add(task); }

  // Executes queued tasks on the calling thread until |pred| holds or |timeout| passes.
  template <class Pred>
  bool PumpUntil(Pred const & pred, std::chrono::steady_clock::duration timeout)
  {
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock lock(m_mutex);
    while (true)
    {
      while (!m_tasks.empty())
      {
        auto const task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lock.unlock();
        task();
        lock.lock();
      }
      if (pred())
        return true;
      if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout)
        return false;
    }
  }

  // Executes everything queued so far (e.g. before the owning framework is destroyed).
  void Drain()
  {
    PumpUntil([]() { return true; }, {});
  }

private:
  PushResult Add(Task task)
  {
    {
      std::lock_guard lock(m_mutex);
      m_tasks.push_back(std::move(task));
    }
    m_cv.notify_one();
    return {true /* m_isSuccess */, kNoId};
  }

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<Task> m_tasks;
};

// Scaffold of the async route-build tests. The member order encodes the lifetime rules: the
// manual GUI loop must be installed before the framework is constructed, and the default GUI
// thread is restored only after the framework is destroyed (members are destroyed in reverse
// order). The destructor drains the queue first, so the tasks queued by the trailing
// interactions run while the framework is still alive.
struct AsyncBuildFixture
{
  // Installs the manually pumped GUI loop, restoring the default GUI thread on destruction.
  struct ScopedManualGuiLoop
  {
    ScopedManualGuiLoop()
    {
      auto ownedGuiLoop = std::make_unique<ManualGuiLoop>();
      m_loop = ownedGuiLoop.get();
      GetPlatform().SetGuiThread(std::move(ownedGuiLoop));
    }
    ~ScopedManualGuiLoop() { GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>()); }

    ManualGuiLoop * m_loop;
  };

  AsyncBuildFixture()
  {
    m_framework.GetRoutingManager().Init(routing::CreateNumMwmIds(m_framework.GetStorage()));
    m_framework.GetRoutingManager().SetRouteBuildingListener(
        [this](routing::RouterResultCode code, storage::CountriesSet const &) { m_buildCode = code; });
  }

  ~AsyncBuildFixture() { m_guiLoop.m_loop->Drain(); }

  // Waits until the async build result actually reaches the listener: a lost result (e.g. an
  // instantly expired RouterDelegate deadline) fails the test instead of being masked.
  void WaitForBuildResult() { TEST_EQUAL(WaitForAnyBuildResult(), routing::RouterResultCode::NoError, ()); }

  // The same wait accepting whatever terminal code arrives (and returning it): the outcome of
  // a real road-router build depends on which mwms are registered on the machine (the data dir
  // may contain developer-downloaded maps), so environment-independent tests assert only the
  // synchronous state plus the delivery of some result.
  routing::RouterResultCode WaitForAnyBuildResult()
  {
    TEST(m_guiLoop.m_loop->PumpUntil([this]() { return m_buildCode.has_value(); }, kAsyncBuildTimeout),
         ("The route build result was not delivered"));
    auto const code = *m_buildCode;
    m_buildCode.reset();
    return code;
  }

  ScopedManualGuiLoop m_guiLoop;
  Framework m_framework{FrameworkParams(false /* m_enableDiffs */)};
  std::optional<routing::RouterResultCode> m_buildCode;
};

// Redirects the settings dir (which hosts the bookmarks dir) to a temporary one, so the tests
// saving bookmark categories don't pollute the default data/bookmarks/ (the bookmarks_test.cpp
// precedent). Construct before the Framework and keep alive until it is destroyed.
class ScopedSettingsDir
{
public:
  ScopedSettingsDir()
    : m_originalDir(GetPlatform().SettingsDir())
    , m_testDir(base::JoinPath(GetPlatform().WritableDir(), "routing_manager_tests"))
  {
    TEST(Platform::MkDirRecursively(base::JoinPath(m_testDir, "bookmarks")), ());
    GetPlatform().SetSettingsDir(m_testDir);
  }

  ~ScopedSettingsDir()
  {
    GetPlatform().SetSettingsDir(m_originalDir);
    Platform::RmDirRecursively(m_testDir);
  }

private:
  std::string m_originalDir;
  std::string m_testDir;
};

kml::TrackId CreateTestTrack(BookmarkManager & bmManager, std::string routerType = {},
                             std::vector<kml::TrackStopData> stops = {},
                             std::initializer_list<geometry::PointWithAltitude> geometry = {})
{
  kml::TrackData trackData;
  trackData.m_layers.emplace_back();
  if (geometry.size() != 0)
    trackData.m_geometry.AddLine(geometry);
  else
    trackData.m_geometry.AddLine({{mercator::FromLatLon(45.0, 19.0), 100},
                                  {mercator::FromLatLon(45.001, 19.0), 110},
                                  {mercator::FromLatLon(45.002, 19.0), 120}});
  trackData.m_geometry.AddTimestamps({});
  trackData.m_stops = std::move(stops);
  trackData.m_routerType = std::move(routerType);
  return bmManager.GetEditSession().CreateTrack(std::move(trackData))->GetId();
}
void UpdateGps(AsyncBuildFixture & fixture, RoutingManager & manager, location::GpsInfo const & info)
{
  bool delivered = false;
  auto const point = mercator::FromLatLon(info.m_latitude, info.m_longitude);
  fixture.m_framework.GetBookmarkManager().MyPositionMark().SetUserPosition(point, true);
  manager.SetUserCurrentPosition(point);
  manager.OnLocationUpdate(info);
  GetPlatform().RunTask(Platform::Thread::Background,
                        [&] { GetPlatform().RunTask(Platform::Thread::Gui, [&] { delivered = true; }); });
  TEST(fixture.m_guiLoop.m_loop->PumpUntil([&] { return delivered; }, kAsyncBuildTimeout), ());
}
}  // namespace

UNIT_TEST(RoutingManager_ReverseRoutePointsRequiresBothEndpoints)
{
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
  auto & manager = framework.GetRoutingManager();

  AddTestRoute(manager, {3, 1, 2});
  manager.RoutingSession().SetState(routing::SessionState::RouteNotStarted);
  TEST(manager.RoutingSession().EnableFollowMode(), ());
  TEST(manager.IsRoutingFollowing(), ());
  TEST(!manager.OptimizeRoutePoints(), ());
  TEST_EQUAL(GetStopCoordinates(manager), (std::vector<double>{3, 1, 2}), ());
}

UNIT_TEST(RoutingManager_SavedRouterFallback)
{
  TestFramework framework;
  auto const & manager = framework.GetRoutingManager();

  settings::Set("router", routing::ToString(routing::RouterType::Pedestrian));
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ());

  settings::Set("router", std::string("future-router"));
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Vehicle, ());
  std::string retained;
  TEST(settings::Get("router", retained), ());
  TEST_EQUAL(retained, "future-router", ());

  settings::Delete("router");
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Vehicle, ());
}

UNIT_TEST(RoutingManager_RulerPreservesMeasurementOrder)
{
  for (bool const enabled : {false, true})
  {
    TestFramework framework;
    auto & manager = framework.GetRoutingManager();
    manager.Init(routing::CreateNumMwmIds(framework.GetStorage()));

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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
    TestFramework framework;
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
  TestFramework framework;
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
    TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
    TestFramework framework;
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

UNIT_TEST(RoutingManager_BuildKeepsResolvedMyPosition)
{
  for (auto const type : {RouteMarkType::Start, RouteMarkType::Intermediate, RouteMarkType::Finish})
  {
    WithSavedRoute([type](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread & guiThread)
    {
      auto const originalRouter = manager.GetLastUsedRouter();
      SCOPE_GUARD(restoreRouter, [&] { manager.SetLastUsedRouter(originalRouter); });
      manager.SetRouter(routing::RouterType::Ruler);
      manager.Init(std::make_shared<routing::NumMwmIds>());
      size_t movingIndex = 0;
      for (size_t i = 0;
           auto const pointType : {RouteMarkType::Start, RouteMarkType::Intermediate, RouteMarkType::Finish})
      {
        auto point = MakeRoutePoint(pointType, 0, 0.001 * ++i);
        point.m_isMyPosition = pointType == type;
        if (point.m_isMyPosition)
          movingIndex = i - 1;
        manager.AddRoutePoint(std::move(point), false /* optimize */);
      }
      m2::PointD const calculationPosition(0.004, 0.004);
      bookmarks.MyPositionMark().SetUserPosition(calculationPosition, true /* hasPosition */);

      bool built = false;
      manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
      {
        TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
        built = true;
      });
      manager.BuildRoute();
      while (!built)
        guiThread.RunNext();
      TEST(manager.IsRouteValid(), ());

      m2::PointD const rebuildPosition(0.008, 0.008);
      bookmarks.MyPositionMark().SetUserPosition(rebuildPosition, true /* hasPosition */);
      auto const points = manager.GetRoutePoints();
      TEST_EQUAL(points[movingIndex].m_position, calculationPosition, ());
      TEST_EQUAL(RoutePointsLayout(bookmarks).GetRoutePoint(type)->GetPivot(), calculationPosition, ());
      TEST(points[movingIndex].m_isMyPosition, ());

      built = false;
      manager.BuildRoute();
      while (!built)
        guiThread.RunNext();
      TEST(manager.IsRouteValid(), ());
      TEST_EQUAL(manager.GetRoutePoints()[movingIndex].m_position, rebuildPosition, ());
      TEST_EQUAL(RoutePointsLayout(bookmarks).GetRoutePoint(type)->GetPivot(), rebuildPosition, ());
      TEST(manager.GetRoutePoints()[movingIndex].m_isMyPosition, ());

      bookmarks.MyPositionMark().SetUserPosition({0.012, 0.012}, true /* hasPosition */);
      // A saved start follows the live position on restart; intermediate and finish points are fixed.
      if (type != RouteMarkType::Start)
      {
        manager.SaveRoutePoints();
        WaitForFileThread();
        manager.RemoveRoutePoints();
        TEST(LoadRoutePoints(manager, guiThread), ());
        TEST_EQUAL(manager.GetRoutePoints()[movingIndex].m_position, rebuildPosition, ());
      }
    });
  }
}

UNIT_TEST(RoutingManager_ContinueRouteToPointAtLimitKeepsFinish)
{
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
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
  TestFramework framework;
  auto & routingManager = framework.GetRoutingManager();

  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());

  auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 1.0);
  TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());
  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsPreservesPassedStops)
{
  TestFramework framework;
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
UNIT_TEST(RoutingManager_RebuildClearsUnavailableAlternatives)
{
  Platform::ThreadRunner const threadRunner;
  df::VisualParams::Init(1.0, 1024);

  class Delegate final : public RoutingManager::Delegate
  {
  public:
    void OnRouteFollow(routing::RouterType) override {}
  } delegate;
  class Router final : public routing::IRouter
  {
  public:
    std::string GetName() const override { return "route choices test"; }
    void ClearState() override {}
    void SetGuides(routing::GuidesTracks &&) override {}
    bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, routing::EdgeProj &) override
    {
      return false;
    }
    routing::RouterResultCode CalculateRoute(routing::Checkpoints const & checkpoints, m2::PointD const & direction,
                                             bool adjust, bool alternatives, routing::RouterDelegate const & delegate,
                                             routing::RoutesResult & result) override
    {
      if (++m_calls == 2)
        return routing::RouterResultCode::RouteNotFound;
      auto const code = static_cast<routing::IRouter &>(m_ruler).CalculateRoute(checkpoints, direction, adjust,
                                                                                alternatives, delegate, result);
      auto alternative = result.GetActive();
      alternative.SetDiffMidpoint({2, 5});
      result.m_routes.emplace_back(std::move(alternative));
      return code;
    }

  private:
    routing::RulerRouter m_ruler;
    size_t m_calls = 0;
  };

  auto & platform = GetPlatform();
  auto gui = std::make_unique<TestGuiThread>();
  auto * guiThread = gui.get();
  platform.SetGuiThread(std::move(gui));
  SCOPE_GUARD(restoreGui, [&platform] { platform.SetGuiThread(std::make_unique<platform::GuiThread>()); });
  BookmarkManager bookmarks(BookmarkManager::Callbacks([]() -> StringsBundle const &
  {
    static StringsBundle const bundle;
    return bundle;
  }, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
  RoutingManager manager(RoutingManager::Callbacks(nullptr, nullptr, nullptr, nullptr, nullptr), delegate);
  manager.SetBookmarkManager(&bookmarks);
  auto & session = manager.RoutingSession();
  session.SetRouter(std::make_unique<Router>(), nullptr);
  bool built = false;
  session.SetRoutingCallbacks([&built](routing::RoutesResult const &, routing::RouterResultCode) { built = true; },
                              nullptr, nullptr, nullptr);
  session.BuildRoute(routing::Checkpoints({1, 2}, {5, 6}), routing::RouterDelegate::kNoTimeout);
  while (!built)
    guiThread->RunNext();
  TEST(session.SwapActiveAlternative(1), ());
  {
    auto edit = bookmarks.GetEditSession();
    edit.CreateUserMark<RouteAltMark>({1, 2});
    edit.CreateUserMark<RouteAltMark>({2, 5});
  }
  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).size(), 2, ());

  bool failed = false;
  session.RebuildRoute({1, 2}, [](routing::RoutesResult const &, routing::RouterResultCode) {}, nullptr,
                       [&failed](routing::RouterResultCode) { failed = true; }, routing::RouterDelegate::kNoTimeout,
                       routing::SessionState::RouteRebuilding, false);
  TEST(bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).empty(),
       ("Unavailable controls must disappear immediately."));
  session.RouteCall([](routing::RoutesResult const & result)
  {
    TEST_EQUAL(result.m_routes.size(), 1, ());
    TEST_EQUAL(result.m_activeIdx, 0, ());
    TEST_EQUAL(result.GetActive().GetDiffMidpoint(), std::optional<m2::PointD>({2, 5}),
               ("The selected route must be retained."));
  });
  while (!failed)
    guiThread->RunNext();
  TEST(bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).empty(), ());

  bool recovered = false;
  session.RebuildRoute({1, 2}, [&recovered](routing::RoutesResult const &, routing::RouterResultCode) {
    recovered = true;
  }, nullptr, nullptr, routing::RouterDelegate::kNoTimeout, routing::SessionState::RouteRebuilding, false);
  while (!recovered)
    guiThread->RunNext();
  TEST(session.SwapActiveAlternative(1), ("Fresh alternatives must become selectable."));
}

UNIT_TEST(RoutingManager_TrackEntryResolvesBothSidesOfGap)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 110}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(500, 0), 120}, {MeterPoint(600, 0), 130}});
  source.m_geometry.AddTimestamps({});
  double const boundary = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0));
  auto const before = ResolveTrackEntry(source.m_geometry, MeterPoint(100, 0), boundary);
  auto const after = ResolveTrackEntry(source.m_geometry, MeterPoint(500, 0), boundary);
  TEST(before && after, ());
  TEST_EQUAL(before->m_line, 0, ());
  TEST_EQUAL(after->m_line, 1, ());
  auto const forward = BuildTrackFollowingData(source, *after, false);
  auto const backward = BuildTrackFollowingData(source, *after, true);
  TEST_EQUAL(forward.m_points.front().GetPoint(), MeterPoint(500, 0), ());
  TEST_EQUAL(forward.m_points.back().GetPoint(), MeterPoint(600, 0), ());
  TEST(backward.m_points.empty(), ("Never connect across the preceding gap"));
  TEST(!ResolveTrackEntry(source.m_geometry, MeterPoint(500, 0), 0.0),
       ("A stale distance must not select a new occurrence"));
}

UNIT_TEST(RoutingManager_TrackPreparationKeepsSelectedStopAndHill)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(400, 0), 250}, {MeterPoint(1000, 0), 100}});
  source.m_geometry.AddTimestamps({100, 200, 300});
  source.m_stops = {{MeterPoint(100, 0), "Before"}, {MeterPoint(400, 0), "Entry"}, {MeterPoint(600, 0), "After"}};
  auto const original = source;
  auto const entry = ResolveTrackEntry(source.m_geometry, MeterPoint(400, 0),
                                       mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0)));
  TEST(entry, ());
  auto const data = BuildTrackFollowingData(source, *entry, false);
  TEST_EQUAL(data.m_points.size(), 3, ());
  TEST_EQUAL(data.m_points[0].GetAltitude(), 250, ());
  TEST_EQUAL(data.m_points[1].GetPoint(), MeterPoint(600, 0), ());
  TEST_EQUAL(data.m_stopPointIndexes, std::vector<size_t>({0, 1}), ());
  TEST_EQUAL(source, original, ());
  auto const reverse = BuildTrackFollowingData(source, *entry, true);
  TEST_EQUAL(reverse.m_points.front().GetPoint(), MeterPoint(400, 0), ());
  TEST_EQUAL(reverse.m_points.back().GetPoint(), MeterPoint(0, 0), ());
  TEST_EQUAL(reverse.m_stopPointIndexes.size(), 2, ());
}

UNIT_TEST(RoutingManager_RepeatedStopsPreferLaterSourceVisit)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(500, 0), "Outbound"}, {MeterPoint(500, 0), "Return"}};
  auto const entry = ResolveTrackEntry(source.m_geometry, MeterPoint(600, 0),
                                       mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(600, 0)));
  TEST(entry, ());
  auto const forward = BuildTrackFollowingData(source, *entry, false);
  TEST_EQUAL(forward.m_stopPointIndexes.size(), 1, ());
  TEST_EQUAL(forward.m_points[forward.m_stopPointIndexes.front()].GetPoint(), MeterPoint(500, 0), ());
  TEST_GREATER(forward.m_stopPointIndexes.front(), 1, ("Keep the return visit after the turnaround"));
  auto const backward = BuildTrackFollowingData(source, *entry, true);
  TEST_EQUAL(backward.m_stopPointIndexes.size(), 1, ());
  source.m_geometry.m_lines[0].pop_back();
  auto const earlier = ResolveTrackEntry(source.m_geometry, MeterPoint(400, 0),
                                         mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0)));
  TEST(earlier, ());
  auto const duplicates = BuildTrackFollowingData(source, *earlier, false);
  TEST_EQUAL(duplicates.m_stopPointIndexes.size(), 2, ());
  TEST_EQUAL(duplicates.m_stopPointIndexes[0], duplicates.m_stopPointIndexes[1], ("Keep true collocated duplicates"));
}

UNIT_TEST(RoutingManager_TrackPreviewLocksEntryWhenGpsIsNearFinish)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  manager.SetRouter(routing::RouterType::Pedestrian);
  auto const id = CreateTestTrack(bookmarks, "bicycle", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  double const entryDistance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), entryDistance}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(900, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST(route, ());
  TEST_EQUAL(route->GetPoly().Front(), MeterPoint(900, 0), ("Retain the actual GPS position"));
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0),
             ("Approach the captured entry, not nearest later geometry"));
  TEST_ALMOST_EQUAL_ABS(manager.GetFollowedTrackInfo()->m_lengthM, 600.0, 2.0, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ("Saved profile is session-only"));

  // Inspecting another graph point must not redefine the preview's original entry.
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(800, 0), 2.0 * entryDistance}, false);
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Vehicle), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ());

  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(1000, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(1000, 0))}, false);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::NoRemainingGeometry, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());

  auto modified = bookmarks.GetTrack(id)->GetData();
  modified.m_geometry.m_lines[0].back() = {MeterPoint(1200, 0), 100};
  bookmarks.GetEditSession().UpdateTrack(id, modified);
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::InvalidSelection, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ("Refuse before teardown"));
  bookmarks.GetEditSession().DeleteTrack(id);
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::NoTrack, ());
  TEST(manager.IsFollowingTrack(), ("The frozen preview remains stoppable"));
  manager.CloseRouting(true);
  TEST(!manager.IsFollowingTrack(), ());
}

UNIT_TEST(RoutingManager_TrackPreviewGpsRefreshDoesNotPassStops)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {{MeterPoint(600, 0), "Stop"}},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  auto const entryDistance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), entryDistance}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(400, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  location::GpsInfo info;
  info.m_source = location::EUser;
  auto const moved = mercator::ToLatLon(MeterPoint(1000, 0));
  info.m_latitude = moved.m_lat;
  info.m_longitude = moved.m_lon;
  info.m_horizontalAccuracy = 5;
  info.m_timestamp = 100;
  info.m_speed = 1;
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(1000, 0), true);
  UpdateGps(fixture, manager, info);
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST(!manager.IsRouteFinished(), ());
  TEST_EQUAL(route->GetCurrentSubrouteIdx(), 0, ());
  TEST_EQUAL(route->GetSubrouteCount(), 2, ());
}

UNIT_TEST(RoutingManager_TrackStartRefreshesGpsWithoutChangingEntry)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(400, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  // The location mark may update before the asynchronous GPS projection task runs.
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(900, 0), true);
  manager.FollowRoute();
  TEST(!manager.IsRoutingFollowing(), ("A stale approach must be refreshed before Start"));
  if (manager.IsRoutingFollowing())
    return;
  TEST(manager.IsRouteBuilding(), ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Front(), MeterPoint(900, 0), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
}

UNIT_TEST(RoutingManager_TrackNavigationKeepsGeometryAcrossDepartureAndReturn)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(0, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.RoutingSession().EnableFollowMode(), ());
  TEST(manager.IsRoutingFollowing(), ());
  auto const * installed = manager.RoutingSession().GetRoute();
  double timestamp = 100;
  auto const feed = [&](double x, double y, double bearing, double elapsed)
  {
    timestamp += elapsed;
    location::GpsInfo info;
    info.m_source = location::EUser;
    auto const p = mercator::ToLatLon(MeterPoint(x, y));
    info.m_latitude = p.m_lat;
    info.m_longitude = p.m_lon;
    info.m_timestamp = timestamp;
    info.m_horizontalAccuracy = 5;
    info.m_speed = 1;
    info.m_bearing = bearing;
    UpdateGps(fixture, manager, info);
  };
  for (double x = 0; x <= 1000; x += 50)
    feed(x, 0, 90, 50);
  for (double x = 950; x >= 300; x -= 50)
    feed(x, 0, 270, 50);
  TEST_ALMOST_EQUAL_ABS(installed->GetCurrentDistanceToEndMeters(), 300.0, 2.0, ());
  for (double y = 20; y <= 400; y += 20)
    feed(300, y, 0, 20);
  TEST_EQUAL(manager.RoutingSession().GetRoute(), installed, ());
  TEST(manager.RoutingSession().IsNavigable(), ("Keep remaining metrics and Stop usable off track"));
  for (double y = 380; y >= 0; y -= 20)
    feed(300, y, 180, 20);
  TEST(manager.IsOnRoute(), ());
  TEST_ALMOST_EQUAL_ABS(installed->GetCurrentDistanceToEndMeters(), 300.0, 2.0, ());
  TEST_ALMOST_EQUAL_ABS(manager.RoutingSession().GetCompletionPercent(), 85.0, 0.1,
                        ("A frozen route must not count departed prefixes twice"));
  TEST_EQUAL(manager.RoutingSession().GetRoute(), installed, ("No geometry-reset rebuild"));
}

UNIT_TEST(RoutingManager_TrackFigureEightDoesNotJumpAtCrossings)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  std::vector<m2::PointD> const path = {MeterPoint(-200, 0),   MeterPoint(0, 0),      MeterPoint(200, 200),
                                        MeterPoint(0, 400),    MeterPoint(-200, 200), MeterPoint(0, 0),
                                        MeterPoint(200, -200), MeterPoint(0, -400),   MeterPoint(-200, -200),
                                        MeterPoint(0, 0),      MeterPoint(200, 0)};
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_routerType = "pedestrian";
  source.m_geometry.m_lines.emplace_back();
  source.m_geometry.m_timestamps.emplace_back();
  for (auto const & point : path)
    source.m_geometry.m_lines.back().emplace_back(point, 100);
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();
  bookmarks.SetTrackSelectionInfo({id, path.front(), 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(path.front(), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.RoutingSession().EnableFollowMode(), ());
  auto const * route = manager.RoutingSession().GetRoute();
  double const total = route->GetTotalDistanceMeters();
  double walked = 0.0;
  double time = 1000.0;
  for (size_t leg = 0; leg + 1 < path.size(); ++leg)
  {
    auto const delta = path[leg + 1] - path[leg];
    double const length = mercator::DistanceOnEarth(path[leg], path[leg + 1]);
    size_t const samples = static_cast<size_t>(std::ceil(length / 20.0));
    for (size_t step = 1; step <= samples; ++step)
    {
      auto const position = path[leg] + delta * (static_cast<double>(step) / samples);
      auto const latlon = mercator::ToLatLon(position);
      location::GpsInfo info;
      info.m_source = location::EUser;
      info.m_horizontalAccuracy = 5;
      info.m_latitude = latlon.m_lat;
      info.m_longitude = latlon.m_lon;
      info.m_timestamp = time += length / samples;
      info.m_speed = 1;
      info.m_bearing = location::AngleToBearing(math::RadToDeg(std::atan2(delta.y, delta.x)));
      UpdateGps(fixture, manager, info);
    }
    walked += length;
    TEST_ALMOST_EQUAL_ABS(route->GetCurrentDistanceToEndMeters(), total - walked, 2.0, (leg));
    if (leg + 2 < path.size())
      TEST(!manager.IsRouteFinished(), (leg));
  }
  TEST(manager.IsRouteFinished(), ());
}

UNIT_TEST(RoutingManager_JoinedTrackEntryDoesNotInventAltitudeExcursion)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 50}, {MeterPoint(100, 0), geometry::kInvalidAltitude}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(100, 0), 100}, {MeterPoint(200, 0), 200}});
  source.m_geometry.AddTimestamps({});
  auto const forward = BuildTrackFollowingData(source, {1, 0, 0.0}, false);
  TEST_EQUAL(forward.m_points.size(), 2, ());
  TEST_EQUAL(forward.m_points.front().GetAltitude(), 100, ());
  TEST_EQUAL(forward.m_points.back().GetAltitude(), 200, ());
  auto const reverse = BuildTrackFollowingData(source, {1, 0, 0.0}, true);
  TEST_EQUAL(reverse.m_points.size(), 3, ());
  TEST_EQUAL(reverse.m_points[0].GetAltitude(), 100, ());
  TEST_EQUAL(reverse.m_points[1].GetAltitude(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(reverse.m_points[2].GetAltitude(), 50, ());
}

UNIT_TEST(RoutingManager_CoarseTrackFixDoesNotArriveOrDepart)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100},
                                   {MeterPoint(-200, 0), 100},
                                   {MeterPoint(-200, 200), 100},
                                   {MeterPoint(1, 200), 100},
                                   {MeterPoint(1, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(0, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  manager.FollowRoute();
  auto const * route = manager.RoutingSession().GetRoute();
  for (size_t i = 0; i < 12; ++i)
  {
    auto const latlon = mercator::ToLatLon(MeterPoint(1, 0));
    location::GpsInfo info;
    info.m_source = location::EUser;
    info.m_latitude = latlon.m_lat;
    info.m_longitude = latlon.m_lon;
    info.m_timestamp = 100 + i;
    info.m_speed = 0;
    info.m_horizontalAccuracy = 1000;
    UpdateGps(fixture, manager, info);
    TEST(!manager.IsRouteFinished(), (i));
    TEST(manager.RoutingSession().IsNavigable(), (i));
    TEST_EQUAL(route->GetCurrentDistanceFromBeginMeters(), 0.0, (i));
  }
  TEST_EQUAL(manager.RoutingSession().GetCompletionPercent(), 0.0, ());
  TEST(manager.IsOnRoute(), ("Coarse fixes do not count as departing the track"));
}

UNIT_TEST(RoutingManager_ReverseTrackPreviewKeepsCapturedEntryAndProfile)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  manager.SetRouter(routing::RouterType::Pedestrian);
  auto const id =
      CreateTestTrack(bookmarks, "bicycle",
                      {{MeterPoint(100, 0), "Before"}, {MeterPoint(400, 0), "Entry"}, {MeterPoint(600, 0), "After"}},
                      {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 200}});
  auto const source = bookmarks.GetTrack(id)->GetData();
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(400, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(manager.GetFollowedTrackInfo()->m_lengthM, 600.0, 2.0, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy,
             ("Ignore rapid taps before the new preview is built"));
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_ALMOST_EQUAL_ABS(route->GetTotalDistanceMeters(), 400.0, 2.0, ());
  TEST_EQUAL(route->GetPoly().Front(), MeterPoint(400, 0), ());
  TEST_EQUAL(route->GetPoly().Back(), MeterPoint(0, 0), ());
  TEST_EQUAL(route->GetSubrouteCount(), 3, ("Keep the entry and earlier-direction stop"));
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ());

  // Neither a later graph selection nor a new GPS fix replaces the captured source entry.
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(800, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(800, 0))}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(900, 0), true);
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().Front(), MeterPoint(900, 0), ());
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST_ALMOST_EQUAL_ABS(manager.GetFollowedTrackInfo()->m_lengthM, 600.0, 2.0, ());
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ());
}

UNIT_TEST(RoutingManager_ReverseTrackRefusalsPreservePreview)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::NoTrack, ());
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(0, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * original = manager.RoutingSession().GetRoute();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::NoRemainingGeometry, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
  manager.DisableFollowMode();
  TEST_EQUAL(manager.FollowTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  original = manager.RoutingSession().GetRoute();
  auto changed = bookmarks.GetTrack(id)->GetData();
  changed.m_geometry.m_lines.front().back().SetPoint(MeterPoint(1100, 0));
  bookmarks.GetEditSession().UpdateTrack(id, changed);
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::InvalidSelection, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
  bookmarks.GetEditSession().DeleteTrack(id);
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::NoTrack, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
}

UNIT_TEST(RoutingManager_ReverseTrackUsesSelectedOccurrenceOnReturnSection)
{
  ScopedVehicleRouter routerSetting;
  ScopedSettingsDir settingsDir;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  double const distance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(1000, 0)) +
                          mercator::DistanceOnEarth(MeterPoint(1000, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), distance}, false);
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(400, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id, false), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(manager.GetFollowedTrackInfo()->m_lengthM, 400.0, 2.0, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(manager.GetFollowedTrackInfo()->m_lengthM, 1600.0, 4.0, ());
  auto const & points = manager.RoutingSession().GetRoute()->GetPoly();
  TEST_EQUAL(points.GetPoint(1), MeterPoint(1000, 0), ("Walk backwards through the exact second source occurrence"));
}

// Stop names need no subtitle; only endpoint names filter unnamed-pin placeholders.
UNIT_TEST(RoutingManager_SaveRouteKeepsStopNames)
{
  WithSavedRoute([](RoutingManager & routingManager, BookmarkManager & bmManager, TestGuiThread & guiThread)
  {
    // Also makes the category saving synchronous: no File-thread write outlives the scoped dir.
    bmManager.EnableTestMode(true);
    SCOPE_GUARD(restoreTestMode, [&] { bmManager.EnableTestMode(false); });

    // The Ruler router builds a real route deterministically, with no map data at all.
    routingManager.SetRouter(routing::RouterType::Ruler);
    routingManager.Init(std::make_shared<routing::NumMwmIds>());

    auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
    start.m_title = "Dropped pin";  // An unnamed tapped spot: a placeholder title, no subtitle.
    routingManager.AddRoutePoint(std::move(start), false /* optimize */);

    std::vector<kml::TrackStopData> const expectedStops = {{{0.0007, 0.0007}, "Calais"}, {{0.0004, 0.0004}, ""}};
    for (size_t i = 0; i < expectedStops.size(); ++i)
    {
      auto stop = MakeRoutePoint(RouteMarkType::Intermediate, i, expectedStops[i].m_point.x);
      stop.m_title = expectedStops[i].m_name;
      stop.m_isMyPosition = i == 1;
      if (stop.m_isMyPosition)
        stop.m_position = {0.0003, 0.0003};
      routingManager.AddRoutePoint(std::move(stop), false /* optimize */);
    }
    bmManager.MyPositionMark().SetUserPosition(expectedStops.back().m_point, true /* hasPosition */);

    auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 0.001);
    finish.m_title = "Paris";
    finish.m_subTitle = "City";
    routingManager.AddRoutePoint(std::move(finish), false /* optimize */);

    bool built = false;
    routingManager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
      built = true;
    });
    routingManager.BuildRoute();
    while (!built)
      guiThread.RunNext();
    TEST(routingManager.IsRouteValid(), ());

    // Save the fix used by the build, even if the user moved again before saving.
    bmManager.MyPositionMark().SetUserPosition({0.0008, 0.0008}, true /* hasPosition */);

    auto const trackId = routingManager.SaveRoute();
    TEST_NOT_EQUAL(trackId, kml::kInvalidTrackId, ());
    auto const * track = bmManager.GetTrack(trackId);
    TEST(track != nullptr, ());
    // The from/to names keep the subtitle guard: the start placeholder is filtered out.
    TEST_EQUAL(track->GetName(), "Paris", ());
    TEST_EQUAL(track->GetData().m_routerType, "ruler", ());
    TEST_EQUAL(track->GetData().m_stops, expectedStops, ());
  });
}
}  // namespace routing_manager_tests
