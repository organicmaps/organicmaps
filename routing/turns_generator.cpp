#include "routing/turns_generator.hpp"

#include "routing/router.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include "std/cmath.hpp"
#include "std/numeric.hpp"
#include "std/string.hpp"

using namespace routing;
using namespace routing::turns;

namespace
{
size_t constexpr kMaxOutgoingPointsCount = 5;
double constexpr kMinOutgoingDistMeters = 200.;
size_t constexpr kMaxIngoingPointsCount = 2;
double constexpr kMinIngoingDistMeters = 300.;
size_t constexpr kNotSoCloseMaxPointsCount = 3;
double constexpr kNotSoCloseMinDistMeters = 30.;

bool IsHighway(ftypes::HighwayClass hwClass, bool isLink)
{
  return (hwClass == ftypes::HighwayClass::Trunk || hwClass == ftypes::HighwayClass::Primary) &&
         !isLink;
}

bool IsLinkOrSmallRoad(ftypes::HighwayClass hwClass, bool isLink)
{
  return isLink || hwClass == ftypes::HighwayClass::LivingStreet ||
         hwClass == ftypes::HighwayClass::Service ||
         hwClass == ftypes::HighwayClass::Pedestrian;
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
      !IsLinkOrSmallRoad(turnInfo.m_outgoing.m_highwayClass, turnInfo.m_outgoing.m_isLink))
  {
    return false;
  }

  if (turnInfo.m_ingoing.m_highwayClass == ftypes::HighwayClass::Primary &&
      !turnInfo.m_outgoing.m_isLink && !IsGoStraightOrSlightTurn(intermediateDirection))
  {
    return false;
  }

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

/*!
 * \brief Returns false when
 * - the route leads from one big road to another one;
 * - and the other possible turns lead to small roads;
 * - and the turn is GoStraight or TurnSlight*.
 * Returns true otherwise.
 */
bool KeepTurnByHighwayClass(CarDirection turn, TurnCandidates const & possibleTurns,
                            TurnInfo const & turnInfo, NumMwmIds const & numMwmIds)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true;  // The road significantly changes its direction here. So this turn shall be kept.

  // There's only one exit from this junction. This turn should be kept here.
  // That means the the decision to keep this turn or not is left for KeepTurnByIngoingEdges()
  // method which should be called after this one.
  if (possibleTurns.candidates.size() == 1)
    return true;

  // The turn should be kept if there's no any information about feature id of outgoing segment
  // just to be on the safe side. It may happen in case of outgoing segment is a finish segment.
  Segment firstOutgoingSegment;
  if (!turnInfo.m_outgoing.m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSegment))
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::Error;
  for (auto const & t : possibleTurns.candidates)
  {
    if (t.m_segment == firstOutgoingSegment)
      continue;
    ftypes::HighwayClass const highwayClass = t.m_highwayClass;
    if (static_cast<int>(highwayClass) > static_cast<int>(maxClassForPossibleTurns))
      maxClassForPossibleTurns = highwayClass;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("One of possible turns follows along an undefined HighwayClass."));
    return true;
  }
  if (maxClassForPossibleTurns == ftypes::HighwayClass::Undefined)
    return false; // Fake edges have HighwayClass::Undefined.

  ftypes::HighwayClass const minClassForTheRoute =
      static_cast<ftypes::HighwayClass>(min(static_cast<int>(turnInfo.m_ingoing.m_highwayClass),
                                            static_cast<int>(turnInfo.m_outgoing.m_highwayClass)));

  if (minClassForTheRoute == ftypes::HighwayClass::Error)
  {
    ASSERT(false, ("The route contains undefined HighwayClass."));
    return false;
  }
  if (minClassForTheRoute == ftypes::HighwayClass::Undefined)
    return false; // Fake edges have HighwayClass::Undefined.

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
    my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPointOneSegment, outgoingPoint));
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

