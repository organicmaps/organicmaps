#include "routing/routing_mapping.h"
#include "routing/turns_generator.hpp"
#include "routing/vehicle_model.hpp"

#include "search/house_detector.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "base/macros.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"

#include "std/numeric.hpp"
#include "std/string.hpp"

using namespace routing;
using namespace routing::turns;

namespace
{
double const kFeaturesNearTurnMeters = 3.0;

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
typedef vector<TurnCandidate> TTurnCandidates;

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
    static CarModel const carModel;
    if (ft.GetFeatureType() != feature::GEOM_LINE || !carModel.IsRoad(ft))
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
  loader.GetFeature(seg.m_fid, ft);
  return ftypes::GetHighwayClass(ft);
}

/*!
 * \brief Returns false when
 * - the route leads from one big road to another one;
 * - and the other possible turns lead to small roads;
 * - and the turn is GoStraight or TurnSlight*.
 */
bool KeepTurnByHighwayClass(ftypes::HighwayClass ingoingClass, ftypes::HighwayClass outgoingClass,
                            NodeID outgoingNode, TurnDirection turn,
                            TTurnCandidates const & possibleTurns,
                            RoutingMapping const & routingMapping, Index const & index)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true;  // The road significantly changes its direction here. So this turn shall be kept.

  // There's only one exit from this junction. NodeID of the exit is outgoingNode.
  if (possibleTurns.size() == 1)
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::Error;
  for (auto const & t : possibleTurns)
  {
    if (t.node == outgoingNode)
      continue;
    ftypes::HighwayClass const highwayClass =
        GetOutgoingHighwayClass(t.node, routingMapping, index);
    if (static_cast<int>(highwayClass) > static_cast<int>(maxClassForPossibleTurns))
      maxClassForPossibleTurns = highwayClass;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("One of possible turns follows along an undefined HighwayClass."));
    return true;
  }

  ftypes::HighwayClass const minClassForTheRoute = static_cast<ftypes::HighwayClass>(
      min(static_cast<int>(ingoingClass), static_cast<int>(outgoingClass)));
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
 * - going from junctionPoint along feature ft according to the direction which is set in GetPointIndex().
 * - until one of following conditions is fulfilled:
 *   - the end of ft is reached; (returns the last feature point)
 *   - more than kMaxPointsCount points are passed; (returns the kMaxPointsCount-th point)
 *   - the length of passed parts of segment exceeds kMinDistMeters; (returns the next point after the event)
 * \param segment is a ingoing or outgoing feature segment.
 * \param ft is a ingoing or outgoing feature.
 * \param junctionPoint is a junction point.
 * \param GetPointIndex is a function for getting points by index.
 * It defines a direction of following along a feature. So it differs for ingoing and outgoing cases.
 * It has following parameters:
 * - start is an index of the start point of a feature segment. For example, FtSeg::m_pointStart.
 * - end is an index of the end point of a feature segment. For example, FtSeg::m_pointEnd.
 * - shift is a number of points which shall be added to end or start index. After that
 * the sum reflects an index of a point of a feature segment which will be used for turn calculation.
 * The sum shall belongs to a range [min(start, end), max(start, end)].
 * shift belongs to a  range [0, abs(end - start)].
 * \return an ingoing or outgoing point for turn calculation.
 */
m2::PointD GetPointForTurn(OsrmMappingTypes::FtSeg const & segment, FeatureType const & ft,
                           m2::PointD const & junctionPoint,
                           size_t (*GetPointIndex)(const size_t start, const size_t end, const size_t shift))
{
  // An ingoing and outgoing point could be farther then kMaxPointsCount points from the junctionPoint
  size_t const kMaxPointsCount = 7;
  // If ft feature is long enough and consist of short segments
  // the point for turn generation is taken as the next point the route after kMinDistMeters.
  double const kMinDistMeters = 300.;
  double curDistanceMeters = 0.;
  m2::PointD point = junctionPoint;
  m2::PointD nextPoint;

  size_t const numSegPoints = abs(segment.m_pointEnd - segment.m_pointStart);
  ASSERT_GREATER(numSegPoints, 0, ());
  ASSERT_LESS(numSegPoints, ft.GetPointsCount(), ());
  size_t const usedFtPntNum = min(kMaxPointsCount, numSegPoints);

  for (size_t i = 1; i <= usedFtPntNum; ++i)
  {
    nextPoint = ft.GetPoint(GetPointIndex(segment.m_pointStart, segment.m_pointEnd, i));
    curDistanceMeters += MercatorBounds::DistanceOnEarth(point, nextPoint);
    if (curDistanceMeters > kMinDistMeters)
      return nextPoint;
    point = nextPoint;
  }
  return nextPoint;
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
  GetTurnGeometry(junctionPoint, ingoingPointOneSegment, geoNodes, mapping, index);
  return geoNodes.size();
}

