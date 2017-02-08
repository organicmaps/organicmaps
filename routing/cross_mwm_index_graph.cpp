#include "routing/cross_mwm_index_graph.hpp"

#include "platform/country_file.hpp"

#include "base/macros.hpp"

#include <utility>

namespace
{
void FillTransitionSegments(routing::OsrmFtSegMapping const & segMapping, routing::TWrittenNodeId nodeId,
                            routing::NumMwmId mwmId, std::set<routing::Segment> & transitionSegments)
{
  auto const range = segMapping.GetSegmentsRange(nodeId);
  for (size_t segmentIndex = range.first; segmentIndex != range.second; ++segmentIndex)
  {
    routing::OsrmMappingTypes::FtSeg seg;
    // The meaning of node id in osrm is an edge between two joints.
    // So, it's possible to consider the first valid segment from the range which returns by GetSegmentsRange().
    segMapping.GetSegmentByIndex(segmentIndex, seg);
    if (!seg.IsValid())
      continue;

    CHECK_NOT_EQUAL(seg.m_pointStart, seg.m_pointEnd, ());
    transitionSegments.emplace(seg.m_fid, min(seg.m_pointStart, seg.m_pointEnd), mwmId, seg.IsForward());
    return;
  }
  LOG(LERROR, ("No valid segments in the range returned by OsrmFtSegMapping::GetSegmentsRange(", nodeId,
               "). Num mwm id:", mwmId));
}
}  // namespace

namespace routing
{
bool CrossMwmIndexGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.

  // Checking if a segment |s| is a transition segment by osrm cross mwm sections.
  auto it = m_transitionCache.find(s.GetMwmId());
  if (it == m_transitionCache.cend())
  {
    platform::CountryFile const & countryFile = m_numMwmIds->GetFile(s.GetMwmId());
    TRoutingMappingPtr mappingPtr = m_indexManager.GetMappingByName(countryFile.GetName());
    CHECK(mappingPtr, ("countryFile:", countryFile));
    mappingPtr->LoadCrossContext();

    TransitionSegments transitionSegments;
    mappingPtr->m_crossContext.ForEachOutgoingNode([&](OutgoingCrossNode const & node)
    {
      FillTransitionSegments(mappingPtr->m_segMapping, node.m_nodeId, s.GetMwmId(),
                             transitionSegments.m_outgoing);
    });
    mappingPtr->m_crossContext.ForEachIngoingNode([&](IngoingCrossNode const & node)
    {
      FillTransitionSegments(mappingPtr->m_segMapping, node.m_nodeId, s.GetMwmId(),
                             transitionSegments.m_ingoing);
    });
    auto const p = m_transitionCache.emplace(s.GetMwmId(), transitionSegments);
    it = p.first;
    CHECK(p.second, ("Mwm num id:", s.GetMwmId(), "has been inserted before. countryFile:",
                     countryFile));
  }

  if (isOutgoing)
    return it->second.m_outgoing.count(s) != 0;
  return it->second.m_ingoing.count(s) != 0;
}

void CrossMwmIndexGraph::GetTwins(Segment const & /* s */, std::vector<Segment> & /* twins */) const
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.
  NOTIMPLEMENTED();
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & /* s */,
                                         bool /* isOutgoing */, std::vector<SegmentEdge> & /* edges */) const
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.
  NOTIMPLEMENTED();
}
}  // namespace routing
