#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_graph.hpp"
#include "routing/base/routing_result.hpp"

#include "routing/routing_tests/routing_algorithm.hpp"

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace astar_algorithm_test
{
using namespace routing;
using namespace routing_test;
using namespace std;

using Algorithm = AStarAlgorithm<uint32_t, SimpleEdge, double>;

void TestAStar(UndirectedGraph & graph, vector<unsigned> const & expectedRoute, double const & expectedDistance)
{
  Algorithm algo;

  Algorithm::ParamsForTests<> params(graph, 0u /* startVertex */, 4u /* finishVertex */);

  RoutingResult<unsigned /* Vertex */, double /* Weight */> actualRoute;
  TEST_EQUAL(Algorithm::Result::OK, algo.FindPath(params, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute.m_path, ());
  TEST_ALMOST_EQUAL_ULPS(expectedDistance, actualRoute.m_distance, ());

  actualRoute.m_path.clear();
  TEST_EQUAL(Algorithm::Result::OK, algo.FindPathBidirectional(params, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute.m_path, ());
  TEST_ALMOST_EQUAL_ULPS(expectedDistance, actualRoute.m_distance, ());
}

UNIT_TEST(AStarAlgorithm_Sample)
{
  UndirectedGraph graph;

  // Inserts edges in a format: <source, target, weight>.
  graph.AddEdge(0, 1, 10);
  graph.AddEdge(1, 2, 5);
  graph.AddEdge(2, 3, 5);
  graph.AddEdge(2, 4, 10);
  graph.AddEdge(3, 4, 3);

  vector<unsigned> const expectedRoute = {0, 1, 2, 3, 4};

  TestAStar(graph, expectedRoute, 23);
}

UNIT_TEST(AStarAlgorithm_CheckLength)
{
  UndirectedGraph graph;

  // Inserts edges in a format: <source, target, weight>.
  graph.AddEdge(0, 1, 10);
  graph.AddEdge(1, 2, 5);
  graph.AddEdge(2, 3, 5);
  graph.AddEdge(2, 4, 10);
  graph.AddEdge(3, 4, 3);

  auto checkLength = [](double weight) { return weight < 23; };
  Algorithm algo;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, 0u /* startVertex */, 4u /* finishVertex */,
                                                          std::move(checkLength));

  RoutingResult<unsigned /* Vertex */, double /* Weight */> routingResult;
  Algorithm::Result result = algo.FindPath(params, routingResult);
  // Best route weight is 23 so we expect to find no route with restriction |weight < 23|.
  TEST_EQUAL(result, Algorithm::Result::NoPath, ());

  routingResult = {};
  result = algo.FindPathBidirectional(params, routingResult);
  // Best route weight is 23 so we expect to find no route with restriction |weight < 23|.
  TEST_EQUAL(result, Algorithm::Result::NoPath, ());
}

UNIT_TEST(AdjustRoute)
{
  UndirectedGraph graph;

  for (unsigned int i = 0; i < 5; ++i)
    graph.AddEdge(i /* from */, i + 1 /* to */, 1 /* weight */);

  graph.AddEdge(6, 0, 1);
  graph.AddEdge(6, 1, 1);
  graph.AddEdge(6, 2, 1);

  // Each edge contains {vertexId, weight}.
  vector<SimpleEdge> const prevRoute = {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}};

  auto checkLength = [](double weight) { return weight <= 1.0; };
  Algorithm algo;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, 6 /* startVertex */, {} /* finishVertex */,
                                                          std::move(checkLength));

  RoutingResult<unsigned /* Vertex */, double /* Weight */> result;
  auto const code = algo.AdjustRoute(params, prevRoute, result);

  vector<unsigned> const expectedRoute = {6, 2, 3, 4, 5};
  TEST_EQUAL(code, Algorithm::Result::OK, ());
  TEST_EQUAL(result.m_path, expectedRoute, ());
  TEST_EQUAL(result.m_distance, 4.0, ());
}

UNIT_TEST(AdjustRouteNoPath)
{
  UndirectedGraph graph;

  for (unsigned int i = 0; i < 5; ++i)
    graph.AddEdge(i /* from */, i + 1 /* to */, 1 /* weight */);

  // Each edge contains {vertexId, weight}.
  vector<SimpleEdge> const prevRoute = {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}};

  auto checkLength = [](double weight) { return weight <= 1.0; };
  Algorithm algo;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, 6 /* startVertex */, {} /* finishVertex */,
                                                          std::move(checkLength));
  RoutingResult<unsigned /* Vertex */, double /* Weight */> result;
  auto const code = algo.AdjustRoute(params, prevRoute, result);

  TEST_EQUAL(code, Algorithm::Result::NoPath, ());
  TEST(result.m_path.empty(), ());
}

UNIT_TEST(AdjustRouteOutOfLimit)
{
  UndirectedGraph graph;

  for (unsigned int i = 0; i < 5; ++i)
    graph.AddEdge(i /* from */, i + 1 /* to */, 1 /* weight */);

  graph.AddEdge(6, 2, 2);

  // Each edge contains {vertexId, weight}.
  vector<SimpleEdge> const prevRoute = {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}};

  auto checkLength = [](double weight) { return weight <= 1.0; };
  Algorithm algo;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, 6 /* startVertex */, {} /* finishVertex */,
                                                          std::move(checkLength));

  RoutingResult<unsigned /* Vertex */, double /* Weight */> result;
  auto const code = algo.AdjustRoute(params, prevRoute, result);

  TEST_EQUAL(code, Algorithm::Result::NoPath, ());
  TEST(result.m_path.empty(), ());
}
}  // namespace astar_algorithm_test
