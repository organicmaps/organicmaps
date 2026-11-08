#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "drape_frontend/visual_params.hpp"

#include "routing/routing_helpers.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/thread_safe_queue.hpp"
#include "base/waiter.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace
{
class SnapshotGui final : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    m_tasks.Push(std::move(task));
    m_ready.Notify();
    return {true, kNoId};
  }
  PushResult Push(Task const & task) override { return Push(Task(task)); }

  template <typename Predicate>
  void RunUntil(Predicate done)
  {
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!done())
    {
      TEST(std::chrono::steady_clock::now() < deadline, ("Snapshot callback timed out"));
      Task task;
      while (!m_tasks.TryPop(task))
      {
        TEST(m_ready.Wait(deadline - std::chrono::steady_clock::now()) != base::Waiter::Result::Timeout,
             ("Snapshot callback timed out"));
        m_ready.Reset();
      }
      task();
    }
  }

private:
  threads::ThreadSafeQueue<Task> m_tasks;
  base::Waiter m_ready;
};

class SnapshotRouter final : public routing::IRouter
{
public:
  std::string GetName() const override { return "navigation snapshot test"; }
  void ClearState() override {}
  void SetGuides(routing::GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, routing::EdgeProj &) override
  {
    return false;
  }
  routing::RouterResultCode CalculateRoute(routing::Checkpoints const &, m2::PointD const &, bool, bool,
                                           routing::RouterDelegate const &, routing::RoutesResult & result) override
  {
    std::vector<m2::PointD> const points = {{1, 1}, {1, 1}, {1.001, 1}, {1.002, 1}};
    std::vector<routing::RouteSegment> segments;
    for (size_t i = 1; i < points.size(); ++i)
    {
      auto const turn = i + 1 == points.size() ? routing::turns::CarDirection::ReachedYourDestination
                                               : routing::turns::CarDirection::None;
      segments.emplace_back(routing::Segment(0, 0, i, true), routing::turns::TurnItem(i, turn),
                            geometry::PointWithAltitude(points[i], 0), routing::RouteSegment::RoadNameInfo{});
      segments.back().SetSpeedLimit(
          routing::Maxspeed(measurement_utils::Units::Metric, i < 3 ? 54 : 36, routing::kInvalidSpeed));
      if (i == 3)
        segments.back().SetSpeedCameraInfo({{0.5, 30}});
    }
    routing::FillSegmentInfo({0, 10, 20}, segments);
    routing::Route route;
    route.SetRouteSegments(std::move(segments));
    route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{
        {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 3}});
    route.SetGeometry(points.begin(), points.end());
    result.MakeFrom(GetName(), std::move(route));
    return routing::RouterResultCode::NoError;
  }
};

class RoutingSnapshotTest
{
public:
  RoutingSnapshotTest()
  {
    df::VisualParams::Init(1.0, 1024);
    m_hadRouter = settings::Get("router", m_previousRouter);
    settings::Set("router", routing::ToString(routing::RouterType::Vehicle));
    auto gui = std::make_unique<SnapshotGui>();
    m_gui = gui.get();
    GetPlatform().SetGuiThread(std::move(gui));
    m_framework = std::make_unique<Framework>(FrameworkParams{}, false /* loadMaps */);
    auto & manager = Manager();
    auto & session = manager.RoutingSession();
    session.SetRouter(std::make_unique<SnapshotRouter>(), nullptr);
    session.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
    manager.EnableTurnNotifications(true);
    manager.GetSpeedCamManager().SetMode(routing::SpeedCameraManagerMode::Auto);
    session.SetLocaleWithJsonForTesting(
        R"({"destination":"arrive","you_have_reached_the_destination":"arrived","recalculating_route":"recalculating"})",
        "en");

    RouteMarkData start;
    start.m_pointType = RouteMarkType::Start;
    start.m_position = {1, 1};
    manager.AddRoutePoint(std::move(start), false);
    RouteMarkData finish;
    finish.m_pointType = RouteMarkType::Finish;
    finish.m_position = {1.002, 1};
    manager.AddRoutePoint(std::move(finish), false);
    bool built = false;
    session.SetRoutingCallbacks([&](routing::RoutesResult const &, routing::RouterResultCode code)
    {
      TEST_EQUAL(code, routing::RouterResultCode::NoError, ());
      built = true;
    }, nullptr, nullptr, nullptr);
    session.BuildRoute(routing::Checkpoints({1, 1}, {1.002, 1}), routing::RouterDelegate::kNoTimeout);
    m_gui->RunUntil([&] { return built; });
    session.SetRoutingCallbacks(nullptr, nullptr, nullptr, nullptr);
    TEST(session.EnableFollowMode(), ());
  }

  ~RoutingSnapshotTest()
  {
    m_framework.reset();
    GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>());
    if (m_hadRouter)
      settings::Set("router", m_previousRouter);
    else
      settings::Delete("router");
  }

  RoutingManager & Manager() { return m_framework->GetRoutingManager(); }

  void Update(double longitude, double speed, double y = 1.0)
  {
    Manager().OnLocationUpdate(Gps(longitude, speed, y));
    TEST_EQUAL(m_received, m_sequence, ("Received measurements publish their snapshot synchronously."));
  }

  void UpdateWithoutListener(double longitude, double speed)
  {
    Manager().OnLocationUpdate(Gps(longitude, speed));
    TEST_GREATER(Manager().RoutingSession().GetCompletionPercent(), 0.0, ());
  }

  location::GpsInfo Gps(double longitude, double speed, double y = 1.0)
  {
    location::GpsInfo gps;
    gps.m_source = location::EAndroidNative;
    gps.m_timestamp = 1700000000.0 + ++m_sequence;
    auto const latLon = mercator::ToLatLon({longitude, y});
    gps.m_latitude = latLon.m_lat;
    gps.m_longitude = latLon.m_lon;
    gps.m_horizontalAccuracy = 1;
    gps.m_speed = speed;
    return gps;
  }

  size_t m_received = 0;

