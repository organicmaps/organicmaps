#include "routing/bicycle_directions.hpp"

#include "routing/road_point.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <utility>

namespace
{
using namespace routing;
using namespace routing::turns;
using namespace std;
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

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint,
                        size_t & ingoingCount, TurnCandidates & outgoingTurns) const override
  {
    CHECK(!segmentRange.IsEmpty(), ("SegmentRange presents a fake feature.",
                                    "junctionPoint:", MercatorBounds::ToLatLon(junctionPoint)));

    ingoingCount = 0;
    outgoingTurns.candidates.clear();

    auto const adjacentEdges = m_adjacentEdges.find(segmentRange);
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
             IRoadGraph::TEdgeVector const & outgoingEdges,
             Edge const & ingoingRouteEdge,
             Edge const & outgoingRouteEdge,
             bool isCurrJunctionFinish,
             bool isInEdgeReal)
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
  if (!isInEdgeReal)
    return true;

  if (isCurrJunctionFinish)
    return true;

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
// BicycleDirectionsEngine::AdjacentEdges ---------------------------------------------------------
bool BicycleDirectionsEngine::AdjacentEdges::IsAlmostEqual(AdjacentEdges const & rhs) const
{
  return m_outgoingTurns.IsAlmostEqual(rhs.m_outgoingTurns) &&
         m_ingoingTurnsCount == rhs.m_ingoingTurnsCount;
}

// BicycleDirectionsEngine ------------------------------------------------------------------------
BicycleDirectionsEngine::BicycleDirectionsEngine(DataSource const & dataSource,
                                                 shared_ptr<NumMwmIds> numMwmIds)
  : m_dataSource(dataSource), m_numMwmIds(numMwmIds)
{
  CHECK(m_numMwmIds, ());
}

bool BicycleDirectionsEngine::Generate(IndexRoadGraph const & graph, vector<Junction> const & path,
                                       base::Cancellable const & cancellable, Route::TTurns & turns,
                                       Route::TStreets & streetNames,
                                       vector<Junction> & routeGeometry, vector<Segment> & segments)
{
  CHECK(m_numMwmIds, ());

  m_adjacentEdges.clear();
  m_pathSegments.clear();
  turns.clear();
  streetNames.clear();
  routeGeometry.clear();
  segments.clear();
  
  size_t const pathSize = path.size();
  // Note. According to Route::IsValid() method route of zero or one point is invalid.
  if (pathSize <= 1)
    return false;

  IRoadGraph::TEdgeVector routeEdges;
  graph.GetRouteEdges(routeEdges);

  if (routeEdges.empty())
    return false;

  if (cancellable.IsCancelled())
    return false;

  FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

  if (cancellable.IsCancelled())
    return false;

  ::RoutingResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;

  MakeTurnAnnotation(resultGraph, *m_numMwmIds, delegate, routeGeometry, turns, streetNames, segments);
  CHECK_EQUAL(routeGeometry.size(), pathSize, ());
  // In case of bicycle routing |m_pathSegments| may have an empty
  // |LoadedPathSegment::m_segments| fields. In that case |segments| is empty
  // so size of |segments| is not equal to size of |routeEdges|.
  if (!segments.empty())
    CHECK_EQUAL(segments.size(), routeEdges.size(), ());
  return true;
}

FeaturesLoaderGuard & BicycleDirectionsEngine::GetLoader(MwmSet::MwmId const & id)
{
  if (!m_loader || id != m_loader->GetId())
    m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id);
  return *m_loader;
}

void BicycleDirectionsEngine::LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment)
{
  if (!featureId.IsValid())
    return;

  FeatureType ft;
  if(!GetLoader(featureId.m_mwmId).GetFeatureByIndex(featureId.m_index, ft))
    return;

  auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(ft));
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

  pathSegment.m_highwayClass = highwayClass;
  pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(ft);
  ft.GetName(StringUtf8Multilang::kDefaultCode, pathSegment.m_name);
  pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(ft);
}

void BicycleDirectionsEngine::GetSegmentRangeAndAdjacentEdges(
    IRoadGraph::TEdgeVector const & outgoingEdges, Edge const & inEdge, uint32_t startSegId,
    uint32_t endSegId, SegmentRange & segmentRange, TurnCandidates & outgoingTurns)
{
  outgoingTurns.isCandidatesAngleValid = true;
  outgoingTurns.candidates.reserve(outgoingEdges.size());
  segmentRange = SegmentRange(inEdge.GetFeatureId(), startSegId, endSegId, inEdge.IsForward(),
                              inEdge.GetStartPoint(), inEdge.GetEndPoint());
  CHECK(segmentRange.IsCorrect(), ());
  m2::PointD const & ingoingPoint = inEdge.GetStartJunction().GetPoint();
  m2::PointD const & junctionPoint = inEdge.GetEndJunction().GetPoint();

  for (auto const & edge : outgoingEdges)
  {
    if (edge.IsFake())
      continue;

    auto const & outFeatureId = edge.GetFeatureId();
    FeatureType ft;
    if (!GetLoader(outFeatureId.m_mwmId).GetFeatureByIndex(outFeatureId.m_index, ft))
      continue;

    auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(ft));
    ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
    ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

    bool const isLink = ftypes::IsLinkChecker::Instance()(ft);

    double angle = 0;

    if (inEdge.GetFeatureId().m_mwmId == edge.GetFeatureId().m_mwmId)
    {
      ASSERT_LESS(MercatorBounds::DistanceOnEarth(junctionPoint, edge.GetStartJunction().GetPoint()),
                  turns::kFeaturesNearTurnMeters, ());
      m2::PointD const & outgoingPoint = edge.GetEndJunction().GetPoint();
      angle = base::RadToDeg(turns::PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
    }
    else
    {
      // Note. In case of crossing mwm border
      // (inEdge.GetFeatureId().m_mwmId != edge.GetFeatureId().m_mwmId)
      // twins of inEdge.GetFeatureId() are considered as outgoing features.
      // In this case that turn candidate angle is invalid and
      // should not be used for turn generation.
      outgoingTurns.isCandidatesAngleValid = false;
    }
    outgoingTurns.candidates.emplace_back(angle, ConvertEdgeToSegment(*m_numMwmIds, edge),
                                          highwayClass, isLink);
  }

  if (outgoingTurns.isCandidatesAngleValid)
    sort(outgoingTurns.candidates.begin(), outgoingTurns.candidates.end(), base::LessBy(&TurnCandidate::m_angle));
}

