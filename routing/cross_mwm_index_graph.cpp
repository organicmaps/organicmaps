#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/cross_mwm_road_graph.hpp"

using namespace std;

namespace routing
{
bool CrossMwmIndexGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithTransitions(s.GetMwmId());
  return c.IsTransition(s, isOutgoing);
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & s, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithWeights(s.GetMwmId());
  c.GetEdgeList(s, isOutgoing, edges);
}

CrossMwmConnector const & CrossMwmIndexGraph::GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
{
  auto const it = m_connectors.find(numMwmId);
  if (it != m_connectors.cend())
    return it->second;

  return Deserialize(
      numMwmId,
      CrossMwmConnectorSerializer::DeserializeTransitions<ReaderSourceFile>);
}

CrossMwmConnector const & CrossMwmIndexGraph::GetCrossMwmConnectorWithWeights(NumMwmId numMwmId)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithTransitions(numMwmId);
  if (c.WeightsWereLoaded())
    return c;

  return Deserialize(
      numMwmId,
      CrossMwmConnectorSerializer::DeserializeWeights<ReaderSourceFile>);
}

TransitionPoints CrossMwmIndexGraph::GetTransitionPoints(Segment const & s, bool isOutgoing)
{
  CrossMwmConnector const & connector = GetCrossMwmConnectorWithTransitions(s.GetMwmId());
  // In case of transition segments of index graph cross-mwm section the front point of segment
  // is used as a point which corresponds to the segment.
  return TransitionPoints({connector.GetPoint(s, true /* front */)});
}
}  // namespace routing
