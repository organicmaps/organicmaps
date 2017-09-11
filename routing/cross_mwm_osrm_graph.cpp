#include "routing/cross_mwm_osrm_graph.hpp"
#include "routing/cross_mwm_road_graph.hpp"

#include "base/stl_helpers.hpp"

using namespace routing;
using namespace std;

namespace
{
struct NodeIds
{
  TOsrmNodeId m_directNodeId;
  TOsrmNodeId m_reverseNodeId;
};

OsrmMappingTypes::FtSeg GetFtSeg(Segment const & segment)
{
  return OsrmMappingTypes::FtSeg(segment.GetFeatureId(), segment.GetMinPointId(),
                                 segment.GetMaxPointId());
}

/// \returns a pair of direct and reverse(backward) node id by |segment|.
/// The first member of the pair is direct node id and the second is reverse node id.
NodeIds GetDirectAndReverseNodeId(OsrmFtSegMapping const & segMapping, Segment const & segment)
{
  OsrmFtSegMapping::TFtSegVec const ftSegs = {GetFtSeg(segment)};
  OsrmFtSegMapping::OsrmNodesT osrmNodes;
  segMapping.GetOsrmNodes(ftSegs, osrmNodes);
  CHECK_LESS(osrmNodes.size(), 2, ());
  if (osrmNodes.empty())
    return {INVALID_NODE_ID, INVALID_NODE_ID};

  NodeIds const forwardNodeIds = {osrmNodes.begin()->second.first,
                                  osrmNodes.begin()->second.second};
  NodeIds const backwardNodeIds = {osrmNodes.begin()->second.second,
                                   osrmNodes.begin()->second.first};

  return segment.IsForward() ? forwardNodeIds : backwardNodeIds;
}

bool GetFirstValidSegment(OsrmFtSegMapping const & segMapping, NumMwmId numMwmId,
                          TWrittenNodeId nodeId, Segment & segment)
{
  auto const range = segMapping.GetSegmentsRange(nodeId);
  for (size_t segmentIndex = range.first; segmentIndex != range.second; ++segmentIndex)
  {
    OsrmMappingTypes::FtSeg seg;
    // The meaning of node id in osrm is an edge between two joints.
    // So, it's possible to consider the first valid segment from the range which returns by
    // GetSegmentsRange().
    segMapping.GetSegmentByIndex(segmentIndex, seg);

    if (seg.IsValid())
    {
      CHECK_NOT_EQUAL(seg.m_pointStart, seg.m_pointEnd, ());
      segment = Segment(numMwmId, seg.m_fid, min(seg.m_pointStart, seg.m_pointEnd), seg.IsForward());
      return true;
    }
  }
  LOG(LDEBUG, ("No valid segments in the range returned by OsrmFtSegMapping::GetSegmentsRange(",
               nodeId, "). Num mwm id:", numMwmId));
  return false;
}

void FillTransitionSegments(OsrmFtSegMapping const & segMapping, TWrittenNodeId nodeId,
                            NumMwmId numMwmId, ms::LatLon const & latLon,
                            map<Segment, vector<ms::LatLon>> & transitionSegments)
{
  Segment key;
  if (!GetFirstValidSegment(segMapping, numMwmId, nodeId, key))
    return;

  transitionSegments[key].push_back(latLon);
}

void AddSegmentEdge(NumMwmIds const & numMwmIds, OsrmFtSegMapping const & segMapping,
                    CrossWeightedEdge const & osrmEdge, bool isOutgoing, NumMwmId numMwmId,
                    vector<SegmentEdge> & edges)
{
  BorderCross const & target = osrmEdge.GetTarget();
  CrossNode const & crossNode = isOutgoing ? target.fromNode : target.toNode;

  if (!crossNode.mwmId.IsAlive())
    return;

  NumMwmId const crossNodeMwmId =
      numMwmIds.GetId(crossNode.mwmId.GetInfo()->GetLocalFile().GetCountryFile());
  CHECK_EQUAL(numMwmId, crossNodeMwmId, ());

  Segment segment;
  if (!GetFirstValidSegment(segMapping, crossNodeMwmId, crossNode.node, segment))
    return;

  // OSRM and AStar have different car models, therefore AStar heuristic doesn't work for OSRM node
  // ids (edges).
  // This factor makes index graph (used in AStar) edge weight smaller than node ids weight.
  //
  // As a result large cross mwm routes with connectors works as Dijkstra, but short and medium
  // routes
  // without connectors works as AStar.
  // Most of routes don't use leaps, therefore it is important to keep AStar performance.
  double constexpr kAstarHeuristicFactor = 100000;
  //// Osrm multiplies seconds to 10, so we need to divide it back.
  double constexpr kOSRMWeightToSecondsMultiplier = 0.1;
  edges.emplace_back(
      segment, RouteWeight::FromCrossMwmWeight(
                   osrmEdge.GetWeight() * kOSRMWeightToSecondsMultiplier * kAstarHeuristicFactor));
}
}  // namespace