/*!
 * \brief GetPointForTurn returns ingoingPoint or outgoingPoint for turns.
 * These points belongs to the route but they often are not neighbor of junctionPoint.
 * To calculate the resulting point the function implements the following steps:
 * - going from junctionPoint along segment path according to the direction which is set in GetPointIndex().
 * - until one of following conditions is fulfilled:
 *   - the end of ft is reached; (returns the last feature point)
 *   - more than kMaxIngoingPointsCount or kMaxOutgoingPointsCount points are passed;
 *     (returns the kMaxIngoingPointsCount-th or kMaxOutgoingPointsCount point)
 *   - the length of passed parts of segment exceeds kMinIngoingDistMeters or kMinOutgoingDistMeters;
 *     (returns the next point after the event)
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
m2::PointD GetPointForTurn(vector<Junction> const & path, m2::PointD const & junctionPoint,
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
    nextPoint = path[GetPointIndex(0, numSegPoints, i)].GetPoint();

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

size_t GetIngoingPointIndex(const size_t start, const size_t end, const size_t i)
{
  return end > start ? end - i : end + i;
}

size_t GetOutgoingPointIndex(const size_t start, const size_t end, const size_t i)
{
  return end > start ? start + i : start - i;
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
}  // namespace

namespace routing
{
namespace turns
{
bool TurnInfo::IsSegmentsValid() const
{
  if (m_ingoing.m_path.empty() || m_outgoing.m_path.empty())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

IRouter::ResultCode MakeTurnAnnotation(turns::IRoutingResult const & result, NumMwmIds const & numMwmIds,
                                       RouterDelegate const & delegate,
                                       vector<Junction> & junctions, Route::TTurns & turnsDir,
                                       Route::TStreets & streets, vector<Segment> & segments)
{
  LOG(LDEBUG, ("Shortest th length:", result.GetPathLength()));

  if (delegate.IsCancelled())
    return IRouter::Cancelled;
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
      turns::TurnItem turnItem;
      turnItem.m_index = static_cast<uint32_t>(junctions.size() - 1);

      size_t segmentIndex = distance(loadedSegments.begin(), loadedSegmentIt);
      skipTurnSegments = CheckUTurnOnRoute(loadedSegments, segmentIndex, turnItem);

      turns::TurnInfo turnInfo(loadedSegments[segmentIndex - 1], *loadedSegmentIt);

      if (turnItem.m_turn == turns::CarDirection::None)
        turns::GetTurnDirection(result, numMwmIds, turnInfo, turnItem);

      //  Lane information.
      if (turnItem.m_turn != turns::CarDirection::None)
      {
        turnItem.m_lanes = turnInfo.m_ingoing.m_lanes;
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
    return IRouter::ResultCode::RouteNotFound;

  junctions.front() = result.GetStartPoint();
  junctions.back() = result.GetEndPoint();

  turnsDir.emplace_back(turns::TurnItem(static_cast<uint32_t>(junctions.size()) - 1, turns::CarDirection::ReachedYourDestination));
  turns::FixupTurns(junctions, turnsDir);

#ifdef DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-",
                 t.m_targetName, "exit:", t.m_exitNum));
  }
#endif
  return IRouter::ResultCode::NoError;
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
      {40., CarDirection::TurnRight},
      {-10., CarDirection::TurnSlightRight},
      {-20., CarDirection::GoStraight},
      {-60., CarDirection::TurnSlightLeft},
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

void GetTurnDirection(IRoutingResult const & result, NumMwmIds const & numMwmIds,
                      TurnInfo & turnInfo, TurnItem & turn)
{
  if (!turnInfo.IsSegmentsValid() || turnInfo.m_ingoing.m_segmentRange.IsEmpty())
    return;

  ASSERT(!turnInfo.m_ingoing.m_path.empty(), ());
  ASSERT(!turnInfo.m_outgoing.m_path.empty(), ());
  ASSERT_LESS(MercatorBounds::DistanceOnEarth(turnInfo.m_ingoing.m_path.back().GetPoint(),
                                              turnInfo.m_outgoing.m_path.front().GetPoint()),
              kFeaturesNearTurnMeters, ());

  m2::PointD const junctionPoint = turnInfo.m_ingoing.m_path.back().GetPoint();
  m2::PointD const ingoingPoint = GetPointForTurn(turnInfo.m_ingoing.m_path, junctionPoint,
                                                  kMaxIngoingPointsCount, kMinIngoingDistMeters,
                                                  GetIngoingPointIndex);
  m2::PointD const outgoingPoint = GetPointForTurn(turnInfo.m_outgoing.m_path, junctionPoint,
                                                   kMaxOutgoingPointsCount, kMinOutgoingDistMeters,
                                                   GetOutgoingPointIndex);

  double const turnAngle = my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
  CarDirection const intermediateDirection = IntermediateDirection(turnAngle);

  turn.m_keepAnyway = (!turnInfo.m_ingoing.m_isLink && turnInfo.m_outgoing.m_isLink);
  turn.m_sourceName = turnInfo.m_ingoing.m_name;
  turn.m_targetName = turnInfo.m_outgoing.m_name;
  turn.m_turn = CarDirection::None;

  ASSERT_GREATER(turnInfo.m_ingoing.m_path.size(), 1, ());
  m2::PointD const ingoingPointOneSegment = turnInfo.m_ingoing.m_path[turnInfo.m_ingoing.m_path.size() - 2].GetPoint();
  TurnCandidates nodes;
  size_t ingoingCount;
  result.GetPossibleTurns(turnInfo.m_ingoing.m_segmentRange, ingoingPointOneSegment, junctionPoint,
                          ingoingCount, nodes);
  if (nodes.isCandidatesAngleValid)
  {
    ASSERT(is_sorted(nodes.candidates.begin(), nodes.candidates
        .end(), my::LessBy(&TurnCandidate::m_angle)),
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

  bool const keepTurnByHighwayClass = KeepTurnByHighwayClass(turn.m_turn, nodes, turnInfo, numMwmIds);
  if (!turn.m_keepAnyway && !keepTurnByHighwayClass)
  {
    turn.m_turn = CarDirection::None;
    return;
  }

  if (IsGoStraightOrSlightTurn(turn.m_turn))
  {
    auto const notSoCloseToTheTurnPoint =
        GetPointForTurn(turnInfo.m_ingoing.m_path, junctionPoint, kNotSoCloseMaxPointsCount,
                        kNotSoCloseMinDistMeters, GetIngoingPointIndex);

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
    return;
  }
}

size_t CheckUTurnOnRoute(TUnpackedPathSegments const & segments,
                         size_t currentSegment, TurnItem & turn)
{
  size_t constexpr kUTurnLookAhead = 3;
  double constexpr kUTurnHeadingSensitivity = math::pi / 10.0;

  // In this function we process the turn between the previous and the current
  // segments. So we need a shift to get the previous segment.
  ASSERT_GREATER(segments.size(), 1, ());
  ASSERT_GREATER(currentSegment, 0, ());
  ASSERT_GREATER(segments.size(), currentSegment, ());
  auto const & masterSegment = segments[currentSegment - 1];
  if (masterSegment.m_path.size() < 2)
    return 0;

  // Roundabout is not the UTurn.
  if (masterSegment.m_onRoundabout)
    return 0;
  for (size_t i = 0; i < kUTurnLookAhead && i + currentSegment < segments.size(); ++i)
  {
    auto const & checkedSegment = segments[currentSegment + i];
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

      if (!my::AlmostEqualAbs(angle, math::pi, kUTurnHeadingSensitivity))
        return 0;

      // Determine turn direction.
      m2::PointD const junctionPoint = masterSegment.m_path.back().GetPoint();
      m2::PointD const ingoingPoint = GetPointForTurn(masterSegment.m_path, junctionPoint,
                                                      kMaxIngoingPointsCount, kMinIngoingDistMeters,
                                                      GetIngoingPointIndex);
      m2::PointD const outgoingPoint = GetPointForTurn(segments[currentSegment].m_path, junctionPoint,
                                                       kMaxOutgoingPointsCount, kMinOutgoingDistMeters,
                                                       GetOutgoingPointIndex);
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
