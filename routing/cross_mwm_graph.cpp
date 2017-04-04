#include "routing/cross_mwm_graph.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

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

struct ClosestSegment
{
  ClosestSegment() = default;
  ClosestSegment(double minDistM, Segment const & bestSeg, bool exactMatchFound)
    : m_bestDistM(minDistM), m_bestSeg(bestSeg), m_exactMatchFound(exactMatchFound)
  {
  }

  double m_bestDistM = kInvalidDistance;
  Segment m_bestSeg;
  bool m_exactMatchFound = false;
};
}  // namespace

namespace routing
{
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
  // Index graph based cross-mwm information.
  if (CrossMwmSectionExists(s.GetMwmId()))
    return m_crossMwmIndexGraph.IsTransition(s, isOutgoing);
  return m_crossMwmOsrmGraph.IsTransition(s, isOutgoing);
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
    auto const findBestTwins = [&](FeatureType & ft) {
      if (ft.GetID().m_mwmId.GetInfo()->GetType() != MwmInfo::COUNTRY)
        return;

      if (!ft.GetID().IsValid())
        return;

      NumMwmId const numMwmId =
          m_numMwmIds->GetId(ft.GetID().m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
      if (numMwmId == s.GetMwmId())
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
          double const distM = MercatorBounds::DistanceOnEarth(p, tp);
          if (distM == 0.0)
          {
            twins.push_back(tc);
            minDistSegs[numMwmId].m_exactMatchFound = true;
          }
          if (!minDistSegs[numMwmId].m_exactMatchFound && distM <= kTransitionEqualityDistM &&
              distM < minDistSegs[numMwmId].m_bestDistM)
          {
            minDistSegs[numMwmId].m_bestDistM = distM;
            minDistSegs[numMwmId].m_bestSeg = tc;
          }
        }
      }
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

  // Osrm based cross-mwm information.

  // Note. According to cross-mwm OSRM sections there is a node id that could be ingoing and
  // outgoing
  // at the same time. For example in Berlin mwm on Nordlicher Berliner Ring (A10) near crossing
  // with A11 there's such node id. It's an extremely rare case. There're probably several such node
  // id
  // for the whole Europe. Such cases are not processed in WorldGraph::GetEdgeList() for the time
  // being.
  // To prevent filling |edges| with twins instead of leap edges and vice versa in
  // WorldGraph::GetEdgeList()
  // CrossMwmGraph::GetEdgeList() does not fill |edges| if |s| is a transition segment which
  // corresponces node id described above.
  if (IsTransition(s, isOutgoing))
    return;

  // Index graph based cross-mwm information.
  if (CrossMwmSectionExists(s.GetMwmId()))
    m_crossMwmIndexGraph.GetEdgeList(s, isOutgoing, edges);
  else
    m_crossMwmOsrmGraph.GetEdgeList(s, isOutgoing, edges);
}

void CrossMwmGraph::Clear()
{
  m_crossMwmOsrmGraph.Clear();
  m_crossMwmIndexGraph.Clear();
}

TransitionPoints CrossMwmGraph::GetTransitionPoints(Segment const & s, bool isOutgoing)
{
  if (CrossMwmSectionExists(s.GetMwmId()))
    return m_crossMwmIndexGraph.GetTransitionPoints(s, isOutgoing);
  return m_crossMwmOsrmGraph.GetTransitionPoints(s, isOutgoing);
}

bool CrossMwmGraph::CrossMwmSectionExists(NumMwmId numMwmId)
{
  if (m_crossMwmIndexGraph.HasCache(numMwmId))
    return true;

  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(m_numMwmIds->GetFile(numMwmId));
  CHECK(handle.IsAlive(), ());
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
