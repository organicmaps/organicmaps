#include "routing/turns_generator.hpp"

#include "routing/router.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "base/checked_cast.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <cmath>
#include <sstream>
#include <numeric>

using namespace routing;
using namespace routing::turns;
using namespace std;

namespace
{
// @TODO(bykoianko) For the time being |kMaxOutgoingPointsCount| and |kMinOutgoingDistMeters|
// have value for car navigation. On the other hand it's used for bicycle navigation.
// But for bicycle navigation the value should be smaller. They should be moved to
// RoutingSettings structure.
size_t constexpr kMaxOutgoingPointsCount = 9;
double constexpr kMinOutgoingDistMeters = 120.0;
size_t constexpr kMaxIngoingPointsCount = 2;
double constexpr kMinIngoingDistMeters = 100.0;
size_t constexpr kNotSoCloseMaxPointsCount = 3;
double constexpr kNotSoCloseMaxDistMeters = 30.0;

/// \brief Contains information about highway classes of the route goes through a turn
/// and about highway classes of possible ways from the turn.
struct TurnHighwayClasses
{
  // When a route goes through a turn there's an ingoing and outgoing segments.
  // |m_smallestRouteRoadClass| is equal to the less important road between ingoing and outgoing
  // segments.
  ftypes::HighwayClass m_smallestRouteRoadClass = ftypes::HighwayClass::Error;
  // Let's consider all possible ways from the turn except for the way along the route and
  // the way which let us to make a U-turn by going along the ingoing segment.
  // |m_biggestPossibleTurnRoadClass| should be equal to the class of the biggest road of such roads.
  ftypes::HighwayClass m_biggestPossibleTurnRoadClass = ftypes::HighwayClass::Error;
};

bool IsHighway(ftypes::HighwayClass hwClass, bool isLink)
{
  return (hwClass == ftypes::HighwayClass::Trunk || hwClass == ftypes::HighwayClass::Primary) &&
         !isLink;
}

bool IsSmallRoad(ftypes::HighwayClass hwClass)
{
  return hwClass == ftypes::HighwayClass::LivingStreet ||
         hwClass == ftypes::HighwayClass::Service || hwClass == ftypes::HighwayClass::Pedestrian;
}

/// \brief Fills |turn| with |CarDirection::ExitHighwayToRight| or |CarDirection::ExitHighwayToLeft|
/// and returns true. Or does not change |turn| and returns false.
/// \note The function makes a decision about |turn| based on geometry of the route and turn
/// candidates, so it works correctly for both left and right hand traffic.
bool IsExit(TurnCandidates const & possibleTurns, TurnInfo const & turnInfo,
            Segment const & firstOutgoingSeg, CarDirection intermediateDirection, CarDirection & turn)
{
  if (!possibleTurns.isCandidatesAngleValid)
    return false;

  if (!IsHighway(turnInfo.m_ingoing.m_highwayClass, turnInfo.m_ingoing.m_isLink) ||
      !(turnInfo.m_outgoing.m_isLink || (IsSmallRoad(turnInfo.m_outgoing.m_highwayClass) &&
                                            IsGoStraightOrSlightTurn(intermediateDirection))))
  {
    return false;
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
        return false; // There are two or more highway candidates from the junction.
      isHighwayCandidateBeforeOutgoing = isCandidateBeforeOutgoing;
    }
  }
  if (highwayCandidateNumber == 1)
  {
    turn = isHighwayCandidateBeforeOutgoing ? CarDirection::ExitHighwayToRight
                                            : CarDirection::ExitHighwayToLeft;
    return true;
  }
  return false;
}

