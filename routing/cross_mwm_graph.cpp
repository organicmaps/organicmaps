#include "routing/cross_mwm_graph.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/transit_graph.hpp"

#include "indexer/data_source.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

using namespace routing;
using namespace std;

namespace routing
{
CrossMwmGraph::CrossMwmGraph(shared_ptr<NumMwmIds> numMwmIds,
                             shared_ptr<m4::Tree<NumMwmId>> numMwmTree,
                             shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                             VehicleType vehicleType, CourntryRectFn const & countryRectFn,
                             DataSource & dataSource)
  : m_dataSource(dataSource)
  , m_numMwmIds(numMwmIds)
  , m_numMwmTree(numMwmTree)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_countryRectFn(countryRectFn)
  , m_crossMwmIndexGraph(dataSource, numMwmIds, vehicleType)
  , m_crossMwmTransitGraph(dataSource, numMwmIds, VehicleType::Transit)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK_NOT_EQUAL(vehicleType, VehicleType::Transit, ());
}

bool CrossMwmGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  if (TransitGraph::IsTransitSegment(s))
  {
    return TransitCrossMwmSectionExists(s.GetMwmId())
               ? m_crossMwmTransitGraph.IsTransition(s, isOutgoing)
               : false;
  }

  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.IsTransition(s, isOutgoing)
                                             : false;
}

void CrossMwmGraph::GetAllLoadedNeighbors(NumMwmId numMwmId,
                                          vector<NumMwmId> & neighbors,
                                          bool & allNeighborsHaveCrossMwmSection)
{
  CHECK(m_numMwmTree, ());
  m2::RectD const rect = m_countryRectFn(m_numMwmIds->GetFile(numMwmId).GetName());
  allNeighborsHaveCrossMwmSection = true;
  m_numMwmTree->ForEachInRect(rect, [&](NumMwmId id) {
    if (id == numMwmId)
      return;
    MwmStatus const status = GetCrossMwmStatus(id);
    if (status == MwmStatus::NotLoaded)
      return;
    if (status == MwmStatus::NoSection)
      allNeighborsHaveCrossMwmSection = false;

    neighbors.push_back(id);
  });
}

void CrossMwmGraph::DeserializeTransitions(vector<NumMwmId> const & mwmIds)
{
  for (auto mwmId : mwmIds)
    m_crossMwmIndexGraph.LoadCrossMwmConnectorWithTransitions(mwmId);
}

void CrossMwmGraph::DeserializeTransitTransitions(vector<NumMwmId> const & mwmIds)
{
  for (auto mwmId : mwmIds)
  {
    // Some mwms may not have cross mwm transit section.
    if (TransitCrossMwmSectionExists(mwmId))
      m_crossMwmTransitGraph.LoadCrossMwmConnectorWithTransitions(mwmId);
  }
}

void CrossMwmGraph::GetTwins(Segment const & s, bool isOutgoing, vector<Segment> & twins)
{
  CHECK(IsTransition(s, isOutgoing),
        ("The segment", s, "is not a transition segment for isOutgoing ==", isOutgoing));
  // Note. There's an extremely rare case when a segment is ingoing and outgoing at the same time.
  // |twins| is not filled for such cases. For details please see a note in
  // CrossMwmGraph::GetOutgoingEdgeList().
  if (IsTransition(s, !isOutgoing))
    return;

  twins.clear();

  vector<NumMwmId> neighbors;
  bool allNeighborsHaveCrossMwmSection = false;
  GetAllLoadedNeighbors(s.GetMwmId(), neighbors, allNeighborsHaveCrossMwmSection);
  MwmStatus const currentMwmStatus = GetCrossMwmStatus(s.GetMwmId());
  CHECK_NOT_EQUAL(currentMwmStatus, MwmStatus::NotLoaded,
                  ("Current mwm is not loaded. Mwm:", m_numMwmIds->GetFile(s.GetMwmId()),
                   "currentMwmStatus:", currentMwmStatus));

  if (TransitGraph::IsTransitSegment(s) && TransitCrossMwmSectionExists(s.GetMwmId()))
  {
    DeserializeTransitTransitions(neighbors);
    m_crossMwmTransitGraph.GetTwinsByCrossMwmId(s, isOutgoing, neighbors, twins);
  }
  else if (allNeighborsHaveCrossMwmSection && currentMwmStatus == MwmStatus::SectionExists)
  {
    DeserializeTransitions(neighbors);
    m_crossMwmIndexGraph.GetTwinsByCrossMwmId(s, isOutgoing, neighbors, twins);
  }
  else
  {
    // TODO (@gmoryes)
    //  May be we should add ErrorCode about "NeedUpdateMaps" and return it here.
    //  but until we haven't it, lets do nothing.
    return;
  }

  for (Segment const & t : twins)
    CHECK_NOT_EQUAL(s.GetMwmId(), t.GetMwmId(), ());
}

