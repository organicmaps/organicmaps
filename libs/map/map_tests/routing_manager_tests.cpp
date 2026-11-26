#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/framework.hpp"
#include "map/routing_mark.hpp"
#include "map/track.hpp"

#include "drape_frontend/visual_params.hpp"

#include "routing/router.hpp"
#include "routing/ruler_router.hpp"

#include "storage/routing_helpers.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/thread_safe_queue.hpp"
#include "base/waiter.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace routing_manager_tests
{
namespace
{
struct ExpectedAssertion
{};

template <typename Fn>
void TestAssertion(Fn && fn)
{
  auto const previous =
      base::SetAssertFunction([](base::SrcPoint const &, std::string const &) -> bool { throw ExpectedAssertion{}; });
  SCOPE_GUARD(restoreAssert, [&] { base::SetAssertFunction(previous); });
  try
  {
    fn();
  }
  catch (ExpectedAssertion const &)
  {
    return;
  }
  TEST(false, ("Expected an assertion"));
}

// A point near the equator built from meter offsets (one mercator unit is one degree there).
m2::PointD MeterPoint(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

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
    m_tasksReady.Notify();
    return {true, kNoId};
  }
  PushResult Push(Task const & task) override { return Push(Task(task)); }

  // Drain queued callbacks before checking pred; wait up to timeout for more.
  template <class Pred>
  bool PumpUntil(Pred const & pred, std::chrono::steady_clock::duration timeout)
  {
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
      Task task;
      while (!m_tasks.TryPop(task))
      {
        if (pred())
          return true;
        if (m_tasksReady.Wait(deadline - std::chrono::steady_clock::now()) == base::Waiter::Result::Timeout)
          return false;
        m_tasksReady.Reset();
      }
      task();
    }
  }

  template <typename Predicate>
  void RunUntil(Predicate && done)
  {
    TEST(PumpUntil(done, std::chrono::seconds(30)), ("GUI callback timed out"));
  }

  // Executes everything queued so far (e.g. before the owning framework is destroyed).
  void Drain()
  {
    PumpUntil([]() { return true; }, {});
  }

private:
  threads::ThreadSafeQueue<Task> m_tasks;
  base::Waiter m_tasksReady;
};

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
  WithSavedRoute([=](RoutingManager & routingManager, BookmarkManager & bookmarks, TestGuiThread &)
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
      expected.front().m_title = "My Position";
      expected.front().m_subTitle.clear();
    }

    TEST(routingManager.LoadRoutePoints(), ());

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

double GetFollowedTrackLengthMeters(RoutingManager const & manager)
{
  auto const * route = manager.RoutingSession().GetRoute();
  TEST(route != nullptr && route->IsTrack(), ());
  return route->GetTotalDistanceMeters() - *route->GetTrackStartDistanceMeters();
}

void AssignRouteGeometry(RoutingManager & manager, kml::TrackGeometry const & points)
{
  auto const & start = points.front();
  auto const & finish = points.back();
  routing::Segment const fake(routing::kFakeNumMwmId, 0, 0, false);
  std::vector<routing::RouteSegment> segments;
  double distance = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    auto const direction = i + 1 == points.size() ? routing::turns::CarDirection::ReachedYourDestination
                                                  : routing::turns::CarDirection::None;
    segments.emplace_back(fake, routing::turns::TurnItem(i, direction), points[i],
                          routing::RouteSegment::RoadNameInfo{});
    distance += mercator::DistanceOnEarth(points[i - 1].GetPoint(), points[i].GetPoint());
    segments.back().SetDistancesAndTime(distance, distance, distance);
  }
  routing::Route route;
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{{start, finish, 0, points.size() - 1}});
  routing::RoutesResult result;
  result.MakeFrom("ruler", std::move(route));
  manager.RoutingSession().AssignRouteForTesting(routing::Route(result.GetActive()),
                                                 routing::RouterResultCode::NoError);
  TEST(manager.IsRouteValid(), ());
}

kml::TrackId SaveRouteGeometry(RoutingManager & manager, BookmarkManager & bookmarks, kml::TrackGeometry const & points)
{
  bookmarks.EnableTestMode(true);
  SCOPE_GUARD(restoreTestMode, [&] { bookmarks.EnableTestMode(false); });
  // Saving is profile-independent; Ruler initializes without map regions.
  manager.SetRouter(routing::RouterType::Ruler);
  manager.Init(std::make_shared<routing::NumMwmIds>());
  auto const & start = points.front();
  auto const & finish = points.back();
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Start, 0, start.GetPoint().x), false);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, finish.GetPoint().x), false);
  AssignRouteGeometry(manager, points);
  return manager.SaveRoute();
}

auto constexpr kAsyncBuildTimeout = std::chrono::seconds(30);

// Isolate saved tracks and route recovery until Framework and its callbacks are destroyed.
class ScopedSettingsDir
{
public:
  ScopedSettingsDir()
    : m_originalDir(GetPlatform().SettingsDir())
    , m_testDir(base::JoinPath(GetPlatform().WritableDir(), "routing_manager_tests"))
    , m_cleanup(m_testDir)
  {
    GetPlatform().SetSettingsDir(m_testDir);
  }

  ~ScopedSettingsDir() { GetPlatform().SetSettingsDir(m_originalDir); }

private:
  std::string const m_originalDir;
  std::string const m_testDir;
  platform::tests_support::ScopedDirCleanup const m_cleanup;
};

// Install the GUI queue before Framework construction and drain callbacks before destruction.
struct AsyncBuildFixture
{
  // Record the renderer notification while exercising the real routing manager and session.
  class FrameworkWithFollowDelegate final : public Framework
  {
  public:
    using Framework::Framework;

    std::vector<routing::RouterType> m_followProfiles;

  private:
    void OnRouteFollow(routing::RouterType type) override { m_followProfiles.push_back(type); }
  };

  // Installs the manually pumped GUI loop, restoring the default GUI thread on destruction.
  struct ScopedTestGuiThread
  {
    ScopedTestGuiThread()
    {
      auto ownedGuiLoop = std::make_unique<TestGuiThread>();
      m_loop = ownedGuiLoop.get();
      GetPlatform().SetGuiThread(std::move(ownedGuiLoop));
    }
    ~ScopedTestGuiThread() { GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>()); }

    TestGuiThread * m_loop;
  };

  AsyncBuildFixture()
  {
    m_framework.GetRoutingManager().SetRouteBuildingListener(
        [this](routing::RouterResultCode code, storage::CountriesSet const &) { m_buildCode = code; });
  }

  ~AsyncBuildFixture() { m_guiLoop.m_loop->Drain(); }

  void SetGpsPosition(m2::PointD const & point)
  {
    auto const latLon = mercator::ToLatLon(point);
    location::GpsInfo fix;
    fix.m_source = location::EUser;
    fix.m_latitude = latLon.m_lat;
    fix.m_longitude = latLon.m_lon;
    fix.m_horizontalAccuracy = 5.0;
    m_framework.OnLocationUpdate(fix);
  }

  // Wait for listener delivery rather than observing routing-session state.
  void WaitForBuildResult() { TEST_EQUAL(WaitForAnyBuildResult(), routing::RouterResultCode::NoError, ()); }

  // Road-router results depend on locally available maps; wait for any terminal result.
  routing::RouterResultCode WaitForAnyBuildResult()
  {
    TEST(m_guiLoop.m_loop->PumpUntil([this]() { return m_buildCode.has_value(); }, kAsyncBuildTimeout),
         ("The route build result was not delivered"));
    auto const code = *m_buildCode;
    m_buildCode.reset();
    return code;
  }

  // Initialize global settings before redirecting Framework's file paths.
  ScopedVehicleRouter const m_routerSetting;
  ScopedSettingsDir const m_settingsDir;
  ScopedTestGuiThread m_guiLoop;
  FrameworkWithFollowDelegate m_framework{FrameworkParams(false /* m_enableDiffs */)};
  std::optional<routing::RouterResultCode> m_buildCode;
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

