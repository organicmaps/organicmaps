#include "routing/cross_mwm_index_graph.hpp"

#include "platform/country_file.hpp"

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
    segment = Segment(seg.m_fid, min(seg.m_pointStart, seg.m_pointEnd), numMwmId, seg.IsForward());
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
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.
  auto segMapping = GetRoutingMapping(s.GetMwmId());
  TRoutingMappingPtr const segMwmMapping = segMapping.first;
  CHECK(segMwmMapping, ("Mwm", s.GetMwmId(), "has not been downloaded. s:", s, ". isOutgoing", isOutgoing));

  twins.clear();
  vector<string> const & neighboringMwm = segMwmMapping->m_crossContext.GetNeighboringMwmList();

  for (string const & name : neighboringMwm)
    InsertWholeMwmTransitionSegments(m_numMwmIds->GetId(CountryFile(name)));

  auto it = m_transitionCache.find(s.GetMwmId());
  CHECK(it != m_transitionCache.cend(), ());

  ms::LatLon const & latLon = isOutgoing ? GetLatLon(it->second.m_outgoing, s)
                                         : GetLatLon(it->second.m_ingoing, s);
  for (string const & name : neighboringMwm)
  {
    NumMwmId const numMwmId = m_numMwmIds->GetId(CountryFile(name));
    auto mapping = GetRoutingMapping(numMwmId);
    TRoutingMappingPtr const mappingPtr = mapping.first;

    if (mappingPtr == nullptr)
      continue;  // mwm was not loaded.

    if (isOutgoing)
    {
      mappingPtr->m_crossContext.ForEachIngoingNodeNearPoint(latLon, [&](IngoingCrossNode const & node){
        AddTransitionSegment(mappingPtr->m_segMapping, node.m_nodeId, numMwmId, twins);
      });
    }
    else
    {
      mappingPtr->m_crossContext.ForEachOutgoingNodeNearPoint(latLon, [&](OutgoingCrossNode const & node){
        AddTransitionSegment(mappingPtr->m_segMapping, node.m_nodeId, numMwmId, twins);
      });
    }
  }

  my::SortUnique(twins);
}

void CrossMwmIndexGraph::GetEdgeList(Segment const & /* s */,
                                     bool /* isOutgoing */, std::vector<SegmentEdge> & /* edges */) const
{
  // @TODO(bykoianko) It's necessary to check if mwm of |s| contains an A* cross mwm section
  // and if so to use it. If not, osrm cross mwm sections should be used.
  NOTIMPLEMENTED();
}

pair<TRoutingMappingPtr, unique_ptr<MappingGuard>> CrossMwmIndexGraph::GetRoutingMapping(NumMwmId numMwmId)
{
  CountryFile const & countryFile = m_numMwmIds->GetFile(numMwmId);
  TRoutingMappingPtr mappingPtr = m_indexManager.GetMappingByName(countryFile.GetName());
  CHECK(mappingPtr, ("No routing mapping file for countryFile:", countryFile));
  auto mappingPtrGuard = make_unique<MappingGuard>(mappingPtr);

  if (!mappingPtr->IsValid())
    return make_pair(nullptr, nullptr); // mwm was not loaded.
  mappingPtr->LoadCrossContext();
  return make_pair(mappingPtr, move(mappingPtrGuard));
}

void CrossMwmIndexGraph::Clear()
{
  m_transitionCache.clear();
}

void CrossMwmIndexGraph::InsertWholeMwmTransitionSegments(NumMwmId numMwmId)
{
  if (m_transitionCache.count(numMwmId) != 0)
    return;

  auto mapping = GetRoutingMapping(numMwmId);
  TRoutingMappingPtr const mappingPtr = mapping.first;

  if (mappingPtr == nullptr)
  {
    m_transitionCache.emplace(numMwmId, TransitionSegments());
    return;
  }

  TransitionSegments transitionSegments;
  mappingPtr->m_crossContext.ForEachOutgoingNode([&](OutgoingCrossNode const & node)
  {
    FillTransitionSegments(mappingPtr->m_segMapping, node.m_nodeId, numMwmId,
                           node.m_point, transitionSegments.m_outgoing);
  });
  mappingPtr->m_crossContext.ForEachIngoingNode([&](IngoingCrossNode const & node)
  {
    FillTransitionSegments(mappingPtr->m_segMapping, node.m_nodeId, numMwmId,
                           node.m_point, transitionSegments.m_ingoing);
  });
  auto const p = m_transitionCache.emplace(numMwmId, move(transitionSegments));
  ASSERT(p.second, ("Mwm num id:", numMwmId, "has been inserted before. Country file name:",
                    mappingPtr->GetCountryName()));
}
}  // namespace routing
