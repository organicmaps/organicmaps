#include "routing/cross_mwm_index_graph.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

using namespace platform;
using namespace routing;
using namespace std;

namespace
{
bool GetTransitionSegment(OsrmFtSegMapping const & segMapping, TWrittenNodeId nodeId,
                          NumMwmId numMwmId, Segment & segment)
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
  LOG(LERROR, ("No valid segments in the range returned by OsrmFtSegMapping::GetSegmentsRange(", nodeId,
               "). Num mwm id:", numMwmId));
  return false;
}

void AddTransitionSegment(OsrmFtSegMapping const & segMapping, TWrittenNodeId nodeId,
                          NumMwmId numMwmId, std::vector<Segment> & segments)
{
  Segment key;
  if (GetTransitionSegment(segMapping, nodeId, numMwmId, key))
    segments.push_back(key);
}

void FillTransitionSegments(OsrmFtSegMapping const & segMapping, TWrittenNodeId nodeId,
                            NumMwmId numMwmId, ms::LatLon const & latLon,
                            std::map<Segment, ms::LatLon> & transitionSegments)
{
  Segment key;
  if (!GetTransitionSegment(segMapping, nodeId, numMwmId, key))
    return;

  auto const p = transitionSegments.emplace(key, latLon);
  if (p.second)
    return;

  // @TODO(bykoianko) It's necessary to investigate this case.
  LOG(LWARNING, ("Trying to insert a transition segment that was previously inserted. The key:",
                 *p.first, ", the value:", latLon));
}

ms::LatLon const & GetLatLon(std::map<Segment, ms::LatLon> const & segMap, Segment const & s)
{
  auto it = segMap.find(s);
  CHECK(it != segMap.cend(), ());
  return it->second;
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
    InsertWholeMwmTransitionSegments(s.GetMwmId());
    it = m_transitionCache.find(s.GetMwmId());
  }
  CHECK(it != m_transitionCache.cend(),
        ("Mwm ", s.GetMwmId(), "has not been downloaded. s:", s, ". isOutgoing", isOutgoing));

  if (isOutgoing)
    return it->second.m_outgoing.count(s) != 0;
  return it->second.m_ingoing.count(s) != 0;
}

void CrossMwmIndexGraph::GetTwins(Segment const & s, bool isOutgoing, std::vector<Segment> & twins)
{
  CHECK(IsTransition(s, isOutgoing), ("The segment is not a transition segment."));
  twins.clear();
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.

  auto const getTwins = [&](NumMwmId /* numMwmId */, TRoutingMappingPtr const & segMapping)
  {
    vector<string> const & neighboringMwm = segMapping->m_crossContext.GetNeighboringMwmList();

    for (string const & name : neighboringMwm)
      InsertWholeMwmTransitionSegments(m_numMwmIds->GetId(CountryFile(name)));

    auto it = m_transitionCache.find(s.GetMwmId());
    CHECK(it != m_transitionCache.cend(), ());

    ms::LatLon const & latLon = isOutgoing ? GetLatLon(it->second.m_outgoing, s)
                                           : GetLatLon(it->second.m_ingoing, s);
    for (string const & name : neighboringMwm)
    {
      auto const addTransitionSegments = [&](NumMwmId numMwmId, TRoutingMappingPtr const & mapping)
      {
        if (isOutgoing)
        {
          mapping->m_crossContext.ForEachIngoingNodeNearPoint(latLon, [&](IngoingCrossNode const & node){
            AddTransitionSegment(mapping->m_segMapping, node.m_nodeId, numMwmId, twins);
          });
        }
        else
        {
          mapping->m_crossContext.ForEachOutgoingNodeNearPoint(latLon, [&](OutgoingCrossNode const & node){
            AddTransitionSegment(mapping->m_segMapping, node.m_nodeId, numMwmId, twins);
          });
        }
      };

      if (!LoadWith(m_numMwmIds->GetId(CountryFile(name)), addTransitionSegments))
        continue;  // mwm was not loaded.
    }
  };

  LoadWith(s.GetMwmId(), getTwins);
  my::SortUnique(twins);
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & /* s */,
                                     bool /* isOutgoing */, std::vector<SegmentEdge> & /* edges */) const
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.
  NOTIMPLEMENTED();
}

void CrossMwmIndexGraph::Clear()
{
  m_transitionCache.clear();
}

void CrossMwmIndexGraph::InsertWholeMwmTransitionSegments(NumMwmId numMwmId)
{
  if (m_transitionCache.count(numMwmId) != 0)
    return;

  auto const fillAllTransitionSegments = [this](NumMwmId numMwmId, TRoutingMappingPtr const & mapping){
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
}  // namespace routing