namespace routing
{
CrossMwmOsrmGraph::CrossMwmOsrmGraph(shared_ptr<NumMwmIds> numMwmIds,
                                     RoutingIndexManager & indexManager)
  : m_numMwmIds(numMwmIds), m_indexManager(indexManager)
{
  Clear();
}

CrossMwmOsrmGraph::~CrossMwmOsrmGraph() {}

bool CrossMwmOsrmGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  TransitionSegments const & t = GetSegmentMaps(s.GetMwmId());

  if (isOutgoing)
    return t.m_outgoing.count(s) != 0;
  return t.m_ingoing.count(s) != 0;
}

void CrossMwmOsrmGraph::GetEdgeList(Segment const & s, bool isOutgoing, vector<SegmentEdge> & edges)
{
  // Note. According to cross-mwm OSRM sections there is a node id that could be ingoing and
  // outgoing
  // at the same time. For example in Berlin mwm on Nordlicher Berliner Ring (A10) near crossing
  // with A11 there's such node id. It's an extremely rare case. There're probably several such node
  // id for the whole Europe. Such cases are not processed in WorldGraph::GetEdgeList() for the time
  // being. To prevent filling |edges| with twins instead of leap edges and vice versa in
  // WorldGraph::GetEdgeList().
  // CrossMwmGraph::GetEdgeList() does not fill |edges| if |s| is a transition segment which
  // corresponces node id described above.
  if (IsTransition(s, isOutgoing))
    return;

  auto const fillEdgeList = [&](TRoutingMappingPtr const & mapping) {
    vector<BorderCross> borderCrosses;
    GetBorderCross(mapping, s, isOutgoing, borderCrosses);

    for (BorderCross const & v : borderCrosses)
    {
      vector<CrossWeightedEdge> adj;
      if (isOutgoing)
        m_crossMwmGraph->GetOutgoingEdgesList(v, adj);
      else
        m_crossMwmGraph->GetIngoingEdgesList(v, adj);

      for (CrossWeightedEdge const & edge : adj)
        AddSegmentEdge(*m_numMwmIds, mapping->m_segMapping, edge, isOutgoing, s.GetMwmId(), edges);
    }
  };

  LoadWith(s.GetMwmId(), fillEdgeList);
  my::SortUnique(edges);
}

void CrossMwmOsrmGraph::Clear()
{
  m_crossMwmGraph = make_unique<CrossMwmRoadGraph>(m_indexManager);
  m_transitionCache.clear();
  m_mappingGuards.clear();
}

TransitionPoints CrossMwmOsrmGraph::GetTransitionPoints(Segment const & s, bool isOutgoing)
{
  vector<ms::LatLon> const & latLons =
      isOutgoing ? GetOutgoingTransitionPoints(s) : GetIngoingTransitionPoints(s);
  TransitionPoints points;
  points.reserve(latLons.size());
  for (auto const & latLon : latLons)
    points.push_back(MercatorBounds::FromLatLon(latLon));
  return points;
}

