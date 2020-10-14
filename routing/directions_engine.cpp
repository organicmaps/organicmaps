#include "routing/directions_engine.hpp"

#include "routing/routing_helpers.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <cstdlib>
#include <utility>

namespace
{
bool IsFakeFeature(uint32_t featureId)
{
  return routing::FakeFeatureIds::IsGuidesFeature(featureId) ||
         routing::FakeFeatureIds::IsTransitFeature(featureId);
}
}  // namespace

namespace routing
{
using namespace routing::turns;
using namespace std;
using namespace traffic;

void DirectionsEngine::Clear()
{
  m_adjacentEdges.clear();
  m_pathSegments.clear();
  m_loader.reset();
}

FeaturesLoaderGuard & DirectionsEngine::GetLoader(MwmSet::MwmId const & id)
{
  if (!m_loader || id != m_loader->GetId())
    m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id);
  return *m_loader;
}

void DirectionsEngine::LoadPathAttributes(FeatureID const & featureId,
                                          LoadedPathSegment & pathSegment)
{
  if (!featureId.IsValid())
    return;

  if (IsFakeFeature(featureId.m_index))
    return;

  auto ft = GetLoader(featureId.m_mwmId).GetFeatureByIndex(featureId.m_index);
  if (!ft)
    return;

  auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

  pathSegment.m_highwayClass = highwayClass;
  pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(*ft);
  ft->GetName(StringUtf8Multilang::kDefaultCode, pathSegment.m_name);
  pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(*ft);
}

void DirectionsEngine::GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeVector const & outgoingEdges,
                                                       Edge const & inEdge, uint32_t startSegId,
                                                       uint32_t endSegId,
                                                       SegmentRange & segmentRange,
                                                       TurnCandidates & outgoingTurns)
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
    if (IsFakeFeature(outFeatureId.m_index))
      continue;

    auto ft = GetLoader(outFeatureId.m_mwmId).GetFeatureByIndex(outFeatureId.m_index);
    if (!ft)
      continue;

    auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
    ASSERT_NOT_EQUAL(
        highwayClass, ftypes::HighwayClass::Error,
        (mercator::ToLatLon(edge.GetStartPoint()), mercator::ToLatLon(edge.GetEndPoint())));
    ASSERT_NOT_EQUAL(
        highwayClass, ftypes::HighwayClass::Undefined,
        (mercator::ToLatLon(edge.GetStartPoint()), mercator::ToLatLon(edge.GetEndPoint())));

    bool const isLink = ftypes::IsLinkChecker::Instance()(*ft);

    double angle = 0;

    if (inEdge.GetFeatureId().m_mwmId == edge.GetFeatureId().m_mwmId)
    {
      ASSERT_LESS(mercator::DistanceOnEarth(junctionPoint, edge.GetStartJunction().GetPoint()),
                  turns::kFeaturesNearTurnMeters, ());
      m2::PointD const & outgoingPoint = edge.GetEndJunction().GetPoint();
      angle =
          base::RadToDeg(turns::PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
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
  {
    sort(outgoingTurns.candidates.begin(), outgoingTurns.candidates.end(),
         base::LessBy(&TurnCandidate::m_angle));
  }
}

void DirectionsEngine::GetEdges(IndexRoadGraph const & graph,
                                geometry::PointWithAltitude const & currJunction,
                                bool isCurrJunctionFinish, IRoadGraph::EdgeVector & outgoing,
                                IRoadGraph::EdgeVector & ingoing)
{
  // Note. If |currJunction| is a finish the outgoing edges
  // from finish are not important for turn generation.
  if (!isCurrJunctionFinish)
    graph.GetOutgoingEdges(currJunction, outgoing);

  graph.GetIngoingEdges(currJunction, ingoing);
}

void DirectionsEngine::FillPathSegmentsAndAdjacentEdgesMap(
    IndexRoadGraph const & graph, vector<geometry::PointWithAltitude> const & path,
    IRoadGraph::EdgeVector const & routeEdges, base::Cancellable const & cancellable)
{
  size_t const pathSize = path.size();
  CHECK_GREATER(pathSize, 1, ());
  CHECK_EQUAL(routeEdges.size() + 1, pathSize, ());
  // Filling |m_adjacentEdges|.
  auto constexpr kInvalidSegId = numeric_limits<uint32_t>::max();
  // |startSegId| is a value to keep start segment id of a new instance of LoadedPathSegment.
  uint32_t startSegId = kInvalidSegId;
  vector<geometry::PointWithAltitude> prevJunctions;
  vector<Segment> prevSegments;
  for (size_t i = 1; i < pathSize; ++i)
  {
    if (cancellable.IsCancelled())
      return;

    geometry::PointWithAltitude const & prevJunction = path[i - 1];
    geometry::PointWithAltitude const & currJunction = path[i];

    IRoadGraph::EdgeVector outgoingEdges;
    IRoadGraph::EdgeVector ingoingEdges;
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

    CHECK_EQUAL(prevJunctions.size(),
                static_cast<size_t>(abs(static_cast<int32_t>(inSegId - startSegId)) + 1), ());

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

    // |prevSegments| contains segments which corresponds to road edges between joints. In case of a
    // fake edge a fake segment is created.
    CHECK_EQUAL(prevSegments.size() + 1, prevJunctionSize, ());
    pathSegment.m_segments = move(prevSegments);

    if (!segmentRange.IsEmpty())
    {
      auto const it = m_adjacentEdges.find(segmentRange);
      m_adjacentEdges.insert(it, make_pair(segmentRange, move(adjacentEdges)));
    }

    m_pathSegments.push_back(move(pathSegment));

    prevJunctions.clear();
    prevSegments.clear();
    startSegId = kInvalidSegId;
  }
}
}  // namespace routing
