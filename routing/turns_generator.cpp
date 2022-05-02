#include "routing/turns_generator.hpp"

#include "routing/router.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/angles.hpp"

#include "base/checked_cast.hpp"
#include "base/stl_helpers.hpp"

#include <cmath>
#include <sstream>
#include <numeric>

using namespace routing;
using namespace routing::turns;
using namespace std;
using namespace ftypes;

namespace
{
// Angles in degrees for finding route segments with no actual forks.
double constexpr kMaxForwardAngleCandidates = 95.0;
double constexpr kMaxForwardAngleActual = 60.0;
double constexpr kMinAngleDiffToNotConfuseStraightAndAlternative = 25.0;

bool IsHighway(HighwayClass hwClass, bool isLink)
{
  return (hwClass == HighwayClass::Trunk || hwClass == HighwayClass::Primary) &&
         !isLink;
}

bool IsSmallRoad(HighwayClass hwClass)
{
  return hwClass == HighwayClass::LivingStreet ||
         hwClass == HighwayClass::Service || hwClass == HighwayClass::Pedestrian;
}

/// \brief Return |CarDirection::ExitHighwayToRight| or |CarDirection::ExitHighwayToLeft|
/// or return |CarDirection::None|.
/// \note The function makes a decision about turn based on geometry of the route and turn
/// candidates, so it works correctly for both left and right hand traffic.
CarDirection TryToGetExitDirection(TurnCandidates const & possibleTurns, TurnInfo const & turnInfo,
            Segment const & firstOutgoingSeg, CarDirection const intermediateDirection)
{
  if (!possibleTurns.isCandidatesAngleValid)
    return CarDirection::None;

  if (!IsHighway(turnInfo.m_ingoing->m_highwayClass, turnInfo.m_ingoing->m_isLink) ||
      !(turnInfo.m_outgoing->m_isLink || (IsSmallRoad(turnInfo.m_outgoing->m_highwayClass) &&
                                            IsGoStraightOrSlightTurn(intermediateDirection))))
  {
    return CarDirection::None;
  }
  // At this point it is known that the route goes form a highway to a link road or to a small road
  // which has a slight angle with the highway.

  // Considering cases when the route goes from a highway to a link or a small road.
  // Checking all turn candidates (sorted by their angles) and looking for the road which is a
  // continuation of the ingoing segment. If the continuation is on the right hand of the route
  // it's an exit to the left. If the continuation is on the left hand of the route
  // it's an exit to the right.
  // Note. The angle which is used for sorting turn candidates in |possibleTurns.candidates|
  // is a counterclockwise angle between the ingoing route edge and corresponding candidate.
  // For left turns the angle is less than zero and for right ones it is more than zero.
  bool isCandidateBeforeOutgoing = true;
  bool isHighwayCandidateBeforeOutgoing = true;
  size_t highwayCandidateNumber = 0;

  for (auto const & c : possibleTurns.candidates)
  {
    if (c.m_segment == firstOutgoingSeg)
    {
      isCandidateBeforeOutgoing = false;
      continue;
    }

    if (IsHighway(c.m_highwayClass, c.m_isLink))
    {
      ++highwayCandidateNumber;
      if (highwayCandidateNumber >= 2)
        return CarDirection::None; // There are two or more highway candidates from the junction.
      isHighwayCandidateBeforeOutgoing = isCandidateBeforeOutgoing;
    }
  }
  if (highwayCandidateNumber == 1)
  {
    return isHighwayCandidateBeforeOutgoing ? CarDirection::ExitHighwayToRight
                                            : CarDirection::ExitHighwayToLeft;
  }
  return CarDirection::None;
}

int CalcDiffRoadClasses(HighwayClass const left, HighwayClass const right)
{
  return static_cast<int>(left) - static_cast<int>(right);
}

/// * \returns true when
/// * - the route leads from one big road to another one;
/// * - and the other possible turns lead to small roads;
/// * Returns false otherwise.
/// \param turnCandidates is all possible ways out from a junction.
/// \param turnInfo is information about ingoing and outgoing segments of the route.
bool CanDiscardTurnByHighwayClass(std::vector<TurnCandidate> const & turnCandidates, 
                                  TurnInfo const & turnInfo,
                                  NumMwmIds const & numMwmIds)
{
  // Maximum difference between HighwayClasses of the route segments and
  // the possible way segments to keep the junction as a turn.
  int constexpr kMinHighwayClassDiff = -2;
  int constexpr kMinHighwayClassDiffForService = -1;

  HighwayClass outgoingRouteRoadClass = turnInfo.m_outgoing->m_highwayClass;
  HighwayClass ingoingRouteRoadClass = turnInfo.m_ingoing->m_highwayClass;

  HighwayClass maxRouteRoadClass = static_cast<HighwayClass>(max(static_cast<int>(ingoingRouteRoadClass), static_cast<int>(outgoingRouteRoadClass)));

  // The turn should be kept if there's no any information about feature id of outgoing segment
  // just to be on the safe side. It may happen in case of outgoing segment is a finish segment.
  Segment firstOutgoingSegment;
  if (!turnInfo.m_outgoing->m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment))
    return false;