CrossMwmOsrmGraph::TransitionSegments const & CrossMwmOsrmGraph::LoadSegmentMaps(NumMwmId numMwmId)
{
  auto it = m_transitionCache.find(numMwmId);
  if (it != m_transitionCache.cend())
    return it->second;

  auto const fillAllTransitionSegments = [&](TRoutingMappingPtr const & mapping) {
    TransitionSegments transitionSegments;
    mapping->m_crossContext.ForEachOutgoingNode([&](OutgoingCrossNode const & node) {
      FillTransitionSegments(mapping->m_segMapping, node.m_nodeId, numMwmId, node.m_point,
                             transitionSegments.m_outgoing);
    });
    mapping->m_crossContext.ForEachIngoingNode([&](IngoingCrossNode const & node) {
      FillTransitionSegments(mapping->m_segMapping, node.m_nodeId, numMwmId, node.m_point,
                             transitionSegments.m_ingoing);
    });
    auto const p = m_transitionCache.emplace(numMwmId, move(transitionSegments));
    UNUSED_VALUE(p);
    ASSERT(p.second, ("Mwm num id:", numMwmId, "has been inserted before. Country file name:",
                      mapping->GetCountryName()));
    it = p.first;
  };

  LoadWith(numMwmId, fillAllTransitionSegments);
  return it->second;
}

void CrossMwmOsrmGraph::GetBorderCross(TRoutingMappingPtr const & mapping, Segment const & s,
                                       bool isOutgoing, vector<BorderCross> & borderCrosses)
{
  // ingoing edge
  NodeIds const nodeIdsTo = GetDirectAndReverseNodeId(mapping->m_segMapping, s);

  vector<ms::LatLon> const & transitionPoints =
      isOutgoing ? GetIngoingTransitionPoints(s) : GetOutgoingTransitionPoints(s);
  CHECK(!transitionPoints.empty(), ("Segment:", s, ", isOutgoing:", isOutgoing));

  // If |isOutgoing| == true |nodes| is "to" cross nodes, otherwise |nodes| is "from" cross nodes.
  vector<CrossNode> nodes;
  for (ms::LatLon const & p : transitionPoints)
    nodes.emplace_back(nodeIdsTo.m_directNodeId, nodeIdsTo.m_reverseNodeId, mapping->GetMwmId(), p);

  for (CrossNode const & node : nodes)
  {
    // If |isOutgoing| == true |otherSideNodes| is "from" cross nodes, otherwise
    // |otherSideNodes| is "to" cross nodes.
    BorderCross bc;
    if (isOutgoing)
      bc.toNode = node;
    else
      bc.fromNode = node;
    borderCrosses.push_back(bc);
  }
}

CrossMwmOsrmGraph::TransitionSegments const & CrossMwmOsrmGraph::GetSegmentMaps(NumMwmId numMwmId)
{
  auto it = m_transitionCache.find(numMwmId);
  if (it == m_transitionCache.cend())
    return LoadSegmentMaps(numMwmId);
  return it->second;
}

vector<ms::LatLon> const & CrossMwmOsrmGraph::GetIngoingTransitionPoints(Segment const & s)
{
  auto const & ingoingSeg = GetSegmentMaps(s.GetMwmId()).m_ingoing;
  auto const it = ingoingSeg.find(s);
  CHECK(it != ingoingSeg.cend(), ("Segment:", s));
  return it->second;
}

vector<ms::LatLon> const & CrossMwmOsrmGraph::GetOutgoingTransitionPoints(Segment const & s)
{
  auto const & outgoingSeg = GetSegmentMaps(s.GetMwmId()).m_outgoing;
  auto const it = outgoingSeg.find(s);
  CHECK(it != outgoingSeg.cend(), ("Segment:", s));
  return it->second;
}
}  // namespace routing
