#include "routing/cross_mwm_index_graph.hpp"

#include "routing/cross_mwm_road_graph.hpp"
#include "routing/osrm_path_segment_factory.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

using namespace platform;
using namespace routing;
using namespace std;

namespace
{
struct NodeIds
{
  TOsrmNodeId m_directNodeId;
  TOsrmNodeId m_reverseNodeId;
};

/// \returns FtSeg by |segment|.
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
    // So, it's possible to consider the first valid segment from the range which returns by GetSegmentsRange().
    segMapping.GetSegmentByIndex(segmentIndex, seg);

    if (!seg.IsValid())
      continue;

    CHECK_NOT_EQUAL(seg.m_pointStart, seg.m_pointEnd, ());
    segment = Segment(numMwmId, seg.m_fid, min(seg.m_pointStart, seg.m_pointEnd), seg.IsForward());
    return true;
  }
  LOG(LDEBUG, ("No valid segments in the range returned by OsrmFtSegMapping::GetSegmentsRange(",
               nodeId, "). Num mwm id:", numMwmId));
  return false;
}

void AddFirstValidSegment(OsrmFtSegMapping const & segMapping, NumMwmId numMwmId,
                          TWrittenNodeId nodeId, vector<Segment> & segments)
{
  Segment key;
  if (GetFirstValidSegment(segMapping, numMwmId, nodeId, key))
    segments.push_back(key);
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

vector<ms::LatLon> const & GetLatLon(map<Segment, vector<ms::LatLon>> const & segMap,
                                     Segment const & s)
{
  auto it = segMap.find(s);
  CHECK(it != segMap.cend(), ());
  return it->second;
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
  // This factor makes index graph (used in AStar) edge weight smaller then node ids weight.
  //
  // As a result large cross mwm routes with connectors works as Dijkstra, but short and medium routes
  // without connectors works as AStar.
  // Most of routes don't use leaps, therefore it is important to keep AStar performance.
  double constexpr kAstarHeuristicFactor = 100000;
  edges.emplace_back(segment,
                     osrmEdge.GetWeight() * kOSRMWeightToSecondsMultiplier * kAstarHeuristicFactor);
}
}  // namespace

namespace routing
{
CrossMwmIndexGraph::CrossMwmIndexGraph(shared_ptr<NumMwmIds> numMwmIds,
                                       RoutingIndexManager & indexManager)
  : m_indexManager(indexManager), m_numMwmIds(numMwmIds)
{
  ResetCrossMwmGraph();
}

CrossMwmIndexGraph::~CrossMwmIndexGraph() {}
bool CrossMwmIndexGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.

  // Checking if a segment |s| is a transition segment by osrm cross mwm sections.
  TransitionSegments const & t = GetSegmentMaps(s.GetMwmId());

  if (isOutgoing)
    return t.m_outgoing.count(s) != 0;
  return t.m_ingoing.count(s) != 0;
}