void UpdateGps(AsyncBuildFixture & fixture, location::GpsInfo const & info)
{
  bool delivered = false;
  fixture.m_framework.OnLocationUpdate(info);
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
    WithSavedRoute([enabled](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
    {
      std::vector<double> const original = {5, 4, 3, 2, 1};
      AddTestRoute(manager, original);
      if (enabled)
        manager.OptimizeRoutePoints();
      manager.MoveRoutePoint(1, 4);
      auto const displayed = GetStopCoordinates(manager);
      manager.SaveRoutePoints();
      manager.RemoveRoutePoints();
      TEST(manager.LoadRoutePoints(), ());
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
      guiThread.RunUntil([&] { return built; });
      TEST(manager.IsRouteValid(), ());

      m2::PointD const rebuildPosition(0.008, 0.008);
      bookmarks.MyPositionMark().SetUserPosition(rebuildPosition, true /* hasPosition */);
      auto const points = manager.GetRoutePoints();
      TEST_EQUAL(points[movingIndex].m_position, calculationPosition, ());
      TEST_EQUAL(RoutePointsLayout(bookmarks).GetRoutePoint(type)->GetPivot(), calculationPosition, ());
      TEST(points[movingIndex].m_isMyPosition, ());

      built = false;
      manager.BuildRoute();
      guiThread.RunUntil([&] { return built; });
      TEST(manager.IsRouteValid(), ());
      TEST_EQUAL(manager.GetRoutePoints()[movingIndex].m_position, rebuildPosition, ());
      TEST_EQUAL(RoutePointsLayout(bookmarks).GetRoutePoint(type)->GetPivot(), rebuildPosition, ());
      TEST(manager.GetRoutePoints()[movingIndex].m_isMyPosition, ());

      bookmarks.MyPositionMark().SetUserPosition({0.012, 0.012}, true /* hasPosition */);
      // A saved start follows the live position on restart; intermediate and finish points are fixed.
      if (type != RouteMarkType::Start)
      {
        manager.SaveRoutePoints();
        manager.CloseRouting(true /* removeRoutePoints */);
        TEST(manager.LoadRoutePoints(), ());
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
  guiThread->RunUntil([&] { return built; });
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
  guiThread->RunUntil([&] { return failed; });
  TEST(bookmarks.GetUserMarkIds(UserMark::Type::ROUTE_ALT).empty(), ());

  bool recovered = false;
  session.RebuildRoute({1, 2}, [&recovered](routing::RoutesResult const &, routing::RouterResultCode) {
    recovered = true;
  }, nullptr, nullptr, routing::RouterDelegate::kNoTimeout, routing::SessionState::RouteRebuilding, false);
  guiThread->RunUntil([&] { return recovered; });
  TEST(session.SwapActiveAlternative(1), ("Fresh alternatives must become selectable."));
}

UNIT_TEST(RoutingManager_RouteElevationDataUsesMeasuredTransitAltitudes)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  manager.SetRouter(routing::RouterType::Transit);
  AssignRouteGeometry(manager, {{{0.0, 0.0}, 100}, {{0.001, 0.0}, 110}});
  auto const data = manager.GetRouteElevationData();
  TEST(data, ());
  TEST_EQUAL(data->m_statistics.m_maxElevation, 110, ());
}

UNIT_TEST(RoutingManager_TrackChartUsesCapturedSourceUntilNavigation)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  TEST(!manager.GetRouteElevationData(), ());
  TEST(!manager.GetRoutePointAtDistance(0.0), ());
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(400, 0), 180}, {MeterPoint(1000, 0), 120}});
  auto const * track = bookmarks.GetTrack(id);
  double const length = track->GetLengthMeters();
  double const entryDistance = length * 0.6;
  bookmarks.SetTrackSelectionInfo({id, track->GetPoint(entryDistance), entryDistance}, false);
  fixture.SetGpsPosition(MeterPoint(500, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  TEST(manager.GetRouteElevationData(), ("The source chart is available while its approach is building"));
  fixture.WaitForBuildResult();
  bookmarks.GetEditSession().DeleteTrack(id);

  auto const checkPreview = [&]
  {
    auto const data = manager.GetRouteElevationData();
    TEST(data, ());
    TEST(data->m_followedSection, ());
    TEST_ALMOST_EQUAL_ABS(data->m_followedSection->m_entryPoint, 1.0 + 1.0 / 3.0, 1e-6, ());
    TEST_EQUAL(data->m_followedSection->m_finishPoint, 2, ());
    TEST_EQUAL(data->m_statistics.m_ascent, 0, ());
    TEST_EQUAL(data->m_statistics.m_descent, 40, ());
    TEST_EQUAL(data->m_statistics.m_maxElevation, 160, ());
    TEST_EQUAL(data->m_statistics.m_minElevation, 120, ());
    TEST_EQUAL(data->m_elevationInfo.GetLines().front()[1].m_altitude, 180, ("Keep the full source hill on the chart"));
    TEST_EQUAL(data->m_statistics.m_length, length, ());
    TEST_EQUAL(manager.GetRoutePointAtDistance(-1.0), std::optional(MeterPoint(0, 0)), ());
    TEST_EQUAL(manager.GetRoutePointAtDistance(length + 1.0), std::optional(MeterPoint(1000, 0)), ());
  };
  checkPreview();

  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  auto const data = manager.GetRouteElevationData();
  TEST(data, ());
  TEST(!data->m_followedSection, ());
  TEST_LESS(data->m_statistics.m_maxElevation, 180, ("The earlier hill is outside the followed remainder"));
  TEST_EQUAL(manager.GetRoutePointAtDistance(0.0), std::optional(MeterPoint(500, 0)), ());

  TEST_EQUAL(manager.ResetFollowedTrackToPlanning(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  checkPreview();

  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  auto const reversed = manager.GetRouteElevationData();
  TEST(reversed, ());
  TEST_ALMOST_EQUAL_ABS(reversed->m_followedSection->m_entryPoint, 1.0 + 1.0 / 3.0, 1e-6, ());
  TEST_EQUAL(reversed->m_followedSection->m_finishPoint, 0, ());
  TEST_EQUAL(reversed->m_statistics.m_ascent, 20, ());
  TEST_EQUAL(reversed->m_statistics.m_descent, 80, ());
  TEST_EQUAL(reversed->m_statistics.m_minElevation, 100, ());
  TEST_EQUAL(reversed->m_statistics.m_maxElevation, 180, ());
  fixture.WaitForBuildResult();
}

UNIT_TEST(RoutingManager_TrackPreviewTotalsSpanJoinedLines)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 150}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(100, 0), 170}, {MeterPoint(200, 0), 140}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();

  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const full = bookmarks.GetTrack(id)->GetStatisticsAndElevationInfo().first;
  auto const checkTotals = [&](double ascent, double descent, int lowest, int highest)
  {
    auto const chart = manager.GetRouteElevationData();
    TEST(chart, ());
    TEST_EQUAL(chart->m_statistics.m_ascent, ascent, ());
    TEST_EQUAL(chart->m_statistics.m_descent, descent, ());
    TEST_EQUAL(chart->m_statistics.m_minElevation, lowest, ());
    TEST_EQUAL(chart->m_statistics.m_maxElevation, highest, ());
  };
  checkTotals(full.m_ascent, full.m_descent, full.m_minElevation, full.m_maxElevation);

  double const leg = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0));
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(0.5 * leg), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  // The two height samples at the joint describe the same location.
  checkTotals(25, 30, 125, 170);

  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(1.5 * leg), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  checkTotals(15, 50, 100, 170);
}

