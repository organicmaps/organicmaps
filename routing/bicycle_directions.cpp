#include "routing/bicycle_directions.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/road_point.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include <cstdlib>
#include <numeric>
#include <utility>

namespace
{
using namespace routing;
using namespace routing::turns;
using namespace traffic;

class RoutingResult : public IRoutingResult
{
public:
  RoutingResult(IRoadGraph::TEdgeVector const & routeEdges,
                BicycleDirectionsEngine::AdjacentEdgesMap const & adjacentEdges,
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
  TUnpackedPathSegments const & GetSegments() const override { return m_pathSegments; }

  void GetPossibleTurns(UniNodeId const & node, m2::PointD const & /* ingoingPoint */,
                        m2::PointD const & /* junctionPoint */, size_t & ingoingCount,
                        TurnCandidates & outgoingTurns) const override
  {
    ingoingCount = 0;
    outgoingTurns.candidates.clear();

    auto const adjacentEdges = m_adjacentEdges.find(node);
    if (adjacentEdges == m_adjacentEdges.cend())
    {
      ASSERT(false, ());
      return;
    }

    ingoingCount = adjacentEdges->second.m_ingoingTurnsCount;
    outgoingTurns = adjacentEdges->second.m_outgoingTurns;
  }

  double GetPathLength() const override { return m_routeLength; }

  Junction GetStartPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.front().GetStartJunction();
  }

  Junction GetEndPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.back().GetEndJunction();
  }

private:
  IRoadGraph::TEdgeVector const & m_routeEdges;
  BicycleDirectionsEngine::AdjacentEdgesMap const & m_adjacentEdges;
  TUnpackedPathSegments const & m_pathSegments;
  double m_routeLength;
};

/// \brief This method should be called for an internal junction of the route with corresponding
/// |ingoingEdges|, |outgoingEdges|, |ingoingRouteEdge| and |outgoingRouteEdge|.
/// \returns false if the junction is an internal point of feature segment and can be considered as
/// a part of LoadedPathSegment and returns true if the junction should be considered as a beginning
/// of a new LoadedPathSegment.
bool IsJoint(IRoadGraph::TEdgeVector const & ingoingEdges,
             IRoadGraph::TEdgeVector const & outgoingEdges, Edge const & ingoingRouteEdge,
             Edge const & outgoingRouteEdge)
{
  // When feature id is changed at a junction this junction should be considered as a joint.
  //
  // If a feature id is not changed at a junction but the junction has some ingoing or outgoing edges with
  // different feature ids, the junction should be considered as a joint.
  //
  // If a feature id is not changed at a junction and all ingoing and outgoing edges of the junction has
  // the same feature id, the junction still may be considered as a joint.
  // It happens in case of self intersected features. For example:
  //            *--Seg3--*
  //            |        |
  //          Seg4      Seg2
  //            |        |
  //   *--Seg0--*--Seg1--*
  // The common point of segments 0, 1 and 4 should be considered as a joint.
  if (ingoingRouteEdge.GetFeatureId() != outgoingRouteEdge.GetFeatureId())
    return true;

  FeatureID const & featureId = ingoingRouteEdge.GetFeatureId();
  uint32_t const segOut = outgoingRouteEdge.GetSegId();
  for (Edge const & e : ingoingEdges)
  {
    if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segOut - e.GetSegId())) != 1)
      return true;
  }

  uint32_t const segIn = ingoingRouteEdge.GetSegId();
  for (Edge const & e : outgoingEdges)
  {
    // It's necessary to compare segments for cases when |featureId| is a loop.
    if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segIn - e.GetSegId())) != 1)
      return true;
  }
  return false;
}
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine(Index const & index,
                                                 shared_ptr<NumMwmIds> numMwmIds)
  : m_index(index), m_numMwmIds(numMwmIds)
{
}

