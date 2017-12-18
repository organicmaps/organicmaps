#include "openlr/graph.hpp"

#include "indexer/index.hpp"

using namespace routing;

namespace openlr
{
Graph::Graph(Index const & index, shared_ptr<CarModelFactory> carModelFactory)
  : m_graph(index, IRoadGraph::Mode::ObeyOnewayTag, carModelFactory)
{
}

void Graph::GetOutgoingEdges(Junction const & junction, EdgeVector & edges) const
{
  m_graph.GetOutgoingEdges(junction, edges);
}

void Graph::GetIngoingEdges(Junction const & junction, EdgeVector & edges) const
{
  m_graph.GetIngoingEdges(junction, edges);
}

void Graph::GetRegularOutgoingEdges(Junction const & junction, EdgeVector & edges) const
{
  m_graph.GetRegularOutgoingEdges(junction, edges);
}

void Graph::GetRegularIngoingEdges(Junction const & junction, EdgeVector & edges) const
{
  m_graph.GetRegularIngoingEdges(junction, edges);
}

void Graph::FindClosestEdges(m2::PointD const & point, uint32_t count,
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