UNIT_TEST(RoutingManager_TrackChartKeepsMapEntryOnSparseSegment)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const from = mercator::FromLatLon(45.0, 19.0);
  auto const to = mercator::FromLatLon(45.25, 19.25);
  auto const selectedPoint = (from + to) / 2.0;
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{from, 100}, {to, 200}});
  auto const * track = bookmarks.GetTrack(id);
  Track::TrackSelectionInfo selection;
  track->UpdateSelectionInfo(selectedPoint, selection);
  bookmarks.SetTrackSelectionInfo(selection, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  double const distance = manager.GetFollowedTrackInfo()->m_entryDistanceM;
  TEST_ALMOST_EQUAL_ABS(distance, track->GetLengthMeters() * 0.5, 0.01, ());
  auto const marker = manager.GetRoutePointAtDistance(distance);
  TEST(marker && marker->EqualDxDy(selectedPoint, mercator::kPointEqualityEps), ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(distance), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const & route = manager.RoutingSession().GetRoute()->GetPoly();
  TEST(route.GetPoint(1).EqualDxDy(selectedPoint, mercator::kPointEqualityEps), ());
}

UNIT_TEST(RoutingManager_TrackEntryAndNavigationCrossTheAntimeridian)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  m2::PointD const start(179.99, 0);
  m2::PointD const finish(-179.99, 0);
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{start, 100}, {finish, 200}});
  auto const * track = bookmarks.GetTrack(id);
  auto const midpoint = track->GetLengthMeters() * 0.5;
  Track::TrackSelectionInfo selection;
  selection.m_squareDist = 1e-10;
  track->UpdateSelectionInfo({-180.0, 0.0}, selection);
  TEST(selection.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(selection.m_distFromBegM, midpoint, 0.01, ());

  bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
  fixture.SetGpsPosition({-179.995, 0.0});
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_LESS(manager.RoutingSession().GetRoute()->GetLimitRect().SizeX(), 0.03, ());
  auto const marker = manager.GetRoutePointAtDistance(midpoint);
  TEST(marker && AlmostEqualAbs(std::abs(marker->x), 180.0, 1e-9), ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(midpoint), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), midpoint, 0.01, ());
  fixture.SetGpsPosition({180.0, 0.0});
  manager.FollowRoute();
  fixture.WaitForBuildResult();
  TEST(manager.IsRoutingFollowing(), ());
  location::GpsInfo gps;
  gps.m_source = location::EUser;
  gps.m_horizontalAccuracy = 5.0;
  gps.m_latitude = 0.0;
  gps.m_longitude = -179.995;
  UpdateGps(fixture, gps);
  TEST_ALMOST_EQUAL_ABS(manager.RoutingSession().GetRoute()->GetCurrentDistanceToEndMeters(), midpoint * 0.5, 0.01, ());
}

UNIT_TEST(RoutingManager_SetRouterReplacesTrackRouter)
{
  for (bool const pendingStart : {false, true})
  {
    AsyncBuildFixture fixture;
    auto & manager = fixture.m_framework.GetRoutingManager();
    auto & bookmarks = fixture.m_framework.GetBookmarkManager();
    if (pendingStart)
      manager.SetRouter(routing::RouterType::Ruler);
    auto const id = CreateTestTrack(bookmarks, "pedestrian");
    auto const start = bookmarks.GetTrack(id)->GetPoint(0.0);
    bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
    fixture.SetGpsPosition(start);
    TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
    if (!pendingStart)
      fixture.WaitForBuildResult();
    manager.FollowRoute();
    TEST_EQUAL(manager.IsStartingFollowedTrack(), pendingStart, ());
    TEST_EQUAL(manager.IsRoutingFollowing(), !pendingStart, ());

    TestAssertion([&] { manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 0.001), false); });
    TestAssertion([&]
    { manager.ReplaceRoutePoint(RouteMarkType::Finish, 0, MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 0.001)); });
    TEST_EQUAL(manager.GetRoutePointsCount(), 0,
               ("Rejected road-point edits must leave the followed track without route marks"));
    TestAssertion([&] { manager.BuildRoute(); });
    TEST_EQUAL(manager.GetFollowedTrackInfo()->m_id, id, ());
    TEST_EQUAL(manager.IsStartingFollowedTrack(), pendingStart, ());
    manager.SetRouter(routing::RouterType::Ruler);
    TEST(!manager.IsFollowingTrack() && !manager.IsRoutingActive(), ());
    manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Start, 0, 0.002), false);
    manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 0.004), false);
    manager.BuildRoute();
    fixture.WaitForBuildResult();
    TEST(!manager.RoutingSession().GetRoute()->IsTrack(), ());
  }
}

UNIT_TEST(RoutingManager_TrackPreviewLocksEntryWhenGpsIsNearFinish)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  manager.SetRouter(routing::RouterType::Pedestrian);
  auto const id = CreateTestTrack(bookmarks, "bicycle", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  double const entryDistance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), entryDistance}, false);
  fixture.SetGpsPosition(MeterPoint(900, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST(route, ());
  TEST_EQUAL(route->GetPoly().Front(), MeterPoint(900, 0), ("Retain the actual GPS position"));
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0),
             ("Approach the captured entry, not nearest later geometry"));
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 600.0, 2.0, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ("Saved profile is session-only"));

  // Inspecting another graph point must not redefine the preview's original entry.
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(800, 0), 2.0 * entryDistance}, false);
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Vehicle), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, entryDistance, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ());
}

UNIT_TEST(RoutingManager_NavigateTrackFromFinish)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const start = MeterPoint(0, 0);
  auto const finish = MeterPoint(1000, 0);
  auto const id = CreateTestTrack(bookmarks, "bicycle", {}, {{start, 100}, {finish, 110}});
  auto const length = bookmarks.GetTrack(id)->GetLengthMeters();
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::InvalidSelection, ("No entry selected"));
  bookmarks.SetTrackSelectionInfo({id, finish, length}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), finish, ());
  TEST_EQUAL(route->GetPoly().Back(), start, ());
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, length, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::NoRemainingGeometry, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());
}

UNIT_TEST(RoutingManager_ReverseClosedSectionFromTrailhead)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const a = MeterPoint(0, 0), b = MeterPoint(625, 0), c = MeterPoint(625, 625), d = MeterPoint(0, 625);
  auto const recordedFinish = MeterPoint(5.4, 0);
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_geometry.AddLine({{MeterPoint(-200, 0), 100}, {MeterPoint(-100, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{a, 100}, {b, 110}, {c, 120}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{c, 120}, {d, 130}, {recordedFinish, 140}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(500, 0), 100}, {MeterPoint(600, 0), 100}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();
  auto const & lengths = bookmarks.GetTrack(id)->GetCumulativeLengths();
  double const start = lengths[1].front(), finish = lengths[2].back();
  double const selected = start + 3.0;
  bookmarks.SetTrackSelectionInfo({id, bookmarks.GetTrack(id)->GetPoint(selected), selected}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  auto const checkLap = [&](bool reverse)
  {
    fixture.WaitForBuildResult();
    auto const & info = *manager.GetFollowedTrackInfo();
    TEST(info.m_canReverse, ());
    TEST_EQUAL(info.m_reversed, reverse, ());
    TEST_EQUAL(info.m_entryDistanceM, reverse ? finish : start, ());
    auto const & line = manager.RoutingSession().GetRoute()->GetPoly();
    TEST_EQUAL(line.GetPoint(1), reverse ? recordedFinish : a, ());
    TEST_EQUAL(line.GetPoint(2), reverse ? d : b, ());
    TEST_EQUAL(line.Back(), reverse ? a : recordedFinish, ());
    TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), finish - start, 0.01, ());
    TEST_EQUAL(bookmarks.GetElevationActivePoint(id), selected, ());
  };
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, selected, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  checkLap(true);
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  checkLap(false);
}

UNIT_TEST(RoutingManager_TrackPreviewGpsDoesNotRebuildOrPassStops)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {{MeterPoint(600, 0), "Stop"}},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  auto const entryDistance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), entryDistance}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  location::GpsInfo info;
  info.m_source = location::EUser;
  auto const moved = mercator::ToLatLon(MeterPoint(1000, 0));
  info.m_latitude = moved.m_lat;
  info.m_longitude = moved.m_lon;
  info.m_horizontalAccuracy = 5;
  info.m_timestamp = 100;
  info.m_speed = 1;
  UpdateGps(fixture, info);
  TEST(!manager.IsRouteBuilding(), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST(!manager.IsRouteFinished(), ());
  TEST_EQUAL(route->GetCurrentSubrouteIdx(), 0, ());
  TEST_EQUAL(route->GetSubrouteCount(), 2, ());
}

