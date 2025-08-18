#include "qt/routing_turns_visualizer.hpp"

namespace qt
{
void RoutingTurnsVisualizer::Visualize(RoutingManager & routingManager, df::DrapeApi & drape)
{
  auto const & polyline = routingManager.GetRoutePolyline().GetPolyline();
  auto const & turns = routingManager.GetTurnsOnRouteForTests();

  for (auto const & turn : turns)
  {
    std::string const id = GetId(turn);

    m_turnIds.insert(id);
    m2::PointD const & turnPoint = polyline.GetPoint(turn.m_index);

    std::vector<m2::PointD> const fakePoly = {turnPoint, turnPoint};
    static dp::Color const orangeColor = dp::Color(242, 138, 2, 255);

    drape.AddLine(id, df::DrapeApiLineData(fakePoly, orangeColor).Width(7.0f).ShowPoints(true).ShowId());
  }
}

void RoutingTurnsVisualizer::ClearTurns(df::DrapeApi & drape)
{
  for (auto const & turnId : m_turnIds)
    drape.RemoveLine(turnId);

  m_turnIds.clear();
}

std::string RoutingTurnsVisualizer::GetId(routing::turns::TurnItem const & turn)
{
  std::string const maneuver = turn.m_pedestrianTurn == routing::turns::PedestrianDirection::None
                                 ? DebugPrint(turn.m_turn)
                                 : DebugPrint(turn.m_pedestrianTurn);

  std::string const index = std::to_string(turn.m_index);

  return index + " " + maneuver + (turn.m_exitNum != 0 ? ":" + std::to_string(turn.m_exitNum) : "");
}
}  // namespace qt
