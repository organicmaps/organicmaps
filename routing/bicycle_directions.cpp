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
  AStarRoutingResultGraph(IRoadGraph::TEdgeVector const & routeEdges,
                          AdjacentEdgesMap const & adjacentEdges,
                          TUnpackedPathSegments const & pathSegments)
    : m_routeEdges(routeEdges), m_adjacentEdges(adjacentEdges), m_pathSegments(pathSegments),
      m_routeLength(0)
  {
    for (auto const & edge : routeEdges)
    {
      m_routeLength += MercatorBounds::DistanceOnEarth(edge.GetStartJunction().GetPoint(),
                                                       edge.GetEndJunction().GetPoint());
    }
  }

  ~AStarRoutingResultGraph() {}

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

private:
   IRoadGraph::TEdgeVector const & m_routeEdges;
   AdjacentEdgesMap const & m_adjacentEdges;
   TUnpackedPathSegments const & m_pathSegments;
   double m_routeLength;
};

ftypes::HighwayClass GetHighwayClass(FeatureID const & featureId, Index const & index)
{
  ftypes::HighwayClass highWayClass = ftypes::HighwayClass::Undefined;
  MwmSet::MwmId const & mwmId = featureId.m_mwmId;
  uint32_t const featureIndex = featureId.m_index;

  FeatureType ft;
  Index::FeaturesLoaderGuard loader(index, mwmId);
  loader.GetFeatureByIndex(featureIndex, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
  highWayClass = ftypes::GetHighwayClass(ft);
  ASSERT_NOT_EQUAL(highWayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highWayClass, ftypes::HighwayClass::Undefined, ());
  return highWayClass;
}
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine(Index const & index) : m_index(index)
{
}

void BicycleDirectionsEngine::Generate(IRoadGraph const & graph, vector<Junction> const & path,
                                       Route::TTimes & times,
                                       Route::TTurns & turnsDir,
                                       vector<m2::PointD> & routeGeometry,
                                       my::Cancellable const & cancellable)
{
  LOG(LINFO, ("BicycleDirectionsEngine::Generate"));
  CHECK_GREATER(path.size(), 1, ());
  times.clear();
  turnsDir.clear();
  routeGeometry.clear();
  m_adjacentEdges.clear();
  m_pathSegments.clear();

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
  size_t const pathSize = path.size();
  m_adjacentEdges.insert(make_pair(0, AdjacentEdges({{} /* outgoingEdges */, 0 /* ingoingEdges.size() */})));
  for (size_t i = 1; i < pathSize; ++i)
  {
    Junction const & formerJunction = path[i - 1];
    Junction const & currentJunction = path[i];
    IRoadGraph::TEdgeVector outgoingEdges, ingoingEdges;
    graph.GetOutgoingEdges(currentJunction, outgoingEdges);
    graph.GetIngoingEdges(currentJunction, ingoingEdges);

    AdjacentEdges adjacentEdges = {{}, ingoingEdges.size()};
    adjacentEdges.outgoingTurns.reserve(outgoingEdges.size());
    for (auto const & outgoingEdge : outgoingEdges)
    {
      auto const & outgoingFeatureId = outgoingEdge.GetFeatureId();
      // Checking for if |outgoingEdge| is a fake edge.
      if (!outgoingFeatureId.m_mwmId.IsAlive())
        continue;
      double const angle = turns::PiMinusTwoVectorsAngle(formerJunction.GetPoint(),
                                                         currentJunction.GetPoint(),
                                                         outgoingEdge.GetEndJunction().GetPoint());
      adjacentEdges.outgoingTurns.emplace_back(angle, outgoingFeatureId.m_index,
                                               GetHighwayClass(outgoingFeatureId, m_index));
    }

    // Filling |m_pathSegments| based on |path|.
    LoadedPathSegment pathSegment;
    pathSegment.m_path = { formerJunction.GetPoint(), currentJunction.GetPoint() };
    pathSegment.m_nodeId = i;

    // @TODO(bykoianko) It's necessary to fill more fields of m_pathSegments.
    m_adjacentEdges.insert(make_pair(i, move(adjacentEdges)));
    m_pathSegments.push_back(move(pathSegment));
  }

  AStarRoutingResultGraph resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;
  Route::TTimes turnAnnotationTimes;
  Route::TStreets streetNames;
  MakeTurnAnnotation(resultGraph, delegate, routeGeometry, turnsDir, turnAnnotationTimes, streetNames);
}
}  // namespace routing