NodeID GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData,
                         RoutingMapping & routingMapping)
{
  ASSERT_NOT_EQUAL(src, SPECIAL_NODEID, ());
  ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());
  if (!edgeData.shortcut)
    return trg;

  ASSERT_LESS(edgeData.id, routingMapping.m_dataFacade.GetNumberOfNodes(), ());
  EdgeID edge = SPECIAL_EDGEID;
  QueryEdge::EdgeData d;
  for (EdgeID e : routingMapping.m_dataFacade.GetAdjacentEdgeRange(edgeData.id))
  {
    if (routingMapping.m_dataFacade.GetTarget(e) == src)
    {
      d = routingMapping.m_dataFacade.GetEdgeData(e, edgeData.id);
      if (d.backward)
      {
        edge = e;
        break;
      }
    }
  }

  if (edge == SPECIAL_EDGEID)
  {
    for (EdgeID const e : routingMapping.m_dataFacade.GetAdjacentEdgeRange(src))
    {
      if (routingMapping.m_dataFacade.GetTarget(e) == edgeData.id)
      {
        d = routingMapping.m_dataFacade.GetEdgeData(e, src);
        if (d.forward)
        {
          edge = e;
          break;
        }
      }
    }
  }
  ASSERT_NOT_EQUAL(edge, SPECIAL_EDGEID, ());

  if (d.shortcut)
    return GetTurnTargetNode(src, edgeData.id, d, routingMapping);

  return edgeData.id;
}

void GetPossibleTurns(Index const & index, NodeID node, m2::PointD const & ingoingPoint,
                      m2::PointD const & junctionPoint, RoutingMapping & routingMapping,
                      TTurnCandidates & candidates)
{
  for (EdgeID e : routingMapping.m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = routingMapping.m_dataFacade.GetEdgeData(e, node);
    if (!data.forward)
      continue;

    NodeID const trg =
        GetTurnTargetNode(node, routingMapping.m_dataFacade.GetTarget(e), data, routingMapping);
    ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());

    auto const range = routingMapping.m_segMapping.GetSegmentsRange(trg);
    OsrmMappingTypes::FtSeg seg;
    routingMapping.m_segMapping.GetSegmentByIndex(range.first, seg);
    if (!seg.IsValid())
      continue;

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(index, routingMapping.GetMwmId());
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    m2::PointD const outgoingPoint = ft.GetPoint(seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1
                                                                        : seg.m_pointStart - 1);
    ASSERT_LESS(MercatorBounds::DistanceOnEarth(junctionPoint, ft.GetPoint(seg.m_pointStart)),
                kFeaturesNearTurnMeters, ());

    double const a = my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));

    candidates.emplace_back(a, trg);
  }

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.node < t2.node;
  });

  auto const last = unique(candidates.begin(), candidates.end(),
                           [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.node == t2.node;
  });
  candidates.erase(last, candidates.end());

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.angle < t2.angle;
  });
}
}  // namespace

