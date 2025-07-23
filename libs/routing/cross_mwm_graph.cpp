#include "routing/cross_mwm_graph.hpp"

#include "routing/data_source.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/transit_graph.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

namespace routing
{
using namespace std;

CrossMwmGraph::CrossMwmGraph(shared_ptr<NumMwmIds> numMwmIds, shared_ptr<m4::Tree<NumMwmId>> numMwmTree,
                             VehicleType vehicleType, CountryRectFn const & countryRectFn, MwmDataSource & dataSource)
  : m_dataSource(dataSource)
  , m_numMwmIds(numMwmIds)
  , m_numMwmTree(numMwmTree)
  , m_countryRectFn(countryRectFn)
  , m_crossMwmIndexGraph(m_dataSource, vehicleType)
  , m_crossMwmTransitGraph(m_dataSource, VehicleType::Transit)
{
  CHECK(m_numMwmIds, ());
  CHECK_NOT_EQUAL(vehicleType, VehicleType::Transit, ());
}

bool CrossMwmGraph::IsTransition(Segment const & s, bool isOutgoing)
{
  if (TransitGraph::IsTransitSegment(s))
    return TransitCrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmTransitGraph.IsTransition(s, isOutgoing) : false;

  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.IsTransition(s, isOutgoing) : false;
}

bool CrossMwmGraph::GetAllLoadedNeighbors(NumMwmId numMwmId, vector<NumMwmId> & neighbors)
{
  CHECK(m_numMwmTree, ());
  m2::RectD const rect = m_countryRectFn(m_numMwmIds->GetFile(numMwmId).GetName());

  bool allNeighborsHaveCrossMwmSection = true;
  m_numMwmTree->ForEachInRect(rect, [&](NumMwmId id)
  {
    if (id == numMwmId)
      return;
    MwmStatus const status = GetCrossMwmStatus(id);
    if (status == MwmStatus::NotLoaded)
      return;

    if (status == MwmStatus::NoSection)
    {
      allNeighborsHaveCrossMwmSection = false;
      LOG(LWARNING, ("No cross-mwm-section for", m_numMwmIds->GetFile(id)));
    }

    neighbors.push_back(id);
  });

  return allNeighborsHaveCrossMwmSection;
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
  ASSERT(IsTransition(s, isOutgoing), (s, isOutgoing));
  // Note. There's an extremely rare case when a segment is ingoing and outgoing at the same time.
  // |twins| is not filled for such cases. For details please see a note in
  // CrossMwmGraph::GetOutgoingEdgeList().
  if (IsTransition(s, !isOutgoing))
    return;

  twins.clear();

  // If you got ASSERTs here, check that m_numMwmIds and m_numMwmTree are initialized with valid
  // country MWMs only, without World*, minsk-pass, or any other test MWMs.
  // This may happen with ill-formed routing integration tests.
#ifdef DEBUG
  auto const & file = m_numMwmIds->GetFile(s.GetMwmId());
#endif

  vector<NumMwmId> neighbors;
  bool const allNeighborsHaveCrossMwmSection = GetAllLoadedNeighbors(s.GetMwmId(), neighbors);
  ASSERT(allNeighborsHaveCrossMwmSection, (file));

  MwmStatus const currentMwmStatus = GetCrossMwmStatus(s.GetMwmId());
  ASSERT_EQUAL(currentMwmStatus, MwmStatus::SectionExists, (file));

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
    ASSERT(false, ("All country MWMs should have cross-mwm-section", file));
    return;
  }

  for (Segment const & t : twins)
    CHECK_NOT_EQUAL(s.GetMwmId(), t.GetMwmId(), ());
}

void CrossMwmGraph::GetOutgoingEdgeList(Segment const & enter, EdgeListT & edges)
{
  ASSERT(IsTransition(enter, false /* isEnter */),
         ("The segment is not a transition segment. IsTransition(", enter, ", false) returns false."));

  // Is not supported yet.
  /*
  if (TransitGraph::IsTransitSegment(enter))
  {
    if (TransitCrossMwmSectionExists(enter.GetMwmId()))
      m_crossMwmTransitGraph.GetOutgoingEdgeList(enter, edges);
    return;
  }
  */

  if (CrossMwmSectionExists(enter.GetMwmId()))
    m_crossMwmIndexGraph.GetOutgoingEdgeList(enter, edges);
}

void CrossMwmGraph::GetIngoingEdgeList(Segment const & exit, EdgeListT & edges)
{
  ASSERT(IsTransition(exit, true /* isEnter */),
         ("The segment is not a transition segment. IsTransition(", exit, ", true) returns false."));

  // Is not supported yet.
  /*
  if (TransitGraph::IsTransitSegment(enter))
  {
    if (TransitCrossMwmSectionExists(enter.GetMwmId()))
      m_crossMwmTransitGraph.GetIngoingEdgeList(enter, edges);
    return;
  }
  */

  if (CrossMwmSectionExists(exit.GetMwmId()))
    m_crossMwmIndexGraph.GetIngoingEdgeList(exit, edges);
}

RouteWeight CrossMwmGraph::GetWeightSure(Segment const & from, Segment const & to)
{
  ASSERT_EQUAL(from.GetMwmId(), to.GetMwmId(), ());
  ASSERT(CrossMwmSectionExists(from.GetMwmId()), ());
  return m_crossMwmIndexGraph.GetWeightSure(from, to);
}

// void CrossMwmGraph::Clear()
//{
//   m_crossMwmIndexGraph.Clear();
//   m_crossMwmTransitGraph.Clear();
// }

void CrossMwmGraph::Purge()
{
  m_crossMwmIndexGraph.Purge();
  m_crossMwmTransitGraph.Purge();
}

void CrossMwmGraph::GetTwinFeature(Segment const & segment, bool isOutgoing, vector<Segment> & twins)
{
  m_crossMwmIndexGraph.ForEachTransitSegmentId(segment.GetMwmId(), segment.GetFeatureId(),
                                               [&](uint32_t transitSegmentId)
  {
    Segment const transitSegment(segment.GetMwmId(), segment.GetFeatureId(), transitSegmentId, segment.IsForward());

    if (IsTransition(transitSegment, isOutgoing))
    {
      // Get twins and exit.
      GetTwins(transitSegment, isOutgoing, twins);
      return true;
    }
    return false;
  });
}

CrossMwmGraph::MwmStatus CrossMwmGraph::GetMwmStatus(NumMwmId numMwmId, string const & sectionName) const
{
  switch (m_dataSource.GetSectionStatus(numMwmId, sectionName))
  {
  case MwmDataSource::MwmNotLoaded: return MwmStatus::NotLoaded;
  case MwmDataSource::SectionExists: return MwmStatus::SectionExists;
  case MwmDataSource::NoSection: return MwmStatus::NoSection;
  }
  CHECK(false, ("Unreachable"));
  return MwmStatus::NoSection;
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
