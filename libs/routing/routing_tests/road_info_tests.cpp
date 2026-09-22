#include "geometry/mercator.hpp"
#include "routing/road_info.hpp"
#include "routing/routing_tests/road_graph_builder.hpp"
#include "testing/testing.hpp"

namespace road_info_tests
{
using namespace routing;
using namespace routing_test;

Edge MakeEdge(uint32_t id, m2::PointD a, m2::PointD b, uint32_t segment = 0)
{
  return Edge::MakeReal(MakeTestFeatureID(id), true, segment, geometry::MakePointWithAltitudeForTesting(a),
                        geometry::MakePointWithAltitudeForTesting(b));
}

UNIT_TEST(RoadInfo_MatchingRejectsAmbiguityAndWrongDirection)
{
  auto east = MakeEdge(0, {0, 0}, {0.001, 0});
  auto west = east.GetReverseEdge();
  auto parallel = MakeEdge(1, {0, 0.00002}, {0.001, 0.00002});
  std::vector<IRoadGraph::EdgeProjectionT> candidates = {
      {west, geometry::MakePointWithAltitudeForTesting({0.0005, 0})},
      {east, geometry::MakePointWithAltitudeForTesting({0.0005, 0})}};
  auto match = MatchRoad({0.0005, 0}, {1, 0}, 5, candidates);
  TEST(match && match->first == east, ());
  TEST(!MatchRoad({0.0005, 0}, {0, 1}, 5, candidates), ());
  TEST(!MatchRoad({0.0005, 0}, {}, 5, candidates), ());
  TEST(!MatchRoad({0.0005, 0}, {1, 0}, 100, candidates), ());
  candidates.emplace_back(parallel, geometry::MakePointWithAltitudeForTesting({0.0005, 0.00002}));
  TEST(!MatchRoad({0.0005, 0}, {1, 0}, 5, candidates), ());
}

UNIT_TEST(RoadInfo_CameraDistanceFollowsBend)
{
  RoadGraphMockSource graph;
  graph.AddRoad(MakeRoadInfoForTesting(true, 50, {{0, 0}, {0.001, 0}, {0.001, 0.001}}));
  RoadInfoSnapshot result;
  FindRoadCamera(graph, MakeEdge(0, {0, 0}, {0.001, 0}), {0.0005, 0}, [](Edge const & edge)
  {
    return edge.GetSegId() == 1 ? std::vector<RouteSegment::SpeedCamera>{{0.5, 60}}
                                : std::vector<RouteSegment::SpeedCamera>{};
  }, result);
  double const expected =
      mercator::DistanceOnEarth({0.0005, 0}, {0.001, 0}) + mercator::DistanceOnEarth({0.001, 0}, {0.001, 0.0005});
  TEST_ALMOST_EQUAL_ABS(result.m_cameraDistance, expected, 0.1, ());
  TEST_ALMOST_EQUAL_ABS(result.m_cameraLimitMps, 60.0 / 3.6, 0.001, ());
}

UNIT_TEST(RoadInfo_CameraDoesNotGuessJunction)
{
  RoadGraphMockSource graph;
  graph.AddRoad(MakeRoadInfoForTesting(true, 50, {{0, 0}, {0.001, 0}}));
  graph.AddRoad(MakeRoadInfoForTesting(true, 50, {{0.001, 0}, {0.002, 0}}));
  graph.AddRoad(MakeRoadInfoForTesting(true, 50, {{0.001, 0}, {0.001, 0.001}}));
  RoadInfoSnapshot result;
  FindRoadCamera(graph, MakeEdge(0, {0, 0}, {0.001, 0}), {0.0005, 0}, [](Edge const & edge)
  {
    return edge.GetFeatureId().m_index == 1 ? std::vector<RouteSegment::SpeedCamera>{{0.5, 60}}
                                            : std::vector<RouteSegment::SpeedCamera>{};
  }, result);
  TEST_LESS(result.m_cameraDistance, 0, ());
}

UNIT_TEST(RoadInfo_ReversePassIgnoresCameraBehind)
{
  RoadGraphMockSource graph;
  graph.AddRoad(MakeRoadInfoForTesting(true, 50, {{0, 0}, {0.001, 0}}));
  RoadInfoSnapshot result;
  FindRoadCamera(graph, MakeEdge(0, {0, 0}, {0.001, 0}).GetReverseEdge(), {0.0008, 0},
                 [](Edge const &) { return std::vector<RouteSegment::SpeedCamera>{{0.2, 40}, {0.9, 60}}; }, result);
  TEST_ALMOST_EQUAL_ABS(result.m_cameraDistance, mercator::DistanceOnEarth({0.0008, 0}, {0.0002, 0}), 0.1, ());
  TEST_ALMOST_EQUAL_ABS(result.m_cameraLimitMps, 40.0 / 3.6, 0.001, ());
}
}  // namespace road_info_tests
