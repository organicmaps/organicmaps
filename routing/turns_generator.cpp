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
}  // namespace

namespace routing
{
namespace turns
{
size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p)
{
  return p.first;
}

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
    ft1.ParseMetadata();

    if (ftypes::IsOneWayChecker::Instance()(ft1))
    {
      string const turnLanes = ft1.GetMetadata().Get(feature::FeatureMetadata::FMD_TURN_LANES);
      ParseLanes(turnLanes, lanes);
      return lanes;
    }
    // two way roads
    if (seg1.m_pointStart < seg1.m_pointEnd)
    {
      // forward direction
      string const turnLanesForward =
          ft1.GetMetadata().Get(feature::FeatureMetadata::FMD_TURN_LANES_FORWARD);
      ParseLanes(turnLanesForward, lanes);
      return lanes;
    }
    // backward direction
    string const turnLanesBackward =
        ft1.GetMetadata().Get(feature::FeatureMetadata::FMD_TURN_LANES_BACKWARD);
    ParseLanes(turnLanesBackward, lanes);
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

bool KeepMultiTurnClassHighwayClass(ftypes::HighwayClass ingoingClass, ftypes::HighwayClass outgoingClass,
                                    NodeID outgoingNode, TurnDirection turn,
                                    TurnCandidatesT const & possibleTurns,
                                    RoutingMapping const & routingMapping, Index const & index)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true; // The road significantly changes the direction here. Leaves this turn.

  if (possibleTurns.size() == 1 /* There's only one exit from this vertex. NodeID of it is outgoingNode */)
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::None;
  for (auto const & t : possibleTurns)
  {
    if (t.m_node == outgoingNode)
      continue;
    ftypes::HighwayClass const highwayClass = GetOutgoingHighwayClass(t.m_node, routingMapping, index);
    if (static_cast<int>(highwayClass) > static_cast<int>(maxClassForPossibleTurns))
      maxClassForPossibleTurns = highwayClass;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::None)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return true;
  }

  ftypes::HighwayClass const minClassForTheRoute =
      static_cast<ftypes::HighwayClass>(min(static_cast<int>(ingoingClass),
                                            static_cast<int>(outgoingClass)));
  if (minClassForTheRoute == ftypes::HighwayClass::None)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return false;
  }

  int const maxHighwayClassDiffToKeepTheTurn = 2;
  if (static_cast<int>(maxClassForPossibleTurns) - static_cast<int>(minClassForTheRoute) >=
      maxHighwayClassDiffToKeepTheTurn)
  {
    // The turn shall be removed if the route passes near small roads with out changing the direction.
    return false;
  }
  return true;
}

bool KeepOnewayOutgoingTurnRoundabout(bool isRound1, bool isRound2)
{
  return !isRound1 && isRound2;
}

TurnDirection RoundaboutDirection(bool isRound1, bool isRound2,
                                                     bool hasMultiTurns)
{
  if (isRound1 && isRound2)
  {
    if (hasMultiTurns)
      return TurnDirection::StayOnRoundAbout;
    else
      return TurnDirection::NoTurn;
  }

  if (!isRound1 && isRound2)
    return TurnDirection::EnterRoundAbout;

  if (isRound1 && !isRound2)
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
  double const lowerSharpRightBound = 23.;
  double const upperSharpRightBound = 67.;
  double const upperRightBound = 140.;
  double const upperSlightRight = 195.;
  double const upperGoStraitBound = 205.;
  double const upperSlightLeftBound = 240.;
  double const upperLeftBound = 336.;

  if (angle >= lowerSharpRightBound && angle < upperSharpRightBound)
    return TurnDirection::TurnSharpRight;
  else if (angle >= upperSharpRightBound && angle < upperRightBound)
    return TurnDirection::TurnRight;
  else if (angle >= upperRightBound && angle < upperSlightRight)
    return TurnDirection::TurnSlightRight;
  else if (angle >= upperSlightRight && angle < upperGoStraitBound)
    return TurnDirection::GoStraight;
  else if (angle >= upperGoStraitBound && angle < upperSlightLeftBound)
    return TurnDirection::TurnSlightLeft;
  else if (angle >= upperSlightLeftBound && angle < upperLeftBound)
    return TurnDirection::TurnLeft;
  return TurnDirection::NoTurn;
}

TurnDirection MostLeftDirection(const double angle)
{
  return InvertDirection(MostRightDirection(360. - angle));
}

TurnDirection IntermediateDirection(const double angle)
{
  double const lowerSharpRightBound = 23.;
  double const upperSharpRightBound = 67.;
  double const upperRightBound = 130.;
  double const upperSlightRight = 170.;
  double const upperGoStraitBound = 190.;
  double const upperSlightLeftBound = 230.;
  double const upperLeftBound = 292.;
  double const upperSharpLeftBound = 336.;

  if (angle >= lowerSharpRightBound && angle < upperSharpRightBound)
    return TurnDirection::TurnSharpRight;
  else if (angle >= upperSharpRightBound && angle < upperRightBound)
    return TurnDirection::TurnRight;
  else if (angle >= upperRightBound && angle < upperSlightRight)
    return TurnDirection::TurnSlightRight;
  else if (angle >= upperSlightRight && angle < upperGoStraitBound)
    return TurnDirection::GoStraight;
  else if (angle >= upperGoStraitBound && angle < upperSlightLeftBound)
    return TurnDirection::TurnSlightLeft;
  else if (angle >= upperSlightLeftBound && angle < upperLeftBound)
    return TurnDirection::TurnLeft;
  else if (angle >= upperLeftBound && angle < upperSharpLeftBound)
    return TurnDirection::TurnSharpLeft;
  return TurnDirection::NoTurn;
}
} // namespace turns
} // namespace routing