UNIT_TEST(RoutingManager_DeletedTrackCategoryKeepsFrozenPreview)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const category = bookmarks.CreateBookmarkCategory("Followed track", false /* autoSave */);
  auto const id = CreateTestTrack(bookmarks, "pedestrian");
  bookmarks.GetEditSession().AttachTrack(id, category);
  auto const start = bookmarks.GetTrack(id)->GetGeometry().front().GetPoint();
  bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const length = bookmarks.GetTrack(id)->GetLengthMeters();

  TEST(bookmarks.GetEditSession().DeleteBmCategory(category, true /* permanently */), ());
  TEST(!bookmarks.GetTrack(id), ());
  TEST(manager.IsFollowingTrack(), ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_id, id, ());
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::NoTrack, ());
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(100.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.ResetFollowedTrackToPlanning(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), length - 100.0, 0.01, ());
  manager.CloseRouting(true);
  TEST(!manager.GetFollowedTrackInfo(), ());
}

UNIT_TEST(RoutingManager_TemporaryTrackReplacementKeepsFrozenPreview)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_routerType = "pedestrian";
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 200}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.SetTempRelationTrack(std::move(source));
  auto replacement = bookmarks.GetTrack(id)->GetData();
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  // Selecting another OSM relation replaces the temporary track under the same ID.
  replacement.m_geometry.m_lines[0] = {{MeterPoint(0, 0), 500}, {MeterPoint(2000, 0), 700}};
  TEST_EQUAL(bookmarks.SetTempRelationTrack(std::move(replacement)), id, ());
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(100.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const chart = manager.GetRouteElevationData();
  TEST(chart, ());
  TEST_EQUAL(chart->m_statistics.m_maxElevation, 200, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(1000, 0), ());

  bookmarks.ClearTempRelationTrack();
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Pedestrian), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(200.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(1000, 0), ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(0, 0), ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 200.0, 0.01, ());
  manager.CloseRouting(true);
}

UNIT_TEST(RoutingManager_TrackPreviewEditsReplacePendingBuild)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  TEST(manager.IsRouteBuilding(), ());

  auto const position = MeterPoint(100, 0);
  fixture.SetGpsPosition(position);
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(400.0), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(600.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  auto const * route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().Front(), position, ());
  TEST(route->GetPoly().GetPoint(1).EqualDxDy(bookmarks.GetTrack(id)->GetPoint(600.0), mercator::kPointEqualityEps),
       ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 600.0, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), bookmarks.GetTrack(id)->GetLengthMeters() - 600.0, 0.01,
                        ());
  TEST(!manager.IsRoutingFollowing() && !manager.IsStartingFollowedTrack(), ());
}

UNIT_TEST(RoutingManager_TrackStartRefreshesGpsWithoutChangingEntry)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(400, 0), true);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  // Start can be pressed before the renderer moves the position mark to the new fix.
  fixture.SetGpsPosition(MeterPoint(900, 0));
  manager.FollowRoute();
  TEST(fixture.m_framework.m_followProfiles.empty(), ());
  TEST(!manager.IsRoutingFollowing(), ("A stale approach must be refreshed before Start"));
  TEST(manager.IsRouteBuilding(), ());
  TEST(manager.IsStartingFollowedTrack(), ());
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(600.0), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.ResetFollowedTrackToPlanning(), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Busy, ());
  TEST(manager.IsStartingFollowedTrack(), ("Refused edits retain the requested Start"));
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Front(), MeterPoint(900, 0), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().GetPoint(1), MeterPoint(400, 0), ());
  TEST(manager.IsRoutingFollowing(), ());
  TEST(!manager.IsStartingFollowedTrack(), ());
  TEST_EQUAL(fixture.m_framework.m_followProfiles, std::vector{routing::RouterType::Pedestrian}, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Vehicle, ());
}

UNIT_TEST(RoutingManager_TrackStartUsesLiveDistanceForTheApproachPrompt)
{
  for (bool const expectApproach : {false, true})
  {
    AsyncBuildFixture fixture;
    auto & manager = fixture.m_framework.GetRoutingManager();
    auto & bookmarks = fixture.m_framework.GetBookmarkManager();
    auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 10}, {MeterPoint(1000, 0), 20}});
    bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
    fixture.SetGpsPosition(MeterPoint(expectApproach ? -10.0 : -25.0, 0));
    TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
    fixture.WaitForBuildResult();
    manager.RoutingSession().SetLocaleWithJsonForTesting(R"({"proceed_to_the_track":"Proceed to the track."})", "en");
    manager.SetTurnNotificationsUnits(measurement_utils::Units::Metric);
    manager.EnableTurnNotifications(true);
    fixture.SetGpsPosition(MeterPoint(expectApproach ? -25.0 : -10.0, 0));
    manager.FollowRoute();
    if (manager.IsRouteBuilding())
      fixture.WaitForBuildResult();
    TEST(manager.IsRoutingFollowing(), ());
    std::vector<std::string> notifications;
    manager.GenerateNotifications(notifications, false);
    TEST_EQUAL(std::ranges::find(notifications, "Proceed to the track.") != notifications.end(), expectApproach,
               (notifications));
  }
}

UNIT_TEST(RoutingManager_TrackStartWaitsForTheLatestApproach)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  size_t readyNotifications = 0;
  bool notifiedWhileBuilding = false;
  manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
  {
    TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
    ++readyNotifications;
    notifiedWhileBuilding |= manager.IsRouteBuilding();
  });
  SCOPE_GUARD(clearListener, [&] { manager.SetRouteBuildingListener({}); });

  fixture.SetGpsPosition(MeterPoint(900, 0));
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(900, 0), true);
  manager.FollowRoute();
  TEST(manager.IsStartingFollowedTrack(), ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy, ());
  // A new fix arrives while the approach is being built and before the mark moves.
  fixture.SetGpsPosition(MeterPoint(200, 0));
  TEST(fixture.m_guiLoop.m_loop->PumpUntil([&] { return manager.IsRoutingFollowing(); }, kAsyncBuildTimeout), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Front(), MeterPoint(200, 0), ());
  TEST_EQUAL(readyNotifications, 1, ());
  TEST(!notifiedWhileBuilding, ("The planner must stay in its building state until Start can complete"));
}

UNIT_TEST(RoutingManager_TrackStartCancelsWhenPositionIsLost)
{
  AsyncBuildFixture fixture;
  auto & framework = fixture.m_framework;
  auto & manager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  size_t readyNotifications = 0;
  manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
  {
    TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
    ++readyNotifications;
  });
  SCOPE_GUARD(clearListener, [&] { manager.SetRouteBuildingListener({}); });

  fixture.SetGpsPosition(MeterPoint(900, 0));
  manager.FollowRoute();
  TEST(manager.IsRouteBuilding() && manager.IsStartingFollowedTrack(), ());
  framework.OnLocationError(location::EDenied);
  TEST(fixture.m_guiLoop.m_loop->PumpUntil([&] { return readyNotifications != 0; }, kAsyncBuildTimeout), ());
  TEST_EQUAL(readyNotifications, 1, ());
  TEST(manager.IsRouteBuilt() && !manager.IsStartingFollowedTrack() && !manager.IsRoutingFollowing(), ());
  TEST(framework.m_followProfiles.empty(), ());
}

