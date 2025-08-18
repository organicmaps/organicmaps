#include "routing/world_graph.hpp"

namespace routing
{
void WorldGraph::GetEdgeList(Segment const & vertex, bool isOutgoing, bool useRoutingOptions, SegmentEdgeListT & edges)
{
  GetEdgeList({vertex, RouteWeight(0.0)}, isOutgoing, useRoutingOptions, false /* useAccessConditional */, edges);
}

void WorldGraph::GetTwins(Segment const & segment, bool isOutgoing, bool useRoutingOptions, SegmentEdgeListT & edges)
{
  std::vector<Segment> twins;
  GetTwinsInner(segment, isOutgoing, twins);

  CHECK_NOT_EQUAL(GetMode(), WorldGraphMode::LeapsOnly, ());

  auto prevMode = GetMode();
  SetMode(WorldGraphMode::SingleMwm);

  for (Segment const & twin : twins)
    GetEdgeList(twin, isOutgoing, useRoutingOptions, edges);

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

std::unique_ptr<TransitInfo> WorldGraph::GetTransitInfo(Segment const &)
{
  return nullptr;
}

std::vector<RouteSegment::SpeedCamera> WorldGraph::GetSpeedCamInfo(Segment const &)
{
  return {};
}

SpeedInUnits WorldGraph::GetSpeedLimit(Segment const &)
{
  return {};
}

void WorldGraph::SetAStarParents(bool forward, Parents<Segment> & parents) {}
void WorldGraph::SetAStarParents(bool forward, Parents<JointSegment> & parents) {}
void WorldGraph::DropAStarParents() {}

bool WorldGraph::AreWavesConnectible(Parents<Segment> & forwardParents, Segment const & commonVertex,
                                     Parents<Segment> & backwardParents)
{
  return true;
}

bool WorldGraph::AreWavesConnectible(Parents<JointSegment> & forwardParents, JointSegment const & commonVertex,
                                     Parents<JointSegment> & backwardParents,
                                     FakeConverterT const & fakeFeatureConverter)
{
  return true;
}

void WorldGraph::SetRoutingOptions(RoutingOptions /* routingOption */) {}

void WorldGraph::ForEachTransition(NumMwmId numMwmId, bool isEnter, TransitionFnT const & fn) {}

CrossMwmGraph & WorldGraph::GetCrossMwmGraph()
{
  UNREACHABLE();
}

RouteWeight WorldGraph::GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
{
  return RouteWeight(0);
}

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
