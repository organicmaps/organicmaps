#include "routing/turns_generator.hpp"
#include "routing/turns_generator_utils.hpp"

#include "routing/router.hpp"
#include "platform/measurement_utils.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/angles.hpp"

#include "base/checked_cast.hpp"
#include "base/stl_helpers.hpp"

#include <cmath>
#include <sstream>
#include <numeric>

namespace routing
{
namespace turns
{
using namespace std;
using namespace ftypes;

// Angles in degrees for finding route segments with no actual forks.
double constexpr kMaxForwardAngleCandidates = 95.0;
double constexpr kMaxForwardAngleActual = 60.0;

// Min difference of route and alternative turn abs angles in degrees
// to ignore alternative turn (when route direction is GoStraight).
double constexpr kMinAbsAngleDiffForStraightRoute = 25.0;

// Min difference of route and alternative turn abs angles in degrees
// to ignore alternative turn (when route direction is bigger and GoStraight).
double constexpr kMinAbsAngleDiffForBigStraightRoute = 5;

// Min difference of route and alternative turn abs angles in degrees 
// to ignore this alternative candidate (when alternative road is the same or smaller).
double constexpr kMinAbsAngleDiffForSameOrSmallerRoad = 35.0;

// Min difference between HighwayClasses of the route segment and alternative turn segment
// to ignore this alternative candidate.
int constexpr kMinHighwayClassDiff = -2;

// Min difference between HighwayClasses of the route segment and alternative turn segment
// to ignore this alternative candidate (when alternative road is service).
int constexpr kMinHighwayClassDiffForService = -1;

/// \brief Return |CarDirection::ExitHighwayToRight| or |CarDirection::ExitHighwayToLeft|
/// or return |CarDirection::None| if no exit is detected.
/// \note The function makes a decision about turn based on geometry of the route and turn
/// candidates, so it works correctly for both left and right hand traffic.
CarDirection TryToGetExitDirection(TurnCandidates const & possibleTurns, TurnInfo const & turnInfo,
                                   Segment const & firstOutgoingSeg, CarDirection const intermediateDirection)
{
  if (!possibleTurns.isCandidatesAngleValid)
    return CarDirection::None;

  if (!IsHighway(turnInfo.m_ingoing->m_highwayClass, turnInfo.m_ingoing->m_isLink))
    return CarDirection::None;

  if (!turnInfo.m_outgoing->m_isLink && !(IsSmallRoad(turnInfo.m_outgoing->m_highwayClass) && IsGoStraightOrSlightTurn(intermediateDirection)))
    return CarDirection::None;
  
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

/// * \returns true when
/// * - the route leads from one big road to another one;
/// * - and the other possible turns lead to small roads or these turns too sharp.
/// * Returns false otherwise.
/// \param routeDirection is route direction.
/// \param routeAngle is route angle.
/// \param turnCandidates is all possible ways out from a junction except uturn.
/// \param turnInfo is information about ingoing and outgoing segments of the route.
bool CanDiscardTurnByHighwayClassOrAngles(CarDirection const routeDirection,
                                          double const routeAngle,
                                          std::vector<TurnCandidate> const & turnCandidates, 
                                          TurnInfo const & turnInfo,
                                          NumMwmIds const & numMwmIds)
{
  if (!IsGoStraightOrSlightTurn(routeDirection))
    return false;
    
  if (turnCandidates.size() < 2)
    return true;

  HighwayClass outgoingRouteRoadClass = turnInfo.m_outgoing->m_highwayClass;
  HighwayClass ingoingRouteRoadClass = turnInfo.m_ingoing->m_highwayClass;

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

    // If route goes from non-link to link and there is not too sharp candidate then turn can't be discarded.
    if (!turnInfo.m_ingoing->m_isLink && turnInfo.m_outgoing->m_isLink && abs(t.m_angle) < abs(routeAngle) + 70)
      return false;

    HighwayClass candidateRoadClass = t.m_highwayClass;

    // If outgoing route road is significantly larger than candidate, the candidate can be ignored.
    if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiff)
      continue;

    // If outgoing route's road is significantly larger than candidate's service road, the candidate can be ignored.
    if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= kMinHighwayClassDiffForService && candidateRoadClass == HighwayClass::Service)
      continue;

