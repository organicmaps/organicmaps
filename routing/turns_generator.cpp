#include "routing/osrm_helpers.hpp"
#include "routing/routing_mapping.hpp"
#include "routing/turns_generator.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "base/macros.hpp"

#include "std/cmath.hpp"
#include "std/numeric.hpp"
#include "std/string.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"

using namespace routing;
using namespace routing::turns;

namespace
{
size_t constexpr kMaxPointsCount = 5;
double constexpr kMinDistMeters = 200.;
size_t constexpr kNotSoCloseMaxPointsCount = 3;
double constexpr kNotSoCloseMinDistMeters = 30.;

/*!
 * \brief Returns false when
 * - the route leads from one big road to another one;
 * - and the other possible turns lead to small roads;
 * - and the turn is GoStraight or TurnSlight*.
 */
bool KeepTurnByHighwayClass(TurnDirection turn, TurnCandidates const & possibleTurns,
                            TurnInfo const & turnInfo)
{
  if (!IsGoStraightOrSlightTurn(turn))
    return true;  // The road significantly changes its direction here. So this turn shall be kept.

  // There's only one exit from this junction. NodeID of the exit is outgoingNode.
  if (possibleTurns.candidates.size() == 1)
    return true;

  ftypes::HighwayClass maxClassForPossibleTurns = ftypes::HighwayClass::Error;
  for (auto const & t : possibleTurns.candidates)
  {
    if (t.m_nodeId == turnInfo.m_outgoing.m_nodeId)
      continue;
    ftypes::HighwayClass const highwayClass = t.highwayClass;
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
bool KeepRoundaboutTurnByHighwayClass(TurnDirection turn, TurnCandidates const & possibleTurns,
                                      TurnInfo const & turnInfo)
{
  for (auto const & t : possibleTurns.candidates)
  {
    if (t.m_nodeId == turnInfo.m_outgoing.m_nodeId)
      continue;
    if (static_cast<int>(t.highwayClass) != static_cast<int>(ftypes::HighwayClass::Service))
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

// turnEdgesCount calculates both ingoing ond outgoing edges without user's edge.
bool KeepTurnByIngoingEdges(m2::PointD const & junctionPoint,
                            m2::PointD const & ingoingPointOneSegment,
                            m2::PointD const & outgoingPoint, bool hasMultiTurns,
                            size_t const turnEdgesCount)
{
  double const turnAngle =
    my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPointOneSegment, outgoingPoint));
  bool const isGoStraightOrSlightTurn = IsGoStraightOrSlightTurn(IntermediateDirection(turnAngle));

  // The code below is resposible for cases when there is only one way to leave the junction.
  // Such junction has to be kept as a turn when it's not a slight turn and it has ingoing edges
  // (one or more);
  return hasMultiTurns || (!isGoStraightOrSlightTurn && turnEdgesCount > 1);
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

IRouter::ResultCode MakeTurnAnnotation(turns::IRoutingResult const & result,
                                       RouterDelegate const & delegate,
                                       vector<Junction> & junctions,
                                       Route::TTurns & turnsDir, Route::TTimes & times,
                                       Route::TStreets & streets,
                                       vector<Segment> & trafficSegs)
{
  double estimatedTime = 0;

  LOG(LDEBUG, ("Shortest th length:", result.GetPathLength()));

#ifdef DEBUG
  size_t lastIdx = 0;
#endif

  if (delegate.IsCancelled())
    return IRouter::Cancelled;
  // Annotate turns.
  size_t skipTurnSegments = 0;
  auto const & loadedSegments = result.GetSegments();
  trafficSegs.reserve(loadedSegments.size());
  for (auto loadedSegmentIt = loadedSegments.cbegin(); loadedSegmentIt != loadedSegments.cend();
       ++loadedSegmentIt)
  {
    // ETA information.
    double const nodeTimeSeconds = loadedSegmentIt->m_weight;

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

      if (turnItem.m_turn == turns::TurnDirection::NoTurn)
        turns::GetTurnDirection(result, turnInfo, turnItem);

#ifdef DEBUG
      double distMeters = 0.0;
      for (size_t k = lastIdx + 1; k < junctions.size(); ++k)
        distMeters += MercatorBounds::DistanceOnEarth(junctions[k - 1].GetPoint(), junctions[k].GetPoint());
      LOG(LDEBUG, ("Speed:", 3.6 * distMeters / nodeTimeSeconds, "kmph; Dist:", distMeters, "Time:",
                   nodeTimeSeconds, "s", lastIdx, "e", junctions.size(), "source:",
                   turnItem.m_sourceName, "target:", turnItem.m_targetName));
      lastIdx = junctions.size();
#endif
      times.push_back(Route::TTimeItem(junctions.size(), estimatedTime));

      //  Lane information.
      if (turnItem.m_turn != turns::TurnDirection::NoTurn)
      {
        turnItem.m_lanes = turnInfo.m_ingoing.m_lanes;
        turnsDir.push_back(move(turnItem));
      }
    }

    estimatedTime += nodeTimeSeconds;
    if (skipTurnSegments > 0)
      --skipTurnSegments;

    // Path geometry.
    junctions.insert(junctions.end(), loadedSegmentIt->m_path.begin(), loadedSegmentIt->m_path.end());
    trafficSegs.insert(trafficSegs.end(), loadedSegmentIt->m_trafficSegs.cbegin(),
                       loadedSegmentIt->m_trafficSegs.cend());
  }

  // Path found. Points will be replaced by start and end edges junctions.
  if (junctions.size() == 1)
    junctions.push_back(junctions.front());

  if (junctions.size() < 2)
    return IRouter::ResultCode::RouteNotFound;

  junctions.front() = result.GetStartPoint();
  junctions.back() = result.GetEndPoint();

  times.push_back(Route::TTimeItem(junctions.size() - 1, estimatedTime));
  turnsDir.emplace_back(turns::TurnItem(static_cast<uint32_t>(junctions.size()) - 1, turns::TurnDirection::ReachedYourDestination));
  turns::FixupTurns(junctions, turnsDir);

#ifdef DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-",
                 t.m_targetName, "exit:", t.m_exitNum));
  }

