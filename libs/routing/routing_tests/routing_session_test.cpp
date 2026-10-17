#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_session.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <future>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace routing_session_test
{
using namespace routing;
using namespace std;

using chrono::seconds;
using chrono::steady_clock;

vector<m2::PointD> kTestRoute = {{0., 1.}, {0., 1.}, {0., 3.}, {0., 4.}};
vector<m2::PointD> const kTestAltRoute = {{0., 1.}, {0., 1.}, {1., 3.}, {0., 4.}};
vector<Segment> const kTestSegments = {{0, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}};
vector<turns::TurnItem> const kTestTurnsReachOnly = {turns::TurnItem(1, turns::CarDirection::None),
                                                     turns::TurnItem(2, turns::CarDirection::None),
                                                     turns::TurnItem(3, turns::CarDirection::ReachedYourDestination)};
vector<turns::TurnItem> const kTestTurns = {turns::TurnItem(1, turns::CarDirection::None),
                                            turns::TurnItem(2, turns::CarDirection::TurnLeft),
                                            turns::TurnItem(3, turns::CarDirection::ReachedYourDestination)};
vector<double> const kTestTimes = {5.0, 10.0, 15.0};
auto const kRouteBuildingMaxDuration = seconds(30);

void FillSubroutesInfo(Route & route, vector<turns::TurnItem> const & turns = kTestTurnsReachOnly);

// Simple router. It returns route given to him on creation.
class DummyRouter : public IRouter
{
private:
  Route m_route;
  size_t & m_buildCount;

public:
  DummyRouter(size_t & buildCounter, vector<turns::TurnItem> const & turns = kTestTurnsReachOnly)
    : m_buildCount(buildCounter)
  {
    m_route.SetGeometry(kTestRoute.begin(), kTestRoute.end());
    if (!turns.empty())
      FillSubroutesInfo(m_route, turns);
  }

  string GetName() const override { return "dummy"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks && /* guides */) override {}

  RouterResultCode CalculateRoute(Checkpoints const & /* checkpoints */, m2::PointD const & /* startDirection */,
                                  bool /* adjust */, bool /* needAlternatives */, RouterDelegate const & /* delegate */,
                                  RoutesResult & result) override
  {
    ++m_buildCount;
    result.MakeFrom(GetName(), Route(m_route));
    return RouterResultCode::NoError;
  }

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    return false;
  }
};

// Router recording whether the session asked for alternative routes.
class AlternativesFlagRouter : public DummyRouter
{
public:
  AlternativesFlagRouter(size_t & buildCounter, vector<bool> & flags) : DummyRouter(buildCounter), m_flags(flags) {}

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection, bool adjust,
                                  bool needAlternatives, RouterDelegate const & delegate,
                                  RoutesResult & result) override
  {
    m_flags.push_back(needAlternatives);
    return DummyRouter::CalculateRoute(checkpoints, startDirection, adjust, needAlternatives, delegate, result);
  }

private:
  vector<bool> & m_flags;
};

// Router returning two route variants and counting the "user picked the other one" notifications.
class AltRoutesRouter : public IRouter
{
public:
  explicit AltRoutesRouter(size_t & swapCount) : m_swapCount(swapCount) {}

  string GetName() const override { return "alt routes"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks && /* guides */) override {}
  void SwapAltRouteToActive() override { ++m_swapCount; }

  RouterResultCode CalculateRoute(Checkpoints const & /* checkpoints */, m2::PointD const & /* startDirection */,
                                  bool /* adjust */, bool /* needAlternatives */, RouterDelegate const & /* delegate */,
                                  RoutesResult & result) override
  {
    Route active;
    active.SetGeometry(kTestRoute.begin(), kTestRoute.end());
    FillSubroutesInfo(active);
    result.MakeFrom(GetName(), std::move(active));

    Route alt;
    alt.SetGeometry(kTestAltRoute.begin(), kTestAltRoute.end());
    FillSubroutesInfo(alt);
    result.m_routes.emplace_back(std::move(static_cast<RouteBase &>(alt)));
    return RouterResultCode::NoError;
  }

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    return false;
  }

private:
  size_t & m_swapCount;
};

