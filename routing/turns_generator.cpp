#include "routing/routing_mapping.h"
#include "routing/turns_generator.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"

#include "std/numeric.hpp"
#include "std/string.hpp"

namespace
{
using namespace routing::turns;

bool FixupLaneSet(TurnDirection turn, vector<SingleLaneInfo> & lanes,
                  function<bool(LaneWay l, TurnDirection t)> checker)
{
  bool isLaneConformed = false;
  // There are two hidden nested loops below (transform and find_if).
  // But the number of calls of the body of inner one (lambda in find_if) is relatively small.
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

// Converts a turn angle (double) into a turn direction (TurnDirection).
// upperBounds is a table of pairs: an angle and a direction.
// upperBounds shall be sorted by the first parameter (angle) from small angles to big angles.
// These angles should be measured in degrees and should belong to the range [0; 360].
// The second paramer (angle) shall be greater than or equal to zero and is measured in degrees.
TurnDirection FindDirectionByAngle(vector<pair<double, TurnDirection>> const & upperBounds,
                                   double angle)
{
  ASSERT_GREATER_OR_EQUAL(angle, 0., (angle));
  ASSERT_LESS_OR_EQUAL(angle, 360., (angle));
  ASSERT_GREATER(upperBounds.size(), 0, ());
  ASSERT(is_sorted(upperBounds.cbegin(), upperBounds.cend(),
             [](pair<double, TurnDirection> const & p1, pair<double, TurnDirection> const & p2)
         {
           return p1.first < p2.first;
         }), ());

  for (auto const & upper : upperBounds)
  {
    if (angle <= upper.first)
      return upper.second;
  }

  ASSERT(false, ("The angle is not covered by the table. angle = ", angle));
  return TurnDirection::NoTurn;
}
}  // namespace

namespace routing
{
namespace turns
{
size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p) { return p.first; }

OsrmMappingTypes::FtSeg GetSegment(NodeID node, RoutingMapping const & routingMapping,
                                   TGetIndexFunction GetIndex)
{
  auto const segmentsRange = routingMapping.m_segMapping.GetSegmentsRange(node);
  OsrmMappingTypes::FtSeg seg;
  routingMapping.m_segMapping.GetSegmentByIndex(GetIndex(segmentsRange), seg);
  return seg;
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

void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir,
                           TurnsGeomT & turnsGeom)
{
  size_t const kNumPoints = points.size();
  /// "Pivot point" is a point of bifurcation (a point of a turn).
  /// kNumPointsBeforePivot is number of points before the pivot point.
  uint32_t const kNumPointsBeforePivot = 10;
  /// kNumPointsAfterPivot is a number of points follows by the pivot point.
  /// kNumPointsAfterPivot is greater because there are half body and the arrow after the pivot point
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

void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir)
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
    // But it is already made once in  HighwayClassFilter(GetOutgoingHighwayClass).
    // So it's a good idea to keep FeatureType for outgoing turns in TTurnCandidates
    // (if they have been calculated). For the time being I decided to postpone the implementation
    // of the feature but it is worth implementing it in the future.
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

void SelectRecommendedLanes(Route::TurnsT & turnsDir)
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

ftypes::HighwayClass GetOutgoingHighwayClass(NodeID outgoingNode,
                                             RoutingMapping const & routingMapping,
                                             Index const & index)
{
  OsrmMappingTypes::FtSeg const seg =
      GetSegment(outgoingNode, routingMapping, GetFirstSegmentPointIndex);
  if (!seg.IsValid())
    return ftypes::HighwayClass::None;
  Index::FeaturesLoaderGuard loader(index, routingMapping.GetMwmId());
  FeatureType ft;
  loader.GetFeature(seg.m_fid, ft);
  return ftypes::GetHighwayClass(ft);
}

bool HighwayClassFilter(ftypes::HighwayClass ingoingClass, ftypes::HighwayClass outgoingClass,
                        NodeID outgoingNode, TurnDirection turn,
                        TTurnCandidates const & possibleTurns,
                        RoutingMapping const & routingMapping, Index const & index)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true;  // The road significantly changes its direction here. So this turn shall be kept.

  // There's only one exit from this junction. NodeID of the exit is outgoingNode.
  if (possibleTurns.size() == 1)
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::None;
  for (auto const & t : possibleTurns)
  {
    if (t.m_node == outgoingNode)
      continue;
    ftypes::HighwayClass const highwayClass =
        GetOutgoingHighwayClass(t.m_node, routingMapping, index);
    if (static_cast<int>(highwayClass) > static_cast<int>(maxClassForPossibleTurns))
      maxClassForPossibleTurns = highwayClass;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::None)
  {
    ASSERT(false, ("One of possible turns follows along an undefined HighwayClass."));
    return true;
  }

  ftypes::HighwayClass const minClassForTheRoute = static_cast<ftypes::HighwayClass>(
      min(static_cast<int>(ingoingClass), static_cast<int>(outgoingClass)));
  if (minClassForTheRoute == ftypes::HighwayClass::None)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return false;
  }

  int const maxHighwayClassDiffToKeepTheTurn = 2;
  if (static_cast<int>(maxClassForPossibleTurns) - static_cast<int>(minClassForTheRoute) >=
      maxHighwayClassDiffToKeepTheTurn)
  {
    // The turn shall be removed if the route goes near small roads without changing the direction.
    return false;
  }
  return true;
}

bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
{
  return !isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout;
}

TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isJunctionOfSeveralTurns)
{
  if (isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
  {
    if (isJunctionOfSeveralTurns)
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

TurnDirection MostRightDirection(const double angle)
{
  static vector<pair<double, TurnDirection>> const kUpperBounds = {
      {23., TurnDirection::NoTurn},
      {67., TurnDirection::TurnSharpRight},
      {140., TurnDirection::TurnRight},
      {195., TurnDirection::TurnSlightRight},
      {205., TurnDirection::GoStraight},
      {240., TurnDirection::TurnSlightLeft},
      {336., TurnDirection::TurnLeft},
      {360., TurnDirection::NoTurn}};

  return FindDirectionByAngle(kUpperBounds, angle);
}

TurnDirection MostLeftDirection(const double angle)
{
  return InvertDirection(MostRightDirection(360. - angle));
}

TurnDirection IntermediateDirection(const double angle)
{
  static vector<pair<double, TurnDirection>> const kUpperBounds = {
      {23., TurnDirection::NoTurn},
      {67., TurnDirection::TurnSharpRight},
      {130., TurnDirection::TurnRight},
      {170., TurnDirection::TurnSlightRight},
      {190., TurnDirection::GoStraight},
      {230., TurnDirection::TurnSlightLeft},
      {292., TurnDirection::TurnLeft},
      {336., TurnDirection::TurnSharpLeft},
      {360., TurnDirection::NoTurn}};

  return FindDirectionByAngle(kUpperBounds, angle);
}
}  // namespace turns
}  // namespace routing
