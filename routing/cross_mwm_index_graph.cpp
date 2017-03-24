#include "routing/cross_mwm_road_graph.hpp"
#include "routing/cross_mwm_connector_serialization.hpp"

using namespace std;

namespace routing
{
bool CrossMwmIndexGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithTransitions(s.GetMwmId());
  return c.IsTransition(s, isOutgoing);
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & s, bool isOutgoing, vector<SegmentEdge> & edges)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithWeights(s.GetMwmId());
  c.GetEdgeList(s, isOutgoing, edges);
}

CrossMwmConnector const & CrossMwmIndexGraph::GetCrossMwmConnectorWithTransitions(NumMwmId numMwmId)
{
  auto const it = m_connectors.find(numMwmId);
  if (it != m_connectors.cend())
    return it->second;

  return Deserialize(numMwmId,
                     CrossMwmConnectorSerializer::DeserializeTransitions<ReaderSource<FilesContainerR::TReader>>);
}

CrossMwmConnector const & CrossMwmIndexGraph::GetCrossMwmConnectorWithWeights(NumMwmId numMwmId)
{
  CrossMwmConnector const & c = GetCrossMwmConnectorWithTransitions(numMwmId);
  if (c.WeightsWereLoaded())
    return c;

  return Deserialize(numMwmId,
                     CrossMwmConnectorSerializer::DeserializeWeights<ReaderSource<FilesContainerR::TReader>>);
}
}  // namespace routing
