#include "routing/bicycle_directions.hpp"
#include "routing/car_model.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "geometry/point2d.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

namespace
{
using namespace routing;
using namespace routing::turns;

class AStarRoutingResultGraph : public IRoutingResultGraph
{
public:
  virtual TUnpackedPathSegments const & GetSegments() const override
  {
    return m_pathSegments;
  }

  virtual void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const override
  {
    if (node >= m_routeEdges.size() || node >= m_adjacentEdges.size())
    {
      ASSERT(false, ());
      return;
    }

    AdjacentEdgesMap::const_iterator adjacentEdges = m_adjacentEdges.find(node);
    if (adjacentEdges == m_adjacentEdges.cend())
    {
      ASSERT(false, ());
      return;
    }

    ingoingCount = adjacentEdges->second.ingoingTurnCount;
    outgoingTurns = adjacentEdges->second.outgoingTurns;
  }

  virtual double GetPathLength() const override { return m_routeLength; }

  virtual m2::PointD const & GetStartPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.front().GetStartJunction().GetPoint();
  }

  virtual m2::PointD const & GetEndPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.back().GetEndJunction().GetPoint();
  }

  AStarRoutingResultGraph(IRoadGraph::TEdgeVector const & routeEdges,
                          AdjacentEdgesMap const & adjacentEdges,
                          TUnpackedPathSegments const & pathSegments)
    : m_routeEdges(routeEdges), m_adjacentEdges(adjacentEdges), m_pathSegments(pathSegments),
      m_routeLength(0)
  {
    // @TODO(bykoianko) Calculate length of routeEdges |m_routeLength|.
  }

  ~AStarRoutingResultGraph() {}

private:
   IRoadGraph::TEdgeVector const & m_routeEdges;
   AdjacentEdgesMap const & m_adjacentEdges;
   TUnpackedPathSegments const & m_pathSegments;
   double m_routeLength;
};
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine()
{
}

void BicycleDirectionsEngine::Generate(IRoadGraph const & graph, vector<Junction> const & path,
                                       Route::TTimes & times,
                                       Route::TTurns & turnsDir,
                                       my::Cancellable const & cancellable)
{
  CHECK_GREATER(path.size(), 1, ());
  times.clear();
  turnsDir.clear();
  m_adjacentEdges.clear();

  CalculateTimes(graph, path, times);

  auto emptyPathWorkaround = [&]()
  {
    turnsDir.emplace_back(path.size() - 1, turns::TurnDirection::ReachedYourDestination);
    this->m_adjacentEdges[0] = {{}, 1}; // There's one ingoing edge to the finish.
  };

  IRoadGraph::TEdgeVector routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path"));
    emptyPathWorkaround();
    return;
  }
  if (routeEdges.empty())
  {
    ASSERT(false, ());
    emptyPathWorkaround();
    return;
  }

  // Filling |m_adjacentEdges|.
  int i = 0;
  for (auto const & junction : path)
  {
    IRoadGraph::TEdgeVector outgoingEdges, ingoingEdges;
    graph.GetOutgoingEdges(junction, outgoingEdges);
    graph.GetIngoingEdges(junction, ingoingEdges);

    AdjacentEdges adjacentEdges = {{}, ingoingEdges.size()};
    adjacentEdges.outgoingTurns.reserve(outgoingEdges.size());
    for (auto const & outgoingEdge : outgoingEdges)
    {
      // @TODO(bykoianko) Calculate angle based on Junction path[i-1], path[i] and outgoingEdge edges.
      // @TODO(bykoianko) Calculate correct HighwayClass.
      adjacentEdges.outgoingTurns.emplace_back(0., outgoingEdge.GetFeatureId().m_index,
                                               ftypes::HighwayClass::Tertiary);
    }
    m_adjacentEdges.insert(make_pair(i, move(adjacentEdges)));

    // @TODO(bykoianko) Fill m_pathSegments based on |path|.
    i += 1;
  }

  AStarRoutingResultGraph resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;
  vector<m2::PointD> routePoints;
  Route::TTimes turnAnnotationTimes;
  Route::TStreets streetNames;
  MakeTurnAnnotation(resultGraph, delegate, routePoints, turnsDir, turnAnnotationTimes, streetNames);
}
}  // namespace routing
