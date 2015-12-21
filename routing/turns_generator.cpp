#include "turns_generator.hpp"

#include "car_model.hpp"
#include "osrm_helpers.hpp"
#include "routing_mapping.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "base/macros.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"

#include "std/cmath.hpp"
#include "std/numeric.hpp"
#include "std/string.hpp"


using namespace routing;
using namespace routing::turns;

namespace
{
double const kFeaturesNearTurnMeters = 3.0;
size_t constexpr kMaxPointsCount = 5;
double constexpr kMinDistMeters = 200.;
size_t constexpr kNotSoCloseMaxPointsCount = 3;
double constexpr kNotSoCloseMinDistMeters = 30.;

typedef vector<double> TGeomTurnCandidate;

double PiMinusTwoVectorsAngle(m2::PointD const & p, m2::PointD const & p1, m2::PointD const & p2)
{
  return math::pi - ang::TwoVectorsAngle(p, p1, p2);
}

/*!
 * \brief The TurnCandidate struct contains information about possible ways from a junction.
 */
struct TurnCandidate
{
  /*!
   * angle is an angle of the turn in degrees. It means angle is 180 minus
   * an angle between the current edge and the edge of the candidate. A counterclockwise rotation.
   * The current edge is an edge which belongs the route and located before the junction.
   * angle belongs to the range [-180; 180];
   */
  double angle;
  /*!
   * node is a possible node (a possible way) from the juction.
   */
  NodeID node;

  TurnCandidate(double a, NodeID n) : angle(a), node(n) {}
};
using TTurnCandidates = vector<TurnCandidate>;

/*!
 * \brief The Point2Geometry class is responsable for looking for all adjacent to junctionPoint
 * road network edges. Including the current edge.
 */
class Point2Geometry
{
  m2::PointD m_junctionPoint, m_ingoingPoint;
  TGeomTurnCandidate & m_candidates;

public:
  Point2Geometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                 TGeomTurnCandidate & candidates)
      : m_junctionPoint(junctionPoint), m_ingoingPoint(ingoingPoint), m_candidates(candidates)
  {
  }

  void operator()(FeatureType const & ft)
  {
    if (!CarModel::Instance().IsRoad(ft))
      return;
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());

    // @TODO(vbykoianko) instead of checking all the feature points probably
    // it's enough just to check the start and the finish of the feature.
    for (size_t i = 0; i < count; ++i)
    {
      if (MercatorBounds::DistanceOnEarth(m_junctionPoint, ft.GetPoint(i)) <
          kFeaturesNearTurnMeters)
      {
        if (i > 0)
          m_candidates.push_back(my::RadToDeg(
              PiMinusTwoVectorsAngle(m_junctionPoint, m_ingoingPoint, ft.GetPoint(i - 1))));
        if (i < count - 1)
          m_candidates.push_back(my::RadToDeg(
              PiMinusTwoVectorsAngle(m_junctionPoint, m_ingoingPoint, ft.GetPoint(i + 1))));
        return;
      }
    }
  }

  DISALLOW_COPY_AND_MOVE(Point2Geometry);
};


OsrmMappingTypes::FtSeg GetSegment(NodeID node, RoutingMapping const & routingMapping,
                                   TGetIndexFunction GetIndex)
{
  auto const segmentsRange = routingMapping.m_segMapping.GetSegmentsRange(node);
  OsrmMappingTypes::FtSeg seg;
  routingMapping.m_segMapping.GetSegmentByIndex(GetIndex(segmentsRange), seg);
  return seg;
}

ftypes::HighwayClass GetOutgoingHighwayClass(NodeID outgoingNode,
                                             RoutingMapping const & routingMapping,
                                             Index const & index)
{
  OsrmMappingTypes::FtSeg const seg =
      GetSegment(outgoingNode, routingMapping, GetFirstSegmentPointIndex);
  if (!seg.IsValid())
    return ftypes::HighwayClass::Error;

  Index::FeaturesLoaderGuard loader(index, routingMapping.GetMwmId());
  FeatureType ft;
  loader.GetFeatureByIndex(seg.m_fid, ft);
  return ftypes::GetHighwayClass(ft);
}

/*!
 * \brief Returns false when
 * - the route leads from one big road to another one;
 * - and the other possible turns lead to small roads;
 * - and the turn is GoStraight or TurnSlight*.
 */