/// \brief If it's possible fills |turnHighwayClasses| and returns true, and returns false otherwise.
bool GetTurnHighwayClasses(TurnCandidates const & possibleTurns, TurnInfo const & turnInfo,
                           NumMwmIds const & numMwmIds, TurnHighwayClasses & turnHighwayClasses)
{
  // The turn should be kept if there's no any information about feature id of outgoing segment
  // just to be on the safe side. It may happen in case of outgoing segment is a finish segment.
  Segment firstOutgoingSegment;
  if (!turnInfo.m_outgoing.m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment))
    return true;

  Segment inversedLastIngoingSegment;
  if (!turnInfo.m_ingoing.m_segmentRange.GetLastSegment(numMwmIds, inversedLastIngoingSegment))
    return true;
  inversedLastIngoingSegment.Inverse();

  turnHighwayClasses.m_biggestPossibleTurnRoadClass = ftypes::HighwayClass::Count;
  for (auto const & t : possibleTurns.candidates)
  {
    // Let's consider all outgoing segments except for
    // (1) route outgoing segment
    // (2) a U-turn segment (inversedLastIngoingSegment)
    if (t.m_segment == firstOutgoingSegment || t.m_segment == inversedLastIngoingSegment)
      continue;
    ftypes::HighwayClass const highwayClass = t.m_highwayClass;
    // Note. The bigger road the less HighwayClass value.
    if (static_cast<int>(highwayClass) < static_cast<int>(turnHighwayClasses.m_biggestPossibleTurnRoadClass))
      turnHighwayClasses.m_biggestPossibleTurnRoadClass = highwayClass;
  }
  if (turnHighwayClasses.m_biggestPossibleTurnRoadClass == ftypes::HighwayClass::Error)
  {
    ASSERT_GREATER(possibleTurns.candidates.size(), 1, ("No turn candidates or there's only one turn candidate."));
    ASSERT(false, ("One of possible turns follows along an undefined HighwayClass."));
    return true;
  }

  if (turnHighwayClasses.m_biggestPossibleTurnRoadClass == ftypes::HighwayClass::Undefined)
    return false; // Fake edges have HighwayClass::Undefined.

  if (turnHighwayClasses.m_biggestPossibleTurnRoadClass == ftypes::HighwayClass::Count)
    return false; // No outgoing segments except for the route.

  // Note. The bigger road the less HighwayClass value.
  turnHighwayClasses.m_smallestRouteRoadClass =
      static_cast<ftypes::HighwayClass>(max(static_cast<int>(turnInfo.m_ingoing.m_highwayClass),
                                            static_cast<int>(turnInfo.m_outgoing.m_highwayClass)));

  if (turnHighwayClasses.m_smallestRouteRoadClass == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return false;
  }
  if (turnHighwayClasses.m_smallestRouteRoadClass == ftypes::HighwayClass::Undefined)
    return false; // Fake edges have HighwayClass::Undefined.

  return true;
}

/// * \returns false when
/// * - the route leads from one big road to another one;
/// * - and the other possible turns lead to small roads;
/// * Returns true otherwise.
/// \param possibleTurns is all possible ways out from a junction.
/// \param turnInfo is information about ingoing and outgoing segments of the route.
/// \param carDirection is a direction about the current turn if it's available.
/// If not, CarDirection::None should be passed.
bool KeepTurnByHighwayClass(TurnCandidates const & possibleTurns, TurnInfo const & turnInfo,
                            NumMwmIds const & numMwmIds, CarDirection carDirection)
{
  // Maximum difference between HighwayClasses of the route segments and
  // the possible way segments to keep the junction as a turn.
  int constexpr kMaxHighwayClassDiff = 2;
  int constexpr kMaxHighwayClassDiffForGoStraight = 1;
  int constexpr kMaxHighwayClassDiffForService = 1;

  TurnHighwayClasses turnHighwayClasses;
  if (!GetTurnHighwayClasses(possibleTurns, turnInfo, numMwmIds, turnHighwayClasses))
    return false;

  int const diff =
      static_cast<int>(turnHighwayClasses.m_biggestPossibleTurnRoadClass) - static_cast<int>(turnHighwayClasses.m_smallestRouteRoadClass);
  if (diff >= kMaxHighwayClassDiff ||
      (diff >= kMaxHighwayClassDiffForGoStraight && carDirection == CarDirection::GoStraight) ||
      (diff >= kMaxHighwayClassDiffForService && turnHighwayClasses.m_biggestPossibleTurnRoadClass == ftypes::HighwayClass::Service))
  {
    // The turn shall be removed if the route goes near small roads.
    return false;
  }
  return true;
}

/*!
 * \brief Returns false when other possible turns leads to service roads;
 */
bool KeepRoundaboutTurnByHighwayClass(CarDirection turn, TurnCandidates const & possibleTurns,
                                      TurnInfo const & turnInfo, NumMwmIds const & numMwmIds)
{
  Segment firstOutgoingSegment;
  bool const validFirstOutgoingSeg =
      turnInfo.m_outgoing.m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment);

  for (auto const & t : possibleTurns.candidates)
  {
    if (!validFirstOutgoingSeg || t.m_segment == firstOutgoingSegment)
      continue;
    if (static_cast<int>(t.m_highwayClass) != static_cast<int>(ftypes::HighwayClass::Service))
      return true;
  }
  return false;
}