UNIT_TEST(RoutingManager_LocationLossDiscardsQueuedFix)
{
  base::Waiter queued;
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const start = MeterPoint(0, 0);
  auto const finish = MeterPoint(100, 0);
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{start, 100}, {finish, 100}});
  bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
  fixture.SetGpsPosition(start);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  manager.FollowRoute();

  fixture.SetGpsPosition(finish);
  GetPlatform().RunTask(Platform::Thread::Background, [&] { queued.Notify(); });
  TEST_NOT_EQUAL(queued.Wait(kAsyncBuildTimeout), base::Waiter::Result::Timeout, ());
  fixture.m_framework.OnLocationError(location::EDenied);
  fixture.m_guiLoop.m_loop->Drain();
  TEST(!manager.IsRouteFinished(), ());

  fixture.SetGpsPosition(finish);
  fixture.m_guiLoop.m_loop->RunUntil([&] { return manager.IsRouteFinished(); });
}

UNIT_TEST(RoutingManager_TrackStartDoesNotSurvivePreviewReplacement)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  fixture.SetGpsPosition(MeterPoint(900, 0));
  manager.FollowRoute();
  TEST(manager.IsStartingFollowedTrack(), ());
  manager.CloseRouting(true);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(!manager.IsStartingFollowedTrack() && !manager.IsRoutingFollowing(), ());
  TEST(fixture.m_framework.m_followProfiles.empty(), ("The cancelled Start cannot enter the replacement preview"));
}

UNIT_TEST(RoutingManager_TrackStartRequiresCurrentPosition)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  auto const checkStartRefused = [&]
  {
    manager.FollowRoute();
    TEST(!manager.IsRoutingFollowing() && !manager.IsStartingFollowedTrack(), ());
    TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());
    TEST(fixture.m_framework.m_followProfiles.empty(), ());
  };
  checkStartRefused();

  fixture.SetGpsPosition(MeterPoint(0, 0));
  bookmarks.MyPositionMark().SetUserPosition(MeterPoint(0, 0), true);
  fixture.m_framework.OnLocationError(location::EDenied);
  checkStartRefused();

  fixture.SetGpsPosition(MeterPoint(0, 0));
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(fixture.m_framework.m_followProfiles, std::vector{routing::RouterType::Pedestrian}, ());
}

UNIT_TEST(RoutingManager_TrackNavigationKeepsGeometryAcrossDepartureAndReturn)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  fixture.SetGpsPosition(MeterPoint(0, 0));
  manager.FollowRoute();
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
    UpdateGps(fixture, info);
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

UNIT_TEST(RoutingManager_MapChangesPreserveTrackPreviewAndNavigation)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());

  // Both map-download and map-deletion callbacks use this soft reset.
  manager.ResetRoutingSession();
  TEST(manager.IsFollowingTrack() && manager.IsRouteBuilding(), ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  manager.ResetRoutingSession();
  TEST(manager.IsFollowingTrack() && manager.IsRouteBuilt(), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());

  manager.FollowRoute();
  manager.ResetRoutingSession();
  TEST(manager.IsFollowingTrack() && manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_id, id, ());

  manager.CloseRouting(true);
  manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Start, 0, 0.0), false);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 0.01), false);
  manager.BuildRoute();
  fixture.WaitForBuildResult();
  manager.ResetRoutingSession();
  TEST(!manager.IsRoutingActive(), ("Ordinary routes still reset when map data changes"));
}

UNIT_TEST(RoutingManager_ExplicitTrackProfileSurvivesLaterSessions)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const withoutProfile = CreateTestTrack(bookmarks);
  auto const withProfile = CreateTestTrack(bookmarks, "bicycle");
  auto const withUnknownProfile = CreateTestTrack(bookmarks, "walking");
  auto const start = bookmarks.GetTrack(withoutProfile)->GetGeometry().front().GetPoint();
  auto const preview = [&](kml::TrackId id)
  {
    bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
    TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
    fixture.WaitForBuildResult();
  };

  preview(withoutProfile);
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Vehicle, ("Initially use the road default"));
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Bicycle), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Pedestrian), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian, ("The latest profile wins"));
  manager.CloseRouting(true);

  preview(withProfile);
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ("Honor an explicit KML profile"));
  manager.CloseRouting(true);
  preview(withoutProfile);
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian,
             ("Remember the explicit choice, not the last track's KML profile"));
  preview(withUnknownProfile);
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian, ());
  TEST_EQUAL(manager.GetRouter(), routing::RouterType::Vehicle, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Vehicle, ());
  TEST(bookmarks.GetTrack(withoutProfile)->GetData().m_routerType.empty(), ());
}

UNIT_TEST(RoutingManager_NavigateTrackRefusesActiveNavigation)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));

  manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Start, 0, 0.0), false);
  manager.AddRoutePoint(MakeRoutePointOnXAxis(RouteMarkType::Finish, 0, 0.01), false);
  manager.BuildRoute();
  fixture.WaitForBuildResult();
  manager.FollowRoute();
  auto const * roadRoute = manager.RoutingSession().GetRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Busy, ());
  TEST(manager.IsRoutingFollowing() && !manager.IsFollowingTrack(), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), roadRoute, ());
  TEST_EQUAL(manager.GetRoutePointsCount(), 2, ());
  manager.CloseRouting(true);

  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ("Pending previews can be replaced"));
  fixture.WaitForBuildResult();
  manager.FollowRoute();
  auto const * trackRoute = manager.RoutingSession().GetRoute();
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Busy, ());
  TEST(manager.IsRoutingFollowing() && manager.IsFollowingTrack(), ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), trackRoute, ());
}

UNIT_TEST(RoutingManager_NavigateTrackDistinguishesGapSides)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 130}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(500, 0), 210}, {MeterPoint(600, 0), 190}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_endsAtGap, ());
  TEST_EQUAL(manager.GetRouteElevationData()->m_statistics.m_ascent, 30, ());
  TEST_EQUAL(manager.GetRouteElevationData()->m_statistics.m_maxElevation, 130, ());
  double const boundary = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(100, 0), boundary}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(0, 0), ());
  TEST(!manager.GetFollowedTrackInfo()->m_endsAtGap, ());
  auto const beforeGap = manager.GetRouteElevationData();
  TEST_EQUAL(beforeGap->m_followedSection->m_entryPoint, 1.0, ());
  TEST_EQUAL(beforeGap->m_followedSection->m_finishPoint, 0, ());
  TEST_EQUAL(beforeGap->m_statistics.m_descent, 30, ());
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(500, 0), boundary}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(!manager.GetFollowedTrackInfo()->m_endsAtGap, ());
  auto const afterGap = manager.GetRouteElevationData();
  TEST_EQUAL(afterGap->m_followedSection->m_entryPoint, 2.0, ());
  TEST_EQUAL(afterGap->m_followedSection->m_finishPoint, 3, ());
  TEST_EQUAL(afterGap->m_statistics.m_descent, 20, ());
  TEST_EQUAL(afterGap->m_statistics.m_minElevation, 190, ());
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(500, 0), ());
  TEST_EQUAL(route->GetPoly().Back(), MeterPoint(600, 0), ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 100.0, 1.0, ());
  TEST_EQUAL(manager.GetRoutePointAtDistance(boundary), std::optional(MeterPoint(500, 0)), ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(boundary), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().GetPoint(1), MeterPoint(500, 0), ());

  double const precedingDistance = std::nextafter(boundary, 0.0);
  auto const precedingPoint = manager.GetRoutePointAtDistance(precedingDistance);
  TEST(precedingPoint && precedingPoint->EqualDxDy(MeterPoint(100, 0), mercator::kPointEqualityEps), ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(precedingDistance), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(route->GetPoly().Back(), MeterPoint(0, 0), ());

  auto const stationary =
      CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(0, 0), 200}});
  bookmarks.SetTrackSelectionInfo({stationary, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(stationary), FollowTrackResult::NoRemainingGeometry, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ("Unusable geometry must not replace the valid preview"));
}