bool KeepTurnByHighwayClass(TurnDirection turn, TTurnCandidates const & possibleTurns,
                            TurnInfo const & turnInfo, Index const & index,
                            RoutingMapping & mapping)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true;  // The road significantly changes its direction here. So this turn shall be kept.

  // There's only one exit from this junction. NodeID of the exit is outgoingNode.
  if (possibleTurns.size() == 1)
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::Error;
  for (auto const & t : possibleTurns)
  {
    if (t.node == turnInfo.m_outgoing.m_nodeId)
      continue;
    ftypes::HighwayClass const highwayClass = GetOutgoingHighwayClass(t.node, mapping, index);
    if (static_cast<int>(highwayClass) > static_cast<int>(maxClassForPossibleTurns))
      maxClassForPossibleTurns = highwayClass;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("One of possible turns follows along an undefined HighwayClass."));
    return true;
  }

  ftypes::HighwayClass const minClassForTheRoute =
      static_cast<ftypes::HighwayClass>(min(static_cast<int>(turnInfo.m_ingoing.m_highwayClass),
                                            static_cast<int>(turnInfo.m_outgoing.m_highwayClass)));
  if (minClassForTheRoute == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return false;
  }

  int const kMaxHighwayClassDiffToKeepTheTurn = 2;
  if (static_cast<int>(maxClassForPossibleTurns) - static_cast<int>(minClassForTheRoute) >=
      kMaxHighwayClassDiffToKeepTheTurn)
  {
    // The turn shall be removed if the route goes near small roads without changing the direction.
    return false;
  }
  return true;
}

/*!
 * \brief Returns false when other possible turns leads to service roads;
 */
bool KeepRoundaboutTurnByHighwayClass(TurnDirection turn, TTurnCandidates const & possibleTurns,
                                      TurnInfo const & turnInfo, Index const & index,
                                      RoutingMapping & mapping)
{
  for (auto const & t : possibleTurns)
  {
    if (t.node == turnInfo.m_outgoing.m_nodeId)
      continue;
    ftypes::HighwayClass const highwayClass = GetOutgoingHighwayClass(t.node, mapping, index);
    if (static_cast<int>(highwayClass) != static_cast<int>(ftypes::HighwayClass::Service))
      return true;
  }
  return false;
}

bool DiscardTurnByIngoingAndOutgoingEdges(TurnDirection intermediateDirection,
                                          TurnInfo const & turnInfo, TurnItem const & turn)
{
  return !turn.m_keepAnyway && !turnInfo.m_ingoing.m_onRoundabout &&
         !turnInfo.m_outgoing.m_onRoundabout && IsGoStraightOrSlightTurn(intermediateDirection) &&
         turnInfo.m_ingoing.m_highwayClass == turnInfo.m_outgoing.m_highwayClass &&
         turn.m_sourceName == turn.m_targetName;
}

/*!
 * \brief GetTurnGeometry looks for all the road network edges near ingoingPoint.
 * GetTurnGeometry fills candidates with angles of all the incoming and outgoint segments.
 * \warning GetTurnGeometry should be used carefully because it's a time-consuming function.
 * \warning In multilevel crossroads there is an insignificant possibility that candidates
 * is filled with redundant segments of roads of different levels.
 */
void GetTurnGeometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                     TGeomTurnCandidate & candidates, RoutingMapping const & mapping,
                     Index const & index)
{
  Point2Geometry getter(junctionPoint, ingoingPoint, candidates);
  index.ForEachInRectForMWM(
      getter, MercatorBounds::RectByCenterXYAndSizeInMeters(junctionPoint, kFeaturesNearTurnMeters),
      scales::GetUpperScale(), mapping.GetMwmId());
}

/*!
 * \param junctionPoint is a point of the junction.
 * \param ingoingPointOneSegment is a point one segment before the junction along the route.
 * \param mapping is a route mapping.
 * \return number of all the segments which joins junctionPoint. That means
 * the number of ingoing segments plus the number of outgoing segments.
 * \warning NumberOfIngoingAndOutgoingSegments should be used carefully because
 * it's a time-consuming function.
 * \warning In multilevel crossroads there is an insignificant possibility that the returned value
 * contains redundant segments of roads of different levels.
 */
size_t NumberOfIngoingAndOutgoingSegments(m2::PointD const & junctionPoint,
                                          m2::PointD const & ingoingPointOneSegment,
                                          RoutingMapping const & mapping, Index const & index)
{
  TGeomTurnCandidate geoNodes;
  // TODO(vbykoianko) It is repeating of a time consumption operation. The first time
  // the geometry is extracted in GetPossibleTurns and the second time here.
  // It shall be fixed. For the time being this repeating time consumption method
  // is called relevantly seldom.
  GetTurnGeometry(junctionPoint, ingoingPointOneSegment, geoNodes, mapping, index);
  return geoNodes.size();
}

bool KeepTurnByIngoingEdges(m2::PointD const & junctionPoint,
                            m2::PointD const & ingoingPointOneSegment,
                            m2::PointD const & outgoingPoint, bool hasMultiTurns,
                            RoutingMapping const & routingMapping, Index const & index)
{
  double const turnAngle =
    my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPointOneSegment, outgoingPoint));
  bool const isGoStraightOrSlightTurn = IsGoStraightOrSlightTurn(IntermediateDirection(turnAngle));

  // The code below is resposible for cases when there is only one way to leave the junction.
  // Such junction has to be kept as a turn when it's not a slight turn and it has ingoing edges
  // (one or more);
  return hasMultiTurns || (!isGoStraightOrSlightTurn &&
                           NumberOfIngoingAndOutgoingSegments(junctionPoint, ingoingPointOneSegment,
                                                              routingMapping, index) > 2);
}

