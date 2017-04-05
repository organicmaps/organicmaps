#include "routing/cross_mwm_graph.hpp"
#include "routing/routing_exceptions.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

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
CrossMwmGraph::CrossMwmGraph(Index & index, shared_ptr<NumMwmIds> numMwmIds,
                             shared_ptr<VehicleModelFactory> vehicleModelFactory,
                             RoutingIndexManager & indexManager)
  : m_index(index)
  , m_numMwmIds(numMwmIds)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_crossMwmIndexGraph(index, numMwmIds)
  , m_crossMwmOsrmGraph(numMwmIds, indexManager)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_vehicleModelFactory, ());
}

bool CrossMwmGraph::IsTransition(Segment const & s, bool isOutgoing)
{
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
    auto const findBestTwins = [&](FeatureType const & ft) {
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

  for (Segment const & t : twins)
    CHECK_NOT_EQUAL(s.GetMwmId(), t.GetMwmId(), ());
}

void CrossMwmGraph::GetEdgeList(Segment const & s, bool isOutgoing, vector<SegmentEdge> & edges)
{
  CHECK(IsTransition(s, !isOutgoing), ("The segment is not a transition segment. IsTransition(", s,
                                       ",", !isOutgoing, ") returns false."));
  edges.clear();

  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.GetEdgeList(s, isOutgoing, edges)
                                             : m_crossMwmOsrmGraph.GetEdgeList(s, isOutgoing, edges);
}

void CrossMwmGraph::Clear()
{
  m_crossMwmOsrmGraph.Clear();
  m_crossMwmIndexGraph.Clear();
}

TransitionPoints CrossMwmGraph::GetTransitionPoints(Segment const & s, bool isOutgoing)
{
  return CrossMwmSectionExists(s.GetMwmId()) ? m_crossMwmIndexGraph.GetTransitionPoints(s, isOutgoing)
                                             : m_crossMwmOsrmGraph.GetTransitionPoints(s, isOutgoing);
}

bool CrossMwmGraph::CrossMwmSectionExists(NumMwmId numMwmId)
{
  if (m_crossMwmIndexGraph.InCache(numMwmId))
    return true;

  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(numMwmId), "cannot be loaded."));

  MwmValue * value = handle.GetValue<MwmValue>();
  CHECK(value != nullptr, ("Country file:", m_numMwmIds->GetFile(numMwmId)));
  return value->m_cont.IsExist(CROSS_MWM_FILE_TAG);
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
}  // namespace routing