UNIT_TEST(RoutingManager_TrackPreviewEntryDistancePreservesMapSelectionAndProfile)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "bicycle", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  auto const source = bookmarks.GetTrack(id)->GetData();
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Pedestrian), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  std::vector<double> notifications;
  bookmarks.SetElevationActivePointChangedCallback([&](kml::TrackId changedId, double distance)
  {
    TEST_EQUAL(changedId, id, ());
    TEST_EQUAL(bookmarks.GetTrackSelectionInfo(id).m_distFromBegM, distance, ());
    notifications.push_back(distance);
  });
  SCOPE_GUARD(clearCallback, [&] { bookmarks.SetElevationActivePointChangedCallback({}); });

  double const legLength = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(1000, 0));
  for (double const distance : {1.6 * legLength, 1.5 * legLength})
  {
    TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(distance), FollowTrackResult::Ok, ());
    TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 0.0, ());
    TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, distance, ());
    fixture.WaitForBuildResult();
    TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian, ());
    TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 2.0 * legLength - distance, 0.01,
                          ("Resolve the return visit using the full source distance"));
    auto const & points = manager.RoutingSession().GetRoute()->GetPoly();
    TEST(points.GetPoint(1).EqualDxDy(bookmarks.GetTrack(id)->GetPoint(distance), mercator::kPointEqualityEps), ());
  }
  TEST(notifications.empty(), ("Entry edits do not change the inspected map selection"));

  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.ResetFollowedTrackToPlanning(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(!manager.IsRoutingFollowing(), ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 0.5 * legLength, 0.01, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian, ());

  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(-100.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 0.0, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 0.0, ());
  TEST_EQUAL(bookmarks.GetTrackSelectionInfo(id).m_trackPoint, MeterPoint(0, 0), ());
  TEST(notifications.empty(), ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ("Entry updates leave the saved track unchanged"));
}

UNIT_TEST(RoutingManager_TrackPreviewEntryDistanceRefusalsPreserveState)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  double const entryDistance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), entryDistance}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());

  size_t notifications = 0;
  bookmarks.SetElevationActivePointChangedCallback([&](kml::TrackId, double) { ++notifications; });
  SCOPE_GUARD(clearCallback, [&] { bookmarks.SetElevationActivePointChangedCallback({}); });
  fixture.WaitForBuildResult();
  bookmarks.SetElevationActivePoint(id, 800.0);
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, entryDistance,
             ("Inspection preserves the captured entry"));
  notifications = 0;

  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(600.0), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), route, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, entryDistance, ());
  TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 800.0, ());

  bookmarks.GetEditSession().DeleteTrack(id);
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(600.0), FollowTrackResult::Busy, ());
  manager.DisableFollowMode();
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(600.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 600.0, ());
  TEST_EQUAL(notifications, 0, ());
}

UNIT_TEST(RoutingManager_TrackPreviewEntryDistanceUsesChartGapBoundary)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(500, 0), 100}, {MeterPoint(600, 0), 100}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  double const boundary = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0));
  TEST_EQUAL(manager.GetRoutePointAtDistance(boundary), std::optional(MeterPoint(100, 0)), ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(boundary), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST(!manager.GetFollowedTrackInfo()->m_canReverse, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(0, 0), ());
  TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 0.0, ());

  double const nextEntry = boundary + mercator::DistanceOnEarth(MeterPoint(500, 0), MeterPoint(550, 0));
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(nextEntry), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_endsAtGap, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, nextEntry, ());
  TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 0.0, ());
  auto const marker = manager.GetRoutePointAtDistance(nextEntry);
  TEST(marker && marker->EqualDxDy(MeterPoint(550, 0), mercator::kPointEqualityEps), ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 50.0, 1.0, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute()->GetPoly().Back(), MeterPoint(500, 0), ());
}

UNIT_TEST(RoutingManager_TrackPreviewEntryDistancePreservesDirection)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "bicycle", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  auto const source = bookmarks.GetTrack(id)->GetData();
  double const legLength = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(1000, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), 0.4 * legLength}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.SetFollowTrackProfile(routing::RouterType::Pedestrian), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();

  size_t notifications = 0;
  bookmarks.SetElevationActivePointChangedCallback([&](kml::TrackId, double) { ++notifications; });
  SCOPE_GUARD(clearCallback, [&] { bookmarks.SetElevationActivePointChangedCallback({}); });
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(1.6 * legLength), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 1.6 * legLength, 0.01,
                        ("Keep the later occurrence while following in reverse"));
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Pedestrian, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 1.6 * legLength, ());

  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(2.0 * legLength + 100.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 2.0 * legLength, 0.01, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 2.0 * legLength, ("Commit the clamped source endpoint"));
  TEST_EQUAL(bookmarks.GetElevationActivePoint(id), 0.4 * legLength, ());
  TEST_EQUAL(notifications, 0, ());
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.ResetFollowedTrackToPlanning(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 2.0 * legLength, 0.01, ());
  TEST_EQUAL(manager.UpdateFollowedTrackEntryAt(0.0), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST(manager.GetFollowedTrackInfo()->m_canReverse, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, 0.0, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 2.0 * legLength, 0.01, ());
  TEST_EQUAL(notifications, 0, ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ());
}

UNIT_TEST(RoutingManager_ReverseTrackPreviewKeepsCapturedEntryAndProfile)
{
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
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 600.0, 2.0, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  TEST_ALMOST_EQUAL_ABS(route->GetTotalDistanceMeters(), 400.0, 2.0, ());
  TEST(route->GetPoly().Front().EqualDxDy(MeterPoint(400, 0), mercator::kPointEqualityEps), ());
  TEST_EQUAL(route->GetPoly().Back(), MeterPoint(0, 0), ());
  TEST_EQUAL(route->GetSubrouteCount(), 3, ("Keep the entry and earlier-direction stop"));
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_EQUAL(manager.GetRenderRouterType(), routing::RouterType::Bicycle, ());
  TEST_EQUAL(manager.GetLastUsedRouter(), routing::RouterType::Pedestrian, ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ());
}

UNIT_TEST(RoutingManager_ReverseTrackWaitsForPendingBuilds)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy, ("Wait for the initial preview"));
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy, ("Ignore a second reversal while building"));
  fixture.WaitForBuildResult();
}

UNIT_TEST(RoutingManager_DoubleReverseKeepsTheEntryAfterSelectionAndGpsChange)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "bicycle", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 200}});
  auto const source = bookmarks.GetTrack(id)->GetData();
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(400, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0))}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  bookmarks.SetTrackSelectionInfo(
      {id, MeterPoint(800, 0), mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(800, 0))}, false);
  fixture.SetGpsPosition(MeterPoint(900, 0));
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * route = manager.RoutingSession().GetRoute();
  // The approach starts from current GPS; the captured entry remains its first endpoint.
  TEST(route->GetPoly().Front().EqualDxDy(MeterPoint(900, 0), mercator::kPointEqualityEps), ());
  TEST(route->GetPoly().GetPoint(1).EqualDxDy(MeterPoint(400, 0), mercator::kPointEqualityEps), ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 600.0, 2.0, ());
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_EQUAL(bookmarks.GetTrack(id)->GetData(), source, ());
}

UNIT_TEST(RoutingManager_ReverseTrackRefusalsPreservePreview)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {}, {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), 0.0}, false);
  fixture.SetGpsPosition(MeterPoint(0, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  auto const * original = manager.RoutingSession().GetRoute();
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::NoRemainingGeometry, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  manager.FollowRoute();
  TEST(manager.IsRoutingFollowing(), ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Busy, ());
  TEST_EQUAL(manager.RoutingSession().GetRoute(), original, ());
}

UNIT_TEST(RoutingManager_ReverseTrackUsesSelectedOccurrenceOnReturnSection)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  auto const id = CreateTestTrack(bookmarks, "pedestrian", {},
                                  {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  double const distance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(1000, 0)) +
                          mercator::DistanceOnEarth(MeterPoint(1000, 0), MeterPoint(400, 0));
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(400, 0), distance}, false);
  fixture.SetGpsPosition(MeterPoint(400, 0));
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 400.0, 2.0, ());
  TEST_EQUAL(manager.ReverseFollowedTrack(), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 1600.0, 4.0, ());
  auto const & points = manager.RoutingSession().GetRoute()->GetPoly();
  TEST_EQUAL(points.GetPoint(2), MeterPoint(1000, 0), ("Walk backwards through the exact second source occurrence"));
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
    guiThread.RunUntil([&] { return built; });
    TEST(routingManager.IsRouteValid(), ());
    TEST(!routingManager.GetRouteElevationData(), ());

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
    for (auto const & point : track->GetGeometry())
      TEST_EQUAL(point.GetAltitude(), geometry::kInvalidAltitude, ());
  });
}