void BicycleDirectionsEngine::Generate(RoadGraphBase const & graph, vector<Junction> const & path,
                                       my::Cancellable const & cancellable, Route::TTimes & times,
                                       Route::TTurns & turns, vector<Junction> & routeGeometry,
                                       vector<Segment> & trafficSegs)
{
  times.clear();
  turns.clear();
  routeGeometry.clear();
  m_adjacentEdges.clear();
  m_pathSegments.clear();
  trafficSegs.clear();

  size_t const pathSize = path.size();
  if (pathSize == 0)
    return;

  auto emptyPathWorkaround = [&]()
  {
    turns.emplace_back(pathSize - 1, turns::TurnDirection::ReachedYourDestination);
    // There's one ingoing edge to the finish.
    this->m_adjacentEdges[UniNodeId(UniNodeId::Type::Mwm)] = AdjacentEdges(1);
  };

  if (pathSize == 1)
  {
    emptyPathWorkaround();
    return;
  }

  // @TODO(bykoianko) This method should be removed. Generally speaking it's wrong to calculate ETA
  // based on max speed as it's done in CalculateTimes(). We'll get a better result and better code
  // if to used ETA calculated in MakeTurnAnnotation(). To do that it's necessary to fill
  // LoadedPathSegment::m_weight in FillPathSegmentsAndAdjacentEdgesMap().
  CalculateTimes(graph, path, times);

  IRoadGraph::TEdgeVector routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path."));
    emptyPathWorkaround();
    return;
  }
  if (routeEdges.empty())
  {
    emptyPathWorkaround();
    return;
  }

  FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

  if (cancellable.IsCancelled())
    return;

  RoutingResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;
  Route::TTimes dummyTimes;
  Route::TStreets streetNames;

  MakeTurnAnnotation(resultGraph, delegate, routeGeometry, turns, dummyTimes, streetNames,
                     trafficSegs);
  CHECK_EQUAL(routeGeometry.size(), pathSize, ());
  if (!trafficSegs.empty())
    CHECK_EQUAL(trafficSegs.size(), routeEdges.size(), ());
}

Index::FeaturesLoaderGuard & BicycleDirectionsEngine::GetLoader(MwmSet::MwmId const & id)
{
  if (!m_loader || id != m_loader->GetId())
    m_loader = make_unique<Index::FeaturesLoaderGuard>(m_index, id);
  return *m_loader;
}

void BicycleDirectionsEngine::LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment)
{
  if (!featureId.IsValid())
    return;

  FeatureType ft;
  if(!GetLoader(featureId.m_mwmId).GetFeatureByIndex(featureId.m_index, ft))
    return;

  auto const highwayClass = ftypes::GetHighwayClass(ft);
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

  pathSegment.m_highwayClass = highwayClass;
  pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(ft);
  ft.GetName(FeatureType::DEFAULT_LANG, pathSegment.m_name);
  pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(ft);
}

void BicycleDirectionsEngine::GetUniNodeIdAndAdjacentEdges(
    IRoadGraph::TEdgeVector const & outgoingEdges, FeatureID const & inFeatureId,
    uint32_t startSegId, uint32_t endSegId, bool inIsForward, UniNodeId & uniNodeId,
    BicycleDirectionsEngine::AdjacentEdges & adjacentEdges)
{
  // Outgoing edge angle is not used for bicycle routing.
  adjacentEdges.m_outgoingTurns.isCandidatesAngleValid = false;
  adjacentEdges.m_outgoingTurns.candidates.reserve(outgoingEdges.size());
  uniNodeId = UniNodeId(inFeatureId, startSegId, endSegId, inIsForward);

  for (auto const & edge : outgoingEdges)
  {
    if (edge.IsFake())
      continue;

    auto const & outFeatureId = edge.GetFeatureId();
    FeatureType ft;
    if (!GetLoader(outFeatureId.m_mwmId).GetFeatureByIndex(outFeatureId.m_index, ft))
      continue;

    auto const highwayClass = ftypes::GetHighwayClass(ft);
    ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
    ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());
    adjacentEdges.m_outgoingTurns.candidates.emplace_back(0.0 /* angle */, uniNodeId, highwayClass);
  }
}