bool DoAllTurnCandidatesGoAlmostStraight(vector<TurnCandidate> const & candidates)
{
  return all_of(candidates.cbegin(), candidates.cend(), [](TurnCandidate const & c) {
    return IsGoStraightOrSlightTurn(IntermediateDirection(c.m_angle));
  });
}

/// \brief Analyzes its args and makes a decision if it's possible to have a turn at this junction
/// or not.
/// \returns true if based on this analysis there's no turn at this junction and
/// false if the junction should be considered as possible turn.
bool DiscardTurnByIngoingAndOutgoingEdges(CarDirection intermediateDirection, bool hasMultiTurns,
                                          TurnInfo const & turnInfo, TurnItem const & turn,
                                          TurnCandidates const & turnCandidates)
{
  if (turn.m_keepAnyway || turnInfo.m_ingoing.m_onRoundabout ||
      turnInfo.m_outgoing.m_onRoundabout ||
      turnInfo.m_ingoing.m_highwayClass != turnInfo.m_outgoing.m_highwayClass)
  {
    return false;
  }

  // @TODO(bykoianko) If all turn candidates go almost straight and there are several ways
  // from the junction (|hasMultiTurns| == true) the turn will be discarded.
  // If all turn candidates go almost straight and there is only one way
  // from the junction (|hasMultiTurns| == false) the turn will not be discarded in this method,
  // and then may be kept. It means that in some cases if there are two or more possible
  // ways from a junction the turn may be discarded and if there is only one way out
  // the turn may be kept. This code should be redesigned.
  if (turnCandidates.isCandidatesAngleValid &&
      DoAllTurnCandidatesGoAlmostStraight(turnCandidates.candidates))
  {
    return !hasMultiTurns;
  }

  return ((!hasMultiTurns && IsGoStraightOrSlightTurn(intermediateDirection)) ||
          (hasMultiTurns && intermediateDirection == CarDirection::GoStraight));
}

