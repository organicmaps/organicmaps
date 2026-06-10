#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"
#include "map/routing_manager.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include <future>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace routing_manager_tests
{
class RoutingManagerTest
{
public:
  RoutingManagerTest() : m_framework(m_frameworkParams), m_manager(m_framework.GetRoutingManager())
  {
    m_manager.RemoveRoutePoints();
  }

  ~RoutingManagerTest()
  {
    // OnRoutePointPassed() saves route points to the settings directory, which for
    // desktop tests is the repo data/ folder. The save is queued on the File thread,
    // so drain it with a barrier task before removing the artifact.
    std::promise<void> drained;
    GetPlatform().RunTask(Platform::Thread::File, [&drained]() { drained.set_value(); });
    drained.get_future().wait();
    Platform::RemoveFileIfExists(GetPlatform().SettingsPathForFile("route_points.dat"));
  }

protected:
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

  FrameworkParams m_frameworkParams;
  Framework m_framework;
  RoutingManager & m_manager;
};

UNIT_CLASS_TEST(RoutingManagerTest, FlushesLatestPendingRoutePointCallback)
{
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1.0, 1.0), false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.5, 1.5, "app://stop", 0),
                          false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 2.0, 2.0, "app://finish"),
                          false /* reorderIntermediatePoints */);

  m_manager.OnRoutePointPassed(RouteMarkType::Intermediate, 0);
  m_manager.OnRoutePointPassed(RouteMarkType::Finish, 0);

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
