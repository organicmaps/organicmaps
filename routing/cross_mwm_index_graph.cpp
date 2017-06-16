#include "routing/cross_mwm_index_graph.hpp"

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

void CrossMwmIndexGraph::GetTwinsByOsmId(Segment const & s, bool isOutgoing,
                                         vector<NumMwmId> const & neighbors,
                                         vector<Segment> & twins)
{
  uint64_t const osmId = GetCrossMwmConnectorWithTransitions(s.GetMwmId()).GetOsmId(s);

  for (NumMwmId const neighbor : neighbors)
  {
    auto const it = m_connectors.find(neighbor);
    CHECK(it != m_connectors.cend(), ("Connector for", m_numMwmIds->GetFile(neighbor), "was not deserialized."));

    CrossMwmConnector const & connector = it->second;
    // Note. Last parameter in the method below (isEnter) should be set to |isOutgoing|.
    // If |isOutgoing| == true |s| should be an exit transition segment and the method below searches enters
    // and the last parameter (|isEnter|) should be set to true.
    // If |isOutgoing| == false |s| should be an enter transition segment and the method below searches exits
    // and the last parameter (|isEnter|) should be set to false.
    Segment const * twinSeg = connector.GetTransition(osmId, s.GetSegmentIdx(), isOutgoing);
    if (twinSeg == nullptr)
      continue;

    CHECK_NOT_EQUAL(twinSeg->GetMwmId(), s.GetMwmId(), ());
    twins.push_back(*twinSeg);
  }
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