  for (auto const & t : turnCandidates)
  {
    // Let's consider all outgoing segments except for route outgoing segment.
    if (t.m_segment == firstOutgoingSegment)
      continue;

    HighwayClass candidateRoadClass = t.m_highwayClass;

    // If route's road is significantly larger than candidate's road, the candidate can be ignored.
    if (CalcDiffRoadClasses(maxRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiff)
      continue;

    // If route's road is significantly larger than candidate's service road, the candidate can be ignored.
    if (CalcDiffRoadClasses(maxRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiffForService && candidateRoadClass == HighwayClass::Service)
      continue;
      
    return false;
  }

  return true;
}

/// * \returns false when
/// * for routes which go straight and don't go to a smaller road:
/// * - any alternative GoStraight or SlightTurn turn
/// * for other routes:
/// * - any alternative turn to a bigger road
/// * - or any alternative turn to a similar road if the turn's angle is less than kMaxAbsAngleSameRoadClass degrees (wider than SlightTurn)
/// * - or any alternative turn to a smaller road if it's GoStraight or SlightTurn.
/// * Returns true otherwise.
/// \param routeDirection is route direction
/// \param routeAngle is route angle
/// \param turnCandidates is all possible ways out from a junction except uturn.
/// \param turnInfo is information about ingoing and outgoing segments of the route.
bool CanDiscardTurnByHighwayClassOrAngles(CarDirection const routeDirection,
                                          double routeAngle,
                                          std::vector<TurnCandidate> const & turnCandidates, 
                                          TurnInfo const & turnInfo,
                                          NumMwmIds const & numMwmIds)
{
  if (!IsGoStraightOrSlightTurn(routeDirection))
  {
    ASSERT(false, ("It is supposed that current turn is GoStraight or SlightTurn"));
    return false;
  }

  // Minimum difference between alternative cadidate's angle and the route to ignore this candidate.
  double constexpr kMinAbsAngleDiffSameRoadClass = 35.0;
  // Maximum difference between HighwayClasses of the route segments and
  // the possible way segments to keep the junction as a turn.
  int constexpr kMinHighwayClassDiff = -2;
  int constexpr kMinHighwayClassDiffForGoStraight = -1;
  int constexpr kMinHighwayClassDiffForService = -1;

  HighwayClass outgoingRouteRoadClass = turnInfo.m_outgoing->m_highwayClass;
  HighwayClass ingoingRouteRoadClass = turnInfo.m_ingoing->m_highwayClass;
//  HighwayClass maxRouteRoadClass = static_cast<HighwayClass>(max(static_cast<int>(ingoingRouteRoadClass), static_cast<int>(outgoingRouteRoadClass)));

  // The turn should be kept if there's no any information about feature id of outgoing segment
  // just to be on the safe side. It may happen in case of outgoing segment is a finish segment.
  Segment firstOutgoingSegment;
  if (!turnInfo.m_outgoing->m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment))
    return false;

  for (auto const & t : turnCandidates)
  {
    // Let's consider all outgoing segments except for route outgoing segment.
    if (t.m_segment == firstOutgoingSegment)
      continue;

    HighwayClass candidateRoadClass = t.m_highwayClass;

    // If outgoing route road is significantly larger than candidate, the candidate can be ignored.
    if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiff)
      continue;

    // If igngoing and outgoing edges are not links and candidate-link can be ignored.
    /// @todo Maybe add condition of links m_highwayClass to be higher.
    if (!turnInfo.m_ingoing->m_isLink && !turnInfo.m_outgoing->m_isLink &&
        turnInfo.m_ingoing->m_highwayClass == turnInfo.m_outgoing->m_highwayClass &&
        t.m_isLink)
      continue;
    
    if (routeDirection == CarDirection::GoStraight)
    {
      // If outgoing route road is significantly larger than candidate, the candidate can be ignored.
      if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiffForGoStraight)
        continue;

      // If outgoing route road is the same or large than ingoing
      // and candidate's angle is sharp enough compared to the route it can be ignored.
      if (CalcDiffRoadClasses(outgoingRouteRoadClass, ingoingRouteRoadClass) <= 0 &&
          abs(t.m_angle) > abs(routeAngle) + kMinAngleDiffToNotConfuseStraightAndAlternative)
        continue;
    }

    // If outgoing route road is smaller than candidate candidate turn cant be discarded.
    if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) > 0)
    {
      return false;
    }
    else if (outgoingRouteRoadClass == candidateRoadClass)
    {
      // If alternative cadidate's road size is the same and it's angle is not enough sharp compared to the route turn can't be discarded.
      if (abs(t.m_angle) < abs(routeAngle) + kMinAbsAngleDiffSameRoadClass)
        return false;
    }
    else // Outgoing route road is larger than candidate.
    {
      // If outgoing route's road is significantly larger than candidate's service road, the candidate can be ignored.
      if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiffForService && candidateRoadClass == HighwayClass::Service)
        continue;

      // Any alternative turn to a smaller road keeps the turn direction if it's GoStraight or SlightTurn.
      if (IsGoStraightOrSlightTurn(IntermediateDirection(t.m_angle)))
        return false;
    }
  }
  return true;
}

/*!
 * \brief Returns false when other possible turns leads to service roads;
 */
bool KeepRoundaboutTurnByHighwayClass(TurnCandidates const & possibleTurns,
                                      TurnInfo const & turnInfo, NumMwmIds const & numMwmIds)
{
  Segment firstOutgoingSegment;
  bool const validFirstOutgoingSeg = turnInfo.m_outgoing->m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment);

  for (auto const & t : possibleTurns.candidates)
  {
    if (!validFirstOutgoingSeg || t.m_segment == firstOutgoingSegment)
      continue;
    if (static_cast<int>(t.m_highwayClass) != static_cast<int>(HighwayClass::Service))
      return true;
  }
  return false;
}

bool FixupLaneSet(CarDirection turn, vector<SingleLaneInfo> & lanes,
                  function<bool(LaneWay l, CarDirection t)> checker)
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
template <class T>
T FindDirectionByAngle(vector<pair<double, T>> const & lowerBounds, double angle)
{
  ASSERT_GREATER_OR_EQUAL(angle, -180., (angle));
  ASSERT_LESS_OR_EQUAL(angle, 180., (angle));
  ASSERT(!lowerBounds.empty(), ());
  ASSERT(is_sorted(lowerBounds.cbegin(), lowerBounds.cend(),
                   [](pair<double, T> const & p1, pair<double, T> const & p2) {
                     return p1.first > p2.first;
                   }),
         ());

  for (auto const & lower : lowerBounds)
  {
    if (angle >= lower.first)
      return lower.second;
  }

  ASSERT(false, ("The angle is not covered by the table. angle = ", angle));
  return T::None;
}

RoutePointIndex GetFirstOutgoingPointIndex(size_t outgoingSegmentIndex)
{
  return RoutePointIndex({outgoingSegmentIndex, 0 /* m_pathIndex */});
}

RoutePointIndex GetLastIngoingPointIndex(TUnpackedPathSegments const & segments,
                                         size_t outgoingSegmentIndex)
{
  ASSERT_GREATER(outgoingSegmentIndex, 0, ());
  ASSERT(segments[outgoingSegmentIndex - 1].IsValid(), ());
  return RoutePointIndex({outgoingSegmentIndex - 1,
                          segments[outgoingSegmentIndex - 1].m_path.size() - 1 /* m_pathIndex */});
}

m2::PointD GetPointByIndex(TUnpackedPathSegments const & segments, RoutePointIndex const & index)
{
  return segments[index.m_segmentIndex].m_path[index.m_pathIndex].GetPoint();
}

double CalcEstimatedTimeToPass(double distance, HighwayClass highwayClass)
{
  double speed = 0;
  switch (highwayClass)
  {
    case HighwayClass::Trunk:         speed = 100000.0/60/60; break;
    case HighwayClass::Primary:       speed = 70000.0/60/60; break;
    case HighwayClass::Secondary:     speed = 70000.0/60/60; break;
    case HighwayClass::Tertiary:      speed = 50000.0/60/60; break;
    case HighwayClass::LivingStreet:  speed = 20000.0/60/60; break;
    case HighwayClass::Service:       speed = 10000.0/60/60; break;
    case HighwayClass::Pedestrian:    speed = 50000.0/60/60; break;
    default:                                  speed = 500000.0/60/60; break;
  }
  return distance / speed;
}

