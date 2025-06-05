#include "routing/routing_tests/tools.hpp"

namespace routing
{
void AsyncGuiThreadTestWithRoutingSession::InitRoutingSession()
{
  m_session = std::make_unique<routing::RoutingSession>();
  m_session->Init(nullptr /* pointCheckCallback */);
  m_session->SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
  m_session->SetOnNewTurnCallback([this]() { ++m_onNewTurnCallbackCounter; });
}

void RouteSegmentsFrom(std::vector<Segment> const & segments, std::vector<m2::PointD> const & path,
                       std::vector<turns::TurnItem> const & turns,
                       std::vector<RouteSegment::RoadNameInfo> const & names, std::vector<RouteSegment> & routeSegments)
{
  size_t size = segments.size();
  if (size == 0)
    size = turns.size();
  if (size == 0)
    size = names.size();
  if (size == 0)
    size = path.size() - 1;

  ASSERT(segments.empty() || segments.size() == size, ());
  ASSERT(turns.empty() || turns.size() == size, ());
  ASSERT(names.empty() || names.size() == size, ());
  ASSERT(path.empty() || path.size() - 1 == size, ());

  for (size_t i = 0; i < size; ++i)
  {
    geometry::PointWithAltitude point;
    if (path.size() > 0)
      point = geometry::PointWithAltitude(path[i + 1], geometry::kDefaultAltitudeMeters);
    Segment segment({0, 0, 0, true});
    if (segments.size() > 0)
      segment = segments[i];
    turns::TurnItem turn;
    if (turns.size() > 0)
    {
      turn = turns[i];
      ASSERT(turn.m_index == i + 1, ());
    }
    RouteSegment::RoadNameInfo name;
    if (names.size() > 0)
      name = names[i];
    routeSegments.emplace_back(segment, turn, point, name);
  }
}
}  // namespace routing