// Router which every next call of CalculateRoute() method return different return codes.
class ReturnCodesRouter : public IRouter
{
public:
  ReturnCodesRouter(initializer_list<RouterResultCode> const & returnCodes, vector<m2::PointD> const & route)
    : m_returnCodes(returnCodes)
    , m_route(route)
  {}

  // IRouter overrides:
  string GetName() const override { return "return codes router"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks && /* guides */) override {}

  RouterResultCode CalculateRoute(Checkpoints const & /* checkpoints */, m2::PointD const & /* startDirection */,
                                  bool /* adjust */, bool /* needAlternatives */, RouterDelegate const & /* delegate */,
                                  RoutesResult & result) override
  {
    TEST_LESS(m_returnCodesIdx, m_returnCodes.size(), ());
    Route route;
    route.SetGeometry(m_route.begin(), m_route.end());
    FillSubroutesInfo(route);
    result.MakeFrom(GetName(), std::move(route));
    return m_returnCodes[m_returnCodesIdx++];
  }

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    return false;
  }

private:
  vector<RouterResultCode> m_returnCodes;
  vector<m2::PointD> m_route;
  size_t m_returnCodesIdx = 0;
};

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
    m_cv.wait_until(lock, time, [this, &time] { return m_flag || steady_clock::now() > time; });
    return m_flag;
  }

private:
  mutex m_waitingMutex;
  condition_variable m_cv;
  bool m_flag;
};

