#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/graph.hpp"
#include "std/map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
struct Edge
{
  Edge(unsigned v, double w) : v(v), w(w) {}

  unsigned GetTarget() const { return v; }
  double GetWeight() const { return w; }

  unsigned v;
  double w;
};

class UndirectedGraph : public Graph<unsigned, Edge, UndirectedGraph>
{
public:
  void AddEdge(unsigned u, unsigned v, unsigned w)
  {
    m_adjs[u].push_back(Edge(v, w));
    m_adjs[v].push_back(Edge(u, w));
  }

private:
  friend class Graph<unsigned, Edge, UndirectedGraph>;

  void GetAdjacencyListImpl(unsigned v, vector<Edge> & adj) const
  {
    auto it = m_adjs.find(v);
    if (it == m_adjs.end())
      return;
    adj.assign(it->second.begin(), it->second.end());
  }

  double HeuristicCostEstimateImpl(unsigned v, unsigned w) const { return 0; }

  map<unsigned, vector<Edge>> m_adjs;
};

void TestAStar(UndirectedGraph const & graph, vector<unsigned> const & expectedRoute)
{
  using TAlgorithm = AStarAlgorithm<UndirectedGraph>;

  TAlgorithm algo;
  algo.SetGraph(graph);
  vector<unsigned> actualRoute;
  TEST_EQUAL(TAlgorithm::Result::OK,
             algo.FindPath(vector<unsigned>{0}, vector<unsigned>{4}, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute, ());

  actualRoute.clear();
  TEST_EQUAL(TAlgorithm::Result::OK,
             algo.FindPathBidirectional(vector<unsigned>{0}, vector<unsigned>{4}, actualRoute), ());
  TEST_EQUAL(expectedRoute, actualRoute, ());
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

  vector<unsigned> expectedRoute = {0, 1, 2, 3, 4};

  TestAStar(graph, expectedRoute);
}
}  // namespace routing
