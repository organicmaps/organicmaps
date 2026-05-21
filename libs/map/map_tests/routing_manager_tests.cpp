#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_manager.hpp"

#include "geometry/mercator.hpp"

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

UNIT_CLASS_TEST(RoutingManagerTest, FlushesPendingRoutePointCallbacksInOrder)
{
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Start, 1.0, 1.0), false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Intermediate, 1.5, 1.5, "app://stop", 0),
                          false /* reorderIntermediatePoints */);
  m_manager.AddRoutePoint(MakePoint(RouteMarkType::Finish, 2.0, 2.0, "app://finish"),
                          false /* reorderIntermediatePoints */);

  m_manager.OnRoutePointPassed(RouteMarkType::Intermediate, 0);
  m_manager.OnRoutePointPassed(RouteMarkType::Finish, 0);

  std::vector<std::string> callbacks;
  m_manager.SetRoutePointCallback([&callbacks](std::string const & callback) { callbacks.push_back(callback); });

  TEST_EQUAL(callbacks.size(), 2, ());
  TEST_EQUAL(callbacks[0], "app://stop", ());
  TEST_EQUAL(callbacks[1], "app://finish", ());
}

UNIT_CLASS_TEST(RoutingManagerTest, AddsRoutePointsInBatch)
{
  std::vector<RouteMarkData> points;
  points.push_back(MakePoint(RouteMarkType::Start, 1.0, 1.0, "app://start"));
  points.push_back(MakePoint(RouteMarkType::Intermediate, 1.5, 1.5, "app://stop", 0));
  points.push_back(MakePoint(RouteMarkType::Finish, 2.0, 2.0, "app://finish"));

  m_manager.AddRoutePoints(std::move(points), false /* reorderIntermediatePoints */);

  auto const routePoints = m_manager.GetRoutePoints();
  TEST_EQUAL(routePoints.size(), 3, ());
  TEST_EQUAL(static_cast<int>(routePoints[0].m_pointType), static_cast<int>(RouteMarkType::Start), ());
  TEST_EQUAL(routePoints[0].m_callback, "app://start", ());
  TEST_EQUAL(static_cast<int>(routePoints[1].m_pointType), static_cast<int>(RouteMarkType::Intermediate), ());
  TEST_EQUAL(routePoints[1].m_callback, "app://stop", ());
  TEST_EQUAL(static_cast<int>(routePoints[2].m_pointType), static_cast<int>(RouteMarkType::Finish), ());
  TEST_EQUAL(routePoints[2].m_callback, "app://finish", ());
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
