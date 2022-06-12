#include "routing/directions_engine.hpp"

#include "routing/data_source.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_callbacks.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <cstdlib>
#include <utility>

namespace routing
{
namespace
{
bool IsFakeFeature(uint32_t featureId)
{
  return routing::FakeFeatureIds::IsGuidesFeature(featureId) ||
         routing::FakeFeatureIds::IsTransitFeature(featureId);
}
}  // namespace

using namespace routing::turns;
using namespace std;
using namespace traffic;

void DirectionsEngine::Clear()
{
  m_adjacentEdges.clear();
  m_pathSegments.clear();
}

std::unique_ptr<FeatureType> DirectionsEngine::GetFeature(FeatureID const & featureId)
{
  if (IsFakeFeature(featureId.m_index))
    return nullptr;
  return m_dataSource.GetFeature(featureId);
}

void DirectionsEngine::LoadPathAttributes(FeatureID const & featureId,
                                          LoadedPathSegment & pathSegment)
{
  if (!featureId.IsValid())
    return;

  auto ft = GetFeature(featureId);
  if (!ft)
    return;

  auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

  pathSegment.m_highwayClass = highwayClass;
  pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(*ft);
  pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(*ft);
  pathSegment.m_isOneWay = ftypes::IsOneWayChecker::Instance()(*ft);

  pathSegment.m_roadNameInfo.m_isLink = pathSegment.m_isLink;
  pathSegment.m_roadNameInfo.m_junction_ref = ft->GetMetadata(feature::Metadata::FMD_JUNCTION_REF);
  pathSegment.m_roadNameInfo.m_destination_ref = ft->GetMetadata(feature::Metadata::FMD_DESTINATION_REF);
  pathSegment.m_roadNameInfo.m_destination = ft->GetMetadata(feature::Metadata::FMD_DESTINATION);
  pathSegment.m_roadNameInfo.m_ref = ft->GetRoadNumber();
  pathSegment.m_roadNameInfo.m_name = ft->GetName(StringUtf8Multilang::kDefaultCode);
}

void DirectionsEngine::GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeListT const & outgoingEdges,
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

    auto ft = GetFeature(edge.GetFeatureId());
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
                                bool isCurrJunctionFinish, IRoadGraph::EdgeListT & outgoing,
                                IRoadGraph::EdgeListT & ingoing)
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

    IRoadGraph::EdgeListT outgoingEdges;
    IRoadGraph::EdgeListT ingoingEdges;
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

    if (!isCurrJunctionFinish && inFeatureId.IsValid() && !IsJoint(ingoingEdges, outgoingEdges, inEdge, routeEdges[i]))
      continue;

    CHECK_EQUAL(prevJunctions.size(), static_cast<size_t>(abs(int(inSegId) - int(startSegId)) + 1), ());

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

bool DirectionsEngine::Generate(IndexRoadGraph const & graph,
                                vector<geometry::PointWithAltitude> const & path,
                                base::Cancellable const & cancellable, Route::TTurns & turns,
                                Route::TStreets & streetNames,
                                vector<geometry::PointWithAltitude> & routeGeometry,
                                vector<Segment> & segments)
{
  CHECK(m_numMwmIds, ());

  m_adjacentEdges.clear();
  m_pathSegments.clear();
  turns.clear();
  streetNames.clear();
  segments.clear();

  IndexRoadGraph::EdgeVector routeEdges;

  CHECK_NOT_EQUAL(m_vehicleType, VehicleType::Count, (m_vehicleType));

  if (m_vehicleType == VehicleType::Transit)
  {
    routeGeometry = path;
    graph.GetRouteSegments(segments);
    graph.GetRouteEdges(routeEdges);
    turns.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
    return true;
  }

  routeGeometry.clear();

  if (path.size() <= 1)
    return false;

  graph.GetRouteEdges(routeEdges);

  if (routeEdges.empty())
    return false;

  if (cancellable.IsCancelled())
    return false;

  FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

  if (cancellable.IsCancelled())
    return false;

  auto const res = MakeTurnAnnotation(routeEdges, cancellable,
                                      routeGeometry, turns, streetNames, segments);

  if (res != RouterResultCode::NoError)
    return false;

  // In case of bicycle routing |m_pathSegments| may have an empty
  // |LoadedPathSegment::m_segments| fields. In that case |segments| is empty
  // so size of |segments| is not equal to size of |routeEdges|.
  if (!segments.empty())
    CHECK_EQUAL(segments.size(), routeEdges.size(), ());


  CHECK_EQUAL(
      routeGeometry.size(), path.size(),
      ("routeGeometry and path have different sizes. routeGeometry size:", routeGeometry.size(),
       "path size:", path.size(), "segments size:", segments.size(), "routeEdges size:", routeEdges.size()));

  return true;
}

/*!
 * \brief Compute turn and time estimation structs for the abstract route result.
 * \param result abstract routing result to annotate.
 * \param delegate Routing callbacks delegate.
 * \param points Storage for unpacked points of the path.
 * \param turnsDir output turns annotation storage.
 * \param streets output street names along the path.
 * \param segments route segments.
 * \return routing operation result code.
 */