namespace routing
{
namespace turns
{
TurnInfo::TurnInfo(RoutingMapping & routeMapping, NodeID ingoingNodeID, NodeID outgoingNodeID)
    : m_routeMapping(routeMapping),
      m_ingoingNodeID(ingoingNodeID),
      m_ingoingHighwayClass(ftypes::HighwayClass::Undefined),
      m_outgoingNodeID(outgoingNodeID),
      m_outgoingHighwayClass(ftypes::HighwayClass::Undefined)
{
  m_ingoingSegment = GetSegment(m_ingoingNodeID, m_routeMapping, GetLastSegmentPointIndex);
  m_outgoingSegment = GetSegment(m_outgoingNodeID, m_routeMapping, GetFirstSegmentPointIndex);
}

bool TurnInfo::IsSegmentsValid() const
{
  if (!m_ingoingSegment.IsValid() || !m_outgoingSegment.IsValid())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

size_t GetLastSegmentPointIndex(pair<size_t, size_t> const & p)
{
  ASSERT_GREATER(p.second, 0, ());
  return p.second - 1;
}

vector<SingleLaneInfo> GetLanesInfo(NodeID node, RoutingMapping const & routingMapping,
                                    TGetIndexFunction GetIndex, Index const & index)
{
  // seg1 is the last segment before a point of bifurcation (before turn)
  OsrmMappingTypes::FtSeg const seg1 = GetSegment(node, routingMapping, GetIndex);
  vector<SingleLaneInfo> lanes;
  if (seg1.IsValid())
  {
    FeatureType ft1;
    Index::FeaturesLoaderGuard loader1(index, routingMapping.GetMwmId());
    loader1.GetFeature(seg1.m_fid, ft1);

    using feature::Metadata;
    ft1.ParseMetadata();
    Metadata const & md = ft1.GetMetadata();

    if (ftypes::IsOneWayChecker::Instance()(ft1))
    {
      ParseLanes(md.Get(Metadata::FMD_TURN_LANES), lanes);
      return lanes;
    }
    // two way roads
    if (seg1.m_pointStart < seg1.m_pointEnd)
    {
      // forward direction
      ParseLanes(md.Get(Metadata::FMD_TURN_LANES_FORWARD), lanes);
      return lanes;
    }
    // backward direction
    ParseLanes(md.Get(Metadata::FMD_TURN_LANES_BACKWARD), lanes);
    return lanes;
  }
  return lanes;
}

void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TTurns const & turnsDir,
                           TTurnsGeom & turnsGeom)
{
  size_t const kNumPoints = points.size();
  // "Pivot point" is a point of bifurcation (a point of a turn).
  // kNumPointsBeforePivot is number of points before the pivot point.
  uint32_t const kNumPointsBeforePivot = 10;
  // kNumPointsAfterPivot is a number of points follows by the pivot point.
  // kNumPointsAfterPivot is greater because there are half body and the arrow after the pivot point
  uint32_t constexpr kNumPointsAfterPivot = kNumPointsBeforePivot + 10;

  for (TurnItem const & t : turnsDir)
  {
    ASSERT_LESS(t.m_index, kNumPoints, ());
    if (t.m_index == 0 || t.m_index == (kNumPoints - 1))
      continue;

    uint32_t const fromIndex = (t.m_index <= kNumPointsBeforePivot) ? 0 : t.m_index - kNumPointsBeforePivot;
    uint32_t toIndex = 0;
    if (t.m_index + kNumPointsAfterPivot >= kNumPoints || t.m_index + kNumPointsAfterPivot < t.m_index)
      toIndex = kNumPoints;
    else
      toIndex = t.m_index + kNumPointsAfterPivot;

    uint32_t const turnIndex = min(t.m_index, kNumPointsBeforePivot);
    turnsGeom.emplace_back(t.m_index, turnIndex, points.begin() + fromIndex,
                           points.begin() + toIndex);
  }
  return;
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
      roundabout->m_exitNum = exitNum + 1;
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

TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isMultiTurnJunction)
{
  if (isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
  {
    if (isMultiTurnJunction)
      return TurnDirection::StayOnRoundAbout;
    return TurnDirection::NoTurn;
  }

  if (!isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
    return TurnDirection::EnterRoundAbout;

  if (isIngoingEdgeRoundabout && !isOutgoingEdgeRoundabout)
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

void GetTurnDirection(Index const & index, TurnInfo & turnInfo, TurnItem & turn)
{
  if (!turnInfo.IsSegmentsValid())
    return;

  // ingoingFeature and outgoingFeature can be used only within the scope.
  FeatureType ingoingFeature, outgoingFeature;
  Index::FeaturesLoaderGuard ingoingLoader(index, turnInfo.m_routeMapping.GetMwmId());
  Index::FeaturesLoaderGuard outgoingLoader(index, turnInfo.m_routeMapping.GetMwmId());
  ingoingLoader.GetFeature(turnInfo.m_ingoingSegment.m_fid, ingoingFeature);
  outgoingLoader.GetFeature(turnInfo.m_outgoingSegment.m_fid, outgoingFeature);

  ingoingFeature.ParseGeometry(FeatureType::BEST_GEOMETRY);
  outgoingFeature.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_LESS(MercatorBounds::DistanceOnEarth(
                  ingoingFeature.GetPoint(turnInfo.m_ingoingSegment.m_pointEnd),
                  outgoingFeature.GetPoint(turnInfo.m_outgoingSegment.m_pointStart)),
              kFeaturesNearTurnMeters, ());

  m2::PointD const junctionPoint = ingoingFeature.GetPoint(turnInfo.m_ingoingSegment.m_pointEnd);
  m2::PointD const ingoingPoint =
      GetPointForTurn(turnInfo.m_ingoingSegment, ingoingFeature, junctionPoint,
                      [](const size_t start, const size_t end, const size_t i)
      {
        return end > start ? end - i : end + i;
      });
  m2::PointD const outgoingPoint =
      GetPointForTurn(turnInfo.m_outgoingSegment, outgoingFeature, junctionPoint,
                      [](const size_t start, const size_t end, const size_t i)
      {
        return end > start ? start + i : start - i;
      });
  double const a = my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));

  m2::PointD const ingoingPointOneSegment = ingoingFeature.GetPoint(
      turnInfo.m_ingoingSegment.m_pointStart < turnInfo.m_ingoingSegment.m_pointEnd
          ? turnInfo.m_ingoingSegment.m_pointEnd - 1
          : turnInfo.m_ingoingSegment.m_pointEnd + 1);
  TTurnCandidates nodes;
  GetPossibleTurns(index, turnInfo.m_ingoingNodeID, ingoingPointOneSegment, junctionPoint,
                   turnInfo.m_routeMapping, nodes);

  turn.m_turn = TurnDirection::NoTurn;
  size_t const nodesSize = nodes.size();
  bool const hasMultiTurns = (nodesSize >= 2);

  if (nodesSize == 0)
    return;

  if (!hasMultiTurns)
  {
    turn.m_turn = IntermediateDirection(a);
  }
  else
  {
    if (nodes.front().node == turnInfo.m_outgoingNodeID)
      turn.m_turn = LeftmostDirection(a);
    else if (nodes.back().node == turnInfo.m_outgoingNodeID)
      turn.m_turn = RightmostDirection(a);
    else
      turn.m_turn = IntermediateDirection(a);
  }

  bool const isIngoingEdgeRoundabout = ftypes::IsRoundAboutChecker::Instance()(ingoingFeature);
  bool const isOutgoingEdgeRoundabout = ftypes::IsRoundAboutChecker::Instance()(outgoingFeature);

  if (isIngoingEdgeRoundabout || isOutgoingEdgeRoundabout)
  {
    turn.m_turn =
        GetRoundaboutDirection(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout, hasMultiTurns);
    return;
  }

  turn.m_keepAnyway = (!ftypes::IsLinkChecker::Instance()(ingoingFeature) &&
                       ftypes::IsLinkChecker::Instance()(outgoingFeature));

  {
    string name1, name2;

    ingoingFeature.GetName(FeatureType::DEFAULT_LANG, turn.m_sourceName);
    outgoingFeature.GetName(FeatureType::DEFAULT_LANG, turn.m_targetName);

    search::GetStreetNameAsKey(turn.m_sourceName, name1);
    search::GetStreetNameAsKey(turn.m_targetName, name2);
  }

  turnInfo.m_ingoingHighwayClass = ftypes::GetHighwayClass(ingoingFeature);
  turnInfo.m_outgoingHighwayClass = ftypes::GetHighwayClass(outgoingFeature);
  if (!turn.m_keepAnyway &&
      !KeepTurnByHighwayClass(turnInfo.m_ingoingHighwayClass, turnInfo.m_outgoingHighwayClass,
                          turnInfo.m_outgoingNodeID, turn.m_turn, nodes, turnInfo.m_routeMapping, index))
  {
    turn.m_turn = TurnDirection::NoTurn;
    return;
  }

  bool const isGoStraightOrSlightTurn = IsGoStraightOrSlightTurn(IntermediateDirection(
      my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPointOneSegment, outgoingPoint))));
  // The code below is resposible for cases when there is only one way to leave the junction.
  // Such junction has to be kept as a turn when:
  // * it's not a slight turn and it has ingoing edges (one or more);
  // * it's an entrance to a roundabout;
  if (!hasMultiTurns && (isGoStraightOrSlightTurn ||
                         NumberOfIngoingAndOutgoingSegments(junctionPoint, ingoingPointOneSegment,
                                                            turnInfo.m_routeMapping, index) <= 2) &&
      !CheckRoundaboutEntrance(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
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

  // @todo(vbykoianko) Checking if it's a uturn or not shall be moved to FindDirectionByAngle.
  if (turn.m_turn == TurnDirection::NoTurn)
    turn.m_turn = TurnDirection::UTurn;
}
}  // namespace turns
}  // namespace routing