void CrossMwmIndexGraph::GetTwins(Segment const & s, bool isOutgoing, vector<Segment> & twins)
{
  CHECK(IsTransition(s, isOutgoing), ("The segment", s, "is not a transition segment for isOutgoing ==", isOutgoing));
  // Note. There's an extremely rare case when a segment is ingoing and outgoing at the same time.
  // |twins| is not filled for such cases. For details please see a note in
  // rossMwmIndexGraph::GetEdgeList().
  if (IsTransition(s, !isOutgoing))
    return;

  twins.clear();
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.

  auto const getTwins = [&](TRoutingMappingPtr const & segMapping) {
    vector<string> const & neighboringMwm = segMapping->m_crossContext.GetNeighboringMwmList();

    for (string const & name : neighboringMwm)
      InsertWholeMwmTransitionSegments(m_numMwmIds->GetId(CountryFile(name)));

    auto it = m_transitionCache.find(s.GetMwmId());
    CHECK(it != m_transitionCache.cend(), ());

    vector<ms::LatLon> const & latLons =
        isOutgoing ? GetLatLon(it->second.m_outgoing, s) : GetLatLon(it->second.m_ingoing, s);
    for (string const & name : neighboringMwm)
    {
      NumMwmId const numMwmId = m_numMwmIds->GetId(CountryFile(name));
      auto const addFirstValidSegment = [&](TRoutingMappingPtr const & mapping) {
        for (auto const & ll : latLons)
        {
          if (isOutgoing)
          {
            mapping->m_crossContext.ForEachIngoingNodeNearPoint(
                ll, [&](IngoingCrossNode const & node) {
                  AddFirstValidSegment(mapping->m_segMapping, numMwmId, node.m_nodeId, twins);
                });
          }
          else
          {
            mapping->m_crossContext.ForEachOutgoingNodeNearPoint(
                ll, [&](OutgoingCrossNode const & node) {
                  AddFirstValidSegment(mapping->m_segMapping, numMwmId, node.m_nodeId, twins);
                });
          }
        }
      };

      if (!LoadWith(numMwmId, addFirstValidSegment))
        continue;  // mwm was not loaded.
    }
  };

  LoadWith(s.GetMwmId(), getTwins);
  my::SortUnique(twins);

  for (Segment const & t : twins)
    CHECK_NOT_EQUAL(s.GetMwmId(), t.GetMwmId(), ());
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & s, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.

  CHECK(IsTransition(s, !isOutgoing), ("The segment is not a transition segment. IsTransition(",
                                       s, ",", !isOutgoing, ") returns false."));

  // Note. According to cross-mwm OSRM sections there're node id which could be ingoing and outgoing
  // at the same time. For example in Berlin mwm on Nordlicher Berliner Ring (A10) near crossing
  // with A11
  // there's such node id. It's an extremely rare case. There're probably several such node id
  // for the whole Europe. Such cases are not processed in WorldGraph::GetEdgeList() for the time
  // being.
  // To prevent filling |edges| with twins instead of leap edges and vice versa in
  // WorldGraph::GetEdgeList()
  // CrossMwmIndexGraph::GetEdgeList() does not fill |edges| if |s| is a transition segment which
  // corresponces node id described above.
  if (IsTransition(s, isOutgoing))
    return;

  edges.clear();
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

void CrossMwmIndexGraph::Clear()
{
  ResetCrossMwmGraph();
  m_transitionCache.clear();
  m_mappingGuards.clear();
}

void CrossMwmIndexGraph::ResetCrossMwmGraph()
{
  m_crossMwmGraph = make_unique<CrossMwmGraph>(m_indexManager);
}

void CrossMwmIndexGraph::InsertWholeMwmTransitionSegments(NumMwmId numMwmId)
{
  if (m_transitionCache.count(numMwmId) != 0)
    return;

  auto const fillAllTransitionSegments = [&](TRoutingMappingPtr const & mapping) {
    TransitionSegments transitionSegments;
    mapping->m_crossContext.ForEachOutgoingNode([&](OutgoingCrossNode const & node)
    {
      FillTransitionSegments(mapping->m_segMapping, node.m_nodeId, numMwmId,
                             node.m_point, transitionSegments.m_outgoing);
    });
    mapping->m_crossContext.ForEachIngoingNode([&](IngoingCrossNode const & node)
    {
      FillTransitionSegments(mapping->m_segMapping, node.m_nodeId, numMwmId,
                             node.m_point, transitionSegments.m_ingoing);
    });
    auto const p = m_transitionCache.emplace(numMwmId, move(transitionSegments));
    UNUSED_VALUE(p);
    ASSERT(p.second, ("Mwm num id:", numMwmId, "has been inserted before. Country file name:",
                      mapping->GetCountryName()));
  };

  if (!LoadWith(numMwmId, fillAllTransitionSegments))
    m_transitionCache.emplace(numMwmId, TransitionSegments());
}

void CrossMwmIndexGraph::GetBorderCross(TRoutingMappingPtr const & mapping, Segment const & s,
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

  // outgoing edge
  vector<Segment> twins;
  GetTwins(s, !isOutgoing, twins);
  CHECK(!twins.empty(), ("Segment:", s, ", isOutgoing:", isOutgoing));
  for (Segment const & twin : twins)
  {
    // If |isOutgoing| == true |otherSideMapping| is mapping outgoing (to) border cross.
    // If |isOutgoing| == false |mapping| is mapping ingoing (from) border cross.
    auto const fillBorderCrossOut = [&](TRoutingMappingPtr const & otherSideMapping) {
      NodeIds const nodeIdsFrom = GetDirectAndReverseNodeId(otherSideMapping->m_segMapping, twin);
      vector<ms::LatLon> const & otherSideTransitionPoints =
          isOutgoing ? GetOutgoingTransitionPoints(twin) : GetIngoingTransitionPoints(twin);

      CHECK(!otherSideTransitionPoints.empty(), ("Segment:", s, ", isOutgoing:", isOutgoing));
      for (CrossNode const & node : nodes)
      {
        // If |isOutgoing| == true |otherSideNodes| is "from" cross nodes, otherwise
        // |otherSideNodes| is "to" cross nodes.
        BorderCross bc;
        if (isOutgoing)
          bc.toNode = node;
        else
          bc.fromNode = node;

        for (ms::LatLon const & ll : otherSideTransitionPoints)
        {
          CrossNode & resultNode = isOutgoing ? bc.fromNode : bc.toNode;
          resultNode = CrossNode(nodeIdsFrom.m_directNodeId, nodeIdsFrom.m_reverseNodeId,
                                 otherSideMapping->GetMwmId(), ll);
          borderCrosses.push_back(bc);
        }
      }
    };
    LoadWith(twin.GetMwmId(), fillBorderCrossOut);
  }
}

CrossMwmIndexGraph::TransitionSegments const & CrossMwmIndexGraph::GetSegmentMaps(NumMwmId numMwmId)
{
  auto it = m_transitionCache.find(numMwmId);
  if (it == m_transitionCache.cend())
  {
    InsertWholeMwmTransitionSegments(numMwmId);
    it = m_transitionCache.find(numMwmId);
  }
  CHECK(it != m_transitionCache.cend(), ("Mwm ", numMwmId, "has not been downloaded."));
  return it->second;
}

vector<ms::LatLon> const & CrossMwmIndexGraph::GetIngoingTransitionPoints(Segment const & s)
{
  auto const & ingoingSeg = GetSegmentMaps(s.GetMwmId()).m_ingoing;
  auto const it = ingoingSeg.find(s);
  CHECK(it != ingoingSeg.cend(), ("Segment:", s));
  return it->second;
}

vector<ms::LatLon> const & CrossMwmIndexGraph::GetOutgoingTransitionPoints(Segment const & s)
{
  auto const & outgoingSeg = GetSegmentMaps(s.GetMwmId()).m_outgoing;
  auto const it = outgoingSeg.find(s);
  CHECK(it != outgoingSeg.cend(), ("Segment:", s));
  return it->second;
}
}  // namespace routing