// Normally loadedSegments structure is:
// - Start point. Fake loop LoadedPathSegment with 1 segment of zero length.
// - Straight jump from start point to the beginning of real route. LoadedPathSegment with 1 segment.
// - Real route. N LoadedPathSegments, each with arbitrary amount of segments. N >= 1.
// - Straight jump from the end of real route to finish point. LoadedPathSegment with 1 segment.
// - Finish point. Fake loop LoadedPathSegment with 1 segment of zero length.
// So minimal amount of segments is 5.

// Resulting structure of turnsDir:
// No Turn for 0th segment (no ingoing). m_index == 0.
// No Turn for 1st segment (ingoing fake loop) - at start point. m_index == 1.
// No Turn for 2nd (ingoing == jump) - at beginning of real route. m_index == 2.
// Possible turn for next N-1 segments. m_index >= 3.
// No Turn for (2 + N + 1)th segment (outgoing jump) - at finish point. m_index = 3 + M.
// No Turn for (2 + N + 2)th segment (outgoing fake loop) - at finish point. m_index == 4 + M.
// Added ReachedYourDestination - at finish point. m_index == 4 + M.
// Where M - total amount of all segments from all LoadedPathSegments (size of |segments|).
// So minimum m_index of ReachedYourDestination is 5 (real route with single segment),
// and minimal |turnsDir| is - single ReachedYourDestination with m_index == 5.

RouterResultCode DirectionsEngine::MakeTurnAnnotation(IndexRoadGraph::EdgeVector const & routeEdges,
                                                      base::Cancellable const & cancellable,
                                                      std::vector<geometry::PointWithAltitude> & junctions,
                                                      Route::TTurns & turnsDir, Route::TStreets & streets,
                                                      std::vector<Segment> & segments)
{
  RoutingEngineResult result(routeEdges, m_adjacentEdges, m_pathSegments);

  LOG(LDEBUG, ("Shortest path length:", result.GetPathLength()));

  if (cancellable.IsCancelled())
    return RouterResultCode::Cancelled;

  size_t skipTurnSegments = 0;
  auto const & loadedSegments = result.GetSegments();
  segments.reserve(loadedSegments.size());

  RoutingSettings const vehicleSettings = GetRoutingSettings(m_vehicleType);

  for (auto loadedSegmentIt = loadedSegments.cbegin(); loadedSegmentIt != loadedSegments.cend(); ++loadedSegmentIt)
  {
    CHECK(loadedSegmentIt->IsValid(), ());

    // Street names contain empty names too for avoiding of freezing of old street name while
    // moving along unnamed street.
    streets.emplace_back(max(junctions.size(), static_cast<size_t>(1)) - 1, loadedSegmentIt->m_roadNameInfo);

    // Turns information.
    if (!junctions.empty() && skipTurnSegments == 0)
    {
      size_t const outgoingSegmentIndex = base::asserted_cast<size_t>(distance(loadedSegments.begin(), loadedSegmentIt));

      TurnItem turnItem;
      turnItem.m_index = static_cast<uint32_t>(junctions.size() - 1);

      skipTurnSegments = GetTurnDirection(result, outgoingSegmentIndex, *m_numMwmIds, vehicleSettings, turnItem);

      if (!turnItem.IsTurnNone())
        turnsDir.push_back(move(turnItem));
    }

    if (skipTurnSegments > 0)
      --skipTurnSegments;

    // Path geometry.
    CHECK_GREATER_OR_EQUAL(loadedSegmentIt->m_path.size(), 2, ());
    // Note. Every LoadedPathSegment in TUnpackedPathSegments contains LoadedPathSegment::m_path
    // of several Junctions. Last PointWithAltitude in a LoadedPathSegment::m_path is equal to first
    // junction in next LoadedPathSegment::m_path in vector TUnpackedPathSegments:
    // *---*---*---*---*       *---*           *---*---*---*
    //                 *---*---*   *---*---*---*
    // To prevent having repetitions in |junctions| list it's necessary to take the first point only
    // from the first item of |loadedSegments|. The beginning should be ignored for the rest
    // |m_path|.
    junctions.insert(junctions.end(), loadedSegmentIt == loadedSegments.cbegin()
                                          ? loadedSegmentIt->m_path.cbegin()
                                          : loadedSegmentIt->m_path.cbegin() + 1,
                     loadedSegmentIt->m_path.cend());
    segments.insert(segments.end(), loadedSegmentIt->m_segments.cbegin(),
                    loadedSegmentIt->m_segments.cend());
  }

  // Path found. Points will be replaced by start and end edges junctions.
  if (junctions.size() == 1)
    junctions.push_back(junctions.front());

  if (junctions.size() < 2)
    return RouterResultCode::RouteNotFound;

  junctions.front() = result.GetStartPoint();
  junctions.back() = result.GetEndPoint();

  FixupTurns(junctions, turnsDir);

  return RouterResultCode::NoError;
}

}  // namespace routing