/*!
 * \brief Returns ingoing point or outgoing point for turns.
 * These points belong to the route but they often are not neighbor of junction point.
 * To calculate the resulting point the function implements the following steps:
 * - going from junction point along route path according to the direction which is set in GetPointIndex().
 * - until one of following conditions is fulfilled:
 *   - more than |maxPointsCount| points are passed (returns the maxPointsCount-th point);
 *   - the length of passed parts of segment exceeds maxDistMeters;
 *     (returns the next point after the event)
 *   - an important bifurcation point is reached in case of outgoing point is looked up (forward == true).
 * \param result information about the route. |result.GetSegments()| is composed of LoadedPathSegment.
 * Each LoadedPathSegment is composed of several Segments. The sequence of Segments belongs to
 * single feature and does not split by other features.
 * \param outgoingSegmentIndex index in |segments|. Junction point noticed above is the first point
 * of |outgoingSegmentIndex| segment in |result.GetSegments()|.
 * \param maxPointsCount maximum number between the returned point and junction point.
 * \param maxDistMeters maximum distance between the returned point and junction point.
 * \param forward is direction of moving along the route to calculate the returned point.
 * If forward == true the direction is to route finish. If forward == false the direction is to route start.
 * \return an ingoing or outgoing point for a turn calculation.
 */
m2::PointD GetPointForTurn(IRoutingResult const & result, size_t outgoingSegmentIndex,
                           NumMwmIds const & numMwmIds, size_t const maxPointsCount,
                           double const maxDistMeters, bool const forward)
{
  auto const & segments = result.GetSegments();
  ASSERT_LESS(outgoingSegmentIndex, segments.size(), ());
  ASSERT_GREATER(outgoingSegmentIndex, 0, ());

  RoutePointIndex index = forward ? GetFirstOutgoingPointIndex(outgoingSegmentIndex)
                                  : GetLastIngoingPointIndex(segments, outgoingSegmentIndex);

  ASSERT_LESS(index.m_pathIndex, segments[index.m_segmentIndex].m_path.size(), ());
  ASSERT_LESS(index.m_segmentIndex, segments.size(), ());

  RoutePointIndex nextIndex;

  ASSERT(!segments[index.m_segmentIndex].m_path.empty(), ());

  m2::PointD point = GetPointByIndex(segments, index);
  m2::PointD nextPoint;
  size_t count = 0;
  double curDistanceMeters = 0.0;
  double curTime = 0.0;

  // There is no need to go too far for low-speed roads.
  // So additional time limit is applied.
  double maxTime = 3.0;

  ASSERT(GetNextRoutePointIndex(result, index, numMwmIds, forward, false, nextIndex), ());

  do
  {
    nextPoint = GetPointByIndex(segments, nextIndex);

    // At start and finish there are two edges with zero length.
    // This function should not be called for the start (|outgoingSegmentIndex| == 0).
    // So there is special processing for the finish below.
    if (point == nextPoint && outgoingSegmentIndex + 1 == segments.size())
      return nextPoint;

    double distanceMeters = mercator::DistanceOnEarth(point, nextPoint);
    curDistanceMeters += distanceMeters;
    curTime += CalcEstimatedTimeToPass(distanceMeters, segments[nextIndex.m_segmentIndex].m_highwayClass);

    if (curTime > maxTime || ++count >= maxPointsCount || curDistanceMeters > maxDistMeters)
      return nextPoint;

    point = nextPoint;
    index = nextIndex;
  }
  while (GetNextRoutePointIndex(result, index, numMwmIds, forward, true, nextIndex));

  return nextPoint;
}

double CalcOneSegmentTurnAngle(TurnInfo const & turnInfo)
{
  ASSERT_GREATER_OR_EQUAL(turnInfo.m_ingoing->m_path.size(), 2, ());
  ASSERT_GREATER_OR_EQUAL(turnInfo.m_outgoing->m_path.size(), 2, ());

  return base::RadToDeg(PiMinusTwoVectorsAngle(turnInfo.m_ingoing->m_path.back().GetPoint(),
                                               turnInfo.m_ingoing->m_path[turnInfo.m_ingoing->m_path.size() - 2].GetPoint(),
                                               turnInfo.m_outgoing->m_path[1].GetPoint()));
}

double CalcPathTurnAngle(LoadedPathSegment const & segment, size_t const pathIndex)
{
  ASSERT_GREATER_OR_EQUAL(segment.m_path.size(), 3, ());
  ASSERT_GREATER(pathIndex, 0, ());
  ASSERT_LESS(pathIndex, segment.m_path.size() - 1, ());

  return base::RadToDeg(PiMinusTwoVectorsAngle(segment.m_path[pathIndex].GetPoint(),
                                               segment.m_path[pathIndex - 1].GetPoint(),
                                               segment.m_path[pathIndex + 1].GetPoint()));
}

/*!
 * \brief Calculates |nextIndex| which is an index of next route point at result.GetSegments()
 * in forward direction.
 * If
 *  - |index| points at the last point of the turn segment:
 *  - and the route at this point leads from one big road to another one
 *  - and the other possible turns lead to small roads or there's no them
 *  - and the turn is GoStraight or TurnSlight*
 *  method returns the second point of the next segment. First point of the next segment is skipped
 *  because it's almost the same with the last point of this segment.
 *  if |index| points at the first or intermediate point in turn segment returns the next one.
 * \returns true if |nextIndex| fills correctly and false otherwise.
 */
bool GetNextCrossSegmentRoutePoint(IRoutingResult const & result, RoutePointIndex const & index, bool const smoothOnly,
                                   NumMwmIds const & numMwmIds, RoutePointIndex & nextIndex)
{
  auto const & segments = result.GetSegments();
  ASSERT_LESS(index.m_segmentIndex, segments.size(), ());
  ASSERT_LESS(index.m_pathIndex, segments[index.m_segmentIndex].m_path.size(), ());

  if (index.m_pathIndex + 1 != segments[index.m_segmentIndex].m_path.size())
  {
    // In segment case.
    nextIndex = {index.m_segmentIndex, index.m_pathIndex + 1};
    return true;
  }

  // Case when the last point of the current segment is reached.
  // So probably it's necessary to cross a segment border.
  if (index.m_segmentIndex + 1 == segments.size())
    return false; // The end of the route is reached.

  TurnInfo const turnInfo(&segments[index.m_segmentIndex], &segments[index.m_segmentIndex + 1]);

  double const oneSegmentTurnAngle = CalcOneSegmentTurnAngle(turnInfo);
  CarDirection const oneSegmentDirection = IntermediateDirection(oneSegmentTurnAngle);
  if (smoothOnly && !IsGoStraightOrSlightTurn(oneSegmentDirection))
    return false; // Too sharp turn angle.

  size_t ingoingCount = 0;
  TurnCandidates possibleTurns;
  result.GetPossibleTurns(turnInfo.m_ingoing->m_segmentRange, GetPointByIndex(segments, index),
                          ingoingCount, possibleTurns);

  if (possibleTurns.candidates.empty())
    return false;

  // |segments| is a vector of |LoadedPathSegment|. Every |LoadedPathSegment::m_path|
  // contains junctions of the segment. The first junction at a |LoadedPathSegment::m_path|
  // is the same (or almost the same) with the last junction at the next |LoadedPathSegment::m_path|.
  // To prevent using the same point twice it's necessary to take the first point only from the
  // first item of |loadedSegments|. The beginning should be ignored for the rest |m_path|.
  // Please see a comment in MakeTurnAnnotation() for more details.
  if (possibleTurns.candidates.size() == 1)
  {
    // Taking the next point of the next segment.
    nextIndex = {index.m_segmentIndex + 1, 1 /* m_pathIndex */};
    return true;
  }

  if (CanDiscardTurnByHighwayClass(possibleTurns.candidates, turnInfo, numMwmIds))
  {
    // Taking the next point of the next segment.
    nextIndex = {index.m_segmentIndex + 1, 1 /* m_pathIndex */};
    return true;
  }
  // Stopping getting next route points because an important bifurcation point is reached.
  return false;
}

