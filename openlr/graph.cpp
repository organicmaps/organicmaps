#include "openlr/graph.hpp"

#include "indexer/index.hpp"

using namespace routing;

namespace openlr
{
namespace
{
using EdgeGetter = void (IRoadGraph::*)(Junction const &, RoadGraphBase::TEdgeVector &) const;

void GetRegularEdges(Junction const & junction, IRoadGraph const & graph,
                     EdgeGetter const edgeGetter,
                     map<openlr::Graph::Junction, Graph::EdgeVector> & cache,
                     Graph::EdgeVector & edges)
{
  auto const it = cache.find(junction);
  if (it == end(cache))
  {
    auto & es = cache[junction];
    (graph.*edgeGetter)(junction, es);
    edges.insert(end(edges), begin(es), end(es));
  }
  else
  {
    auto const & es = it->second;
    edges.insert(end(edges), begin(es), end(es));
  }
}
}  // namespace

Graph::Graph(Index const & index, shared_ptr<CarModelFactory> carModelFactory)
  : m_graph(index, IRoadGraph::Mode::ObeyOnewayTag, carModelFactory)
{
}

void Graph::GetOutgoingEdges(Junction const & junction, EdgeVector & edges)
{
  GetRegularOutgoingEdges(junction, edges);
  m_graph.GetFakeOutgoingEdges(junction, edges);
}

void Graph::GetIngoingEdges(Junction const & junction, EdgeVector & edges)
{
  GetRegularIngoingEdges(junction, edges);
  m_graph.GetFakeIngoingEdges(junction, edges);
}

void Graph::GetRegularOutgoingEdges(Junction const & junction, EdgeVector & edges)
{
  GetRegularEdges(junction, m_graph, &IRoadGraph::GetRegularOutgoingEdges, m_outgoingCache, edges);
}

void Graph::GetRegularIngoingEdges(Junction const & junction, EdgeVector & edges)
{
  GetRegularEdges(junction, m_graph, &IRoadGraph::GetRegularIngoingEdges, m_ingoingCache, edges);
}

void Graph::FindClosestEdges(m2::PointD const & point, uint32_t const count,
                             vector<pair<Edge, Junction>> & vicinities) const
{
  m_graph.FindClosestEdges(point, count, vicinities);
}

void Graph::AddFakeEdges(Junction const & junction, vector<pair<Edge, Junction>> const & vicinities)
{
  m_graph.AddFakeEdges(junction, vicinities);
}

void Graph::AddIngoingFakeEdge(Edge const & e)
{
  m_graph.AddIngoingFakeEdge(e);
}

void Graph::AddOutgoingFakeEdge(Edge const & e)
{
  m_graph.AddOutgoingFakeEdge(e);
}
}  // namespace openlr