// turnEdgesCount calculates both ingoing ond outgoing edges without user's edge.
bool KeepTurnByIngoingEdges(m2::PointD const & junctionPoint,
                            m2::PointD const & ingoingPointOneSegment,
                            m2::PointD const & outgoingPoint, bool hasMultiTurns,
                            size_t const turnEdgesCount)
{
  double const turnAngle =
    base::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPointOneSegment, outgoingPoint));
  bool const isGoStraightOrSlightTurn = IsGoStraightOrSlightTurn(IntermediateDirection(turnAngle));

  // The code below is responsible for cases when there is only one way to leave the junction.
  // Such junction has to be kept as a turn when it's not a slight turn and it has ingoing edges
  // (one or more);
  return hasMultiTurns || (!isGoStraightOrSlightTurn && turnEdgesCount > 1);
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
CarDirection FindDirectionByAngle(vector<pair<double, CarDirection>> const & lowerBounds,
                                   double angle)
{
  ASSERT_GREATER_OR_EQUAL(angle, -180., (angle));
  ASSERT_LESS_OR_EQUAL(angle, 180., (angle));
  ASSERT(!lowerBounds.empty(), ());
  ASSERT(is_sorted(lowerBounds.cbegin(), lowerBounds.cend(),
             [](pair<double, CarDirection> const & p1, pair<double, CarDirection> const & p2)
         {
           return p1.first > p2.first;
         }), ());

  for (auto const & lower : lowerBounds)
  {
    if (angle >= lower.first)
      return lower.second;
  }

  ASSERT(false, ("The angle is not covered by the table. angle = ", angle));
  return CarDirection::None;
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

/*!
 * \brief GetPointForTurn returns ingoing point or outgoing point for turns.
 * These points belongs to the route but they often are not neighbor of junction point.
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
                           double const maxDistMeters, bool forward)
{
  auto const & segments = result.GetSegments();
  ASSERT_LESS(outgoingSegmentIndex, segments.size(), ());
  ASSERT_GREATER(outgoingSegmentIndex, 0, ());

  RoutePointIndex index = forward ? GetFirstOutgoingPointIndex(outgoingSegmentIndex)
                                  : GetLastIngoingPointIndex(segments, outgoingSegmentIndex);
  ASSERT_LESS(index.m_pathIndex, segments[index.m_segmentIndex].m_path.size(), ());
  ASSERT_GREATER_OR_EQUAL(index.m_segmentIndex, 0, ());
  ASSERT_LESS(index.m_segmentIndex, segments.size(), ());
  RoutePointIndex nextIndex;

  ASSERT(!segments[index.m_segmentIndex].m_path.empty(), ());
  m2::PointD point = GetPointByIndex(segments, index);
  m2::PointD nextPoint;
  size_t count = 0;
  double curDistanceMeters = 0.0;

  ASSERT(GetNextRoutePointIndex(result, index, numMwmIds, forward, nextIndex), ());
  while (GetNextRoutePointIndex(result, index, numMwmIds, forward, nextIndex))
  {
    nextPoint = GetPointByIndex(segments, nextIndex);

    // At start and finish there are two edges with zero length.
    // GetPointForTurn() should not be called for the start (|outgoingSegmentIndex| == 0).
    // So there is special processing for the finish below.
    if (point == nextPoint && outgoingSegmentIndex + 1 == segments.size())
      return nextPoint;

    curDistanceMeters += MercatorBounds::DistanceOnEarth(point, nextPoint);
    if (curDistanceMeters > maxDistMeters || ++count >= maxPointsCount)
      return nextPoint;

    point = nextPoint;
    index = nextIndex;
  }

  return nextPoint;
}

size_t GetLinkCount(vector<TurnCandidate> const & candidates)
{
  size_t numLinks = 0;
  for (auto const & c : candidates)
  {
    if (c.m_isLink)
      ++numLinks;
  }
  return numLinks;
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

  TurnInfo const turnInfo(segments[index.m_segmentIndex], segments[index.m_segmentIndex + 1]);
  ASSERT_GREATER_OR_EQUAL(turnInfo.m_ingoing.m_path.size(), 2, ());
  ASSERT_GREATER_OR_EQUAL(turnInfo.m_outgoing.m_path.size(), 2, ());

  double const oneSegmentTurnAngle = base::RadToDeg(
      PiMinusTwoVectorsAngle(turnInfo.m_ingoing.m_path.back().GetPoint(),
                             turnInfo.m_ingoing.m_path[turnInfo.m_ingoing.m_path.size() - 2].GetPoint(),
                             turnInfo.m_outgoing.m_path[1].GetPoint()));
  CarDirection const oneSegmentDirection = IntermediateDirection(oneSegmentTurnAngle);
  if (!IsGoStraightOrSlightTurn(oneSegmentDirection))
    return false; // Too sharp turn angle.

  size_t ingoingCount = 0;
  TurnCandidates possibleTurns;
  result.GetPossibleTurns(turnInfo.m_ingoing.m_segmentRange, GetPointByIndex(segments, index),
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

  if (!KeepTurnByHighwayClass(possibleTurns, turnInfo, numMwmIds, CarDirection::None))
  {
    // Taking the next point of the next segment.
    nextIndex = {index.m_segmentIndex + 1, 1 /* m_pathIndex */};
    return true;
  }
  // Stopping getting next route points because an important bifurcation point is reached.
  return false;
}

bool GetPrevInSegmentRoutePoint(RoutePointIndex const & index, RoutePointIndex & nextIndex)
{
  if (index.m_pathIndex == 0)
    return false;

  nextIndex = {index.m_segmentIndex, index.m_pathIndex - 1};
  return true;
}

/*!
 * \brief Corrects |turn.m_turn| if |turn.m_turn == CarDirection::GoStraight| and there're only
 * two ways out from this junction. In that case the other way (the way which is not covered by
 * the route) is checked. If the other way is "go straight" or "slight turn", turn.m_turn is set
 * to |turnToSet|.
 */
