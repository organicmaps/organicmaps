#include "routing/pedestrian_directions.hpp"

#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"
#include "routing/turns_generator_utils.hpp"

#include <utility>

namespace routing
{
using namespace std;
using namespace turns;

PedestrianDirectionsEngine::PedestrianDirectionsEngine(MwmDataSource & dataSource, shared_ptr<NumMwmIds> numMwmIds)
  : DirectionsEngine(dataSource, std::move(numMwmIds))
{}

// Angles in degrees for finding route segments with no actual forks.
double constexpr kMaxForwardAngleCandidates = 95.0;
double constexpr kMaxForwardAngleActual = 60.0;

size_t PedestrianDirectionsEngine::GetTurnDirection(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                                    NumMwmIds const & numMwmIds,
                                                    RoutingSettings const & vehicleSettings, TurnItem & turn)
{
  if (outgoingSegmentIndex == result.GetSegments().size())
  {
    turn.m_pedestrianTurn = PedestrianDirection::ReachedYourDestination;
    return 0;
  }

  // See comment for the same in CarDirectionsEngine::GetTurnDirection().
  if (outgoingSegmentIndex == 2)  // The same as turnItem.m_index == 2.
    return 0;

  TurnInfo turnInfo;
  if (!GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo))
    return 0;

  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, numMwmIds, vehicleSettings);

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

  /// @todo Proper handling of isCandidatesAngleValid == False, when we don't have angles of candidates.

  if (nodes.candidates.size() == 0)
    return 0;

  turn.m_pedestrianTurn = IntermediateDirectionPedestrian(turnAngle);

  if (turn.m_pedestrianTurn == PedestrianDirection::GoStraight)
  {
    turn.m_pedestrianTurn = PedestrianDirection::None;
    return 0;
  }

  Segment firstOutgoingSeg;
  if (!turnInfo.m_outgoing->m_segmentRange.GetFirstSegment(numMwmIds, firstOutgoingSeg))
    return 0;

  CorrectCandidatesSegmentByOutgoing(turnInfo, firstOutgoingSeg, nodes);
  RemoveUTurnCandidate(turnInfo, numMwmIds, nodes.candidates);

  // If there is no fork on the road we don't need to generate any turn. It is pointless because
  // there is no possibility of leaving the route.
  if (nodes.candidates.size() <= 1)
    turn.m_pedestrianTurn = PedestrianDirection::None;
  if (fabs(CalcOneSegmentTurnAngle(turnInfo)) < kMaxForwardAngleActual &&
      HasSingleForwardTurn(nodes, kMaxForwardAngleCandidates))
    turn.m_pedestrianTurn = PedestrianDirection::None;

  return 0;
}

void PedestrianDirectionsEngine::FixupTurns(vector<RouteSegment> & routeSegments)
{
  double const kMergeDistMeters = 15.0;

  for (size_t idx = 0; idx < routeSegments.size(); ++idx)
  {
    auto const & turn = routeSegments[idx].GetTurn();
    if (turn.IsTurnNone())
      continue;

    bool const prevStepNoTurn =
        idx > 0 && routeSegments[idx - 1].GetTurn().m_pedestrianTurn == PedestrianDirection::GoStraight;
    bool const needToTurn = routeSegments[idx].GetTurn().m_pedestrianTurn == PedestrianDirection::TurnLeft ||
                            routeSegments[idx].GetTurn().m_pedestrianTurn == PedestrianDirection::TurnRight;

    // Merging turns which are closer to each other under some circumstance.
    if (prevStepNoTurn && needToTurn)
    {
      auto const & junction = routeSegments[idx].GetJunction();
      auto const & prevJunction = routeSegments[idx - 1].GetJunction();
      if (mercator::DistanceOnEarth(junction.GetPoint(), prevJunction.GetPoint()) < kMergeDistMeters)
        routeSegments[idx - 1].ClearTurn();
    }
  }
}

}  // namespace routing
