#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include <algorithm>
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

UNIT_TEST(RoutingManager_OptimizeRoutePointsUsesRouteMarks)
{
  std::vector<double> const intermediateCoordinates = {3.0, 1.0, 2.0};
  std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0};

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
  start.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */);
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
  finish.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(finish), false /* reorderIntermediatePoints */);
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
    point.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */);
  }

  routingManager.OptimizeRoutePoints();
  auto const actualPoints = routingManager.GetRoutePoints();

  TEST_EQUAL(actualPoints.size(), expectedCoordinates.size() + 2, ());
  for (size_t i = 0; i < expectedCoordinates.size(); ++i)
    TEST_EQUAL(actualPoints[i + 1].m_position, m2::PointD(expectedCoordinates[i], 0.0), (i));
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsPreservesPassedStops)
{
  std::vector<double> const intermediateCoordinates = {1.0, 4.0, 2.0, 3.0};
  std::vector<double> const expectedCoordinates = {1.0, 2.0, 3.0, 4.0};

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();
  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 0.0);
  start.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */);
  auto finish = MakeRoutePoint(RouteMarkType::Finish, 0, 10.0);
  finish.m_position.y = 0.0;
  routingManager.AddRoutePoint(std::move(finish), false /* reorderIntermediatePoints */);
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]);
    point.m_position.y = 0.0;
    routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */);
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
}  // namespace routing_manager_tests
