#include "routing/world_graph.hpp"

namespace routing
{
void WorldGraph::GetTwins(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges)
{
  std::vector<Segment> twins;
  GetTwinsInner(segment, isOutgoing, twins);

  if (GetMode() == Mode::LeapsOnly)
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
  SetMode(Mode::SingleMwm);

  for (Segment const & twin : twins)
    GetEdgeList(twin, isOutgoing, edges);

  SetMode(prevMode);
}

std::string DebugPrint(WorldGraph::Mode mode)
{
  switch (mode)
  {
  case WorldGraph::Mode::LeapsOnly: return "LeapsOnly";
  case WorldGraph::Mode::NoLeaps: return "NoLeaps";
  case WorldGraph::Mode::SingleMwm: return "SingleMwm";
  }

  UNREACHABLE();
}
}  // namespace routing