UNIT_TEST(RoutingManager_RecoveryOperationsCompleteBeforeReturning)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    manager.SaveRoutePoints();
    TEST(manager.HasSavedRoutePoints(), ());
    manager.RemoveRoutePoints();
    TEST(manager.LoadRoutePoints(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 2, ());
    TEST(!manager.HasSavedRoutePoints(), ());

    manager.SaveRoutePoints();
    manager.DeleteSavedRoutePoints();
    TEST(!manager.HasSavedRoutePoints(), ());
    manager.RemoveRoutePoints();
    TEST(!manager.LoadRoutePoints(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 0, ());
  });
}

UNIT_TEST(RoutingManager_RecoveryDoesNotReplaceANewItinerary)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    manager.SaveRoutePoints();
    manager.RemoveRoutePoints();
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 3), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 4), false);
    TEST(!manager.LoadRoutePoints(), ());
    auto const points = manager.GetRoutePoints();
    TEST_EQUAL(points.size(), 2, ());
    TEST_EQUAL(points.front().m_position, m2::PointD(3, 3), ());
    TEST_EQUAL(points.back().m_position, m2::PointD(4, 4), ());
    TEST(manager.HasSavedRoutePoints(), ());
    manager.DeleteSavedRoutePoints();
    manager.CloseRouting(true);
    TEST(!manager.LoadRoutePoints(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 0, ());
  });
}

UNIT_TEST(RoutingManager_PartialAndFinishedRoutesDiscardRecovery)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread &)
  {
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    manager.SaveRoutePoints();
    manager.RemoveRoutePoint(RouteMarkType::Finish);
    manager.SaveRoutePoints();
    TEST(!manager.HasSavedRoutePoints(), ());

    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    manager.SaveRoutePoints();
    TEST(manager.HasSavedRoutePoints(), ());
    manager.OnRoutePointPassed(RouteMarkType::Finish, 0);
    TEST(!manager.HasSavedRoutePoints(), ());
  });
}

UNIT_TEST(RoutingManager_RecoveryWhileActivePreservesTheSnapshot)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    bool built = false;
    manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST(code == routing::RouterResultCode::NoError, ());
      built = true;
    });
    manager.BuildRoute();
    gui.RunUntil([&] { return built; });
    TEST(manager.RoutingSession().EnableFollowMode(), ());
    manager.SaveRoutePoints();
    manager.RemoveRoutePoints();
    TEST(manager.IsRoutingActive(), ("Point edits do not retire the followed session."));
    TEST(!manager.LoadRoutePoints(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 0, ());
    TEST(manager.HasSavedRoutePoints(), ());
    manager.CloseRouting(true);
    TEST(manager.LoadRoutePoints(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 2, ());
    TEST(!manager.HasSavedRoutePoints(), ());
  });
}

UNIT_TEST(RoutingManager_ValidationObserverCanStartANewBuild)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
    bool requested = false;
    bool built = false;
    manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      if (!requested)
      {
        TEST(code == routing::RouterResultCode::Cancelled, ());
        TEST(!manager.IsRoutingActive(), ());
        requested = true;
        manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
        manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
        manager.BuildRoute();
      }
      else
      {
        TEST(code == routing::RouterResultCode::NoError, ());
        built = true;
      }
    });
    manager.BuildRoute();
    TEST(requested, ());
    TEST(manager.IsRouteBuilding(), ());
    gui.RunUntil([&] { return built; });
    TEST(manager.IsRouteBuilt(), ());
  });
}

UNIT_TEST(RoutingManager_CheckpointPassUpdatesRecoveryBeforeReturning)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    manager.SaveRoutePoints();
    TEST(manager.HasSavedRoutePoints(), ());
    bool built = false;
    manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST(code == routing::RouterResultCode::NoError, ());
      built = true;
    });
    manager.BuildRoute();
    gui.RunUntil([&] { return built; });
    auto & session = manager.RoutingSession();
    TEST(session.EnableFollowMode(), ());
    location::GpsInfo info;
    info.m_longitude = 2;
    info.m_latitude = mercator::YToLat(2);
    info.m_horizontalAccuracy = 1;
    session.OnLocationPositionChanged(info);
    TEST(session.IsFinished(), ());
    TEST(manager.GetRoutePoints().back().m_isPassed, ());
    TEST(!manager.HasSavedRoutePoints(), ());
  });
}

UNIT_TEST(RoutingManager_RemovedCheckpointDoesNotUpdateRecovery)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 2), false);
    bool built = false;
    manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST(code == routing::RouterResultCode::NoError, ());
      built = true;
    });
    manager.BuildRoute();
    gui.RunUntil([&] { return built; });
    auto & session = manager.RoutingSession();
    TEST(session.EnableFollowMode(), ());
    manager.SaveRoutePoints();
    manager.RemoveRoutePoint(RouteMarkType::Finish);
    TEST(session.IsFollowing(), ("Editing marks does not reset the session."));

    location::GpsInfo info;
    info.m_longitude = 2;
    info.m_latitude = mercator::YToLat(2);
    info.m_horizontalAccuracy = 1;
    session.OnLocationPositionChanged(info);
    TEST(session.IsFinished(), ());
    TEST_EQUAL(manager.GetRoutePointsCount(), 1, ());
    TEST(!manager.GetRoutePoints().front().m_isPassed, ());
    TEST(manager.HasSavedRoutePoints(), ("A removed checkpoint must not save an unrelated layout."));
  });
}

UNIT_TEST(RoutingManager_RebuildAfterEditingNavigationRetiresOldCheckpoints)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager &, TestGuiThread & gui)
  {
    manager.RoutingSession().SetRouter(std::make_unique<routing::RulerRouter>(), nullptr);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 1), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 0, 2), false);
    manager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 3), false);
    bool built = false;
    manager.SetRouteBuildingListener([&](routing::RouterResultCode code, storage::CountriesSet const &)
    {
      TEST(code == routing::RouterResultCode::NoError, ());
      built = true;
    });
    manager.BuildRoute();
    gui.RunUntil([&] { return built; });
    auto & session = manager.RoutingSession();
    TEST(session.EnableFollowMode(), ());

    manager.RemoveRoutePoint(RouteMarkType::Intermediate, 0);
    built = false;
    manager.BuildRoute();
    TEST(!session.IsFollowing(), ());
    location::GpsInfo info;
    info.m_longitude = 2;
    info.m_latitude = mercator::YToLat(2);
    info.m_horizontalAccuracy = 1;
    manager.CheckLocationForRouting(info);
    TEST(manager.IsRouteBuilding(), ());
    TEST(!manager.GetRoutePoints().back().m_isPassed, ());
    gui.RunUntil([&] { return built; });

    TEST(session.EnableFollowMode(), ());
    manager.SaveRoutePoints();
    manager.CheckLocationForRouting(info);
    TEST(!session.IsFinished(), ());
    TEST(!manager.GetRoutePoints().back().m_isPassed, ());
    info.m_longitude = 3;
    info.m_latitude = mercator::YToLat(3);
    manager.CheckLocationForRouting(info);
    TEST(session.IsFinished(), ());
    TEST(manager.GetRoutePoints().back().m_isPassed, ());
    TEST(!manager.HasSavedRoutePoints(), ());
  });
}

