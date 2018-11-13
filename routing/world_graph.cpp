#include "routing/world_graph.hpp"

namespace routing
{
void WorldGraph::GetTwins(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges)
{
  std::vector<Segment> twins;
  GetTwinsInner(segment, isOutgoing, twins);

  if (GetMode() == WorldGraphMode::LeapsOnly)
  {
    // Ingoing edges listing is not supported in LeapsOnly mode because we do not have enough
    // information to calculate |segment| weight. See https://jira.mail.ru/browse/MAPSME-5743 for details.
    CHECK(isOutgoing, ("Ingoing edges listing is not supported in LeapsOnly mode."));
    // We need both enter to mwm and exit from mwm in LeapsOnly mode to reconstruct leap.
    // That's why we need to duplicate twin segment here and than remove duplicate
    // while processing leaps.
    m2::PointD const & from = GetPoint(segment, isOutgoing /* front */);
    for (Segment const & twin : twins)
    {
      m2::PointD const & to = GetPoint(twin, isOutgoing /* front */);
      // Weight is usually zero because twins correspond the same feature
      // in different mwms. But if we have mwms with different versions and a feature
      // was moved in one of them the weight is greater than zero.
      edges.emplace_back(twin, HeuristicCostEstimate(from, to));
    }
    return;
  }

  auto prevMode = GetMode();
  SetMode(WorldGraphMode::SingleMwm);

  for (Segment const & twin : twins)
    GetEdgeList(twin, isOutgoing, edges);

  SetMode(prevMode);
}

RoutingOptions WorldGraph::GetRoutingOptions(Segment const & /* segment */)
{
  return {};
}

bool WorldGraph::IsRoutingOptionsGood(Segment const & /* segment */)
{
  return true;
}

void WorldGraph::SetRoutingOptions(RoutingOptions /* routingOption */) {}

std::string DebugPrint(WorldGraphMode mode)
{
  switch (mode)
  {
  case WorldGraphMode::LeapsOnly: return "LeapsOnly";
  case WorldGraphMode::NoLeaps: return "NoLeaps";
  case WorldGraphMode::SingleMwm: return "SingleMwm";
  case WorldGraphMode::Joints: return "Joints";
  case WorldGraphMode::JointSingleMwm: return "JointsSingleMwm";
  case WorldGraphMode::Undefined: return "Undefined";
  }

  UNREACHABLE();
}
}  // namespace routing
