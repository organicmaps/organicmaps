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

UNIT_TEST(TestRouteBuilding)
{
  RoutingSession session;
  session.Init(nullptr, nullptr);
  vector<m2::PointD> routePoints = kTestRoute;
  Route masterRoute("dummy", routePoints.begin(), routePoints.end());
  size_t counter = 0;
  bool routeBuilt(false);
  mutex waitingMutex;
  unique_lock<mutex> lock(waitingMutex);
  condition_variable cv;
  unique_ptr<DummyRouter> router = make_unique<DummyRouter>(masterRoute, DummyRouter::NoError, counter);
  session.SetRouter(move(router), nullptr);
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(),
                     [&routeBuilt, &waitingMutex, &cv](Route const &, IRouter::ResultCode)
                     {
                       lock_guard<mutex> guard(waitingMutex);
                       routeBuilt = true;
                       cv.notify_one();
                     },
                     nullptr, 0);
  // Manual check of the routeBuilded mutex to avoid spurious results.
  auto const time = steady_clock::now() + kRouteBuildingMaxDuration;
  cv.wait_until(lock, time, [&routeBuilt, &time]{return routeBuilt || steady_clock::now() > time;});
  TEST(routeBuilt, ("Route was not built."));
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
  bool routeBuilt(false);
  mutex waitingMutex;
  unique_lock<mutex> lock(waitingMutex);
  condition_variable cv;
  auto fn = [&routeBuilt, &waitingMutex, &cv](Route const &, IRouter::ResultCode)
            {
              lock_guard<mutex> guard(waitingMutex);
              routeBuilt = true;
              cv.notify_one();
            };
  unique_ptr<DummyRouter> router = make_unique<DummyRouter>(masterRoute, DummyRouter::NoError, counter);
  session.SetRouter(move(router), nullptr);

  // Go along the route.
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(), fn, nullptr, 0);
  // Manual check of the routeBuilded mutex to avoid spurious results.
  auto time = steady_clock::now() + kRouteBuildingMaxDuration;
  cv.wait_until(lock, time, [&routeBuilt, &time]{return routeBuilt || steady_clock::now() > time;});
  TEST(routeBuilt, ("Route was not built."));

  location::GpsInfo info;
  info.m_horizontalAccuracy = 0.01;
  info.m_verticalAccuracy = 0.01;
  info.m_longitude = 0.;
  info.m_latitude = 1.;
  RoutingSession::State code;
  while (info.m_latitude < kTestRoute.back().y)
  {
    code = session.OnLocationPositionChanged(
        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude), info, index);
    TEST_EQUAL(code, RoutingSession::State::OnRoute, ());
    info.m_latitude += 0.01;
  }
  TEST_EQUAL(counter, 1, ());

  // Rebuild route and go in opposite direction. So initiate a route rebuilding flag.
  time = steady_clock::now() + kRouteBuildingMaxDuration;
  counter = 0;
  routeBuilt = false;
  session.BuildRoute(kTestRoute.front(), kTestRoute.back(), fn, nullptr, 0);
  cv.wait_until(lock, time, [&routeBuilt, &time]{return routeBuilt || steady_clock::now() > time;});
  TEST(routeBuilt, ("Route was not built."));

  info.m_longitude = 0.;
  info.m_latitude = 1.;
  for (size_t i = 0; i < 10; ++i)
  {
    code = session.OnLocationPositionChanged(
        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude), info, index);
    info.m_latitude -= 0.1;
  }
  TEST_EQUAL(code, RoutingSession::State::RouteNeedRebuild, ());
}
}  // namespace routing
