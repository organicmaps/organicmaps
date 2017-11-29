#include "routing/cross_mwm_graph.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/transit_graph.hpp"

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

namespace
{
double constexpr kTransitionEqualityDistM = 20.0;
double constexpr kInvalidDistance = numeric_limits<double>::max();
}  // namespace

namespace routing
{
// ClosestSegment ---------------------------------------------------------------------------------
CrossMwmGraph::ClosestSegment::ClosestSegment() : m_bestDistM(kInvalidDistance), m_exactMatchFound(false) {}

CrossMwmGraph::ClosestSegment::ClosestSegment(double minDistM, Segment const & bestSeg, bool exactMatchFound)
  : m_bestDistM(minDistM), m_bestSeg(bestSeg), m_exactMatchFound(exactMatchFound)
{
}

void CrossMwmGraph::ClosestSegment::Update(double distM, Segment const & bestSeg)
{
  if (!m_exactMatchFound && distM <= kTransitionEqualityDistM && distM < m_bestDistM)
  {
    m_bestDistM = distM;
    m_bestSeg = bestSeg;
  }
}

// CrossMwmGraph ----------------------------------------------------------------------------------
CrossMwmGraph::CrossMwmGraph(shared_ptr<NumMwmIds> numMwmIds,
                             shared_ptr<m4::Tree<NumMwmId>> numMwmTree,
                             shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                             VehicleType vehicleType, CourntryRectFn const & countryRectFn,
                             Index & index, RoutingIndexManager & indexManager)
  : m_index(index)
  , m_numMwmIds(numMwmIds)
  , m_numMwmTree(numMwmTree)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_countryRectFn(countryRectFn)
  , m_crossMwmIndexGraph(index, numMwmIds, vehicleType)
  , m_crossMwmTransitGraph(index, numMwmIds, VehicleType::Transit)
  , m_crossMwmOsrmGraph(numMwmIds, indexManager)
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
                                             : m_crossMwmOsrmGraph.IsTransition(s, isOutgoing);
}

void CrossMwmGraph::FindBestTwins(NumMwmId sMwmId, bool isOutgoing, FeatureType const & ft, m2::PointD const & point,
                                  map<NumMwmId, ClosestSegment> & minDistSegs, vector<Segment> & twins)
{
  if (!ft.GetID().IsValid())
    return;

  if (ft.GetID().m_mwmId.GetInfo()->GetType() != MwmInfo::COUNTRY)
    return;

  NumMwmId const numMwmId =
      m_numMwmIds->GetId(ft.GetID().m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
  if (numMwmId == sMwmId)
    return;

  if (!m_vehicleModelFactory->GetVehicleModelForCountry(ft.GetID().GetMwmName())->IsRoad(ft))
    return;

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
  vector<Segment> twinCandidates;
  GetTwinCandidates(ft, !isOutgoing, twinCandidates);
  for (Segment const & tc : twinCandidates)
  {
    TransitionPoints const twinPoints = GetTransitionPoints(tc, !isOutgoing);
    for (m2::PointD const & tp : twinPoints)
    {
      double const distM = MercatorBounds::DistanceOnEarth(point, tp);
      ClosestSegment & closestSegment = minDistSegs[numMwmId];
      double constexpr kEpsMeters = 2.0;
      if (my::AlmostEqualAbs(distM, 0.0, kEpsMeters))
      {
        twins.push_back(tc);
        closestSegment.m_exactMatchFound = true;
        continue;
      }
      closestSegment.Update(distM, tc);
    }
  }
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
  // CrossMwmGraph::GetEdgeList().
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
    // Note. The code below looks for twins based on geometry index. This code works for
    // any combination mwms with different cross mwm sections.
    // It's possible to implement a faster version for two special cases:
    // * all neighboring mwms have cross_mwm section
    // * all neighboring mwms have osrm cross mwm sections
    TransitionPoints const points = GetTransitionPoints(s, isOutgoing);
    for (m2::PointD const & p : points)
    {
      // Node. The map below is necessary because twin segments could belong to several mwm.
      // It happens when a segment crosses more than one feature.
      map<NumMwmId, ClosestSegment> minDistSegs;
      auto const findBestTwins = [&](FeatureType const & ft)
      {
        FindBestTwins(s.GetMwmId(), isOutgoing, ft, p, minDistSegs, twins);
      };

      m_index.ForEachInRect(
        findBestTwins, MercatorBounds::RectByCenterXYAndSizeInMeters(p, kTransitionEqualityDistM),
        scales::GetUpperScale());

      for (auto const & kv : minDistSegs)
      {
        if (kv.second.m_exactMatchFound)
          continue;
        if (kv.second.m_bestDistM == kInvalidDistance)
          continue;
        twins.push_back(kv.second.m_bestSeg);
      }
    }

    my::SortUnique(twins);
  }

  for (Segment const & t : twins)
    CHECK_NOT_EQUAL(s.GetMwmId(), t.GetMwmId(), ());
}

void CrossMwmGraph::GetEdgeList(Segment const & s, bool isOutgoing, vector<SegmentEdge> & edges)
{
  CHECK(IsTransition(s, !isOutgoing), ("The segment is not a transition segment. IsTransition(", s,
                                       ",", !isOutgoing, ") returns false."));
  edges.clear();

  if (TransitGraph::IsTransitSegment(s))
  {
    if (TransitCrossMwmSectionExists(s.GetMwmId()))
      m_crossMwmTransitGraph.GetEdgeList(s, isOutgoing, edges);
    return;
  }

  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.GetEdgeList(s, isOutgoing, edges)
                                             : m_crossMwmOsrmGraph.GetEdgeList(s, isOutgoing, edges);
}

void CrossMwmGraph::Clear()
{
  m_crossMwmOsrmGraph.Clear();
  m_crossMwmIndexGraph.Clear();
  m_crossMwmTransitGraph.Clear();
}

TransitionPoints CrossMwmGraph::GetTransitionPoints(Segment const & s, bool isOutgoing)
{
  CHECK(!TransitGraph::IsTransitSegment(s),
        ("Geometry index based twins search is not supported for transit."));

  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.GetTransitionPoints(s, isOutgoing)
                                             : m_crossMwmOsrmGraph.GetTransitionPoints(s, isOutgoing);
}

CrossMwmGraph::MwmStatus CrossMwmGraph::GetMwmStatus(NumMwmId numMwmId,
                                                     string const & sectionName) const
{
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
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

void CrossMwmGraph::GetTwinCandidates(FeatureType const & ft, bool isOutgoing,
                                      vector<Segment> & twinCandidates)
{
  NumMwmId const numMwmId =
      m_numMwmIds->GetId(ft.GetID().m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
  bool const isOneWay =
      m_vehicleModelFactory->GetVehicleModelForCountry(ft.GetID().GetMwmName())->IsOneWay(ft);

  for (uint32_t segIdx = 0; segIdx + 1 < ft.GetPointsCount(); ++segIdx)
  {
    Segment const segForward(numMwmId, ft.GetID().m_index, segIdx, true /* forward */);
    if (IsTransition(segForward, isOutgoing))
      twinCandidates.push_back(segForward);

    if (isOneWay)
      continue;

    Segment const segBackward(numMwmId, ft.GetID().m_index, segIdx, false /* forward */);
    if (IsTransition(segBackward, isOutgoing))
      twinCandidates.push_back(segBackward);
  }
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