private:
  SnapshotGui * m_gui = nullptr;
  std::unique_ptr<Framework> m_framework;
  size_t m_sequence = 0;
  std::string m_previousRouter;
  bool m_hadRouter = false;
};

UNIT_CLASS_TEST(RoutingSnapshotTest, SpeedLimitUsesTheProcessedMeasurement)
{
  std::vector<std::pair<double, double>> snapshots;
  Manager().SetRouteFollowingListener([&](routing::FollowingInfo const & info)
  {
    TEST(info.IsValid(), ());
    snapshots.emplace_back(info.m_currentSpeedMps, info.m_speedLimitMps);
    ++m_received;
  });
  Update(1.0005, 7);
  Update(1.0015, 20);
  TEST_EQUAL(snapshots.size(), 2, ());
  TEST_ALMOST_EQUAL_ABS(snapshots[0].first, 7.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(snapshots[0].second, 15.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(snapshots[1].first, 20.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(snapshots[1].second, 10.0, 1e-8, ());
}

UNIT_CLASS_TEST(RoutingSnapshotTest, ArrivalCanBeVoicedBeforeClosingTheRoute)
{
  bool voicedArrival = false;
  bool closed = false;
  Manager().SetRouteFollowingListener([&](routing::FollowingInfo const & info)
  {
    TEST(info.IsValid(), ());
    std::vector<std::string> notifications;
    Manager().GenerateNotifications(notifications, false);
    ++m_received;
    if (!Manager().IsRouteFinished())
      return;

    TEST_EQUAL(Manager().GetRoutePointsCount(), 2, ());
    voicedArrival = std::any_of(notifications.begin(), notifications.end(),
                                [](std::string const & text) { return text.find("arriv") != std::string::npos; });
    TEST(voicedArrival, (notifications));
    Manager().CloseRouting(false);
    closed = true;
  });
  Update(1.0015, 5);
  Update(1.002, 5);
  TEST(voicedArrival, ());
  TEST(closed, ());
  TEST(!Manager().IsRoutingActive(), ());
}

UNIT_CLASS_TEST(RoutingSnapshotTest, SnapshotCapturesTheBeepOnceAndPollingDoesNotConsumeIt)
{
  bool captured = false;
  Manager().SetRouteFollowingListener([&](routing::FollowingInfo const & info)
  {
    captured = info.m_shouldPlayWarningSignal;
    TEST(info.m_isSpeedCamLimitExceeded, ());
    routing::FollowingInfo polled;
    Manager().GetRouteFollowingInfo(polled);
    TEST(!polled.m_shouldPlayWarningSignal, ());
    TEST(info.m_shouldPlayWarningSignal, ("Reading another snapshot must not mutate the event."));
    TEST(!Manager().GetSpeedCamManager().ShouldPlayBeepSignal(), ("The event already owns the pending beep."));
    ++m_received;
  });
  Update(1.0015, 20);
  TEST(captured, ());
}

UNIT_CLASS_TEST(RoutingSnapshotTest, NoSnapshotObserverPreservesExistingCameraConsumption)
{
  UpdateWithoutListener(1.0015, 20);
  routing::FollowingInfo polled;
  Manager().GetRouteFollowingInfo(polled);
  TEST(!polled.m_shouldPlayWarningSignal, ());
  TEST(Manager().GetSpeedCamManager().ShouldPlayBeepSignal(),
       ("Platforms without the snapshot observer retain their existing event consumption."));
}

UNIT_CLASS_TEST(RoutingSnapshotTest, UnmatchedMeasurementDoesNotReuseAPreviousCameraAlert)
{
  std::vector<bool> alerts;
  std::vector<double> limits;
  Manager().SetRouteFollowingListener([&](routing::FollowingInfo const & info)
  {
    alerts.push_back(info.m_isSpeedCamLimitExceeded);
    limits.push_back(info.m_speedLimitMps);
    ++m_received;
  });
  Update(1.0015, 20);
  Update(1.0015, 0, 1.01);
  TEST_EQUAL(alerts.size(), 2, ());
  TEST(alerts[0], ());
  TEST(!alerts[1], ());
  TEST_LESS(limits[1], 0.0, ("The retained route does not establish the new fix's road limit."));
}

UNIT_CLASS_TEST(RoutingSnapshotTest, HistoricalMeasurementCannotPublishGuidanceOrConsumeThePendingBeep)
{
  UpdateWithoutListener(1.0015, 20);
  auto const completion = Manager().RoutingSession().GetCompletionPercent();
  bool published = false;
  Manager().SetRouteFollowingListener([&](routing::FollowingInfo const &) { published = true; });
  auto historical = Gps(1.002, 40);
  auto const now = location::Clock::now();
  historical.m_timing = location::GpsInfo::Timing{now - std::chrono::seconds(40), now - std::chrono::seconds(35)};
  Manager().OnLocationUpdate(historical);

  TEST(!published, ());
  TEST_EQUAL(Manager().RoutingSession().GetCompletionPercent(), completion, ());
  TEST(!Manager().IsRouteFinished(), ());
  TEST(Manager().GetSpeedCamManager().ShouldPlayBeepSignal(), ("Historical output must not consume the live alert."));
}
}  // namespace