bool FixupLaneSet(TurnDirection turn, vector<SingleLaneInfo> & lanes,
                  function<bool(LaneWay l, TurnDirection t)> checker)
{
  bool isLaneConformed = false;
  // There are two nested loops below. (There is a for-loop in checker.)
  // But the number of calls of the body of inner one (in checker) is relatively small.
  // Less than 10 in most cases.
  for (auto & singleLane : lanes)
  {
    for (LaneWay laneWay : singleLane.m_lane)
    {
      if (checker(laneWay, turn))
      {
        singleLane.m_isRecommended = true;
        isLaneConformed = true;
        break;
      }
    }
  }
  return isLaneConformed;
}

/*!
 * \brief Converts a turn angle into a turn direction.
 * \note lowerBounds is a table of pairs: an angle and a direction.
 * lowerBounds shall be sorted by the first parameter (angle) from big angles to small angles.
 * These angles should be measured in degrees and should belong to the range [-180; 180].
 * The second paramer (angle) shall belong to the range [-180; 180] and is measured in degrees.
 */
TurnDirection FindDirectionByAngle(vector<pair<double, TurnDirection>> const & lowerBounds,
                                   double angle)
{
  ASSERT_GREATER_OR_EQUAL(angle, -180., (angle));
  ASSERT_LESS_OR_EQUAL(angle, 180., (angle));
  ASSERT(!lowerBounds.empty(), ());
  ASSERT(is_sorted(lowerBounds.cbegin(), lowerBounds.cend(),
             [](pair<double, TurnDirection> const & p1, pair<double, TurnDirection> const & p2)
         {
           return p1.first > p2.first;
         }), ());

  for (auto const & lower : lowerBounds)
  {
    if (angle >= lower.first)
      return lower.second;
  }

  ASSERT(false, ("The angle is not covered by the table. angle = ", angle));
  return TurnDirection::NoTurn;
}

/*!
 * \brief GetPointForTurn returns ingoingPoint or outgoingPoint for turns.
 * These points belongs to the route but they often are not neighbor of junctionPoint.
 * To calculate the resulting point the function implements the following steps:
 * - going from junctionPoint along segment path according to the direction which is set in GetPointIndex().
 * - until one of following conditions is fulfilled:
 *   - the end of ft is reached; (returns the last feature point)
 *   - more than kMaxPointsCount points are passed; (returns the kMaxPointsCount-th point)
 *   - the length of passed parts of segment exceeds kMinDistMeters; (returns the next point after the event)
 * \param path geometry of the segment.
 * \param junctionPoint is a junction point.
 * \param maxPointsCount returned poit could't be more than maxPointsCount poins away from junctionPoint
 * \param minDistMeters returned point should be minDistMeters away from junctionPoint if ft is long and consists of short segments
 * \param GetPointIndex is a function for getting points by index.
 * It defines a direction of following along a feature. So it differs for ingoing and outgoing
 * cases.
 * It has following parameters:
 * - start is an index of the start point of a feature segment. For example, path.back().
 * - end is an index of the end point of a feature segment. For example, path.front().
 * - shift is a number of points which shall be added to end or start index. After that
 *   the sum reflects an index of a feature segment point which will be used for a turn calculation.
 * The sum shall belongs to a range [min(start, end), max(start, end)].
 * shift belongs to a  range [0, abs(end - start)].
 * \return an ingoing or outgoing point for a turn calculation.
 */
m2::PointD GetPointForTurn(vector<m2::PointD> const & path, m2::PointD const & junctionPoint,
                           size_t const maxPointsCount, double const minDistMeters,
                           function<size_t(const size_t start, const size_t end, const size_t shift)> GetPointIndex)
{
  ASSERT(!path.empty(), ());

  double curDistanceMeters = 0.;
  m2::PointD point = junctionPoint;
  m2::PointD nextPoint;

  size_t const numSegPoints = path.size() - 1;
  ASSERT_GREATER(numSegPoints, 0, ());
  size_t const usedFtPntNum = min(maxPointsCount, numSegPoints);
  ASSERT_GREATER_OR_EQUAL(usedFtPntNum, 1, ());

  for (size_t i = 1; i <= usedFtPntNum; ++i)
  {
    nextPoint = path[GetPointIndex(0, numSegPoints, i)];

    // TODO The code below is a stub for compatability with older versions with this function.
    // Remove it, fix tests cases when it works (integration test
    // RussiaMoscowTTKKashirskoeShosseOutTurnTest)
    // and remove point duplication when we get geometry from feature segments.
    if (point == nextPoint)
      return nextPoint;

    curDistanceMeters += MercatorBounds::DistanceOnEarth(point, nextPoint);
    if (curDistanceMeters > minDistMeters)
      return nextPoint;
    point = nextPoint;
  }

  return nextPoint;
}