void GoStraightCorrection(TurnCandidate const & notRouteCandidate, CarDirection turnToSet,
                          TurnItem & turn)
{
  CHECK_EQUAL(turn.m_turn, CarDirection::GoStraight, ());

  if (!IsGoStraightOrSlightTurn(IntermediateDirection(notRouteCandidate.m_angle)))
    return;

  turn.m_turn = turnToSet;
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
  if (m_ingoing.m_path.empty() || m_outgoing.m_path.empty())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

bool GetNextRoutePointIndex(IRoutingResult const & result, RoutePointIndex const & index,
                            NumMwmIds const & numMwmIds, bool forward, RoutePointIndex & nextIndex)
{
  if (forward)
  {
    if (!GetNextCrossSegmentRoutePoint(result, index, numMwmIds, nextIndex))
      return false;
  }
  else
  {
    if (!GetPrevInSegmentRoutePoint(index, nextIndex))
      return false;
  }

  ASSERT_LESS(nextIndex.m_segmentIndex, result.GetSegments().size(), ());
  ASSERT_LESS(nextIndex.m_pathIndex, result.GetSegments()[nextIndex.m_segmentIndex].m_path.size(), ());
  return true;
}

RouterResultCode MakeTurnAnnotation(IRoutingResult const & result, NumMwmIds const & numMwmIds,
                                       RouterDelegate const & delegate,
                                       vector<Junction> & junctions, Route::TTurns & turnsDir,
                                       Route::TStreets & streets, vector<Segment> & segments)
{
  LOG(LDEBUG, ("Shortest th length:", result.GetPathLength()));

  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;
  // Annotate turns.
  size_t skipTurnSegments = 0;
  auto const & loadedSegments = result.GetSegments();
  segments.reserve(loadedSegments.size());
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

      skipTurnSegments = CheckUTurnOnRoute(result, outgoingSegmentIndex, numMwmIds, turnItem);

      if (turnItem.m_turn == CarDirection::None)
        GetTurnDirection(result, outgoingSegmentIndex, numMwmIds, turnItem);

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
    // of several Junctions. Last Junction in a LoadedPathSegment::m_path is equal to first junction
    // in next LoadedPathSegment::m_path in vector TUnpackedPathSegments:
    // *---*---*---*---*       *---*           *---*---*---*
    //                 *---*---*   *---*---*---*
    // To prevent having repetitions in |junctions| list it's necessary to take the first point only from the
    // first item of |loadedSegments|. The beginning should be ignored for the rest |m_path|.
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
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-",
                 t.m_targetName, "exit:", t.m_exitNum));
  }
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