UNIT_TEST(RoutingManager_SaveRoutePreservesSingleSegmentGeometry)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
  {
    geometry::PointWithAltitude const start({0.0, 0.0}, 100);
    geometry::PointWithAltitude const finish({0.001, 0.0}, 110);
    auto const id = SaveRouteGeometry(manager, bookmarks, {start, finish});
    TEST_NOT_EQUAL(id, kml::kInvalidTrackId, ());
    auto const * track = bookmarks.GetTrack(id);
    TEST(track != nullptr, ());
    TEST(manager.GetRouteElevationData(), ());
    TEST(track->GetElevationInfo(), ());
    TEST_EQUAL(track->GetData().m_geometry.m_lines.size(), 1, ());
    TEST_EQUAL(track->GetGeometry(), (kml::TrackGeometry{start, finish}), ());
    TEST_EQUAL(track->GetData().m_routerType, "ruler", ());
  });
}

UNIT_TEST(RoutingManager_SaveRouteWithMissingStartHidesElevation)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
  {
    kml::TrackGeometry const points = {{{19, 0}, geometry::kInvalidAltitude}, {{19.001, 0}, 100}, {{19.002, 0}, 110}};
    auto const id = SaveRouteGeometry(manager, bookmarks, points);
    TEST_NOT_EQUAL(id, kml::kInvalidTrackId, ());
    TEST(!manager.GetRouteElevationData(), ());
    auto const * track = bookmarks.GetTrack(id);
    TEST(track, ());
    TEST_EQUAL(track->GetGeometry(), points, ());
    TEST(!track->GetElevationInfo(), ());
  });
}

UNIT_TEST(RoutingManager_RoutePreviewHidesZeroOnlyProfilesAfterSimplification)
{
  struct Case
  {
    geometry::Altitude m_endAltitude;
    geometry::Altitude m_midAltitude;
    bool m_expectPreview;
  };
  Case const cases[] = {{0, 0, false}, {0, 1, false}, {30, 30, true}, {0, 5, true}};
  for (auto const & [endAltitude, midAltitude, expectPreview] : cases)
  {
    WithSavedRoute([&](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
    {
      kml::TrackGeometry const points = {
          {{19, 0}, endAltitude}, {{19.001, 0}, midAltitude}, {{19.002, 0}, endAltitude}};
      auto const id = SaveRouteGeometry(manager, bookmarks, points);
      TEST_NOT_EQUAL(id, kml::kInvalidTrackId, (endAltitude, midAltitude));
      auto const * track = bookmarks.GetTrack(id);
      TEST(track, ());
      TEST_EQUAL(track->GetGeometry(), points, ());

      TEST_EQUAL(manager.GetRouteElevationData().has_value(), expectPreview, (endAltitude, midAltitude));
    });
  }
}

UNIT_TEST(RoutingManager_SaveRouteRejectsCollapsedGeometry)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
  {
    for (auto const x : {0.0, 1e-7})
    {
      for (auto const altitude : {100, 110})
      {
        geometry::PointWithAltitude const finish({x, 0.0}, altitude);
        TEST_EQUAL(SaveRouteGeometry(manager, bookmarks, {{{0.0, 0.0}, 100}, finish}), kml::kInvalidTrackId,
                   (x, altitude));
      }
    }
  });
}

UNIT_TEST(RoutingManager_SaveRouteDeduplicatesWithoutLosingMovement)
{
  WithSavedRoute([](RoutingManager & manager, BookmarkManager & bookmarks, TestGuiThread &)
  {
    using Line = kml::TrackGeometry;
    std::pair<Line, Line> const cases[] = {
        {Line{{{0.0, 0.0}, 100}, {{0.0, 0.0}, 100}, {{0.001, 0.0}, 100}}, Line{{{0.0, 0.0}, 100}, {{0.001, 0.0}, 100}}},
        {Line{{{19.0, 0.0}, 100}, {{19.000009, 0.0}, 100}, {{19.000017, 0.0}, 100}},
         Line{{{19.0, 0.0}, 100}, {{19.000017, 0.0}, 100}}},
        {Line{{{-180.0, 0.0}, 100}, {{179.999991, 0.0}, 100}, {{179.999983, 0.0}, 100}},
         Line{{{-180.0, 0.0}, 100}, {{179.999983, 0.0}, 100}}},
        {Line{{{-180.0, 0.0}, 100}, {{179.999991, 0.0}, 200}, {{179.999983, 0.0}, 200}},
         Line{{{-180.0, 0.0}, 100}, {{179.999983, 0.0}, 200}}}};
    for (auto const & [source, expected] : cases)
    {
      auto const id = SaveRouteGeometry(manager, bookmarks, source);
      TEST_NOT_EQUAL(id, kml::kInvalidTrackId, (source));
      auto const * track = bookmarks.GetTrack(id);
      TEST(track, (source));
      TEST_EQUAL(track->GetGeometry(), expected, (source));

      kml::FileData data;
      data.m_tracksData.push_back(track->GetData());
      std::string kmlText;
      MemWriter writer(kmlText);
      TEST(SaveKmlData(data, writer, FileType::Kml), (source));
      auto const reloaded = LoadKmlData(MemReader(kmlText), FileType::Kml);
      TEST(reloaded, (source));
      TEST_EQUAL(reloaded->m_tracksData.size(), 1, (source));
      TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, track->GetData().m_geometry, (source));
    }
  });
}

UNIT_TEST(RoutingManager_TrackCommandsRequireRoutingAndCapturedSession)
{
  ScopedVehicleRouter const routerSetting;
  {
    Framework framework({}, false /* loadMaps */);
    auto & manager = framework.GetRoutingManager();
    TEST(!manager.GetFollowedTrackInfo(), ());
    TestAssertion([&] { manager.NavigateTrack(kml::kInvalidTrackId); });
    TestAssertion([&] { manager.BuildRoute(); });
    TEST_EQUAL(manager.GetRoutePointsCount(), 0, ());
  }
  TestFramework framework;
  auto & manager = framework.GetRoutingManager();
  TestAssertion([&] { manager.ResetFollowedTrackToPlanning(); });
  TestAssertion([&] { manager.UpdateFollowedTrackEntryAt(0.0); });
  TestAssertion([&] { manager.ReverseFollowedTrack(); });
  TestAssertion([&] { manager.SetFollowTrackProfile(routing::RouterType::Pedestrian); });
  TEST(!manager.IsFollowingTrack(), ());
  TEST_EQUAL(manager.GetRoutePointsCount(), 0, ());
}

UNIT_TEST(RoutingManager_NavigateTrackDistinguishesShortRemainderFromLoopFinish)
{
  AsyncBuildFixture fixture;
  auto & manager = fixture.m_framework.GetRoutingManager();
  auto & bookmarks = fixture.m_framework.GetBookmarkManager();
  kml::TrackData source;
  source.m_layers.emplace_back();
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(0, 0), 110}, {MeterPoint(100, 0), 120}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine(
      {{MeterPoint(100, 0), 120}, {MeterPoint(200, 0), 130}, {MeterPoint(0, 0), 140}, {MeterPoint(0, 0), 150}});
  source.m_geometry.AddTimestamps({});
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(source))->GetId();
  auto const length = bookmarks.GetTrack(id)->GetLengthMeters();
  // A short fractional remainder is usable even below the full source's KML storage tolerance.
  auto const nearEnd = MeterPoint(0.1, 0);
  bookmarks.SetTrackSelectionInfo({id, nearEnd, length - mercator::DistanceOnEarth(nearEnd, MeterPoint(0, 0))}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(!manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), 0.1, 0.01, ());
  bookmarks.SetTrackSelectionInfo({id, MeterPoint(0, 0), length}, false);
  TEST_EQUAL(manager.NavigateTrack(id), FollowTrackResult::Ok, ());
  fixture.WaitForBuildResult();
  TEST(manager.GetFollowedTrackInfo()->m_reversed, ());
  TEST(manager.GetFollowedTrackInfo()->m_canReverse, ());
  TEST_ALMOST_EQUAL_ABS(GetFollowedTrackLengthMeters(manager), length, 0.01, ());
  TEST_EQUAL(manager.GetFollowedTrackInfo()->m_entryDistanceM, length, ());
}
}  // namespace routing_manager_tests
