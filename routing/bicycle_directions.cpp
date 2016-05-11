#include "routing/bicycle_directions.hpp"
#include "routing/car_model.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

namespace
{
using namespace routing;
using namespace routing::turns;

class AStarRoutingResult : public IRoutingResult
{
public:
  AStarRoutingResult(IRoadGraph::TEdgeVector const & routeEdges,
                     AdjacentEdgesMap const & adjacentEdges,
                     TUnpackedPathSegments const & pathSegments)
    : m_routeEdges(routeEdges)
    , m_adjacentEdges(adjacentEdges)
    , m_pathSegments(pathSegments)
    , m_routeLength(0)
  {
    for (auto const & edge : routeEdges)
    {
      m_routeLength += MercatorBounds::DistanceOnEarth(edge.GetStartJunction().GetPoint(),
                                                       edge.GetEndJunction().GetPoint());
    }
  }

  // turns::IRoutingResult overrides:
  virtual TUnpackedPathSegments const & GetSegments() const override { return m_pathSegments; }

  virtual void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint, size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const override
  {
    ingoingCount = 0;
    outgoingTurns.clear();

    if (node >= m_routeEdges.size())
    {
      ASSERT(false, (m_routeEdges.size()));
      return;
    }

    auto adjacentEdges = m_adjacentEdges.find(node);
    if (adjacentEdges == m_adjacentEdges.cend())
    {
      ASSERT(false, ());
      return;
    }

    ingoingCount = adjacentEdges->second.m_ingoingTurnsCount;
    outgoingTurns = adjacentEdges->second.m_outgoingTurns;
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
  highWayClass = ftypes::GetHighwayClass(ft);
  ASSERT_NOT_EQUAL(highWayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highWayClass, ftypes::HighwayClass::Undefined, ());
  return highWayClass;
}
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine(Index const & index) : m_index(index) {}

void BicycleDirectionsEngine::Generate(IRoadGraph const & graph, vector<Junction> const & path,
                                       Route::TTimes & times, Route::TTurns & turns,
                                       vector<m2::PointD> & routeGeometry,
                                       my::Cancellable const & cancellable)
{
  size_t const pathSize = path.size();
  CHECK_NOT_EQUAL(pathSize, 0, ());

  times.clear();
  turns.clear();
  routeGeometry.clear();
  m_adjacentEdges.clear();
  m_pathSegments.clear();

  auto emptyPathWorkaround = [&]()
  {
    turns.emplace_back(pathSize - 1, turns::TurnDirection::ReachedYourDestination);
    this->m_adjacentEdges[0] = AdjacentEdges(1);  // There's one ingoing edge to the finish.
  };

  if (pathSize <= 1)
  {
    ASSERT(false, (pathSize));
    emptyPathWorkaround();
    return;
  }

  CalculateTimes(graph, path, times);

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
  m_adjacentEdges.insert(make_pair(0, AdjacentEdges(0)));
  for (size_t i = 1; i < pathSize; ++i)
  {
    Junction const & prevJunction = path[i - 1];
    Junction const & currJunction = path[i];
    IRoadGraph::TEdgeVector outgoingEdges, ingoingEdges;
    graph.GetOutgoingEdges(currJunction, outgoingEdges);
    graph.GetIngoingEdges(currJunction, ingoingEdges);

    AdjacentEdges adjacentEdges = AdjacentEdges(ingoingEdges.size());
    adjacentEdges.m_outgoingTurns.reserve(outgoingEdges.size());
    for (auto const & edge : outgoingEdges)
    {
      auto const & featureId = edge.GetFeatureId();
      // Checking for if |edge| is a fake edge.
      if (!featureId.m_mwmId.IsAlive())
        continue;
      double const angle = turns::PiMinusTwoVectorsAngle(
          currJunction.GetPoint(), prevJunction.GetPoint(), edge.GetEndJunction().GetPoint());
      adjacentEdges.m_outgoingTurns.emplace_back(angle, featureId.m_index,
                                                 GetHighwayClass(featureId, m_index));
    }

    // Filling |m_pathSegments| based on |path|.
    LoadedPathSegment pathSegment;
    pathSegment.m_path = {prevJunction.GetPoint(), currJunction.GetPoint()};
    pathSegment.m_nodeId = i;

    // @TODO(bykoianko) It's necessary to fill more fields of m_pathSegments.
    m_adjacentEdges.insert(make_pair(i, move(adjacentEdges)));
    m_pathSegments.push_back(move(pathSegment));
  }

  AStarRoutingResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;
  Route::TTimes turnAnnotationTimes;
  Route::TStreets streetNames;
  MakeTurnAnnotation(resultGraph, delegate, routeGeometry, turns, turnAnnotationTimes, streetNames);
}
}  // namespace routing