// OSRM graph contains preprocessed edges without proper information about adjecency.
// So, to determine we must read the nearest geometry and check its adjacency by OSRM road graph.
void GetPossibleTurns(Index const & index, NodeID node, m2::PointD const & ingoingPoint,
                      m2::PointD const & junctionPoint, RoutingMapping & routingMapping,
                      TTurnCandidates & candidates)
{
  double const kReadCrossEpsilon = 1.0E-4;

  // Geting nodes by geometry.
  vector<NodeID> geomNodes;
  helpers::Point2Node p2n(routingMapping, geomNodes);

  index.ForEachInRectForMWM(
      p2n, m2::RectD(junctionPoint.x - kReadCrossEpsilon, junctionPoint.y - kReadCrossEpsilon,
                     junctionPoint.x + kReadCrossEpsilon, junctionPoint.y + kReadCrossEpsilon),
      scales::GetUpperScale(), routingMapping.GetMwmId());

  sort(geomNodes.begin(), geomNodes.end());
  geomNodes.erase(unique(geomNodes.begin(), geomNodes.end()), geomNodes.end());

  // Filtering virtual edges.
  vector<NodeID> adjacentNodes;
  for (EdgeID const e : routingMapping.m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = routingMapping.m_dataFacade.GetEdgeData(e, node);
    if (data.forward && !data.shortcut)
    {
      adjacentNodes.push_back(routingMapping.m_dataFacade.GetTarget(e));
      ASSERT_NOT_EQUAL(routingMapping.m_dataFacade.GetTarget(e), SPECIAL_NODEID, ());
    }
  }

  for (NodeID const adjacentNode : geomNodes)
  {
    if (adjacentNode == node)
      continue;
    for (EdgeID const e : routingMapping.m_dataFacade.GetAdjacentEdgeRange(adjacentNode))
    {
      if (routingMapping.m_dataFacade.GetTarget(e) != node)
        continue;
      QueryEdge::EdgeData const data = routingMapping.m_dataFacade.GetEdgeData(e, adjacentNode);
      if (!data.shortcut && data.backward)
        adjacentNodes.push_back(adjacentNode);
    }
  }

  // Preparing candidates.
  for (NodeID const targetNode : adjacentNodes)
  {
    auto const range = routingMapping.m_segMapping.GetSegmentsRange(targetNode);
    OsrmMappingTypes::FtSeg seg;
    routingMapping.m_segMapping.GetSegmentByIndex(range.first, seg);
    if (!seg.IsValid())
      continue;

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(index, routingMapping.GetMwmId());
    loader.GetFeatureByIndex(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    m2::PointD const outgoingPoint = ft.GetPoint(
        seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
    ASSERT_LESS(MercatorBounds::DistanceOnEarth(junctionPoint, ft.GetPoint(seg.m_pointStart)),
                kFeaturesNearTurnMeters, ());

    double const a = my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
    candidates.emplace_back(a, targetNode);
  }

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.angle < t2.angle;
  });
}

size_t GetIngoingPointIndex(const size_t start, const size_t end, const size_t i)
{
  return end > start ? end - i : end + i;
}

size_t GetOutgoingPointIndex(const size_t start, const size_t end, const size_t i)
{
  return end > start ? start + i : start - i;
}
}  // namespace