  size_t last = 0;
  double lastTime = 0;
  for (Route::TTimeItem & t : times)
  {
    double dist = 0;
    for (size_t i = last + 1; i <= t.first; ++i)
      dist += MercatorBounds::DistanceOnEarth(junctions[i - 1].GetPoint(), junctions[i].GetPoint());

    double time = t.second - lastTime;

    LOG(LDEBUG, ("distance:", dist, "start:", last, "end:", t.first, "Time:", time, "Speed:",
                 3.6 * dist / time));
    last = t.first;
    lastTime = t.second;
  }
#endif
  LOG(LDEBUG, ("Estimated time:", estimatedTime, "s"));
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

void GetTurnDirection(IRoutingResult const & result, TurnInfo & turnInfo, TurnItem & turn)
{
  if (!turnInfo.IsSegmentsValid())
    return;

  ASSERT(!turnInfo.m_ingoing.m_path.empty(), ());
  ASSERT(!turnInfo.m_outgoing.m_path.empty(), ());
  ASSERT_LESS(MercatorBounds::DistanceOnEarth(turnInfo.m_ingoing.m_path.back().GetPoint(),
                                              turnInfo.m_outgoing.m_path.front().GetPoint()),
              kFeaturesNearTurnMeters, ());

  m2::PointD const junctionPoint = turnInfo.m_ingoing.m_path.back().GetPoint();
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
  m2::PointD const ingoingPointOneSegment = turnInfo.m_ingoing.m_path[turnInfo.m_ingoing.m_path.size() - 2].GetPoint();
  TurnCandidates nodes;
  size_t ingoingCount;
  result.GetPossibleTurns(turnInfo.m_ingoing.m_nodeId, ingoingPointOneSegment, junctionPoint,
                          ingoingCount, nodes);

  size_t const numNodes = nodes.candidates.size();
  bool const hasMultiTurns = numNodes > 1;

  if (numNodes == 0)
    return;

  if (!hasMultiTurns || !nodes.isCandidatesAngleValid)
  {
    turn.m_turn = intermediateDirection;
  }
  else
  {
    if (nodes.candidates.front().m_nodeId == turnInfo.m_outgoing.m_nodeId)
      turn.m_turn = LeftmostDirection(turnAngle);
    else if (nodes.candidates.back().m_nodeId == turnInfo.m_outgoing.m_nodeId)
      turn.m_turn = RightmostDirection(turnAngle);
    else
      turn.m_turn = intermediateDirection;
  }

  if (turnInfo.m_ingoing.m_onRoundabout || turnInfo.m_outgoing.m_onRoundabout)
  {
    bool const keepTurnByHighwayClass =
        KeepRoundaboutTurnByHighwayClass(turn.m_turn, nodes, turnInfo);
    turn.m_turn = GetRoundaboutDirection(turnInfo.m_ingoing.m_onRoundabout,
                                         turnInfo.m_outgoing.m_onRoundabout, hasMultiTurns,
                                         keepTurnByHighwayClass);
    return;
  }

  bool const keepTurnByHighwayClass = KeepTurnByHighwayClass(turn.m_turn, nodes, turnInfo);
  if (!turn.m_keepAnyway && !keepTurnByHighwayClass)
  {
    turn.m_turn = TurnDirection::NoTurn;
    return;
  }

  auto const notSoCloseToTheTurnPoint =
      GetPointForTurn(turnInfo.m_ingoing.m_path, junctionPoint, kNotSoCloseMaxPointsCount,
                      kNotSoCloseMinDistMeters, GetIngoingPointIndex);

  if (!KeepTurnByIngoingEdges(junctionPoint, notSoCloseToTheTurnPoint, outgoingPoint, hasMultiTurns,
                              nodes.candidates.size() + ingoingCount))
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

      m2::PointD const v1 = path[path.size() - 1].GetPoint() - path[path.size() - 2].GetPoint();
      m2::PointD const v2 = checkedSegment.m_path[1].GetPoint() - checkedSegment.m_path[0].GetPoint();

      auto angle = ang::TwoVectorsAngle(m2::PointD::Zero(), v1, v2);

      if (!my::AlmostEqualAbs(angle, math::pi, kUTurnHeadingSensitivity))
        return 0;

      // Determine turn direction.
      m2::PointD const junctionPoint = masterSegment.m_path.back().GetPoint();
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
