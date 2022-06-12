#include "routing/pedestrian_directions.hpp"

#include "routing/turns_generator.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator_utils.hpp"

#include <utility>

namespace routing
{
using namespace std;
using namespace turns;

PedestrianDirectionsEngine::PedestrianDirectionsEngine(MwmDataSource & dataSource, shared_ptr<NumMwmIds> numMwmIds)
  : DirectionsEngine(dataSource, move(numMwmIds))
{
}

// Angles in degrees for finding route segments with no actual forks.
double constexpr kMaxForwardAngleCandidates = 95.0;
double constexpr kMaxForwardAngleActual = 60.0;

size_t PedestrianDirectionsEngine::GetTurnDirection(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                   NumMwmIds const & numMwmIds,
                                   RoutingSettings const & vehicleSettings, TurnItem & turn)
{
  TurnInfo turnInfo;
  if (!GetTurnInfo(result, outgoingSegmentIndex, vehicleSettings, turnInfo))
    return 0;

  double const turnAngle = CalcTurnAngle(result, outgoingSegmentIndex, numMwmIds, vehicleSettings);

  turn.m_sourceName = turnInfo.m_ingoing->m_roadNameInfo.m_name;
  turn.m_targetName = turnInfo.m_outgoing->m_roadNameInfo.m_name;
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
  if (std::fabs(CalcOneSegmentTurnAngle(turnInfo)) < kMaxForwardAngleActual && HasSingleForwardTurn(nodes, kMaxForwardAngleCandidates))
    turn.m_pedestrianTurn = PedestrianDirection::None;

  return 0;
}

void PedestrianDirectionsEngine::FixupTurns(std::vector<geometry::PointWithAltitude> const & junctions,
                                            Route::TTurns & turnsDir)
{
  uint32_t turn_index = base::asserted_cast<uint32_t>(junctions.size() - 1);
  turnsDir.emplace_back(TurnItem(turn_index, PedestrianDirection::ReachedYourDestination));

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

#ifdef DEBUG
  for (auto const & t : turnsDir)
    LOG(LDEBUG, (GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-",
                 t.m_targetName, "exit:", t.m_exitNum));
#endif
}

}  // namespace routing
