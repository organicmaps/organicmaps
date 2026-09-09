#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"
#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"

#include "geometry/mercator.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/settings.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
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
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1.0, 1.0), false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.5, 1.5, "app://passed", 0),
                          false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.7, 1.7, "app://next", 1),
                          false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 2.0, 2.0, "app://finish"),
                          false /* reorderIntermediatePoints */);

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