bool GetPrevInSegmentRoutePoint(IRoutingResult const & result, RoutePointIndex const & index, bool const smoothOnly, RoutePointIndex & nextIndex)
{
  if (index.m_pathIndex == 0)
    return false;

  auto const & segments = result.GetSegments();
  if (smoothOnly && segments[index.m_segmentIndex].m_path.size() >= 3 && index.m_pathIndex < segments[index.m_segmentIndex].m_path.size() - 1)
  {
    double const oneSegmentTurnAngle = CalcPathTurnAngle(segments[index.m_segmentIndex], index.m_pathIndex);
    CarDirection const oneSegmentDirection = IntermediateDirection(oneSegmentTurnAngle);
    if (!IsGoStraightOrSlightTurn(oneSegmentDirection))
      return false; // Too sharp turn angle.
  }

  nextIndex = {index.m_segmentIndex, index.m_pathIndex - 1};
  return true;
}

/*!
 * \brief Corrects |turn.m_turn| if |turn.m_turn == CarDirection::GoStraight| and there're only
 * two ways out from this junction. In that case the other way (the way which is not covered by
 * the route) is checked. If the other way is "go straight" or "slight turn", turn.m_turn is set
 * to |turnToSet|.
 */
void CorrectGoStraight(TurnCandidate const & notRouteCandidate, double const routeAngle, CarDirection const & turnToSet,
                          TurnItem & turn)
{
  if (turn.m_turn != CarDirection::GoStraight)
    return;
  
  // No need to coorect turn if alternative cadidate's angle is sharp enough compared to the route.
  if (abs(notRouteCandidate.m_angle) > abs(routeAngle) + kMinAngleDiffToNotConfuseStraightAndAlternative)
    return;

  if (!IsGoStraightOrSlightTurn(IntermediateDirection(notRouteCandidate.m_angle)))
    return;

  turn.m_turn = turnToSet;
}

// Returns distance in meters between |junctions[start]| and |junctions[end]|.
double CalcRouteDistanceM(vector<geometry::PointWithAltitude> const & junctions, uint32_t start,
                          uint32_t end)
{
  double res = 0.0;

  for (uint32_t i = start + 1; i < end; ++i)
    res += mercator::DistanceOnEarth(junctions[i - 1].GetPoint(), junctions[i].GetPoint());

  return res;
}
}  // namespace