void CrossMwmGraph::GetOutgoingEdgeList(Segment const & s, vector<SegmentEdge> & edges)
{
  CHECK(IsTransition(s, false /* isEnter */),
        ("The segment is not a transition segment. IsTransition(", s, ", false) returns false."));
  edges.clear();

  if (TransitGraph::IsTransitSegment(s))
  {
    if (TransitCrossMwmSectionExists(s.GetMwmId()))
      m_crossMwmTransitGraph.GetOutgoingEdgeList(s, edges);
    return;
  }

  if (CrossMwmSectionExists(s.GetMwmId()))
    m_crossMwmIndexGraph.GetOutgoingEdgeList(s, edges);
}

void CrossMwmGraph::Clear()
{
  m_crossMwmIndexGraph.Clear();
  m_crossMwmTransitGraph.Clear();
}

CrossMwmGraph::MwmStatus CrossMwmGraph::GetMwmStatus(NumMwmId numMwmId,
                                                     string const & sectionName) const
{
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
  if (!handle.IsAlive())
    return MwmStatus::NotLoaded;

  MwmValue * value = handle.GetValue<MwmValue>();
  CHECK(value != nullptr, ("Country file:", m_numMwmIds->GetFile(numMwmId)));
  return value->m_cont.IsExist(sectionName) ? MwmStatus::SectionExists : MwmStatus::NoSection;
}

CrossMwmGraph::MwmStatus CrossMwmGraph::GetCrossMwmStatus(NumMwmId numMwmId) const
{
  if (m_crossMwmIndexGraph.InCache(numMwmId))
    return MwmStatus::SectionExists;
  return GetMwmStatus(numMwmId, CROSS_MWM_FILE_TAG);
}

CrossMwmGraph::MwmStatus CrossMwmGraph::GetTransitCrossMwmStatus(NumMwmId numMwmId) const
{
  if (m_crossMwmTransitGraph.InCache(numMwmId))
    return MwmStatus::SectionExists;
  return GetMwmStatus(numMwmId, TRANSIT_CROSS_MWM_FILE_TAG);
}

bool CrossMwmGraph::CrossMwmSectionExists(NumMwmId numMwmId) const
{
  CrossMwmGraph::MwmStatus const status = GetCrossMwmStatus(numMwmId);
  if (status == MwmStatus::NotLoaded)
    MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(numMwmId), "cannot be loaded."));

  return status == MwmStatus::SectionExists;
}

bool CrossMwmGraph::TransitCrossMwmSectionExists(NumMwmId numMwmId) const
{
  CrossMwmGraph::MwmStatus const status = GetTransitCrossMwmStatus(numMwmId);
  if (status == MwmStatus::NotLoaded)
    MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(numMwmId), "cannot be loaded."));

  return status == MwmStatus::SectionExists;
}

string DebugPrint(CrossMwmGraph::MwmStatus status)
{
  switch (status)
  {
  case CrossMwmGraph::MwmStatus::NotLoaded: return "CrossMwmGraph::NotLoaded";
  case CrossMwmGraph::MwmStatus::SectionExists: return "CrossMwmGraph::SectionExists";
  case CrossMwmGraph::MwmStatus::NoSection: return "CrossMwmGraph::NoSection";
  }
  return string("Unknown CrossMwmGraph::MwmStatus.");
}
}  // namespace routing
