#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

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
  size_t count = 0;
  for (auto const & point : points)
    if (point.m_pointType == RouteMarkType::Intermediate)
      ++count;
  return count;
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
  TEST_EQUAL(pointsBefore.size(), RoutePointsLayout::kMaxIntermediatePointsCount + 2, ());
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
}  // namespace routing_manager_tests