namespace routing
{
namespace turns
{
// RoutePointIndex ---------------------------------------------------------------------------------
bool RoutePointIndex::operator==(RoutePointIndex const & index) const
{
  return m_segmentIndex == index.m_segmentIndex && m_pathIndex == index.m_pathIndex;
}

// TurnInfo ----------------------------------------------------------------------------------------
bool TurnInfo::IsSegmentsValid() const
{
  if (m_ingoing->m_path.empty() || m_outgoing->m_path.empty())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

bool GetNextRoutePointIndex(IRoutingResult const & result, RoutePointIndex const & index,
                            NumMwmIds const & numMwmIds, bool const forward, bool const smoothOnly,
                            RoutePointIndex & nextIndex)
{
  if (forward)
  {
    if (!GetNextCrossSegmentRoutePoint(result, index, smoothOnly, numMwmIds, nextIndex))
      return false;
  }
  else
  {
    if (!GetPrevInSegmentRoutePoint(result, index, smoothOnly, nextIndex))
      return false;
  }

  ASSERT_LESS(nextIndex.m_segmentIndex, result.GetSegments().size(), ());
  ASSERT_LESS(nextIndex.m_pathIndex, result.GetSegments()[nextIndex.m_segmentIndex].m_path.size(), ());
  return true;
}

RouterResultCode MakeTurnAnnotation(IRoutingResult const & result, NumMwmIds const & numMwmIds,
                                    VehicleType const & vehicleType,
                                    base::Cancellable const & cancellable,
                                    vector<geometry::PointWithAltitude> & junctions,
                                    Route::TTurns & turnsDir, Route::TStreets & streets,
                                    vector<Segment> & segments)
{
  LOG(LDEBUG, ("Shortest path length:", result.GetPathLength()));

  if (cancellable.IsCancelled())
    return RouterResultCode::Cancelled;
  // Annotate turns.
  size_t skipTurnSegments = 0;
  auto const & loadedSegments = result.GetSegments();
  segments.reserve(loadedSegments.size());

  RoutingSettings const vehicleSettings = GetRoutingSettings(vehicleType);

  for (auto loadedSegmentIt = loadedSegments.cbegin(); loadedSegmentIt != loadedSegments.cend();
       ++loadedSegmentIt)
  {
    CHECK(loadedSegmentIt->IsValid(), ());

    // Street names. I put empty names too, to avoid freezing old street name while riding on
    // unnamed street.
    streets.emplace_back(max(junctions.size(), static_cast<size_t>(1)) - 1, loadedSegmentIt->m_name);

    // Turns information.
    if (!junctions.empty() && skipTurnSegments == 0)
    {
      TurnItem turnItem;
      turnItem.m_index = static_cast<uint32_t>(junctions.size() - 1);

      auto const outgoingSegmentDist = distance(loadedSegments.begin(), loadedSegmentIt);
      CHECK_GREATER(outgoingSegmentDist, 0, ());
      auto const outgoingSegmentIndex = static_cast<size_t>(outgoingSegmentDist);

      skipTurnSegments = CheckUTurnOnRoute(result, outgoingSegmentIndex, numMwmIds, vehicleSettings, turnItem);

      if (turnItem.m_turn == CarDirection::None)
        GetTurnDirection(result, outgoingSegmentIndex, numMwmIds, vehicleSettings, turnItem);

      // Lane information.
      if (turnItem.m_turn != CarDirection::None)
      {
        auto const & ingoingSegment = loadedSegments[outgoingSegmentIndex - 1];
        turnItem.m_lanes = ingoingSegment.m_lanes;
        turnsDir.push_back(move(turnItem));
      }
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

  turnsDir.emplace_back(TurnItem(base::asserted_cast<uint32_t>(junctions.size()) - 1,
      CarDirection::ReachedYourDestination));
  FixupTurns(junctions, turnsDir);

#ifdef DEBUG
  for (auto const & t : turnsDir)
  {
    LOG(LDEBUG, (GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-",
                 t.m_targetName, "exit:", t.m_exitNum));
  }
#endif
  return RouterResultCode::NoError;
}

RouterResultCode MakeTurnAnnotationPedestrian(
    IRoutingResult const & result, NumMwmIds const & numMwmIds, VehicleType const & vehicleType,
    base::Cancellable const & cancellable, vector<geometry::PointWithAltitude> & junctions,
    Route::TTurns & turnsDir, Route::TStreets & streets, vector<Segment> & segments)
{
  LOG(LDEBUG, ("Shortest path length:", result.GetPathLength()));

  if (cancellable.IsCancelled())
    return RouterResultCode::Cancelled;

  size_t skipTurnSegments = 0;
  auto const & loadedSegments = result.GetSegments();
  segments.reserve(loadedSegments.size());

  RoutingSettings const vehicleSettings = GetRoutingSettings(vehicleType);

  for (auto loadedSegmentIt = loadedSegments.cbegin(); loadedSegmentIt != loadedSegments.cend();
       ++loadedSegmentIt)
  {
    CHECK(loadedSegmentIt->IsValid(), ());

    // Street names contain empty names too for avoiding of freezing of old street name while
    // moving along unnamed street.
    streets.emplace_back(max(junctions.size(), static_cast<size_t>(1)) - 1,
                         loadedSegmentIt->m_name);

    // Turns information.
    if (!junctions.empty() && skipTurnSegments == 0)
    {
      auto const outgoingSegmentDist = distance(loadedSegments.begin(), loadedSegmentIt);
      CHECK_GREATER(outgoingSegmentDist, 0, ());

      auto const outgoingSegmentIndex = static_cast<size_t>(outgoingSegmentDist);

      TurnItem turnItem;
      turnItem.m_index = static_cast<uint32_t>(junctions.size() - 1);
      GetTurnDirectionPedestrian(result, outgoingSegmentIndex, numMwmIds, vehicleSettings,
                                 turnItem);

      if (turnItem.m_pedestrianTurn != PedestrianDirection::None)
        turnsDir.push_back(move(turnItem));
    }

    if (skipTurnSegments > 0)
      --skipTurnSegments;

    CHECK_GREATER_OR_EQUAL(loadedSegmentIt->m_path.size(), 2, ());

    junctions.insert(junctions.end(),
                     loadedSegmentIt == loadedSegments.cbegin()
                         ? loadedSegmentIt->m_path.cbegin()
                         : loadedSegmentIt->m_path.cbegin() + 1,
                     loadedSegmentIt->m_path.cend());

    segments.insert(segments.end(), loadedSegmentIt->m_segments.cbegin(),
                    loadedSegmentIt->m_segments.cend());
  }

  if (junctions.size() == 1)
    junctions.push_back(junctions.front());

  if (junctions.size() < 2)
    return RouterResultCode::RouteNotFound;

  junctions.front() = result.GetStartPoint();
  junctions.back() = result.GetEndPoint();

  turnsDir.emplace_back(TurnItem(base::asserted_cast<uint32_t>(junctions.size()) - 1,
                                 PedestrianDirection::ReachedYourDestination));

  FixupTurnsPedestrian(junctions, turnsDir);

#ifdef DEBUG
  for (auto t : turnsDir)
    LOG(LDEBUG, (t.m_pedestrianTurn, ":", t.m_index, t.m_sourceName, "-", t.m_targetName));
#endif
  return RouterResultCode::NoError;
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

void FixupTurns(vector<geometry::PointWithAltitude> const & junctions, Route::TTurns & turnsDir)
{
  double const kMergeDistMeters = 15.0;
  // For turns that are not EnterRoundAbout exitNum is always equal to zero.
  // If a turn is EnterRoundAbout exitNum is a number of turns between two junctions:
  // (1) the route enters to the roundabout;
  // (2) the route leaves the roundabout;
  uint32_t exitNum = 0;
  // If a roundabout is worked up the roundabout value junctions to the turn
  // of the enter to the roundabout. If not, roundabout is equal to nullptr.
  TurnItem * roundabout = nullptr;

  for (size_t idx = 0; idx < turnsDir.size(); )
  {
    TurnItem & t = turnsDir[idx];
    if (roundabout && t.m_turn != CarDirection::StayOnRoundAbout &&
        t.m_turn != CarDirection::LeaveRoundAbout)
    {
      exitNum = 0;
      roundabout = nullptr;
    }
    else if (t.m_turn == CarDirection::EnterRoundAbout)
    {
      ASSERT(!roundabout, ());
      roundabout = &t;
    }
    else if (t.m_turn == CarDirection::StayOnRoundAbout)
    {
      ++exitNum;
      turnsDir.erase(turnsDir.begin() + idx);
      continue;
    }
    else if (roundabout && t.m_turn == CarDirection::LeaveRoundAbout)
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
        CalcRouteDistanceM(junctions, turnsDir[idx - 1].m_index, turnsDir[idx].m_index) <
            kMergeDistMeters)
    {
      turnsDir.erase(turnsDir.begin() + idx - 1);
      continue;
    }

    ++idx;
  }
  SelectRecommendedLanes(turnsDir);
}

void FixupTurnsPedestrian(vector<geometry::PointWithAltitude> const & junctions,
                          Route::TTurns & turnsDir)
{
  double const kMergeDistMeters = 15.0;

  for (size_t idx = 0; idx < turnsDir.size();)
  {
    bool const prevStepNoTurn =
        idx > 0 && turnsDir[idx - 1].m_pedestrianTurn == PedestrianDirection::GoStraight;
    bool const needToTurn = turnsDir[idx].m_pedestrianTurn == PedestrianDirection::TurnLeft ||
                            turnsDir[idx].m_pedestrianTurn == PedestrianDirection::TurnRight;

    // Merging turns which are closer to each other under some circumstance.
    if (prevStepNoTurn && needToTurn &&
        CalcRouteDistanceM(junctions, turnsDir[idx - 1].m_index, turnsDir[idx].m_index) <
            kMergeDistMeters)
    {
      turnsDir.erase(turnsDir.begin() + idx - 1);
      continue;
    }

    ++idx;
  }
}

void SelectRecommendedLanes(Route::TTurns & turnsDir)
{
  for (auto & t : turnsDir)
  {
    vector<SingleLaneInfo> & lanes = t.m_lanes;
    if (lanes.empty())
      continue;
    CarDirection const turn = t.m_turn;
    // Checking if threre are elements in lanes which correspond with the turn exactly.
    // If so fixing up all the elements in lanes which correspond with the turn.
    if (FixupLaneSet(turn, lanes, &IsLaneWayConformedTurnDirection))
      continue;
    // If not checking if there are elements in lanes which corresponds with the turn
    // approximately. If so fixing up all these elements.
    FixupLaneSet(turn, lanes, &IsLaneWayConformedTurnDirectionApproximately);
  }
}

CarDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isMultiTurnJunction, bool keepTurnByHighwayClass)
{
  if (isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
  {
    if (isMultiTurnJunction)
      return keepTurnByHighwayClass ? CarDirection::StayOnRoundAbout : CarDirection::None;
    return CarDirection::None;
  }

  if (!isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout)
    return CarDirection::EnterRoundAbout;

  if (isIngoingEdgeRoundabout && !isOutgoingEdgeRoundabout)
    return CarDirection::LeaveRoundAbout;

  ASSERT(false, ());
  return CarDirection::None;
}

CarDirection GetRoundaboutDirection(TurnInfo const turnInfo, TurnCandidates const & nodes, NumMwmIds const & numMwmIds)
{  
  bool const keepTurnByHighwayClass = KeepRoundaboutTurnByHighwayClass(nodes, turnInfo, numMwmIds);
  return GetRoundaboutDirection(turnInfo.m_ingoing->m_onRoundabout, turnInfo.m_outgoing->m_onRoundabout, 
                                nodes.candidates.size() > 1, keepTurnByHighwayClass);
}

CarDirection InvertDirection(CarDirection dir)
{
  switch (dir)
  {
    case CarDirection::TurnSlightRight:
      return CarDirection::TurnSlightLeft;
    case CarDirection::TurnRight:
      return CarDirection::TurnLeft;
    case CarDirection::TurnSharpRight:
      return CarDirection::TurnSharpLeft;
    case CarDirection::TurnSlightLeft:
      return CarDirection::TurnSlightRight;
    case CarDirection::TurnLeft:
      return CarDirection::TurnRight;
    case CarDirection::TurnSharpLeft:
      return CarDirection::TurnSharpRight;
    default:
      return dir;
  };
}

CarDirection RightmostDirection(const double angle)
{
  static vector<pair<double, CarDirection>> const kLowerBounds = {
      {145., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {10., CarDirection::TurnSlightRight},
      // For sure it's incorrect to give directions TurnLeft or TurnSlighLeft if we need the rightmost turn.
      // So GoStraight direction is given even for needed sharp left turn when other turns on the left and are more sharp.
      // The reason: the rightmost turn is the most straight one here.
      {-180., CarDirection::GoStraight}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

CarDirection LeftmostDirection(const double angle)
{
  return InvertDirection(RightmostDirection(-angle));
}

CarDirection IntermediateDirection(const double angle)
{
  static vector<pair<double, CarDirection>> const kLowerBounds = {
      {145., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {10., CarDirection::TurnSlightRight},
      {-10., CarDirection::GoStraight},
      {-50., CarDirection::TurnSlightLeft},
      {-145., CarDirection::TurnLeft},
      {-180., CarDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

PedestrianDirection IntermediateDirectionPedestrian(const double angle)
{
  static vector<pair<double, PedestrianDirection>> const kLowerBounds = {
      {10.0, PedestrianDirection::TurnRight},
      {-10.0, PedestrianDirection::GoStraight},
      {-180.0, PedestrianDirection::TurnLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

void RemoveUTurnCandidate(TurnInfo const & turnInfo, NumMwmIds const & numMwmIds, std::vector<TurnCandidate> & turnCandidates)
{
  Segment lastIngoingSegment;
  if (turnInfo.m_ingoing->m_segmentRange.GetLastSegment(numMwmIds, lastIngoingSegment))
  {
    if (turnCandidates.front().m_segment.IsInverse(lastIngoingSegment))
      turnCandidates.erase(turnCandidates.begin());
    else if (turnCandidates.back().m_segment.IsInverse(lastIngoingSegment))
      turnCandidates.pop_back();
  }
}

/// \returns true if there is exactly 1 turn in |turnCandidates| with angle less then
/// |kMaxForwardAngleCandidates|.
bool HasSingleForwardTurn(TurnCandidates const & turnCandidates)
{
  bool foundForwardTurn = false;

  for (auto const & turn : turnCandidates.candidates)
  {
    if (std::fabs(turn.m_angle) < kMaxForwardAngleCandidates)
    {
      if (foundForwardTurn)
        return false;

      foundForwardTurn = true;
    }
  }

  return foundForwardTurn;
}

/// \returns true if |path| is loop connected to the PartOfReal segments.
bool PathIsFakeLoop(std::vector<geometry::PointWithAltitude> const & path)
{
  return path.size() == 2 && path[0] == path[1];
}

double CalcTurnAngle(IRoutingResult const & result, 
                     size_t const outgoingSegmentIndex,
                     m2::PointD const & junctionPoint, 
                     NumMwmIds const & numMwmIds,
                     RoutingSettings const & vehicleSettings)
{
  bool const isStartFakeLoop = PathIsFakeLoop(result.GetSegments()[outgoingSegmentIndex - 1].m_path);
  size_t const segmentIndexForIngoingPoint = isStartFakeLoop ? outgoingSegmentIndex - 1 : outgoingSegmentIndex;

  m2::PointD const ingoingPoint = GetPointForTurn(
      result, segmentIndexForIngoingPoint, numMwmIds, vehicleSettings.m_maxIngoingPointsCount,
      vehicleSettings.m_minIngoingDistMeters, false /* forward */);
  m2::PointD const outgoingPoint = GetPointForTurn(
      result, outgoingSegmentIndex, numMwmIds, vehicleSettings.m_maxOutgoingPointsCount,
      vehicleSettings.m_minOutgoingDistMeters, true /* forward */);

  double const turnAngle = base::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
  return turnAngle;
}

// It's possible that |firstOutgoingSeg| is not contained in |turnCandidates|.
// It may happened if |firstOutgoingSeg| and candidates in |turnCandidates| are
// from different mwms.
// Let's identify it turnCandidates by angle and update according turnCandidate.
void CorrectCandidatesSegmentByOutgoing(TurnInfo const & turnInfo,
                                        Segment const & firstOutgoingSeg,
                                        std::vector<TurnCandidate> & candidates)
{
  auto IsFirstOutgoingSeg = [&firstOutgoingSeg](TurnCandidate const & turnCandidate) { return turnCandidate.m_segment == firstOutgoingSeg; };
  if (find_if(candidates.begin(), candidates.end(), IsFirstOutgoingSeg) == candidates.end())
  {
    double const turnAngle = CalcOneSegmentTurnAngle(turnInfo);
    auto DoesAngleMatch = [&turnAngle](TurnCandidate const & turnCandidate) { return base::AlmostEqualAbs(turnCandidate.m_angle, turnAngle, 0.001); };
    auto it = find_if(candidates.begin(), candidates.end(), DoesAngleMatch);
    if (it != candidates.end())
    {
      ASSERT(it->m_segment.GetMwmId() != firstOutgoingSeg.GetMwmId(), ());
      ASSERT(it->m_segment.GetSegmentIdx() == firstOutgoingSeg.GetSegmentIdx() && it->m_segment.IsForward() == firstOutgoingSeg.IsForward(), ());
      it->m_segment = firstOutgoingSeg;
    }
  }
}

bool GetTurnInfo(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                 RoutingSettings const & vehicleSettings,
                 TurnInfo & turnInfo)
{
  auto const & segments = result.GetSegments();
  CHECK_LESS(outgoingSegmentIndex, segments.size(), ());
  CHECK_GREATER(outgoingSegmentIndex, 0, ());

  if (PathIsFakeLoop(segments[outgoingSegmentIndex].m_path))
    return false;

  bool const isStartFakeLoop = PathIsFakeLoop(segments[outgoingSegmentIndex - 1].m_path);

  if (isStartFakeLoop && outgoingSegmentIndex < 2)
    return false;

  size_t const prevIndex = isStartFakeLoop ? outgoingSegmentIndex - 2 : outgoingSegmentIndex - 1;
  turnInfo = TurnInfo(&segments[prevIndex], &segments[outgoingSegmentIndex]);

  if (!turnInfo.IsSegmentsValid() || turnInfo.m_ingoing->m_segmentRange.IsEmpty())
    return false;

  if (isStartFakeLoop)
  {
    if (mercator::DistanceOnEarth(turnInfo.m_ingoing->m_path.front().GetPoint(),
                                  turnInfo.m_ingoing->m_path.back().GetPoint()) < vehicleSettings.m_minIngoingDistMeters ||
        mercator::DistanceOnEarth(turnInfo.m_outgoing->m_path.front().GetPoint(),
                                  turnInfo.m_outgoing->m_path.back().GetPoint()) < vehicleSettings.m_minOutgoingDistMeters)
    {
      return false;
    }
  }

  ASSERT(!turnInfo.m_ingoing->m_path.empty(), ());
  ASSERT(!turnInfo.m_outgoing->m_path.empty(), ());
  ASSERT_LESS(mercator::DistanceOnEarth(turnInfo.m_ingoing->m_path.back().GetPoint(),
                                        turnInfo.m_outgoing->m_path.front().GetPoint()),
              kFeaturesNearTurnMeters, ());

  return true;
}

// If the route goes along the rightmost or the leftmost way among available ones:
// 1. RightmostDirection or LeftmostDirection is selected
// 2. If the turn direction is |CarDirection::GoStraight| 
// and there's another GoStraight or SlightTurn turn
// GoStraight is corrected to TurnSlightRight/TurnSlightLeft
// to avoid ambiguity: 2 or more almost straight turns and GoStraight direction.

// turnCandidates are sorted by angle from leftmost to rightmost.
// Normally no duplicates should be found. But if they are present we can't identify the leftmost/rightmost by order.
void CorrectRightmostAndLeftmost(std::vector<TurnCandidate> const & turnCandidates, 
                                    Segment const & firstOutgoingSeg, 
                                    double turnAngle,
                                    TurnItem & turn)
{
  if (adjacent_find(turnCandidates.begin(), turnCandidates.end(), base::EqualsBy(&TurnCandidate::m_angle)) == turnCandidates.end())
  {
    for (auto candidate = turnCandidates.begin(); candidate != turnCandidates.end(); ++candidate)
    {
      if (candidate->m_segment == firstOutgoingSeg && candidate + 1 != turnCandidates.end())
      {
          // The route goes along the leftmost candidate. 
          turn.m_turn = LeftmostDirection(turnAngle);
          // Compare with the next candidate to the right.
            CorrectGoStraight(*(candidate + 1), candidate->m_angle, CarDirection::TurnSlightLeft, turn);
          break;
      }
      // Ignoring too sharp alternative candidates.
      if (candidate->m_angle > -90)
        break;
    }
    for (auto candidate = turnCandidates.rbegin(); candidate != turnCandidates.rend(); ++candidate)
    {
      if (candidate->m_segment == firstOutgoingSeg && candidate + 1 != turnCandidates.rend())
      {
        // The route goes along the rightmost candidate. 
        turn.m_turn = RightmostDirection(turnAngle);
        // Compare with the next candidate to the left.
        CorrectGoStraight(*(candidate + 1), candidate->m_angle, CarDirection::TurnSlightRight, turn);
        break;
      }
      // Ignoring too sharp alternative candidates.
      if (candidate->m_angle < 90)
        break;
    };
  }
  else
    LOG(LWARNING, ("nodes.candidates are not expected to have same m_angle."));
}

void GetTurnDirection(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                      NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings,
                      TurnItem & turn)
{
  TurnInfo turnInfo;
  bool validTurnInfo = GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo);
  if (!validTurnInfo)
    return;

  turn.m_keepAnyway = (!turnInfo.m_ingoing->m_isLink && turnInfo.m_outgoing->m_isLink);
  turn.m_sourceName = turnInfo.m_ingoing->m_name;
  turn.m_targetName = turnInfo.m_outgoing->m_name;
  turn.m_turn = CarDirection::None;

  ASSERT_GREATER(turnInfo.m_ingoing->m_path.size(), 1, ());
  TurnCandidates nodes;
  size_t ingoingCount;
  m2::PointD const junctionPoint = turnInfo.m_ingoing->m_path.back().GetPoint();
  result.GetPossibleTurns(turnInfo.m_ingoing->m_segmentRange, junctionPoint, ingoingCount, nodes);
  if (nodes.isCandidatesAngleValid)
  {
    ASSERT(is_sorted(nodes.candidates.begin(), nodes.candidates.end(), base::LessBy(&TurnCandidate::m_angle)),
           ("Turn candidates should be sorted by its angle field."));
  }

  if (nodes.candidates.size() == 0)
    return;

  // No turns are needed on transported road.
  if (turnInfo.m_ingoing->m_highwayClass == HighwayClass::Transported && turnInfo.m_outgoing->m_highwayClass == HighwayClass::Transported)
    return;

  Segment firstOutgoingSeg;
  bool const isFirstOutgoingSegValid = turnInfo.m_outgoing->m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSeg);
  if (!isFirstOutgoingSegValid)
    return;
  
  CorrectCandidatesSegmentByOutgoing(turnInfo, firstOutgoingSeg, nodes.candidates);

  RemoveUTurnCandidate(turnInfo, numMwmIds, nodes.candidates);
  auto const & turnCandidates = nodes.candidates;
  
  // Check for enter or exit to/from roundabout.
  if (turnInfo.m_ingoing->m_onRoundabout || turnInfo.m_outgoing->m_onRoundabout)
  {
    turn.m_turn = GetRoundaboutDirection(turnInfo, nodes, numMwmIds);
    return;
  }

  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, junctionPoint, numMwmIds, vehicleSettings);

  CarDirection const intermediateDirection = IntermediateDirection(turnAngle);

  // Checking for exits from highways.
  turn.m_turn = TryToGetExitDirection(nodes, turnInfo, firstOutgoingSeg, intermediateDirection);
  if (turn.m_turn != CarDirection::None)
    return;

  if (turnCandidates.size() == 1)
  {
    ASSERT(turnCandidates.front().m_segment == firstOutgoingSeg, ());
    // IngoingCount includes ingoing segment and reversed outgoing (if it is not one-way).
    // Both of them should be isnored. If any other one is present, turn is kept to prevent user from going to oneway alternative.
    if (ingoingCount <= 1 + size_t(!turnInfo.m_outgoing->m_isOneWay))
      return;
  }

  turn.m_turn = intermediateDirection;

  if (IsGoStraightOrSlightTurn(turn.m_turn))
  {
    if (!turn.m_keepAnyway && CanDiscardTurnByHighwayClassOrAngles(turn.m_turn, turnAngle, turnCandidates, turnInfo, numMwmIds))
    {
      turn.m_turn = CarDirection::None;
      return;
    }
  }

  if (turnCandidates.size() >= 2)
    CorrectRightmostAndLeftmost(turnCandidates, firstOutgoingSeg, turnAngle, turn);
}

void GetTurnDirectionPedestrian(IRoutingResult const & result, size_t outgoingSegmentIndex,
                                NumMwmIds const & numMwmIds,
                                RoutingSettings const & vehicleSettings, TurnItem & turn)
{
  TurnInfo turnInfo;
  bool validTurnInfo = GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo);
  if (!validTurnInfo)
    return;

  m2::PointD const junctionPoint = turnInfo.m_ingoing->m_path.back().GetPoint();
  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, junctionPoint, numMwmIds, vehicleSettings);

  turn.m_sourceName = turnInfo.m_ingoing->m_name;
  turn.m_targetName = turnInfo.m_outgoing->m_name;
  turn.m_pedestrianTurn = PedestrianDirection::None;

  ASSERT_GREATER(turnInfo.m_ingoing->m_path.size(), 1, ());

  TurnCandidates nodes;
  size_t ingoingCount = 0;
  result.GetPossibleTurns(turnInfo.m_ingoing->m_segmentRange, junctionPoint, ingoingCount, nodes);
  if (nodes.isCandidatesAngleValid)
  {
    ASSERT(is_sorted(nodes.candidates.begin(), nodes.candidates.end(),
                     base::LessBy(&TurnCandidate::m_angle)),
           ("Turn candidates should be sorted by its angle field."));
  }

  if (nodes.candidates.size() == 0)
    return;

  turn.m_pedestrianTurn = IntermediateDirectionPedestrian(turnAngle);

  if (turn.m_pedestrianTurn == PedestrianDirection::GoStraight)
  {
    turn.m_pedestrianTurn = PedestrianDirection::None;
    return;
  }

  RemoveUTurnCandidate(turnInfo, numMwmIds, nodes.candidates);

  // If there is no fork on the road we don't need to generate any turn. It is pointless because
  // there is no possibility of leaving the route.
  if (nodes.candidates.size() <= 1 || (std::fabs(CalcOneSegmentTurnAngle(turnInfo)) < kMaxForwardAngleActual && HasSingleForwardTurn(nodes)))
    turn.m_pedestrianTurn = PedestrianDirection::None;
}

size_t CheckUTurnOnRoute(IRoutingResult const & result, size_t outgoingSegmentIndex,
                         NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings,
                         TurnItem & turn)
{
  size_t constexpr kUTurnLookAhead = 3;
  double constexpr kUTurnHeadingSensitivity = math::pi / 10.0;
  auto const & segments = result.GetSegments();

  // In this function we process the turn between the previous and the current
  // segments. So we need a shift to get the previous segment.
  ASSERT_GREATER(segments.size(), 1, ());
  ASSERT_GREATER(outgoingSegmentIndex, 0, ());
  ASSERT_GREATER(segments.size(), outgoingSegmentIndex, ());
  auto const & masterSegment = segments[outgoingSegmentIndex - 1];
  if (masterSegment.m_path.size() < 2)
    return 0;

  // Roundabout is not the UTurn.
  if (masterSegment.m_onRoundabout)
    return 0;
  for (size_t i = 0; i < kUTurnLookAhead && i + outgoingSegmentIndex < segments.size(); ++i)
  {
    auto const & checkedSegment = segments[outgoingSegmentIndex + i];
    if (checkedSegment.m_path.size() < 2)
      return 0;

    if (checkedSegment.m_name == masterSegment.m_name &&
        checkedSegment.m_highwayClass == masterSegment.m_highwayClass &&
        checkedSegment.m_isLink == masterSegment.m_isLink && !checkedSegment.m_onRoundabout)
    {
      auto const & path = masterSegment.m_path;
      auto const & pointBeforeTurn = path[path.size() - 2];
      auto const & turnPoint = path[path.size() - 1];
      auto const & pointAfterTurn = checkedSegment.m_path[1];
      // Same segment UTurn case.
      if (i == 0)
      {
        // TODO Fix direction calculation.
        // Warning! We can not determine UTurn direction in single edge case. So we use UTurnLeft.
        // We decided to add driving rules (left-right sided driving) to mwm header.
        if (pointBeforeTurn == pointAfterTurn && turnPoint != pointBeforeTurn)
        {
          turn.m_turn = CarDirection::UTurnLeft;
          return 1;
        }
        // Wide UTurn must have link in it's middle.
        return 0;
      }

      // Avoid the UTurn on unnamed roads inside the rectangle based distinct.
      if (checkedSegment.m_name.empty())
        return 0;

      // Avoid returning to the same edge after uturn somewere else.
      if (pointBeforeTurn == pointAfterTurn)
        return 0;

      m2::PointD const v1 = turnPoint.GetPoint() - pointBeforeTurn.GetPoint();
      m2::PointD const v2 = pointAfterTurn.GetPoint() - checkedSegment.m_path[0].GetPoint();

      auto angle = ang::TwoVectorsAngle(m2::PointD::Zero(), v1, v2);

      if (!base::AlmostEqualAbs(angle, math::pi, kUTurnHeadingSensitivity))
        return 0;

      // Determine turn direction.
      m2::PointD const junctionPoint = masterSegment.m_path.back().GetPoint();

      m2::PointD const ingoingPoint = GetPointForTurn(
          result, outgoingSegmentIndex, numMwmIds, vehicleSettings.m_maxIngoingPointsCount,
          vehicleSettings.m_minIngoingDistMeters, false /* forward */);
      m2::PointD const outgoingPoint = GetPointForTurn(
          result, outgoingSegmentIndex, numMwmIds, vehicleSettings.m_maxOutgoingPointsCount,
          vehicleSettings.m_minOutgoingDistMeters, true /* forward */);

      if (PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint) < 0)
        turn.m_turn = CarDirection::UTurnLeft;
      else
        turn.m_turn = CarDirection::UTurnRight;
      return i + 1;
    }
  }

  return 0;
}

string DebugPrint(RoutePointIndex const & index)
{
  stringstream out;
  out << "RoutePointIndex [ m_segmentIndex == " << index.m_segmentIndex
      << ", m_pathIndex == " << index.m_pathIndex << " ]" << endl;
  return out.str();
}
}  // namespace turns
}  // namespace routing
