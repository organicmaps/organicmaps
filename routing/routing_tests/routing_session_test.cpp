#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_session.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace routing
{
using namespace std;

using chrono::seconds;
using chrono::steady_clock;

// Simple router. It returns route given to him on creation.
class DummyRouter : public IRouter
{
private:
  Route m_route;
  RouterResultCode m_code;
  size_t & m_buildCount;

public:
  DummyRouter(Route & route, RouterResultCode code, size_t & buildCounter)
    : m_route(route), m_code(code), m_buildCount(buildCounter)
  {
  }

  string GetName() const override { return "dummy"; }
  void ClearState() override {}
  RouterResultCode CalculateRoute(Checkpoints const & /* checkpoints */,
                                  m2::PointD const & /* startDirection */, bool /* adjust */,
                                  RouterDelegate const & /* delegate */, Route & route) override
  {
    ++m_buildCount;
    route = m_route;
    return m_code;
  }
};

static vector<m2::PointD> kTestRoute = {{0., 1.}, {0., 2.}, {0., 3.}, {0., 4.}};
static vector<Segment> const kTestSegments({{0, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}});
static Route::TTurns const kTestTurns = {
    turns::TurnItem(3, turns::CarDirection::ReachedYourDestination)};
static Route::TTimes const kTestTimes({Route::TTimeItem(1, 5), Route::TTimeItem(2, 10),
                                       Route::TTimeItem(3, 15)});

static auto kRouteBuildingMaxDuration = seconds(30);

class TimedSignal
{
public:
  TimedSignal() : m_flag(false) {}
  void Signal()
  {
    lock_guard<mutex> guard(m_waitingMutex);
    m_flag = true;
    m_cv.notify_one();
  }

  bool WaitUntil(steady_clock::time_point const & time)
  {
    unique_lock<mutex> lock(m_waitingMutex);
    m_cv.wait_until(lock, time, [this, &time]
                    {
                      return m_flag || steady_clock::now() > time;
                    });
    return m_flag;
  }

private:
  mutex m_waitingMutex;
  condition_variable m_cv;
  bool m_flag;
};

void FillSubroutesInfo(Route & route)
{
  vector<Junction> junctions;
  for (auto const & point : kTestRoute)
    junctions.emplace_back(point, feature::kDefaultAltitudeMeters);

  vector<RouteSegment> segmentInfo;
  FillSegmentInfo(kTestSegments, junctions, kTestTurns, {}, kTestTimes,
                  nullptr /* trafficStash */, segmentInfo);
  route.SetRouteSegments(move(segmentInfo));
  route.SetSubroteAttrs(vector<Route::SubrouteAttrs>(
      {Route::SubrouteAttrs(junctions.front(), junctions.back(), 0, kTestSegments.size())}));
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteBuilding)
{
  // Multithreading synchronization note. |counter| and |session| are constructed on the main thread,
  // then used on gui thread and then if timeout in timedSignal.WaitUntil() is not reached,
  // |counter| is used again.
  TimedSignal timedSignal;
  size_t counter = 0;

  GetPlatform().RunTask(Platform::Thread::Gui, [&timedSignal, &counter, this]() {
    InitRoutingSession();
    Route masterRoute("dummy", kTestRoute.begin(), kTestRoute.end(), 0 /* route id */);

    unique_ptr<DummyRouter> router =
        make_unique<DummyRouter>(masterRoute, RouterResultCode::NoError, counter);
    m_session->SetRouter(move(router), nullptr);
    m_session->SetRoutingCallbacks(
        [&timedSignal](Route const &, RouterResultCode) {
          LOG(LINFO, ("Ready"));
          timedSignal.Signal();
        },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });

  // Manual check of the routeBuilded mutex to avoid spurious results.
  TEST(timedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());
}

// Test on route rebuilding when current position moving from the route. Each next position
// is farther from the route then previous one.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteRebuildingMovingAway)
{
  location::GpsInfo info;
  FrozenDataSource dataSource;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]() {
    InitRoutingSession();
    Route masterRoute("dummy", kTestRoute.begin(), kTestRoute.end(), 0 /* route id */);
    FillSubroutesInfo(masterRoute);

    unique_ptr<DummyRouter> router =
        make_unique<DummyRouter>(masterRoute, RouterResultCode::NoError, counter);
    m_session->SetRouter(move(router), nullptr);

    // Go along the route.
    m_session->SetRoutingCallbacks(
        [&alongTimedSignal](Route const &, RouterResultCode) { alongTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());

  TimedSignal oppositeTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&oppositeTimedSignal, &info, this]() {
    info.m_horizontalAccuracy = 0.01;
    info.m_verticalAccuracy = 0.01;
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    RoutingSession::State code;
    while (info.m_latitude < kTestRoute.back().y)
    {
      code = m_session->OnLocationPositionChanged(info);
      TEST_EQUAL(code, RoutingSession::State::OnRoute, ());
      info.m_latitude += 0.01;
    }

    // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
    m_session->SetRoutingCallbacks(
        [&oppositeTimedSignal](Route const &, RouterResultCode) { oppositeTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(oppositeTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  // Going away from route to set rebuilding flag.
  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, &info, this]() {
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    RoutingSession::State code = RoutingSession::State::RoutingNotActive;
    for (size_t i = 0; i < 10; ++i)
    {
      code = m_session->OnLocationPositionChanged(info);
      info.m_latitude -= 0.1;
    }
    TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration),
       ("Route was not rebuilt."));
}

// Test on route rebuilding when current position moving to the route starting far from the route.
// Each next position is closer to the route then previous one but is not covered by route matching passage.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteRebuildingMovingToRoute)
{
  location::GpsInfo info;
  FrozenDataSource dataSource;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]() {
    InitRoutingSession();
    Route masterRoute("dummy", kTestRoute.begin(), kTestRoute.end(), 0 /* route id */);
    FillSubroutesInfo(masterRoute);

    unique_ptr<DummyRouter> router =
        make_unique<DummyRouter>(masterRoute, RouterResultCode::NoError, counter);
    m_session->SetRouter(move(router), nullptr);

    m_session->SetRoutingCallbacks(
        [&alongTimedSignal](Route const &, RouterResultCode) { alongTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());

  // Going starting far from the route and moving to the route but rebuild flag still is set.
  TimedSignal checkTimedSignalAway;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignalAway, &info, this]() {
    info.m_longitude = 0.0;
    info.m_latitude = 0.0;
    RoutingSession::State code = RoutingSession::State::RoutingNotActive;
    for (size_t i = 0; i < 8; ++i)
    {
      code = m_session->OnLocationPositionChanged(info);
      info.m_latitude += 0.1;
    }
    TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
    checkTimedSignalAway.Signal();
  });
  TEST(checkTimedSignalAway.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration),
       ("Route was not rebuilt."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestFollowRouteFlagPersistence)
{
  location::GpsInfo info;
  FrozenDataSource dataSource;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]() {
    InitRoutingSession();
    Route masterRoute("dummy", kTestRoute.begin(), kTestRoute.end(), 0 /* route id */);
    FillSubroutesInfo(masterRoute);
    unique_ptr<DummyRouter> router =
        make_unique<DummyRouter>(masterRoute, RouterResultCode::NoError, counter);
    m_session->SetRouter(move(router), nullptr);

    // Go along the route.
    m_session->SetRoutingCallbacks(
        [&alongTimedSignal](Route const &, RouterResultCode) { alongTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal oppositeTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&oppositeTimedSignal, &info, this, &counter]() {
    TEST(!m_session->IsFollowing(), ());
    m_session->EnableFollowMode();
    TEST(m_session->IsFollowing(), ());

    info.m_horizontalAccuracy = 0.01;
    info.m_verticalAccuracy = 0.01;
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    while (info.m_latitude < kTestRoute.back().y)
    {
      m_session->OnLocationPositionChanged(info);
      TEST(m_session->IsOnRoute(), ());
      TEST(m_session->IsFollowing(), ());
      info.m_latitude += 0.01;
    }
    TEST_EQUAL(counter, 1, ());

    // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
    m_session->SetRoutingCallbacks(
        [&oppositeTimedSignal](Route const &, RouterResultCode) { oppositeTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(oppositeTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal rebuildTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&rebuildTimedSignal, &info, this] {
    // Manual route building resets the following flag.
    TEST(!m_session->IsFollowing(), ());
    m_session->EnableFollowMode();
    TEST(m_session->IsFollowing(), ());
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    RoutingSession::State code;
    for (size_t i = 0; i < 10; ++i)
    {
      code = m_session->OnLocationPositionChanged(info);
      info.m_latitude -= 0.1;
    }
    TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
    TEST(m_session->IsFollowing(), ());

    m_session->RebuildRoute(
        kTestRoute.front(),
        [&rebuildTimedSignal](Route const &, RouterResultCode) { rebuildTimedSignal.Signal(); },
        nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */, 0,
        RoutingSession::State::RouteBuilding, false /* adjust */);
  });
  TEST(rebuildTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, this] {
    TEST(m_session->IsFollowing(), ());
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route checking timeout."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestFollowRoutePercentTest)
{
  FrozenDataSource dataSource;
  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this]() {
    InitRoutingSession();
    Route masterRoute("dummy", kTestRoute.begin(), kTestRoute.end(), 0 /* route id */);
    FillSubroutesInfo(masterRoute);

    size_t counter = 0;
    unique_ptr<DummyRouter> router =
        make_unique<DummyRouter>(masterRoute, RouterResultCode::NoError, counter);
    m_session->SetRouter(move(router), nullptr);

    // Get completion percent of unexisted route.
    TEST_EQUAL(m_session->GetCompletionPercent(), 0, (m_session->GetCompletionPercent()));
    // Go along the route.
    m_session->SetRoutingCallbacks(
        [&alongTimedSignal](Route const &, RouterResultCode) { alongTimedSignal.Signal(); },
        nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */,
        nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), 0);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, this] {
    // Get completion percent of unstarted route.
    TEST_EQUAL(m_session->GetCompletionPercent(), 0, (m_session->GetCompletionPercent()));

    location::GpsInfo info;
    info.m_horizontalAccuracy = 0.01;
    info.m_verticalAccuracy = 0.01;

    // Go through the route.
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    m_session->OnLocationPositionChanged(info);
    TEST(base::AlmostEqualAbs(m_session->GetCompletionPercent(), 0., 0.5),
         (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 2.;
    m_session->OnLocationPositionChanged(info);
    TEST(base::AlmostEqualAbs(m_session->GetCompletionPercent(), 33.3, 0.5),
         (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 3.;
    m_session->OnLocationPositionChanged(info);
    TEST(base::AlmostEqualAbs(m_session->GetCompletionPercent(), 66.6, 0.5),
         (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 3.99;
    m_session->OnLocationPositionChanged(info);
    TEST(base::AlmostEqualAbs(m_session->GetCompletionPercent(), 100., 0.5),
         (m_session->GetCompletionPercent()));
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration),
       ("Route checking timeout."));
}
}  // namespace routing
