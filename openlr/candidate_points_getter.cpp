#include "openlr/candidate_points_getter.hpp"

#include "openlr/helpers.hpp"

#include "routing/road_graph.hpp"

#include "storage/country_info_getter.hpp"

using namespace routing;

namespace openlr
{
void CandidatePointsGetter::GetJunctionPointCandidates(m2::PointD const & p,
                                                       vector<m2::PointD> & candidates)
{
  // TODO(mgsergio): Get optimal value using experiments on a sample.
  // Or start with small radius and scale it up when there are too few points.
  size_t const kRectSideMeters = 110;

  auto const mwmId = m_mwmIdByPointFn(p);

  auto const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(p, kRectSideMeters);
  auto const selectCandidates = [&rect, &candidates](FeatureType & ft) {
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    ft.ForEachPoint(
        [&rect, &candidates](m2::PointD const & candidate) {
          if (rect.IsPointInside(candidate))
            candidates.emplace_back(candidate);
        },
        scales::GetUpperScale());
  };

  m_index.ForEachInRect(selectCandidates, rect, scales::GetUpperScale());

  // TODO: Move this to a separate stage.
  // 1030292476 Does not match. Some problem occur with points.
  // Either points duplicatate or something alike. Check this
  // later. The idea to fix this was to move SortUnique to the stage
  // after enriching with projections.

  my::SortUnique(candidates,
                 [&p](m2::PointD const & a, m2::PointD const & b) {
                   return MercatorBounds::DistanceOnEarth(a, p) <
                          MercatorBounds::DistanceOnEarth(b, p);
                 },
                 [](m2::PointD const & a, m2::PointD const & b) { return a == b; });

  LOG(LDEBUG,
      (candidates.size(), "candidate points are found for point", MercatorBounds::ToLatLon(p)));
  candidates.resize(min(m_maxJunctionCandidates, candidates.size()));
  LOG(LDEBUG,
      (candidates.size(), "candidates points are remained for point", MercatorBounds::ToLatLon(p)));
}

void CandidatePointsGetter::EnrichWithProjectionPoints(m2::PointD const & p,
                                                       vector<m2::PointD> & candidates)
{
  m_graph.ResetFakes();

  vector<pair<Graph::Edge, Junction>> vicinities;
  m_graph.FindClosestEdges(p, m_maxProjectionCandidates, vicinities);
  for (auto const & v : vicinities)
  {
    auto const & edge = v.first;
    auto const & junction = v.second;

    ASSERT(edge.HasRealPart() && !edge.IsFake(), ());

    if (PointsAreClose(edge.GetStartPoint(), junction.GetPoint()) ||
        PointsAreClose(edge.GetEndPoint(), junction.GetPoint()))
    {
      continue;
    }

    auto const firstHalf = Edge::MakeFake(edge.GetStartJunction(), junction, edge);
    auto const secondHalf = Edge::MakeFake(junction, edge.GetEndJunction(), edge);

    m_graph.AddIngoingFakeEdge(firstHalf);
    m_graph.AddOutgoingFakeEdge(secondHalf);
    candidates.push_back(junction.GetPoint());
  }
  LOG(LDEBUG, (vicinities.size(), "projections candidates were added"));
}
}  // namespace openlr