void BicycleDirectionsEngine::GetEdges(RoadGraphBase const & graph, Junction const & currJunction,
                                       bool isCurrJunctionFinish, IRoadGraph::TEdgeVector & outgoing,
                                       IRoadGraph::TEdgeVector & ingoing)
{
  // Note. If |currJunction| is a finish the outgoing edges
  // from finish are not important for turn generation.
  if (!isCurrJunctionFinish)
    graph.GetOutgoingEdges(currJunction, outgoing);
  graph.GetIngoingEdges(currJunction, ingoing);
}

void BicycleDirectionsEngine::FillPathSegmentsAndAdjacentEdgesMap(
    IndexRoadGraph const & graph, vector<Junction> const & path,
    IRoadGraph::TEdgeVector const & routeEdges, base::Cancellable const & cancellable)
{
  size_t const pathSize = path.size();
  CHECK_GREATER(pathSize, 1, ());
  CHECK_EQUAL(routeEdges.size() + 1, pathSize, ());
  // Filling |m_adjacentEdges|.
  auto constexpr kInvalidSegId = numeric_limits<uint32_t>::max();
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

    IRoadGraph::TEdgeVector outgoingEdges;
    IRoadGraph::TEdgeVector ingoingEdges;
    bool const isCurrJunctionFinish = (i + 1 == pathSize);
    GetEdges(graph, currJunction, isCurrJunctionFinish, outgoingEdges, ingoingEdges);

    Edge const & inEdge = routeEdges[i - 1];
    // Note. |inFeatureId| may be invalid in case of adding fake features.
    // It happens for example near starts and a finishes.
    FeatureID const & inFeatureId = inEdge.GetFeatureId();
    uint32_t const inSegId = inEdge.GetSegId();

    if (startSegId == kInvalidSegId)
      startSegId = inSegId;

    prevJunctions.push_back(prevJunction);
    prevSegments.push_back(ConvertEdgeToSegment(*m_numMwmIds, inEdge));

    if (!IsJoint(ingoingEdges, outgoingEdges, inEdge, routeEdges[i], isCurrJunctionFinish,
                 inFeatureId.IsValid()))
    {
      continue;
    }

    CHECK_EQUAL(prevJunctions.size(), static_cast<size_t>(
                    abs(static_cast<int32_t>(inSegId - startSegId)) + 1), ());

    prevJunctions.push_back(currJunction);

    AdjacentEdges adjacentEdges(ingoingEdges.size());
    SegmentRange segmentRange;
    GetSegmentRangeAndAdjacentEdges(outgoingEdges, inEdge, startSegId, inSegId, segmentRange,
                                    adjacentEdges.m_outgoingTurns);

    size_t const prevJunctionSize = prevJunctions.size();
    LoadedPathSegment pathSegment;
    LoadPathAttributes(segmentRange.GetFeature(), pathSegment);
    pathSegment.m_segmentRange = segmentRange;
    pathSegment.m_path = move(prevJunctions);
    // @TODO(bykoianko) |pathSegment.m_weight| should be filled here.

    // |prevSegments| contains segments which corresponds to road edges between joints. In case of a fake edge
    // a fake segment is created.
    CHECK_EQUAL(prevSegments.size() + 1, prevJunctionSize, ());
    pathSegment.m_segments = move(prevSegments);

    if (!segmentRange.IsEmpty())
    {
      auto const it = m_adjacentEdges.find(segmentRange);
      // A route may be built through intermediate points. So it may contain the same |segmentRange|
      // several times. But in that case |adjacentEdges| corresponding to |segmentRange|
      // should be the same.
      ASSERT(it == m_adjacentEdges.cend() || it->second.IsAlmostEqual(adjacentEdges),
             ("segmentRange:", segmentRange, "corresponds to adjacent edges which aren't equal."));

      m_adjacentEdges.insert(it, make_pair(segmentRange, move(adjacentEdges)));
    }

    m_pathSegments.push_back(move(pathSegment));

    prevJunctions.clear();
    prevSegments.clear();
    startSegId = kInvalidSegId;
  }
}
}  // namespace routing
