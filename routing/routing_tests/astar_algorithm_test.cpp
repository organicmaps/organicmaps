#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "std/map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing_test
{

using namespace  routing;

struct Edge
{
  Edge(unsigned v, double w) : v(v), w(w) {}

  unsigned GetTarget() const { return v; }
  double GetWeight() const { return w; }

  unsigned v;
  double w;
};

class UndirectedGraph
{
public:
  using TVertexType = unsigned;
  using TEdgeType = Edge;

  void AddEdge(unsigned u, unsigned v, unsigned w)
  {
    m_adjs[u].push_back(Edge(v, w));
    m_adjs[v].push_back(Edge(u, w));
  }

  void GetAdjacencyList(unsigned v, vector<Edge> & adj) const
  {
    adj.clear();
    auto const it = m_adjs.find(v);
    if (it != m_adjs.end())
      adj = it->second;
  }

  void GetIngoingEdgesList(unsigned v, vector<Edge> & adj) const
  {
    GetAdjacencyList(v, adj);
  }

  void GetOutgoingEdgesList(unsigned v, vector<Edge> & adj) const
  {
    GetAdjacencyList(v, adj);
  }

  double HeuristicCostEstimate(unsigned v, unsigned w) const { return 0; }

private:
  map<unsigned, vector<Edge>> m_adjs;
};

void TestAStar(UndirectedGraph & graph, vector<unsigned> const & expectedRoute, double const & expectedDistance)
{
  using TAlgorithm = AStarAlgorithm<UndirectedGraph>;

  TAlgorithm algo;

  RoutingResult<unsigned> actualRoute;
  TEST_EQUAL(TAlgorithm::Result::OK, algo.FindPath(graph, 0u, 4u, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute.path, ());
  TEST_ALMOST_EQUAL_ULPS(expectedDistance, actualRoute.distance, ());

  actualRoute.path.clear();
  TEST_EQUAL(TAlgorithm::Result::OK, algo.FindPathBidirectional(graph, 0u, 4u, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute.path, ());
  TEST_ALMOST_EQUAL_ULPS(expectedDistance, actualRoute.distance, ());
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

}  // namespace routing_test
