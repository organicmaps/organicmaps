#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
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
}  // namespace routing_manager_tests