    // If igngoing and outgoing edges are not links 
    // and outgoing route road is the same or large than ingoing 
    // then candidate-link can be ignored.
    if (!turnInfo.m_ingoing->m_isLink && !turnInfo.m_outgoing->m_isLink &&
        CalcDiffRoadClasses(outgoingRouteRoadClass, ingoingRouteRoadClass) <= 0 &&
        t.m_isLink)
      continue;

    // If alternative cadidate's road size is the same or smaller
    // and it's angle is sharp enough compared to the route it can be ignored.
    if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) <= 0 &&
        abs(t.m_angle) > abs(routeAngle) + kMinAbsAngleDiffForSameOrSmallerRoad)
      continue;
    
    if (routeDirection == CarDirection::GoStraight)
    {
      // If alternative cadidate's road size is smaller
      // and it's angle is not very close to the route's one - it can be ignored.
      if (CalcDiffRoadClasses(outgoingRouteRoadClass, candidateRoadClass) < 0 &&
          abs(t.m_angle) > abs(routeAngle) + kMinAbsAngleDiffForBigStraightRoute)
        continue;

      // If outgoing route road is the same or large than ingoing
      // and candidate's angle is sharp enough compared to the route it can be ignored.
      if (CalcDiffRoadClasses(outgoingRouteRoadClass, ingoingRouteRoadClass) <= 0 &&
          abs(t.m_angle) > abs(routeAngle) + kMinAbsAngleDiffForStraightRoute)
        continue;
    }

    return false;
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
    if (t.m_highwayClass != HighwayClass::Service)
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
  ASSERT(!segments[index.m_segmentIndex].m_path.empty(), ());

  RoutePointIndex nextIndex;
  ASSERT(GetNextRoutePointIndex(result, index, numMwmIds, forward, nextIndex), ());

  // There is no need for looking too far for low-speed roads.
  // So additional time limit is applied.
  double constexpr kMaxTimeSeconds = 3.0;

  m2::PointD point = GetPointByIndex(segments, index);

  size_t count = 0;
  double curDistanceMeters = 0.0;
  double curTimeSeconds = 0.0;

  while (GetNextRoutePointIndex(result, index, numMwmIds, forward, nextIndex))
  {
    m2::PointD nextPoint = GetPointByIndex(segments, nextIndex);

    // At start and finish there are two edges with zero length.
    // This function should not be called for the start (|outgoingSegmentIndex| == 0).
    // So there is special processing for the finish below.
    if (point == nextPoint && outgoingSegmentIndex + 1 == segments.size())
      return nextPoint;

    double distanceMeters = mercator::DistanceOnEarth(point, nextPoint);
    curDistanceMeters += distanceMeters;
    curTimeSeconds += CalcEstimatedTimeToPass(distanceMeters, segments[nextIndex.m_segmentIndex].m_highwayClass);

    if (curTimeSeconds > kMaxTimeSeconds || ++count >= maxPointsCount || curDistanceMeters > maxDistMeters)
      return nextPoint;

    point = nextPoint;
    index = nextIndex;
  }

  return point;
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
bool GetNextCrossSegmentRoutePoint(IRoutingResult const & result, RoutePointIndex const & index,
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
  if (!IsGoStraightOrSlightTurn(oneSegmentDirection))
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

bool GetPrevInSegmentRoutePoint(IRoutingResult const & result, RoutePointIndex const & index, RoutePointIndex & nextIndex)
{
  if (index.m_pathIndex == 0)
    return false;

  auto const & segments = result.GetSegments();
  if (segments[index.m_segmentIndex].m_path.size() >= 3 && index.m_pathIndex < segments[index.m_segmentIndex].m_path.size() - 1)
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
 * \brief Corrects |turn.m_turn| if |turn.m_turn == CarDirection::GoStraight|. 
 * If the other way is not sharp enough, turn.m_turn is set to |turnToSet|.
 */
void CorrectGoStraight(TurnCandidate const & notRouteCandidate, double const routeAngle, CarDirection const & turnToSet,
                       TurnItem & turn)
{
  if (turn.m_turn != CarDirection::GoStraight)
    return;
  
  // Correct turn if alternative cadidate's angle is not sharp enough compared to the route.
  if (abs(notRouteCandidate.m_angle) < abs(routeAngle) + kMinAbsAngleDiffForStraightRoute)
  {
    LOG(LDEBUG, ("Turn: ", turn.m_index, " GoStraight correction."));
    turn.m_turn = turnToSet;
  }
}

bool GetNextRoutePointIndex(IRoutingResult const & result, RoutePointIndex const & index,
                            NumMwmIds const & numMwmIds, bool const forward,
                            RoutePointIndex & nextIndex)
{
  if (forward)
  {
    if (!GetNextCrossSegmentRoutePoint(result, index, numMwmIds, nextIndex))
      return false;
  }
  else
  {
    if (!GetPrevInSegmentRoutePoint(result, index, nextIndex))
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

void FixupTurns(vector<geometry::PointWithAltitude> const & junctions, Route::TTurns & turnsDir)
{
  double const kMergeDistMeters = 15.0;
  // For turns that are not EnterRoundAbout/ExitRoundAbout exitNum is always equal to zero.
  // If a turn is EnterRoundAbout exitNum is a number of turns between two junctions:
  // (1) the route enters to the roundabout;
  // (2) the route leaves the roundabout;
  uint32_t exitNum = 0;
  TurnItem * currentEnterRoundAbout = nullptr;

  for (size_t idx = 0; idx < turnsDir.size(); )
  {
    TurnItem & t = turnsDir[idx];
    if (currentEnterRoundAbout && t.m_turn != CarDirection::StayOnRoundAbout && t.m_turn != CarDirection::LeaveRoundAbout)
    {
      ASSERT(false, ("Only StayOnRoundAbout or LeaveRoundAbout are expected after EnterRoundAbout."));
      exitNum = 0;
      currentEnterRoundAbout = nullptr;
    }
    else if (t.m_turn == CarDirection::EnterRoundAbout)
    {
      ASSERT(!currentEnterRoundAbout, ("It's not expected to find new EnterRoundAbout until previous EnterRoundAbout was leaved."));
      currentEnterRoundAbout = &t;
      ASSERT(exitNum == 0, ("exitNum is reset at start and after LeaveRoundAbout."));
      exitNum = 0;
    }
    else if (t.m_turn == CarDirection::StayOnRoundAbout)
    {
      ++exitNum;
      turnsDir.erase(turnsDir.begin() + idx);
      continue;
    }
    else if (t.m_turn == CarDirection::LeaveRoundAbout)
    {
      // It's possible for car to be on roundabout without entering it
      // if route calculation started at roundabout (e.g. if user made full turn on roundabout).
      if (currentEnterRoundAbout)
        currentEnterRoundAbout->m_exitNum = exitNum + 1;
      t.m_exitNum = exitNum + 1; // For LeaveRoundAbout turn.
      currentEnterRoundAbout = nullptr;
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

CarDirection GetRoundaboutDirection(TurnInfo const & turnInfo, TurnCandidates const & nodes, NumMwmIds const & numMwmIds)
{  
  bool const keepTurnByHighwayClass = KeepRoundaboutTurnByHighwayClass(nodes, turnInfo, numMwmIds);
  return GetRoundaboutDirection(turnInfo.m_ingoing->m_onRoundabout, turnInfo.m_outgoing->m_onRoundabout, 
                                nodes.candidates.size() > 1, keepTurnByHighwayClass);
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

/// \returns angle, wchis is calculated using several backward and forward segments 
/// from junction to consider smooth turns and remove noise.
double CalcTurnAngle(IRoutingResult const & result, 
                     size_t const outgoingSegmentIndex, 
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

  m2::PointD const junctionPoint = result.GetSegments()[outgoingSegmentIndex].m_path.front().GetPoint();
  return base::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
}

// It's possible that |firstOutgoingSeg| is not contained in |turnCandidates|.
// It may happened if |firstOutgoingSeg| and candidates in |turnCandidates| are from different mwms.
// Let's identify it in turnCandidates by angle and update according turnCandidate.
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
// and there's another not sharp enough turn
// GoStraight is corrected to TurnSlightRight/TurnSlightLeft
// to avoid ambiguity for GoStraight direction: 2 or more almost straight turns.
void CorrectRightmostAndLeftmost(std::vector<TurnCandidate> const & turnCandidates, 
                                 Segment const & firstOutgoingSeg, double const turnAngle,
                                 TurnItem & turn)
{
  // turnCandidates are sorted by angle from leftmost to rightmost.
  // Normally no duplicates should be found. But if they are present we can't identify the leftmost/rightmost by order.
  if (adjacent_find(turnCandidates.begin(), turnCandidates.end(), base::EqualsBy(&TurnCandidate::m_angle)) != turnCandidates.end())
  {
    LOG(LWARNING, ("nodes.candidates are not expected to have same m_angle."));
    return;
  }

  double constexpr kMaxAbsAngleConsideredLeftOrRightMost = 90;

  // Go from left to right to findout if the route goes through the leftmost candidate and fixes can be applied.
  // Other candidates which are sharper than kMaxAbsAngleConsideredLeftOrRightMost are ignored.
  for (auto candidate = turnCandidates.begin(); candidate != turnCandidates.end(); ++candidate)
  {
    if (candidate->m_segment == firstOutgoingSeg && candidate + 1 != turnCandidates.end())
    {
      // The route goes along the leftmost candidate. 
      turn.m_turn = LeftmostDirection(turnAngle);
      if (IntermediateDirection(turnAngle) != turn.m_turn)
        LOG(LDEBUG, ("Turn: ", turn.m_index, " LeftmostDirection correction."));
      // Compare with the next candidate to the right.
      CorrectGoStraight(*(candidate + 1), candidate->m_angle, CarDirection::TurnSlightLeft, turn);
      break;
    }
    // Check if this candidate is considered as leftmost as not too sharp.
    // If yes - this candidate is leftmost, not route's one. 
    if (candidate->m_angle > -kMaxAbsAngleConsideredLeftOrRightMost)
      break;
  }
  // Go from right to left to findout if the route goes through the rightmost candidate anf fixes can be applied.
  // Other candidates which are sharper than kMaxAbsAngleConsideredLeftOrRightMost are ignored.
  for (auto candidate = turnCandidates.rbegin(); candidate != turnCandidates.rend(); ++candidate)
  {
    if (candidate->m_segment == firstOutgoingSeg && candidate + 1 != turnCandidates.rend())
    {
      // The route goes along the rightmost candidate. 
      turn.m_turn = RightmostDirection(turnAngle);
      if (IntermediateDirection(turnAngle) != turn.m_turn)
        LOG(LDEBUG, ("Turn: ", turn.m_index, " RighmostDirection correction."));
      // Compare with the next candidate to the left.
      CorrectGoStraight(*(candidate + 1), candidate->m_angle, CarDirection::TurnSlightRight, turn);
      break;
    }
    // Check if this candidate is considered as rightmost as not too sharp.
    // If yes - this candidate is rightmost, not route's one.
    if (candidate->m_angle < kMaxAbsAngleConsideredLeftOrRightMost)
      break;
  };
}

void GetTurnDirection(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                      NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings,
                      TurnItem & turn)
{
  TurnInfo turnInfo;
  if (!GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo))
    return;

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

  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, numMwmIds, vehicleSettings);

  CarDirection const intermediateDirection = IntermediateDirection(turnAngle);

  // Checking for exits from highways.
  turn.m_turn = TryToGetExitDirection(nodes, turnInfo, firstOutgoingSeg, intermediateDirection);
  if (turn.m_turn != CarDirection::None)
    return;

  if (turnCandidates.size() == 1)
  {
    ASSERT(turnCandidates.front().m_segment == firstOutgoingSeg, ());

    if (IsGoStraightOrSlightTurn(intermediateDirection))
      return;
    // IngoingCount includes ingoing segment and reversed outgoing (if it is not one-way).
    // If any other one is present, turn (not straight or slight) is kept to prevent user from going to oneway alternative.

    /// @todo Min abs angle of ingoing ones should be considered. If it's much bigger than route angle - ignore ingoing ones.
    /// Now this data is not available from IRoutingResult::GetPossibleTurns().
    if (ingoingCount <= 1 + size_t(!turnInfo.m_outgoing->m_isOneWay))
      return;
  }

  // This angle is calculated using only 1 segment back and forward, not like turnAngle.
  double turnOneSegmentAngle = CalcOneSegmentTurnAngle(turnInfo);

  // To not discard some disputable turns let's use max by modulus from turnOneSegmentAngle and turnAngle.
  // It's natural since angles of turnCandidates are calculated in IRoutingResult::GetPossibleTurns() 
  // according to CalcOneSegmentTurnAngle logic. And to be safe turnAngle is used too.
  double turnAngleToCompare = turnAngle;
  if (turnOneSegmentAngle <= 0 && turnAngle <= 0)
    turnAngleToCompare = min(turnOneSegmentAngle, turnAngle);
  else if (turnOneSegmentAngle >= 0 && turnAngle >= 0)
    turnAngleToCompare = max(turnOneSegmentAngle, turnAngle);
  else if (abs(turnOneSegmentAngle) > 10)
    LOG(LWARNING, ("Significant angles are expected to have the same sign."));

  if (CanDiscardTurnByHighwayClassOrAngles(intermediateDirection, turnAngleToCompare, turnCandidates, turnInfo, numMwmIds))
    return;

  turn.m_turn = intermediateDirection;

  if (turnCandidates.size() >= 2)  
    CorrectRightmostAndLeftmost(turnCandidates, firstOutgoingSeg, turnAngle, turn);
}

void GetTurnDirectionPedestrian(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                NumMwmIds const & numMwmIds,
                                RoutingSettings const & vehicleSettings, TurnItem & turn)
{
  TurnInfo turnInfo;
  if (!GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo))
    return;

  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, numMwmIds, vehicleSettings);

  turn.m_sourceName = turnInfo.m_ingoing->m_name;
  turn.m_targetName = turnInfo.m_outgoing->m_name;
  turn.m_pedestrianTurn = PedestrianDirection::None;

  ASSERT_GREATER(turnInfo.m_ingoing->m_path.size(), 1, ());

  TurnCandidates nodes;
  size_t ingoingCount = 0;
  m2::PointD const junctionPoint = turnInfo.m_ingoing->m_path.back().GetPoint();
  result.GetPossibleTurns(turnInfo.m_ingoing->m_segmentRange, junctionPoint, ingoingCount, nodes);
  if (nodes.isCandidatesAngleValid)
  {
    ASSERT(is_sorted(nodes.candidates.begin(), nodes.candidates.end(), base::LessBy(&TurnCandidate::m_angle)),
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

size_t CheckUTurnOnRoute(IRoutingResult const & result, size_t const outgoingSegmentIndex,
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

}  // namespace turns
}  // namespace routing