/// \brief This class is developed to test callback on RoutingSession::m_state changing.
/// An new instance of the class should be constructed for every new test.
class SessionStateTest
{
public:
  SessionStateTest(initializer_list<SessionState> expectedStates, RoutingSession & routingSession)
    : m_expectedStates(expectedStates)
    , m_session(routingSession)
  {
    for (size_t i = 1; i < expectedStates.size(); ++i)
    {
      // Note. Change session state callback is called only if the state is change.
      if (m_expectedStates[i - 1] != m_expectedStates[i])
        ++m_expectedNumberOfStateChanging;
    }

    TimedSignal timedSignal;
    GetPlatform().RunTask(Platform::Thread::Gui, [this, &timedSignal]()
    {
      m_session.SetChangeSessionStateCallback([this](SessionState previous, SessionState current)
      { TestChangeSessionStateCallbackCall(previous, current); });
      timedSignal.Signal();
    });
    TEST(timedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Callback is not set."));
  }

  ~SessionStateTest()
  {
    TEST_EQUAL(m_numberOfTestCalls, m_expectedNumberOfStateChanging,
               ("Wrong number of calls of SetState() callback.", m_expectedStates));
    TimedSignal timedSignal;
    GetPlatform().RunTask(Platform::Thread::Gui, [this, &timedSignal]()
    {
      m_session.SetChangeSessionStateCallback(nullptr);
      timedSignal.Signal();
    });
    TEST(timedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Callback is not set."));
  }

private:
  void TestChangeSessionStateCallbackCall(SessionState previous, SessionState current)
  {
    TEST_LESS(m_numberOfTestCalls + 1, m_expectedStates.size(),
              ("Too many calls of the method. previous:", previous, ", current:", current));
    TEST_EQUAL(previous, m_expectedStates[m_numberOfTestCalls], (previous, current));
    TEST_EQUAL(current, m_expectedStates[m_numberOfTestCalls + 1], (previous, current));

    ++m_numberOfTestCalls;
  }

  size_t m_numberOfTestCalls = 0;
  vector<SessionState> const m_expectedStates;
  size_t m_expectedNumberOfStateChanging = 0;
  RoutingSession & m_session;
};

void FillSubroutesInfo(Route & route, vector<turns::TurnItem> const & turns /* = kTestTurnsReachOnly */)
{
  // Build segments from the route's actual polyline points (not the global kTestRoute) so segments
  // are consistent with the followed polyline. This matters because RouteBase no longer stores
  // the polyline: promoting a sliced RouteBase back to a Route reconstructs the FollowedPolyline
  // from segments (subroute.front().GetStart() + each segment's junction).
  auto const & polyPts = route.GetPoly().GetPoints();

  vector<geometry::PointWithAltitude> junctions;
  for (auto const & point : polyPts)
    junctions.emplace_back(point, geometry::kDefaultAltitudeMeters);

  vector<RouteSegment> segmentInfo;
  RouteSegmentsFrom(kTestSegments, polyPts, turns, {}, segmentInfo);
  FillSegmentInfo(kTestTimes, segmentInfo);
  route.SetRouteSegments(std::move(segmentInfo));
  route.SetSubroutes(vector<Route::SubrouteAttrs>(
      {Route::SubrouteAttrs(junctions.front(), junctions.back(), 0, kTestSegments.size())}));
}

void TestMovingByUpdatingLat(SessionStateTest const & sessionState, vector<double> const & lats,
                             location::GpsInfo const & info, RoutingSession & session)
{
  location::GpsInfo uptInfo(info);
  TimedSignal signal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&session, &signal, &lats, &uptInfo]()
  {
    for (auto const lat : lats)
    {
      uptInfo.m_latitude = lat;
      session.OnLocationPositionChanged(uptInfo);
    }

    signal.Signal();
  });
  TEST(signal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Along route moving timeout."));
}

void TestLeavingRoute(RoutingSession & session, location::GpsInfo const & info)
{
  vector<double> const latitudes = {0.0,    -0.001, -0.002, -0.003, -0.004, -0.005,
                                    -0.006, -0.007, -0.008, -0.009, -0.01,  -0.011};
  SessionStateTest sessionStateTest({SessionState::OnRoute, SessionState::RouteNeedRebuild}, session);
  TestMovingByUpdatingLat(sessionStateTest, latitudes, info, session);
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteBuilding)
{
  // Multithreading synchronization note. |counter| and |session| are constructed on the main thread,
  // then used on gui thread and then if timeout in timedSignal.WaitUntil() is not reached,
  // |counter| is used again.
  TimedSignal timedSignal;
  size_t counter = 0;

  GetPlatform().RunTask(Platform::Thread::Gui, [&timedSignal, &counter, this]()
  {
    InitRoutingSession();

    m_session->SetRouter(unique_ptr<DummyRouter>(new DummyRouter(counter, {})), nullptr);
    m_session->SetRoutingCallbacks([&timedSignal](RoutesResult const &, RouterResultCode)
    {
      LOG(LINFO, ("Ready"));
      timedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });

  // Manual check of the routeBuilding mutex to avoid spurious results.
  TEST(timedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());
}

// Test on route rebuilding when current position moving from the route. Each next position
// is farther from the route then previous one.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteRebuildingMovingAway)
{
  location::GpsInfo info;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]()
  {
    InitRoutingSession();

    m_session->SetRouter(make_unique<DummyRouter>(counter), nullptr);

    // Go along the route.
    m_session->SetRoutingCallbacks([&alongTimedSignal](RoutesResult const &, RouterResultCode) {
      alongTimedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()),
                          RouterDelegate::RouterDelegate::kNoTimeout);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());

  {
    SessionStateTest sessionStateTest({SessionState::RouteNotStarted, SessionState::OnRoute,
                                       SessionState::RouteBuilding, SessionState::RouteNotStarted},
                                      *m_session);
    TimedSignal oppositeTimedSignal;
    GetPlatform().RunTask(Platform::Thread::Gui, [&oppositeTimedSignal, &info, this]()
    {
      info.m_horizontalAccuracy = 0.01;
      info.m_verticalAccuracy = 0.01;
      info.m_longitude = 0.;
      info.m_latitude = 1.;
      SessionState code = SessionState::NoValidRoute;

      {
        while (info.m_latitude < kTestRoute.back().y)
        {
          code = m_session->OnLocationPositionChanged(info);
          TEST_EQUAL(code, SessionState::OnRoute, ());
          info.m_latitude += 0.01;
        }
      }

      // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
      m_session->SetRoutingCallbacks([&oppositeTimedSignal](RoutesResult const &, RouterResultCode) {
        oppositeTimedSignal.Signal();
      }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
      {
        m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
      }
    });
    TEST(oppositeTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  }

  // Going away from route to set rebuilding flag.
  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, &info, this]()
  {
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    info.m_speed = measurement_utils::KmphToMps(60);
    SessionState code = SessionState::NoValidRoute;
    for (size_t i = 0; i < 10; ++i)
    {
      code = m_session->OnLocationPositionChanged(info);
      info.m_latitude -= 0.1;
    }
    TEST_EQUAL(code, SessionState::RouteNeedRebuild, ());
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not rebuilt."));
}

// Test on route rebuilding when current position moving to the route starting far from the route.
// Each next position is closer to the route then previous one but is not covered by route matching passage.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteRebuildingMovingToRoute)
{
  location::GpsInfo info;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]()
  {
    InitRoutingSession();

    m_session->SetRouter(make_unique<DummyRouter>(counter), nullptr);

    m_session->SetRoutingCallbacks([&alongTimedSignal](RoutesResult const &, RouterResultCode) {
      alongTimedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());

  // Going starting far from the route and moving to the route but rebuild flag still is set.
  {
    SessionStateTest sessionStateTest({SessionState::RouteNotStarted, SessionState::RouteNeedRebuild}, *m_session);
    TimedSignal checkTimedSignalAway;
    GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignalAway, &info, this]()
    {
      info.m_longitude = 0.0;
      info.m_latitude = 0.0;
      info.m_speed = measurement_utils::KmphToMps(60);
      SessionState code = SessionState::NoValidRoute;
      {
        for (size_t i = 0; i < 8; ++i)
        {
          code = m_session->OnLocationPositionChanged(info);
          info.m_latitude += 0.1;
        }
      }
      TEST_EQUAL(code, SessionState::RouteNeedRebuild, ());
      checkTimedSignalAway.Signal();
    });
    TEST(checkTimedSignalAway.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not rebuilt."));
  }
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestFollowRouteFlagPersistence)
{
  location::GpsInfo info;
  size_t counter = 0;

  TimedSignal alongTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, this, &counter]()
  {
    InitRoutingSession();

    m_session->SetRouter(make_unique<DummyRouter>(counter, kTestTurns), nullptr);

    // Go along the route.
    m_session->SetRoutingCallbacks([&alongTimedSignal](RoutesResult const &, RouterResultCode) {
      alongTimedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(m_onNewTurnCallbackCounter, 0, ());

  TimedSignal oppositeTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&oppositeTimedSignal, &info, this, &counter]()
  {
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
    m_session->SetRoutingCallbacks([&oppositeTimedSignal](RoutesResult const &, RouterResultCode) {
      oppositeTimedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(oppositeTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(m_onNewTurnCallbackCounter, 1, ());

  TimedSignal rebuildTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&rebuildTimedSignal, &info, this]
  {
    // Manual route building resets the following flag.
    TEST(!m_session->IsFollowing(), ());
    m_session->EnableFollowMode();
    TEST(m_session->IsFollowing(), ());
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    info.m_speed = measurement_utils::KmphToMps(60);
    SessionState code = SessionState::NoValidRoute;
    for (size_t i = 0; i < 10; ++i)
    {
      code = m_session->OnLocationPositionChanged(info);
      info.m_latitude -= 0.1;
    }
    TEST_EQUAL(code, SessionState::RouteNeedRebuild, ());
    TEST(m_session->IsFollowing(), ());

    m_session->RebuildRoute(kTestRoute.front(), [&rebuildTimedSignal](RoutesResult const &, RouterResultCode)
    { rebuildTimedSignal.Signal(); }, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */,
                            RouterDelegate::kNoTimeout, SessionState::RouteBuilding, false /* adjust */);
  });
  TEST(rebuildTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(m_onNewTurnCallbackCounter, 1, ());

  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, this]
  {
    TEST(m_session->IsFollowing(), ());
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route checking timeout."));
  TEST_EQUAL(m_onNewTurnCallbackCounter, 1, ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestFollowRoutePercentTest)
{
  TimedSignal alongTimedSignal;
  size_t counter = 0;
  GetPlatform().RunTask(Platform::Thread::Gui, [&alongTimedSignal, &counter, this]()
  {
    InitRoutingSession();

    m_session->SetRouter(make_unique<DummyRouter>(counter), nullptr);

    // Get completion percent of unexisted route.
    TEST_EQUAL(m_session->GetCompletionPercent(), 0, (m_session->GetCompletionPercent()));
    // Go along the route.
    m_session->SetRoutingCallbacks([&alongTimedSignal](RoutesResult const &, RouterResultCode) {
      alongTimedSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(alongTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal checkTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&checkTimedSignal, this]
  {
    // Get completion percent of unstarted route.
    TEST_EQUAL(m_session->GetCompletionPercent(), 0, (m_session->GetCompletionPercent()));

    location::GpsInfo info;
    info.m_horizontalAccuracy = 0.01;
    info.m_verticalAccuracy = 0.01;

    // Go through the route.
    info.m_longitude = 0.;
    info.m_latitude = 1.;
    m_session->OnLocationPositionChanged(info);
    TEST(AlmostEqualAbs(m_session->GetCompletionPercent(), 0., 0.5), (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 2.;
    m_session->OnLocationPositionChanged(info);
    TEST(AlmostEqualAbs(m_session->GetCompletionPercent(), 33.3, 0.5), (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 3.;
    m_session->OnLocationPositionChanged(info);
    TEST(AlmostEqualAbs(m_session->GetCompletionPercent(), 66.6, 0.5), (m_session->GetCompletionPercent()));

    info.m_longitude = 0.;
    info.m_latitude = 3.99;
    m_session->OnLocationPositionChanged(info);
    TEST(AlmostEqualAbs(m_session->GetCompletionPercent(), 100., 0.5), (m_session->GetCompletionPercent()));
    checkTimedSignal.Signal();
  });
  TEST(checkTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route checking timeout."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestRouteRebuildingError)
{
  vector<m2::PointD> const kRoute = {{0.0, 0.001}, {0.0, 0.002}, {0.0, 0.003}, {0.0, 0.004}};
  // Creation RoutingSession.
  TimedSignal createTimedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [this, &kRoute, &createTimedSignal]()
  {
    InitRoutingSession();
    unique_ptr<ReturnCodesRouter> router = make_unique<ReturnCodesRouter>(
        initializer_list<RouterResultCode>{RouterResultCode::NoError, RouterResultCode::InternalError}, kRoute);
    m_session->SetRouter(std::move(router), nullptr);
    createTimedSignal.Signal();
  });
  TEST(createTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("RouteSession was not created."));

  // Building a route.
  {
    SessionStateTest sessionStateTest(
        {SessionState::NoValidRoute, SessionState::RouteBuilding, SessionState::RouteNotStarted}, *m_session);
    TimedSignal buildTimedSignal;
    GetPlatform().RunTask(Platform::Thread::Gui, [this, &kRoute, &buildTimedSignal]()
    {
      m_session->SetRoutingCallbacks([&buildTimedSignal](RoutesResult const &, RouterResultCode) {
        buildTimedSignal.Signal();
      }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);

      m_session->BuildRoute(Checkpoints(kRoute.front(), kRoute.back()), RouterDelegate::kNoTimeout);
    });
    TEST(buildTimedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  }

  location::GpsInfo info;
  info.m_horizontalAccuracy = 5.0;  // meters
  info.m_verticalAccuracy = 5.0;    // meters
  info.m_longitude = 0.0;

  // Moving along route.
  {
    SessionStateTest sessionStateTest({SessionState::RouteNotStarted, SessionState::OnRoute}, *m_session);
    vector<double> const latitudes = {0.001, 0.0015, 0.002};
    TestMovingByUpdatingLat(sessionStateTest, latitudes, info, *m_session);
  }

  // Test 1. Leaving the route and returning to the route when state is |SessionState::RouteNeedRebuil|.
  TestLeavingRoute(*m_session, info);

  // Continue moving along the route. Stay short of lat=0.003 so the session stays in OnRoute —
  // reaching that point would trigger RouteFinished (segments[2] ends there for the 4-point kRoute)
  // and the subsequent TestLeavingRoute call would not transition out of RouteFinished.
  {
    SessionStateTest sessionStateTest({SessionState::RouteNeedRebuild, SessionState::OnRoute}, *m_session);
    vector<double> const latitudes = {0.002, 0.0025, 0.0028};
    TestMovingByUpdatingLat(sessionStateTest, latitudes, info, *m_session);
  }

  // Test 2. Leaving the route until, can not rebuilding it, and going along an old route.
  // It happens we the route is left and it's impossible to build a new route.
  // In this case the navigation is continued based on the former route.
  TestLeavingRoute(*m_session, info);
  {
    SessionStateTest sessionStateTest({SessionState::RouteNeedRebuild, SessionState::RouteRebuilding}, *m_session);
    TimedSignal signal;
    GetPlatform().RunTask(Platform::Thread::Gui, [this, &signal]()
    {
      m_session->SetState(SessionState::RouteRebuilding);
      signal.Signal();
    });
    TEST(signal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("State was not set."));
  }

  // Continue moving along the route until the destination is reached. The first GPS at lat=0.003
  // already triggers the route-finished check (segments[2] ends there for the 4-point kRoute), so
  // the state jumps directly RouteRebuilding -> RouteFinished without an intermediate OnRoute.
  {
    SessionStateTest sessionStateTest({SessionState::RouteRebuilding, SessionState::RouteFinished}, *m_session);
    vector<double> const latitudes = {0.003, 0.0035, 0.004};
    TestMovingByUpdatingLat(sessionStateTest, latitudes, info, *m_session);
  }
}

// Alternatives are drawn only outside navigation (RoutingManager::InsertRoute), so a rebuild in
// follow mode must not pay for computing them.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestAlternativesSkippedWhileFollowing)
{
  size_t counter = 0;
  vector<bool> flags;

  TimedSignal builtSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&builtSignal, &counter, &flags, this]()
  {
    InitRoutingSession();
    m_session->SetRouter(make_unique<AlternativesFlagRouter>(counter, flags), nullptr);
    m_session->SetRoutingCallbacks([&builtSignal](RoutesResult const &, RouterResultCode) {
      builtSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(builtSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));
  TEST_EQUAL(flags, vector<bool>({true}), ("The initial build computes alternatives."));

  TimedSignal rebuiltSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&rebuiltSignal, this]()
  {
    TEST(m_session->EnableFollowMode(), ());
    m_session->RebuildRoute(kTestRoute.front(), [&rebuiltSignal](RoutesResult const &, RouterResultCode)
    { rebuiltSignal.Signal(); }, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */,
                            RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true /* adjustToPrevRoute */);
  });
  TEST(rebuiltSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not rebuilt."));
  TEST_EQUAL(flags, vector<bool>({true, false}), ("A rebuild while navigating skips alternatives."));
}

// Picking an alternative route must reach the router: IndexRouter remembers the chosen variant
// there and keeps its weights on the next rebuild.
// https://github.com/organicmaps/organicmaps/issues/13205
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, TestSwapActiveAlternative)
{
  TimedSignal builtSignal;
  size_t swapCount = 0;

  GetPlatform().RunTask(Platform::Thread::Gui, [&builtSignal, &swapCount, this]()
  {
    InitRoutingSession();
    m_session->SetRouter(make_unique<AltRoutesRouter>(swapCount), nullptr);
    m_session->SetRoutingCallbacks([&builtSignal](RoutesResult const &, RouterResultCode) {
      builtSignal.Signal();
    }, nullptr /* rebuildReadyCallback */, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */);
    m_session->BuildRoute(Checkpoints(kTestRoute.front(), kTestRoute.back()), RouterDelegate::kNoTimeout);
  });
  TEST(builtSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Route was not built."));

  TimedSignal swappedSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&swappedSignal, &swapCount, this]()
  {
    TEST(m_session->SwapActiveAlternative(1), ());
    TEST_EQUAL(swapCount, 1, ());

    // Picking the route which is already active changes nothing.
    TEST(!m_session->SwapActiveAlternative(1), ());
    TEST_EQUAL(swapCount, 1, ());

    // Switching back is forwarded just the same.
    TEST(m_session->SwapActiveAlternative(0), ());
    TEST_EQUAL(swapCount, 2, ());

    swappedSignal.Signal();
  });
  TEST(swappedSignal.WaitUntil(steady_clock::now() + kRouteBuildingMaxDuration), ("Alternative was not swapped."));
}

// Session ownership is shared by queued tasks and callbacks. Tear it down on the GUI queue,
// including when a test times out before a callback arrives.
class AsyncGuiThreadTestWithProgressSession : public AsyncGuiThreadTest
{
public:
  struct State
  {
    unique_ptr<RoutingSession> m_session;
  };

  shared_ptr<State> const m_state = make_shared<State>();

  ~AsyncGuiThreadTestWithProgressSession()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state] { state->m_session.reset(); });
  }
};

class ProgressRouter : public IRouter
{
public:
  explicit ProgressRouter(bool failFirstRebuild = false) : m_failFirstRebuild(failFirstRebuild) {}

  string GetName() const override { return "progress"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    return false;
  }

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                  RouterDelegate const &, RoutesResult & result) override
  {
    if (++m_builds == 2 && m_failFirstRebuild)
      return RouterResultCode::RouteNotFound;

    auto const from = checkpoints.GetPointFrom();
    auto const to = checkpoints.GetFinish();
    vector<m2::PointD> const points = {from, from, (from + to) * 0.5, to};
    Route route;
    route.SetGeometry(points.begin(), points.end());
    FillSubroutesInfo(route);
    result.MakeFrom(GetName(), std::move(route));
    return RouterResultCode::NoError;
  }

private:
  size_t m_builds = 0;
  bool const m_failFirstRebuild;
};

void MoveSessionTo(RoutingSession & session, m2::PointD const & point)
{
  session.SetUserCurrentPosition(point);
  location::GpsInfo gps;
  gps.m_latitude = mercator::YToLat(point.y);
  gps.m_longitude = mercator::XToLon(point.x);
  gps.m_horizontalAccuracy = 5;
  session.OnLocationPositionChanged(gps);
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithProgressSession, TrafficRefreshKeepsTripProgress)
{
  for (m2::PointD const current : {m2::PointD(0, 2), m2::PointD(0, 2.5)})
  {
    auto built = make_shared<promise<void>>();
    auto builtFuture = built->get_future();
    auto rebuilt = make_shared<promise<double>>();
    auto rebuiltFuture = rebuilt->get_future();
    GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, built, rebuilt, current]
    {
      state->m_session = make_unique<RoutingSession>();
      state->m_session->Init(nullptr);
      state->m_session->SetRouter(make_unique<ProgressRouter>(), nullptr);
      state->m_session->SetRoutingCallbacks([built](RoutesResult const &, RouterResultCode) { built->set_value(); },
                                            [state, rebuilt, current](RoutesResult const &, RouterResultCode)
      {
        MoveSessionTo(*state->m_session, current);
        rebuilt->set_value(state->m_session->GetCompletionPercent());
      }, nullptr, nullptr);
      state->m_session->BuildRoute(Checkpoints({0, 1}, {0, 4}), RouterDelegate::kNoTimeout);
    });
    TEST(builtFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
    GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, current]
    {
      state->m_session->EnableFollowMode();
      MoveSessionTo(*state->m_session, {0, 2});
      state->m_session->OnTrafficInfoClear();
      // Ready callbacks cannot run until this GUI task ends, so this movement precedes adoption.
      MoveSessionTo(*state->m_session, current);
    });
    TEST(rebuiltFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
    double const expected =
        100 * mercator::DistanceOnEarth({0, 1}, current) / mercator::DistanceOnEarth({0, 1}, {0, 4});
    double const percent = rebuiltFuture.get();
    TEST_ALMOST_EQUAL_ABS(percent, expected, 0.1, (current));
  }
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithProgressSession, FailedRebuildDoesNotCountPassedDistance)
{
  auto built = make_shared<promise<void>>();
  auto builtFuture = built->get_future();
  auto progress = make_shared<promise<vector<double>>>();
  auto progressFuture = progress->get_future();
  auto failed = make_shared<promise<RouterResultCode>>();
  auto failedFuture = failed->get_future();
  auto rebuilt = make_shared<promise<double>>();
  auto rebuiltFuture = rebuilt->get_future();
  GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, built]
  {
    state->m_session = make_unique<RoutingSession>();
    state->m_session->Init(nullptr);
    state->m_session->SetRouter(make_unique<ProgressRouter>(true), nullptr);
    state->m_session->SetRoutingCallbacks([built](RoutesResult const &, RouterResultCode) { built->set_value(); },
                                          nullptr, nullptr, nullptr);
    state->m_session->BuildRoute(Checkpoints({0, 1}, {0, 4}), RouterDelegate::kNoTimeout);
  });
  TEST(builtFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, progress, failed]
  {
    auto & session = *state->m_session;
    session.EnableFollowMode();
    MoveSessionTo(session, {0, 2});
    vector<double> percent = {session.GetCompletionPercent()};
    for (size_t i = 1; i <= 12; ++i)
      MoveSessionTo(session, {0.001 * i, 2});
    percent.push_back(session.GetCompletionPercent());
    MoveSessionTo(session, {0, 2});
    percent.push_back(session.GetCompletionPercent());
    progress->set_value(std::move(percent));
    session.RebuildRoute({0, 2}, nullptr, nullptr, [failed](RouterResultCode code) { failed->set_value(code); },
                         RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true /* adjustToPrevRoute */);
  });
  TEST(progressFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  double const expected = 100 * mercator::DistanceOnEarth({0, 1}, {0, 2}) / mercator::DistanceOnEarth({0, 1}, {0, 4});
  for (double const percent : progressFuture.get())
    TEST_ALMOST_EQUAL_ABS(percent, expected, 0.1, ());
  TEST(failedFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  auto const code = failedFuture.get();
  TEST_EQUAL(code, RouterResultCode::RouteNotFound, ());
  GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, rebuilt]
  {
    MoveSessionTo(*state->m_session, {0, 2});
    state->m_session->RebuildRoute({0, 2}, [state, rebuilt](RoutesResult const &, RouterResultCode)
    {
      MoveSessionTo(*state->m_session, {0, 2});
      rebuilt->set_value(state->m_session->GetCompletionPercent());
    }, nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true /* adjustToPrevRoute */);
  });
  TEST(rebuiltFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  double const percent = rebuiltFuture.get();
  TEST_ALMOST_EQUAL_ABS(percent, expected, 0.1, ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTestWithProgressSession, RepeatedAndSupersededRebuildsKeepTripProgress)
{
  auto built = make_shared<promise<void>>();
  auto builtFuture = built->get_future();
  GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, built]
  {
    state->m_session = make_unique<RoutingSession>();
    state->m_session->Init(nullptr);
    state->m_session->SetRouter(make_unique<ProgressRouter>(), nullptr);
    state->m_session->SetRoutingCallbacks([built](RoutesResult const &, RouterResultCode) { built->set_value(); },
                                          nullptr, nullptr, nullptr);
    state->m_session->BuildRoute(Checkpoints({0, 1}, {0, 4}), RouterDelegate::kNoTimeout);
  });
  TEST(builtFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  for (double const y : {2.0, 2.5})
  {
    auto superseded = make_shared<promise<void>>();
    auto supersededFuture = superseded->get_future();
    auto rebuilt = make_shared<promise<double>>();
    auto rebuiltFuture = rebuilt->get_future();
    GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, superseded, rebuilt, y]
    {
      auto & session = *state->m_session;
      session.EnableFollowMode();
      MoveSessionTo(session, {0, y});
      session.RebuildRoute({0, y}, [superseded](RoutesResult const &, RouterResultCode) { superseded->set_value(); },
                           nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, false);
      MoveSessionTo(session, {0, y + 0.25});
      session.RebuildRoute({0, y + 0.25}, [state, rebuilt, y](RoutesResult const &, RouterResultCode)
      {
        MoveSessionTo(*state->m_session, {0, y + 0.5});
        rebuilt->set_value(state->m_session->GetCompletionPercent());
      }, nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, false);
      MoveSessionTo(session, {0, y + 0.5});
    });
    TEST(rebuiltFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
    TEST(supersededFuture.wait_for(seconds(0)) != future_status::ready, ("The obsolete callback was delivered."));
    double const expected =
        100 * mercator::DistanceOnEarth({0, 1}, {0, y + 0.5}) / mercator::DistanceOnEarth({0, 1}, {0, 4});
    double const percent = rebuiltFuture.get();
    TEST_ALMOST_EQUAL_ABS(percent, expected, 0.1, (y));
  }

  auto newRoute = make_shared<promise<double>>();
  auto newRouteFuture = newRoute->get_future();
  GetPlatform().RunTask(Platform::Thread::Gui, [state = m_state, newRoute]
  {
    state->m_session->Reset();
    state->m_session->SetRoutingCallbacks([state, newRoute](RoutesResult const &, RouterResultCode)
    { newRoute->set_value(state->m_session->GetCompletionPercent()); }, nullptr, nullptr, nullptr);
    state->m_session->BuildRoute(Checkpoints({0, 10}, {0, 13}), RouterDelegate::kNoTimeout);
  });
  TEST(newRouteFuture.wait_for(kRouteBuildingMaxDuration) == future_status::ready, ());
  double const percent = newRouteFuture.get();
  TEST_EQUAL(percent, 0, ("A new journey must not retain the previous distance."));
}

}  // namespace routing_session_test