namespace routing
{
namespace turns
{
using TSeg = OsrmMappingTypes::FtSeg;

LoadedPathSegment::LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                                     RawPathData const & osrmPathSegment)
  : m_highwayClass(ftypes::HighwayClass::Undefined)
  , m_onRoundabout(false)
  , m_isLink(false)
  , m_weight(osrmPathSegment.segmentWeight)
  , m_nodeId(osrmPathSegment.node)
{
  buffer_vector<TSeg, 8> buffer;
  mapping.m_segMapping.ForEachFtSeg(osrmPathSegment.node, MakeBackInsertFunctor(buffer));
  LoadPathGeometry(buffer, 0, buffer.size(), index, mapping, FeatureGraphNode(), FeatureGraphNode(),
                   false /* isStartNode */, false /*isEndNode*/);
}

void LoadedPathSegment::LoadPathGeometry(buffer_vector<TSeg, 8> const & buffer, size_t startIndex,
                                         size_t endIndex, Index const & index, RoutingMapping & mapping,
                                         FeatureGraphNode const & startGraphNode,
                                         FeatureGraphNode const & endGraphNode, bool isStartNode,
                                         bool isEndNode)
{
  ASSERT_LESS(startIndex, endIndex, ());
  ASSERT_LESS_OR_EQUAL(endIndex, buffer.size(), ());
  ASSERT(!buffer.empty(), ());
  for (size_t k = startIndex; k < endIndex; ++k)
  {
    auto const & segment = buffer[k];
    if (!segment.IsValid())
    {
      m_path.clear();
      return;
    }
    // Load data from drive.
    FeatureType ft;
    Index::FeaturesLoaderGuard loader(index, mapping.GetMwmId());
    loader.GetFeatureByIndex(segment.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    // Get points in proper direction.
    auto startIdx = segment.m_pointStart;
    auto endIdx = segment.m_pointEnd;
    if (isStartNode && k == startIndex && startGraphNode.segment.IsValid())
      startIdx = (segment.m_pointEnd > segment.m_pointStart) ? startGraphNode.segment.m_pointStart
                                                             : startGraphNode.segment.m_pointEnd;
    if (isEndNode && k == endIndex - 1 && endGraphNode.segment.IsValid())
      endIdx = (segment.m_pointEnd > segment.m_pointStart) ? endGraphNode.segment.m_pointEnd
                                                           : endGraphNode.segment.m_pointStart;
    if (startIdx < endIdx)
    {
      for (auto idx = startIdx; idx <= endIdx; ++idx)
        m_path.push_back(ft.GetPoint(idx));
    }
    else
    {
      // I use big signed type because endIdx can be 0.
      for (int64_t idx = startIdx; idx >= static_cast<int64_t>(endIdx); --idx)
        m_path.push_back(ft.GetPoint(idx));
    }

    // Load lanes if it is a last segment before junction.
    if (buffer.back() == segment)
    {
      using feature::Metadata;
      ft.ParseMetadata();
      Metadata const & md = ft.GetMetadata();

      auto directionType = Metadata::FMD_TURN_LANES;

      if (!ftypes::IsOneWayChecker::Instance()(ft))
      {
        directionType = (startIdx < endIdx) ? Metadata::FMD_TURN_LANES_FORWARD
                                            : Metadata::FMD_TURN_LANES_BACKWARD;
      }
      ParseLanes(md.Get(directionType), m_lanes);
    }
    // Calculate node flags.
    m_onRoundabout |= ftypes::IsRoundAboutChecker::Instance()(ft);
    m_isLink |= ftypes::IsLinkChecker::Instance()(ft);
    m_highwayClass = ftypes::GetHighwayClass(ft);
    string name;
    ft.GetName(FeatureType::DEFAULT_LANG, name);
    if (!name.empty())
      m_name = name;
  }
}

LoadedPathSegment::LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                                     RawPathData const & osrmPathSegment,
                                     FeatureGraphNode const & startGraphNode,
                                     FeatureGraphNode const & endGraphNode, bool isStartNode,
                                     bool isEndNode)
  : m_highwayClass(ftypes::HighwayClass::Undefined)
  , m_onRoundabout(false)
  , m_isLink(false)
  , m_weight(0)
  , m_nodeId(osrmPathSegment.node)
{
  ASSERT(isStartNode || isEndNode, ("This function process only corner cases."));
  if (!startGraphNode.segment.IsValid() || !endGraphNode.segment.IsValid())
    return;
  buffer_vector<TSeg, 8> buffer;
  mapping.m_segMapping.ForEachFtSeg(osrmPathSegment.node, MakeBackInsertFunctor(buffer));

  auto findIntersectingSeg = [&buffer](TSeg const & seg) -> size_t
  {
    ASSERT(seg.IsValid(), ());
    auto const it = find_if(buffer.begin(), buffer.end(), [&seg](OsrmMappingTypes::FtSeg const & s)
                            {
                              return s.IsIntersect(seg);
                            });

    ASSERT(it != buffer.end(), ());
    return distance(buffer.begin(), it);
  };

  // Calculate estimated time for a start and a end node cases.
  if (isStartNode && isEndNode)
  {
    double const forwardWeight = (osrmPathSegment.node == startGraphNode.node.forward_node_id)
                                     ? startGraphNode.node.forward_weight
                                     : startGraphNode.node.reverse_weight;
    double const backwardWeight = (osrmPathSegment.node == endGraphNode.node.forward_node_id)
                                      ? endGraphNode.node.forward_weight
                                      : endGraphNode.node.reverse_weight;
    double const wholeWeight = (osrmPathSegment.node == startGraphNode.node.forward_node_id)
                                   ? startGraphNode.node.forward_offset
                                   : startGraphNode.node.reverse_offset;
    // Sum because weights in forward/backward_weight fields are negative. Look osrm_helpers for
    // more info.
    m_weight = wholeWeight + forwardWeight + backwardWeight;
  }
  else
  {
    PhantomNode const * node = nullptr;
    if (isStartNode)
      node = &startGraphNode.node;
    if (isEndNode)
      node = &endGraphNode.node;
    if (node)
    {
      m_weight = (osrmPathSegment.node == node->forward_weight)
                  ? node->GetForwardWeightPlusOffset() : node->GetReverseWeightPlusOffset();
    }
  }

  size_t startIndex = isStartNode ? findIntersectingSeg(startGraphNode.segment) : 0;
  size_t endIndex = isEndNode ? findIntersectingSeg(endGraphNode.segment) + 1 : buffer.size();
  LoadPathGeometry(buffer, startIndex, endIndex, index, mapping, startGraphNode, endGraphNode, isStartNode,
                   isEndNode);
}

bool TurnInfo::IsSegmentsValid() const
{
  if (m_ingoing.m_path.empty() || m_outgoing.m_path.empty())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

double CalculateMercatorDistanceAlongPath(uint32_t startPointIndex, uint32_t endPointIndex,
                                          vector<m2::PointD> const & points)
{
  ASSERT_LESS(endPointIndex, points.size(), ());
  ASSERT_LESS_OR_EQUAL(startPointIndex, endPointIndex, ());

  double mercatorDistanceBetweenTurns = 0;
  for (uint32_t i = startPointIndex; i != endPointIndex; ++i)
    mercatorDistanceBetweenTurns += points[i].Length(points[i + 1]);

  return mercatorDistanceBetweenTurns;
}

void FixupTurns(vector<m2::PointD> const & points, Route::TTurns & turnsDir)
{
  double const kMergeDistMeters = 30.0;
  // For turns that are not EnterRoundAbout exitNum is always equal to zero.
  // If a turn is EnterRoundAbout exitNum is a number of turns between two points:
  // (1) the route enters to the roundabout;
  // (2) the route leaves the roundabout;
  uint32_t exitNum = 0;
  // If a roundabout is worked up the roundabout value points to the turn
  // of the enter to the roundabout. If not, roundabout is equal to nullptr.
  TurnItem * roundabout = nullptr;

  auto routeDistanceMeters = [&points](uint32_t start, uint32_t end)
  {
    double res = 0.0;
    for (uint32_t i = start + 1; i < end; ++i)
      res += MercatorBounds::DistanceOnEarth(points[i - 1], points[i]);
    return res;
  };

  for (size_t idx = 0; idx < turnsDir.size(); )
  {
    TurnItem & t = turnsDir[idx];
    if (roundabout && t.m_turn != TurnDirection::StayOnRoundAbout &&
        t.m_turn != TurnDirection::LeaveRoundAbout)
    {
      exitNum = 0;
      roundabout = nullptr;
    }
    else if (t.m_turn == TurnDirection::EnterRoundAbout)
    {
      ASSERT(!roundabout, ());
      roundabout = &t;
    }
    else if (t.m_turn == TurnDirection::StayOnRoundAbout)
    {
      ++exitNum;
      turnsDir.erase(turnsDir.begin() + idx);
      continue;
    }
    else if (roundabout && t.m_turn == TurnDirection::LeaveRoundAbout)
    {
      roundabout->m_exitNum = exitNum + 1; // For EnterRoundAbout turn.
      t.m_exitNum = roundabout->m_exitNum; // For LeaveRoundAbout turn.
      roundabout = nullptr;
      exitNum = 0;
    }

    // Merging turns which are closed to each other under some circumstance.
    // distance(turnsDir[idx - 1].m_index, turnsDir[idx].m_index) < kMergeDistMeters
    // means the distance in meters between the former turn (idx - 1)
    // and the current turn (idx).
    if (idx > 0 && IsStayOnRoad(turnsDir[idx - 1].m_turn) &&
        IsLeftOrRightTurn(turnsDir[idx].m_turn) &&
        routeDistanceMeters(turnsDir[idx - 1].m_index, turnsDir[idx].m_index) < kMergeDistMeters)
    {
      turnsDir.erase(turnsDir.begin() + idx - 1);
      continue;
    }

    // @todo(vbykoianko) The sieve below is made for filtering unnecessary turns on Moscow's MKAD
    // and roads like it. It's a quick fix but it's possible to do better.
    // The better solution is to remove all "slight" turns if the route goes form one not-link road
    // to another not-link road and other possible turns are links. But it's not possible to
    // implement it quickly. To do that you need to calculate FeatureType for most possible turns.
    // But it is already made once in  KeepTurnByHighwayClass(GetOutgoingHighwayClass).
    // So it's a good idea to keep FeatureType for outgoing turns in TTurnCandidates
    // (if they have been calculated). For the time being I decided to postpone the implementation
    // of the feature but it is worth implementing it in the future.
    // To implement the new sieve (the better solution) use TurnInfo structure.
    if (!t.m_keepAnyway && IsGoStraightOrSlightTurn(t.m_turn) && !t.m_sourceName.empty() &&
        strings::AlmostEqual(t.m_sourceName, t.m_targetName, 2 /* mismatched symbols count */))
    {
      turnsDir.erase(turnsDir.begin() + idx);
      continue;
    }

    ++idx;
  }
  SelectRecommendedLanes(turnsDir);
  return;
}

void SelectRecommendedLanes(Route::TTurns & turnsDir)
{
  for (auto & t : turnsDir)
  {
    vector<SingleLaneInfo> & lanes = t.m_lanes;
    if (lanes.empty())
      continue;
    TurnDirection const turn = t.m_turn;
    // Checking if threre are elements in lanes which correspond with the turn exactly.
    // If so fixing up all the elements in lanes which correspond with the turn.
    if (FixupLaneSet(turn, lanes, &IsLaneWayConformedTurnDirection))
      continue;
    // If not checking if there are elements in lanes which corresponds with the turn
    // approximately. If so fixing up all these elements.
    FixupLaneSet(turn, lanes, &IsLaneWayConformedTurnDirectionApproximately);
  }
}

bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
{
  return !isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout;
}

bool CheckRoundaboutExit(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
{
  return isIngoingEdgeRoundabout && !isOutgoingEdgeRoundabout;
}

TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isMultiTurnJunction, bool keepTurnByHighwayClass)
{
  if (isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
  {
    if (isMultiTurnJunction)
      return keepTurnByHighwayClass ? TurnDirection::StayOnRoundAbout : TurnDirection::NoTurn;
    return TurnDirection::NoTurn;
  }

  if (CheckRoundaboutEntrance(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
    return TurnDirection::EnterRoundAbout;

  if (CheckRoundaboutExit(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
    return TurnDirection::LeaveRoundAbout;

  ASSERT(false, ());
  return TurnDirection::NoTurn;
}

TurnDirection InvertDirection(TurnDirection dir)
{
  switch (dir)
  {
    case TurnDirection::TurnSlightRight:
      return TurnDirection::TurnSlightLeft;
    case TurnDirection::TurnRight:
      return TurnDirection::TurnLeft;
    case TurnDirection::TurnSharpRight:
      return TurnDirection::TurnSharpLeft;
    case TurnDirection::TurnSlightLeft:
      return TurnDirection::TurnSlightRight;
    case TurnDirection::TurnLeft:
      return TurnDirection::TurnRight;
    case TurnDirection::TurnSharpLeft:
      return TurnDirection::TurnSharpRight;
    default:
      return dir;
  };
}

TurnDirection RightmostDirection(const double angle)
{
  static vector<pair<double, TurnDirection>> const kLowerBounds = {
      {157., TurnDirection::TurnSharpRight},
      {40., TurnDirection::TurnRight},
      {-10., TurnDirection::TurnSlightRight},
      {-20., TurnDirection::GoStraight},
      {-60., TurnDirection::TurnSlightLeft},
      {-157., TurnDirection::TurnLeft},
      {-180., TurnDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

TurnDirection LeftmostDirection(const double angle)
{
  return InvertDirection(RightmostDirection(-angle));
}

TurnDirection IntermediateDirection(const double angle)
{
  static vector<pair<double, TurnDirection>> const kLowerBounds = {
      {157., TurnDirection::TurnSharpRight},
      {50., TurnDirection::TurnRight},
      {10., TurnDirection::TurnSlightRight},
      {-10., TurnDirection::GoStraight},
      {-50., TurnDirection::TurnSlightLeft},
      {-157., TurnDirection::TurnLeft},
      {-180., TurnDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

void GetTurnDirection(Index const & index, RoutingMapping & mapping, TurnInfo & turnInfo,
                      TurnItem & turn)
{
  if (!turnInfo.IsSegmentsValid())
    return;

  ASSERT_LESS(MercatorBounds::DistanceOnEarth(turnInfo.m_ingoing.m_path.back(),
                                              turnInfo.m_outgoing.m_path.front()),
              kFeaturesNearTurnMeters, ());

  m2::PointD const junctionPoint = turnInfo.m_ingoing.m_path.back();
  m2::PointD const ingoingPoint = GetPointForTurn(turnInfo.m_ingoing.m_path, junctionPoint,
                                                  kMaxPointsCount, kMinDistMeters,
                                                  GetIngoingPointIndex);
  m2::PointD const outgoingPoint = GetPointForTurn(turnInfo.m_outgoing.m_path, junctionPoint,
                                                   kMaxPointsCount, kMinDistMeters,
                                                   GetOutgoingPointIndex);

  double const turnAngle = my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
  TurnDirection const intermediateDirection = IntermediateDirection(turnAngle);

  turn.m_keepAnyway = (!turnInfo.m_ingoing.m_isLink && turnInfo.m_outgoing.m_isLink);
  turn.m_sourceName = turnInfo.m_ingoing.m_name;
  turn.m_targetName = turnInfo.m_outgoing.m_name;
  turn.m_turn = TurnDirection::NoTurn;
  // Early filtering based only on the information about ingoing and outgoing edges.
  if (DiscardTurnByIngoingAndOutgoingEdges(intermediateDirection, turnInfo, turn))
    return;

  ASSERT_GREATER(turnInfo.m_ingoing.m_path.size(), 1, ());
  m2::PointD const ingoingPointOneSegment = turnInfo.m_ingoing.m_path[turnInfo.m_ingoing.m_path.size() - 2];
  TTurnCandidates nodes;
  GetPossibleTurns(index, turnInfo.m_ingoing.m_nodeId, ingoingPointOneSegment, junctionPoint,
                   mapping, nodes);

  size_t const numNodes = nodes.size();
  bool const hasMultiTurns = numNodes > 1;

  if (numNodes == 0)
    return;

  if (!hasMultiTurns)
  {
    turn.m_turn = intermediateDirection;
  }
  else
  {
    if (nodes.front().node == turnInfo.m_outgoing.m_nodeId)
      turn.m_turn = LeftmostDirection(turnAngle);
    else if (nodes.back().node == turnInfo.m_outgoing.m_nodeId)
      turn.m_turn = RightmostDirection(turnAngle);
    else
      turn.m_turn = intermediateDirection;
  }

  if (turnInfo.m_ingoing.m_onRoundabout || turnInfo.m_outgoing.m_onRoundabout)
  {
    bool const keepTurnByHighwayClass = KeepRoundaboutTurnByHighwayClass(turn.m_turn, nodes, turnInfo, index, mapping);
    turn.m_turn = GetRoundaboutDirection(turnInfo.m_ingoing.m_onRoundabout,
                                         turnInfo.m_outgoing.m_onRoundabout, hasMultiTurns,
                                         keepTurnByHighwayClass);
    return;
  }

  bool const keepTurnByHighwayClass = KeepTurnByHighwayClass(turn.m_turn, nodes, turnInfo, index, mapping);
  if (!turn.m_keepAnyway && !keepTurnByHighwayClass)
  {
    turn.m_turn = TurnDirection::NoTurn;
    return;
  }

  auto const notSoCloseToTheTurnPoint =
      GetPointForTurn(turnInfo.m_ingoing.m_path, junctionPoint, kNotSoCloseMaxPointsCount,
                      kNotSoCloseMinDistMeters, GetIngoingPointIndex);

  if (!KeepTurnByIngoingEdges(junctionPoint, notSoCloseToTheTurnPoint, outgoingPoint, hasMultiTurns,
                              mapping, index))
  {
    turn.m_turn = TurnDirection::NoTurn;
    return;
  }

  if (turn.m_turn == TurnDirection::GoStraight)
  {
    if (!hasMultiTurns)
      turn.m_turn = TurnDirection::NoTurn;
    return;
  }
}

size_t CheckUTurnOnRoute(vector<LoadedPathSegment> const & segments, size_t currentSegment, TurnItem & turn)
{
  size_t constexpr kUTurnLookAhead = 3;
  double constexpr kUTurnHeadingSensitivity = math::pi / 10.0;

  // In this function we process the turn between the previous and the current
  // segments. So we need a shift to get the previous segment.
  ASSERT_GREATER(segments.size(), 1, ());
  ASSERT_GREATER(currentSegment, 0, ());
  ASSERT_GREATER(segments.size(), currentSegment, ());
  auto const & masterSegment = segments[currentSegment - 1];
  ASSERT_GREATER(masterSegment.m_path.size(), 1, ());
  // Roundabout is not the UTurn.
  if (masterSegment.m_onRoundabout)
    return 0;
  for (size_t i = 0; i < kUTurnLookAhead && i + currentSegment < segments.size(); ++i)
  {
    auto const & checkedSegment = segments[currentSegment + i];
    if (checkedSegment.m_name == masterSegment.m_name &&
        checkedSegment.m_highwayClass == masterSegment.m_highwayClass &&
        checkedSegment.m_isLink == masterSegment.m_isLink && !checkedSegment.m_onRoundabout)
    {
      auto const & path = masterSegment.m_path;
      // Same segment UTurn case.
      if (i == 0)
      {
        // TODO Fix direction calculation.
        // Warning! We can not determine UTurn direction in single edge case. So we use UTurnLeft.
        // We decided to add driving rules (left-right sided driving) to mwm header.
        if (path[path.size() - 2] == checkedSegment.m_path[1])
        {
          turn.m_turn = TurnDirection::UTurnLeft;
          return 1;
        }
        // Wide UTurn must have link in it's middle.
        return 0;
      }

      // Avoid the UTurn on unnamed roads inside the rectangle based distinct.
      if (checkedSegment.m_name.empty())
        return 0;

      // Avoid returning to the same edge after uturn somewere else.
      if (path[path.size() - 2] == checkedSegment.m_path[1])
        return 0;

      m2::PointD const v1 = path[path.size() - 1] - path[path.size() - 2];
      m2::PointD const v2 = checkedSegment.m_path[1] - checkedSegment.m_path[0];

      auto angle = ang::TwoVectorsAngle(m2::PointD::Zero(), v1, v2);

      if (!my::AlmostEqualAbs(angle, math::pi, kUTurnHeadingSensitivity))
        return 0;

      // Determine turn direction.
      m2::PointD const junctionPoint = masterSegment.m_path.back();
      m2::PointD const ingoingPoint = GetPointForTurn(masterSegment.m_path, junctionPoint,
                                                      kMaxPointsCount, kMinDistMeters,
                                                      GetIngoingPointIndex);
      m2::PointD const outgoingPoint = GetPointForTurn(segments[currentSegment].m_path, junctionPoint,
                                                       kMaxPointsCount, kMinDistMeters,
                                                       GetOutgoingPointIndex);
      if (PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint) < 0)
        turn.m_turn = TurnDirection::UTurnLeft;
      else
        turn.m_turn = TurnDirection::UTurnRight;
      return i + 1;
    }
  }

  return 0;
}
}  // namespace turns
}  // namespace routing