bool BicycleDirectionsEngine::GetSegment(FeatureID const & featureId, uint32_t segId,
                                         bool forward, Segment & segment) const
{
  if (!m_numMwmIds)
    return false;

  if (!featureId.m_mwmId.IsAlive())
    return false;

  NumMwmId const numMwmId =
    m_numMwmIds->GetId(featureId.m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
  segment = Segment(numMwmId, featureId.m_index, segId, forward);
  return true;
}

void BicycleDirectionsEngine::FillPathSegmentsAndAdjacentEdgesMap(
    RoadGraphBase const & graph, vector<Junction> const & path,
    IRoadGraph::TEdgeVector const & routeEdges, my::Cancellable const & cancellable)
{
  size_t const pathSize = path.size();
  CHECK_GREATER(pathSize, 1, ());
  CHECK_EQUAL(routeEdges.size(), pathSize - 1, ());
  // Filling |m_adjacentEdges|.
  m_adjacentEdges.insert(make_pair(UniNodeId(UniNodeId::Type::Mwm), AdjacentEdges(0)));
  auto constexpr kInvalidSegId = std::numeric_limits<uint32_t>::max();
  // |startSegId| is a value to keep start segment id of a new instance of LoadedPathSegment.
  uint32_t startSegId = kInvalidSegId;
  vector<Junction> prevJunctions;
  vector<Segment> prevSegments;
  for (size_t i = 1; i < pathSize; ++i)
  {
    if (cancellable.IsCancelled())
      return;

    Junction const & prevJunction = path[i - 1];
    Junction const & currJunction = path[i];
    IRoadGraph::TEdgeVector ingoingEdges;
    IRoadGraph::TEdgeVector outgoingEdges;
    graph.GetOutgoingEdges(currJunction, outgoingEdges);
    graph.GetIngoingEdges(currJunction, ingoingEdges);

    Edge const & inEdge = routeEdges[i - 1];
    // Note. |inFeatureId| may be invalid in case of adding fake features.
    // It happens for example near starts and a finishes.
    FeatureID const & inFeatureId = inEdge.GetFeatureId();
    uint32_t const inSegId = inEdge.GetSegId();
    bool const inIsForward = inEdge.IsForward();

    if (startSegId == kInvalidSegId)
      startSegId = inSegId;

    if (inFeatureId.IsValid() && i + 1 != pathSize &&
        !IsJoint(ingoingEdges, outgoingEdges, inEdge, routeEdges[i]))
    {
      prevJunctions.push_back(prevJunction);
      Segment inSegment;
      if (GetSegment(inFeatureId, inSegId, inIsForward, inSegment))
        prevSegments.push_back(inSegment);
      continue;
    }
    CHECK_EQUAL(prevJunctions.size(), abs(static_cast<int32_t>(inSegId - startSegId)), ());

    prevJunctions.push_back(prevJunction);
    prevJunctions.push_back(currJunction);
    Segment segment;
    if (GetSegment(inFeatureId, inSegId, inIsForward, segment))
      prevSegments.push_back(segment);

    AdjacentEdges adjacentEdges(ingoingEdges.size());
    UniNodeId uniNodeId(UniNodeId::Type::Mwm);
    GetUniNodeIdAndAdjacentEdges(outgoingEdges, inFeatureId, startSegId, inSegId + 1, inIsForward,
                                 uniNodeId, adjacentEdges);

    size_t const prevJunctionSize = prevJunctions.size();
    LoadedPathSegment pathSegment(UniNodeId::Type::Mwm);
    LoadPathAttributes(uniNodeId.GetFeature(), pathSegment);
    pathSegment.m_nodeId = uniNodeId;
    pathSegment.m_path = std::move(prevJunctions);
    // @TODO(bykoianko) |pathSegment.m_weight| should be filled here.

    // For LoadedPathSegment instances which contain fake feature id(s), |LoadedPathSegment::m_trafficSegs|
    // should be empty because there's no traffic information for fake features.
    // Traffic information is not supported for bicycle routes as well.
    // Method BicycleDirectionsEngine::GetSegment() returns false for fake features and for bicycle routes.
    // It leads to preventing pushing item to |prevSegments|. So if there's no enough items in |prevSegments|
    // |pathSegment.m_trafficSegs| should be empty.
    // Note. For the time being BicycleDirectionsEngine is used for turn generation for bicycle and car routes.
    if (prevSegments.size() + 1 == prevJunctionSize)
      pathSegment.m_trafficSegs = std::move(prevSegments);

    auto const it = m_adjacentEdges.insert(make_pair(uniNodeId, std::move(adjacentEdges)));
    ASSERT(it.second, ());
    UNUSED_VALUE(it);
    m_pathSegments.push_back(std::move(pathSegment));

    prevJunctions.clear();
    prevSegments.clear();
    startSegId = kInvalidSegId;
  }
}
}  // namespace routing
