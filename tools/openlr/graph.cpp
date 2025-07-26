#include "openlr/graph.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

using namespace routing;
using namespace std;

namespace openlr
{
namespace
{
using EdgeGetter = void (IRoadGraph::*)(geometry::PointWithAltitude const &, RoadGraphBase::EdgeListT &) const;

void GetRegularEdges(geometry::PointWithAltitude const & junction, IRoadGraph const & graph,
                     EdgeGetter const edgeGetter, Graph::EdgeCacheT & cache, Graph::EdgeListT & edges)
{
  auto const it = cache.find(junction);
  if (it == end(cache))
  {
    auto & es = cache[junction];
    (graph.*edgeGetter)(junction, es);
    edges.append(begin(es), end(es));
  }
  else
  {
    auto const & es = it->second;
    edges.append(begin(es), end(es));
  }
}
}  // namespace

Graph::Graph(DataSource & dataSource, shared_ptr<CarModelFactory> carModelFactory)
  : m_dataSource(dataSource, nullptr /* numMwmIDs */)
  , m_graph(m_dataSource, IRoadGraph::Mode::ObeyOnewayTag, carModelFactory)
{}

void Graph::GetOutgoingEdges(Junction const & junction, EdgeListT & edges)
{
  GetRegularOutgoingEdges(junction, edges);
  m_graph.GetFakeOutgoingEdges(junction, edges);
}

void Graph::GetIngoingEdges(Junction const & junction, EdgeListT & edges)
{
  GetRegularIngoingEdges(junction, edges);
  m_graph.GetFakeIngoingEdges(junction, edges);
}

void Graph::GetRegularOutgoingEdges(Junction const & junction, EdgeListT & edges)
{
  GetRegularEdges(junction, m_graph, &IRoadGraph::GetRegularOutgoingEdges, m_outgoingCache, edges);
}

void Graph::GetRegularIngoingEdges(Junction const & junction, EdgeListT & edges)
{
  GetRegularEdges(junction, m_graph, &IRoadGraph::GetRegularIngoingEdges, m_ingoingCache, edges);
}

void Graph::FindClosestEdges(m2::PointD const & point, uint32_t const count,
                             vector<pair<Edge, Junction>> & vicinities) const
{
  m_graph.FindClosestEdges(mercator::RectByCenterXYAndSizeInMeters(point, FeaturesRoadGraph::kClosestEdgesRadiusM),
                           count, vicinities);
}

void Graph::AddIngoingFakeEdge(Edge const & e)
{
  m_graph.AddIngoingFakeEdge(e);
}

void Graph::AddOutgoingFakeEdge(Edge const & e)
{
  m_graph.AddOutgoingFakeEdge(e);
}

void Graph::GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const
{
  m_graph.GetFeatureTypes(featureId, types);
}
}  // namespace openlr