void FixupTurns(vector<Junction> const & junctions, Route::TTurns & turnsDir)
{
  double const kMergeDistMeters = 30.0;
  // For turns that are not EnterRoundAbout exitNum is always equal to zero.
  // If a turn is EnterRoundAbout exitNum is a number of turns between two junctions:
  // (1) the route enters to the roundabout;
  // (2) the route leaves the roundabout;
  uint32_t exitNum = 0;
  // If a roundabout is worked up the roundabout value junctions to the turn
  // of the enter to the roundabout. If not, roundabout is equal to nullptr.
  TurnItem * roundabout = nullptr;

  auto routeDistanceMeters = [&junctions](uint32_t start, uint32_t end)
  {
    double res = 0.0;
    for (uint32_t i = start + 1; i < end; ++i)
      res += MercatorBounds::DistanceOnEarth(junctions[i - 1].GetPoint(), junctions[i].GetPoint());
    return res;
  };

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
        routeDistanceMeters(turnsDir[idx - 1].m_index, turnsDir[idx].m_index) < kMergeDistMeters)
    {
      turnsDir.erase(turnsDir.begin() + idx - 1);
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

bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
{
  return !isIngoingEdgeRoundabout && isOutgoingEdgeRoundabout;
}

bool CheckRoundaboutExit(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
{
  return isIngoingEdgeRoundabout && !isOutgoingEdgeRoundabout;
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

  if (CheckRoundaboutEntrance(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
    return CarDirection::EnterRoundAbout;

  if (CheckRoundaboutExit(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
    return CarDirection::LeaveRoundAbout;

  ASSERT(false, ());
  return CarDirection::None;
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
      {157., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {2., CarDirection::TurnSlightRight},
      {-10., CarDirection::GoStraight},
      {-50., CarDirection::TurnSlightLeft},
      {-157., CarDirection::TurnLeft},
      {-180., CarDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

CarDirection LeftmostDirection(const double angle)
{
  return InvertDirection(RightmostDirection(-angle));
}

CarDirection IntermediateDirection(const double angle)
{
  static vector<pair<double, CarDirection>> const kLowerBounds = {
      {157., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {10., CarDirection::TurnSlightRight},
      {-10., CarDirection::GoStraight},
      {-50., CarDirection::TurnSlightLeft},
      {-157., CarDirection::TurnLeft},
      {-180., CarDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

/// \returns true iff one of the turn candidates goes along the ingoing route segment.
bool OneOfTurnCandidatesGoesAlongIngoingSegment(NumMwmIds const & numMwmIds,
                                                TurnCandidates const & turnCandidates,
                                                TurnInfo const & turnInfo)
{
  Segment ingoingSegment;
  if (!turnInfo.m_ingoing.m_segmentRange.GetLastSegment(numMwmIds, ingoingSegment))
    return false;

  for (auto const & c : turnCandidates.candidates)
  {
    if (c.m_segment.IsInverse(ingoingSegment))
      return true;
  }
  return false;
}

/// \returns true if there are two or more possible ways which don't go along an ingoing segment
/// and false otherwise.
/// \example If a route goes along such graph edges:
/// ...-->*<------>*<--->*<---------------->*<---...
/// for each point which is drawn above HasMultiTurns() returns false
/// despite the fact that for each point it's possible to go to two directions.
bool HasMultiTurns(NumMwmIds const & numMwmIds, TurnCandidates const & turnCandidates,
                   TurnInfo const & turnInfo)
{
  size_t const numTurnCandidates = turnCandidates.candidates.size();
  if (numTurnCandidates <= 1)
    return false;
  if (numTurnCandidates > 2)
    return true;

  return !OneOfTurnCandidatesGoesAlongIngoingSegment(numMwmIds, turnCandidates, turnInfo);
}

void GetTurnDirection(IRoutingResult const & result, size_t outgoingSegmentIndex,
                      NumMwmIds const & numMwmIds, TurnItem & turn)
{
  auto const & segments = result.GetSegments();
  CHECK_LESS(outgoingSegmentIndex, segments.size(), ());
  CHECK_GREATER(outgoingSegmentIndex, 0, ());

  TurnInfo turnInfo(segments[outgoingSegmentIndex - 1], segments[outgoingSegmentIndex]);
  if (!turnInfo.IsSegmentsValid() || turnInfo.m_ingoing.m_segmentRange.IsEmpty())
    return;

  ASSERT(!turnInfo.m_ingoing.m_path.empty(), ());
  ASSERT(!turnInfo.m_outgoing.m_path.empty(), ());
  ASSERT_LESS(MercatorBounds::DistanceOnEarth(turnInfo.m_ingoing.m_path.back().GetPoint(),
                                              turnInfo.m_outgoing.m_path.front().GetPoint()),
              kFeaturesNearTurnMeters, ());

  m2::PointD const junctionPoint = turnInfo.m_ingoing.m_path.back().GetPoint();
  m2::PointD const ingoingPoint =
      GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, kMaxIngoingPointsCount,
                      kMinIngoingDistMeters, false /* forward */);
  m2::PointD const outgoingPoint =
      GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, kMaxOutgoingPointsCount,
                      kMinOutgoingDistMeters, true /* forward */);

  double const turnAngle = base::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
  CarDirection const intermediateDirection = IntermediateDirection(turnAngle);

  turn.m_keepAnyway = (!turnInfo.m_ingoing.m_isLink && turnInfo.m_outgoing.m_isLink);
  turn.m_sourceName = turnInfo.m_ingoing.m_name;
  turn.m_targetName = turnInfo.m_outgoing.m_name;
  turn.m_turn = CarDirection::None;

  ASSERT_GREATER(turnInfo.m_ingoing.m_path.size(), 1, ());
  TurnCandidates nodes;
  size_t ingoingCount;
  result.GetPossibleTurns(turnInfo.m_ingoing.m_segmentRange, junctionPoint, ingoingCount, nodes);
  if (nodes.isCandidatesAngleValid)
  {
    ASSERT(is_sorted(nodes.candidates.begin(), nodes.candidates
        .end(), base::LessBy(&TurnCandidate::m_angle)),
           ("Turn candidates should be sorted by its angle field."));
  }

  if (nodes.candidates.size() == 0)
    return;

  bool const hasMultiTurns = HasMultiTurns(numMwmIds, nodes, turnInfo);

  // Checking for exits from highways.
  Segment firstOutgoingSeg;
  bool const isFirstOutgoingSegValid =
      turnInfo.m_outgoing.m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSeg);
  if (!turnInfo.m_ingoing.m_onRoundabout && isFirstOutgoingSegValid &&
      IsExit(nodes, turnInfo, firstOutgoingSeg, intermediateDirection, turn.m_turn))
  {
    return;
  }

  if (DiscardTurnByIngoingAndOutgoingEdges(intermediateDirection, hasMultiTurns, turnInfo, turn, nodes))
    return;

  if (!hasMultiTurns || !nodes.isCandidatesAngleValid)
  {
    turn.m_turn = intermediateDirection;
  }
  else
  {
    if (isFirstOutgoingSegValid)
    {
      if (nodes.candidates.front().m_segment == firstOutgoingSeg)
        turn.m_turn = LeftmostDirection(turnAngle);
      else if (nodes.candidates.back().m_segment == firstOutgoingSeg)
        turn.m_turn = RightmostDirection(turnAngle);
      else
        turn.m_turn = intermediateDirection;
    }
    else
    {
      turn.m_turn = intermediateDirection;
    }
  }

  if (turnInfo.m_ingoing.m_onRoundabout || turnInfo.m_outgoing.m_onRoundabout)
  {
    bool const keepTurnByHighwayClass =
        KeepRoundaboutTurnByHighwayClass(turn.m_turn, nodes, turnInfo, numMwmIds);
    turn.m_turn = GetRoundaboutDirection(turnInfo.m_ingoing.m_onRoundabout,
                                         turnInfo.m_outgoing.m_onRoundabout, hasMultiTurns,
                                         keepTurnByHighwayClass);
    return;
  }

  // Note 1. If the road significantly changes its direction this turn shall be kept here.
  // Note 2. If there's only one exit from this junction (nodes.candidates.size() != 1)
  // this turn should be kept.
  // Note 3. Keeping a turn at this point means that the decision to keep this turn or not
  // will be made after.
  if (!turn.m_keepAnyway && IsGoStraightOrSlightTurn(turn.m_turn) && nodes.candidates.size() != 1 &&
      !KeepTurnByHighwayClass(nodes, turnInfo, numMwmIds, turn.m_turn))
  {
    turn.m_turn = CarDirection::None;
    return;
  }

  if (IsGoStraightOrSlightTurn(turn.m_turn))
  {
    auto const notSoCloseToTheTurnPoint =
        GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, kNotSoCloseMaxPointsCount,
                        kNotSoCloseMaxDistMeters, false /* forward */);

    // Removing a slight turn if there's only one way to leave the turn and there's no ingoing edges.
    if (!KeepTurnByIngoingEdges(junctionPoint, notSoCloseToTheTurnPoint, outgoingPoint, hasMultiTurns,
                                nodes.candidates.size() + ingoingCount))
    {
      turn.m_turn = CarDirection::None;
      return;
    }

    // Removing a slight turn if ingoing and outgoing edges are not links and all other
    // possible ways out are links.
    if (!turnInfo.m_ingoing.m_isLink && !turnInfo.m_outgoing.m_isLink &&
        turnInfo.m_ingoing.m_highwayClass == turnInfo.m_outgoing.m_highwayClass &&
        GetLinkCount(nodes.candidates) + 1 == nodes.candidates.size())
    {
      turn.m_turn = CarDirection::None;
      return;
    }
  }

  if (turn.m_turn == CarDirection::GoStraight)
  {
    if (!hasMultiTurns)
      turn.m_turn = CarDirection::None;

    // Note. If the turn direction is |CarDirection::GoStraight| and there's only one extra way out
    // from the junction the direction may be corrected in some cases. So two turn candidates
    // (one for the route and an extra one) require special processing.
    if (nodes.candidates.size() != 2)
      return;

    if (nodes.candidates.front().m_segment == firstOutgoingSeg)
    {
      // The route goes along the leftmost candidate.
      GoStraightCorrection(nodes.candidates.back(), CarDirection::TurnSlightLeft, turn);
    }
    else if (nodes.candidates.back().m_segment == firstOutgoingSeg)
    {
      // The route goes along the rightmost candidate.
      GoStraightCorrection(nodes.candidates.front(), CarDirection::TurnSlightRight, turn);
    }
    // Note. It's possible that |firstOutgoingSeg| is not contained in |nodes.candidates|.
    // It may happened if |firstOutgoingSeg| and candidates in |nodes.candidates| are
    // from different mwms.

  }
}

size_t CheckUTurnOnRoute(IRoutingResult const & result, size_t outgoingSegmentIndex,
                         NumMwmIds const & numMwmIds, TurnItem & turn)
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

      m2::PointD const ingoingPoint =
          GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, kMaxIngoingPointsCount,
                          kMinIngoingDistMeters, false /* forward */);
      m2::PointD const outgoingPoint =
          GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, kMaxOutgoingPointsCount,
                          kMinOutgoingDistMeters, true /* forward */);

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
