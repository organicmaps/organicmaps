#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_session.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include "std/chrono.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
// Simple router. It returns route given to him on creation.
class DummyRouter : public IRouter
{
private:
  Route & m_route;
  ResultCode m_code;
  size_t & m_buildCount;

public:
  DummyRouter(Route & route, ResultCode code, size_t & buildCounter)
    : m_route(route), m_code(code), m_buildCount(buildCounter)
  {
  }
  string GetName() const override { return "dummy"; }
  void ClearState() override {}
  ResultCode CalculateRoute(m2::PointD const & /* startPoint */,
                            m2::PointD const & /* startDirection */,
                            m2::PointD const & /* finalPoint */,
                            RouterDelegate const & /* delegate */, Route & route) override
  {
    ++m_buildCount;
    route = m_route;
    return m_code;
  }
};

static vector<m2::PointD> kTestRoute = {{0., 1.}, {0., 2.}, {0., 3.}, {0., 4.}};
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

UNIT_TEST(TestRouteBuilding)
{
  RoutingSession session;
  session.Init(nullptr, nullptr);
  vector<m2::PointD> routePoints = kTestRoute;
  Route masterRoute("dummy", routePoints.begin(), routePoints.end());
  size_t counter = 0;
  TimedSignal timedSignal;
  unique_ptr<DummyRouter> router = make_unique<DummyRouter>(masterRoute, DummyRouter::NoError, counter);
  session.SetRouter(move(router), nullptr);
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&timedSignal](Route const &, IRouter::ResultCode)
                     {
                       timedSignal.Signal();
                     },
                     nullptr, 0);
  // Manual check of the routeBuilded mutex to avoid spurious results.
  auto const time = steady_clock::now() + kRouteBuildingMaxDuration;
  TEST(timedSignal.WaitUntil(time), ("Route was not built."));
  TEST_EQUAL(counter, 1, ());
}

UNIT_TEST(TestRouteRebuilding)
{
  Index index;
  RoutingSession session;
  session.Init(nullptr, nullptr);
  vector<m2::PointD> routePoints = kTestRoute;
  Route masterRoute("dummy", routePoints.begin(), routePoints.end());
  size_t counter = 0;
  unique_ptr<DummyRouter> router = make_unique<DummyRouter>(masterRoute, DummyRouter::NoError, counter);
  session.SetRouter(move(router), nullptr);

  // Go along the route.
  TimedSignal alongTimedSignal;
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&alongTimedSignal](Route const &, IRouter::ResultCode)
                     {
                       alongTimedSignal.Signal();
                     },
                     nullptr, 0);
  // Manual check of the routeBuilded mutex to avoid spurious results.
  auto time = steady_clock::now() + kRouteBuildingMaxDuration;
  TEST(alongTimedSignal.WaitUntil(time), ("Route was not built."));

  location::GpsInfo info;
  info.m_horizontalAccuracy = 0.01;
  info.m_verticalAccuracy = 0.01;
  info.m_longitude = 0.;
  info.m_latitude = 1.;
  RoutingSession::State code;
  while (info.m_latitude < kTestRoute.back().y)
  {
    code = session.OnLocationPositionChanged(info, index);
    TEST_EQUAL(code, RoutingSession::State::OnRoute, ());
    info.m_latitude += 0.01;
  }
  TEST_EQUAL(counter, 1, ());

  // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
  time = steady_clock::now() + kRouteBuildingMaxDuration;
  counter = 0;
  TimedSignal oppositeTimedSignal;
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&oppositeTimedSignal](Route const &, IRouter::ResultCode)
                     {
                       oppositeTimedSignal.Signal();
                     },
                     nullptr, 0);
  TEST(oppositeTimedSignal.WaitUntil(time), ("Route was not built."));

  info.m_longitude = 0.;
  info.m_latitude = 1.;
  for (size_t i = 0; i < 10; ++i)
  {
    code = session.OnLocationPositionChanged(info, index);
    info.m_latitude -= 0.1;
  }
  TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
}

UNIT_TEST(TestFollowRouteFlagPersistence)
{
  Index index;
  RoutingSession session;
  session.Init(nullptr, nullptr);
  vector<m2::PointD> routePoints = kTestRoute;
  Route masterRoute("dummy", routePoints.begin(), routePoints.end());
  size_t counter = 0;
  unique_ptr<DummyRouter> router = make_unique<DummyRouter>(masterRoute, DummyRouter::NoError, counter);
  session.SetRouter(move(router), nullptr);

  // Go along the route.
  TimedSignal alongTimedSignal;
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&alongTimedSignal](Route const &, IRouter::ResultCode)
                     {
                       alongTimedSignal.Signal();
                     },
                     nullptr, 0);
  // Manual check of the routeBuilded mutex to avoid spurious results.
  auto time = steady_clock::now() + kRouteBuildingMaxDuration;
  TEST(alongTimedSignal.WaitUntil(time), ("Route was not built."));

  TEST(!session.IsFollowing(), ());
  session.EnableFollowMode();
  TEST(session.IsFollowing(), ());

  location::GpsInfo info;
  info.m_horizontalAccuracy = 0.01;
  info.m_verticalAccuracy = 0.01;
  info.m_longitude = 0.;
  info.m_latitude = 1.;
  RoutingSession::State code;
  while (info.m_latitude < kTestRoute.back().y)
  {
    session.OnLocationPositionChanged(info, index);
    TEST(session.IsOnRoute() , ());
    TEST(session.IsFollowing(), ());
    info.m_latitude += 0.01;
  }
  TEST_EQUAL(counter, 1, ());

  // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
  time = steady_clock::now() + kRouteBuildingMaxDuration;
  counter = 0;
  TimedSignal oppositeTimedSignal;
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&oppositeTimedSignal](Route const &, IRouter::ResultCode)
                     {
                       oppositeTimedSignal.Signal();
                     },
                     nullptr, 0);
  TEST(oppositeTimedSignal.WaitUntil(time), ("Route was not built."));

  // Manual route building resets the following flag.
  TEST(!session.IsFollowing(), ());
  session.EnableFollowMode();
  TEST(session.IsFollowing(), ());
  info.m_longitude = 0.;
  info.m_latitude = 1.;
  for (size_t i = 0; i < 10; ++i)
  {
    code = session.OnLocationPositionChanged(info, index);
    info.m_latitude -= 0.1;
  }
  TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
  TEST(session.IsFollowing(), ());

  TimedSignal rebuildTimedSignal;
  session.RebuildRoute(kTestRoute.front(),
                       [&rebuildTimedSignal](Route const &, IRouter::ResultCode)
                       {
                         rebuildTimedSignal.Signal();
                       },
                       nullptr, 0);
  TEST(rebuildTimedSignal.WaitUntil(time), ("Route was not built."));
  TEST(session.IsFollowing(), ());
}
}  // namespace routing
