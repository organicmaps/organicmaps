#include "openlr/score_candidate_points_getter.hpp"

#include "openlr/helpers.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <set>
#include <utility>

using namespace routing;

namespace
{
// Ends of segments and intermediate points of segments are considered only within this radius.
double const kRadius = 30.0;
}  //  namespace

namespace openlr
{
void ScoreCandidatePointsGetter::GetJunctionPointCandidates(m2::PointD const & p, bool isLastPoint,
                                                            ScoreEdgeVec & edgeCandidates)
{
  ScorePointVec pointCandidates;
  auto const selectCandidates = [&p, &pointCandidates, this](FeatureType & ft)
  {
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    if (ft.GetGeomType() != feature::GeomType::Line || !routing::IsRoad(feature::TypesHolder(ft)))
      return;

    ft.ForEachPoint([&p, &pointCandidates, this](m2::PointD const & candidate)
    {
      if (mercator::DistanceOnEarth(p, candidate) < kRadius)
        pointCandidates.emplace_back(GetScoreByDistance(p, candidate), candidate);
    }, scales::GetUpperScale());
  };

  m_dataSource.ForEachInRect(selectCandidates, mercator::RectByCenterXYAndSizeInMeters(p, kRadius),
                             scales::GetUpperScale());

  base::SortUnique(pointCandidates);
  std::reverse(pointCandidates.begin(), pointCandidates.end());

  pointCandidates.resize(std::min(m_maxJunctionCandidates, pointCandidates.size()));

  for (auto const & pc : pointCandidates)
  {
    Graph::EdgeListT edges;
    if (!isLastPoint)
      m_graph.GetOutgoingEdges(geometry::PointWithAltitude(pc.m_point, 0 /* altitude */), edges);
    else
      m_graph.GetIngoingEdges(geometry::PointWithAltitude(pc.m_point, 0 /* altitude */), edges);

    for (auto const & e : edges)
      edgeCandidates.emplace_back(pc.m_score, e);
  }
}

void ScoreCandidatePointsGetter::EnrichWithProjectionPoints(m2::PointD const & p, ScoreEdgeVec & edgeCandidates)
{
  m_graph.ResetFakes();

  std::vector<std::pair<Graph::Edge, geometry::PointWithAltitude>> vicinities;
  m_graph.FindClosestEdges(p, static_cast<uint32_t>(m_maxProjectionCandidates), vicinities);
  for (auto const & v : vicinities)
  {
    auto const & edge = v.first;
    auto const & proj = v.second;

    CHECK(edge.HasRealPart(), ());
    CHECK(!edge.IsFake(), ());

    if (mercator::DistanceOnEarth(p, proj.GetPoint()) >= kRadius)
      continue;

    edgeCandidates.emplace_back(GetScoreByDistance(p, proj.GetPoint()), edge);
  }
}

bool ScoreCandidatePointsGetter::IsJunction(m2::PointD const & p)
{
  Graph::EdgeListT outgoing;
  m_graph.GetRegularOutgoingEdges(geometry::PointWithAltitude(p, 0 /* altitude */), outgoing);

  Graph::EdgeListT ingoing;
  m_graph.GetRegularIngoingEdges(geometry::PointWithAltitude(p, 0 /* altitude */), ingoing);

  // Note. At mwm borders the size of |ids| may be bigger than two in case of straight
  // road because of road feature duplication at borders.
  std::set<std::pair<uint32_t, uint32_t>> ids;
  for (auto const & e : outgoing)
    ids.insert(std::make_pair(e.GetFeatureId().m_index, e.GetSegId()));

  for (auto const & e : ingoing)
    ids.insert(std::make_pair(e.GetFeatureId().m_index, e.GetSegId()));

  // Size of |ids| is number of different pairs of (feature id, segment id) starting from
  // |p| plus going to |p|. The size 0, 1 or 2 is considered |p| is not a junction of roads.
  // If the size is 3 or more it means |p| is an intersection of 3 or more roads.
  return ids.size() >= 3;
}

Score ScoreCandidatePointsGetter::GetScoreByDistance(m2::PointD const & point, m2::PointD const & candidate)
{
  // Maximum possible score for the distance between an openlr segment ends and an osm segments.
  Score constexpr kMaxScoreForDist = 70;
  // If the distance between an openlr segments end and an osm segments end is less or equal
  // |kMaxScoreDistM| the point gets |kMaxScoreForDist| score.
  double constexpr kMaxScoreDistM = 5.0;
  // According to the standard openlr edge should be started from a junction. Despite the fact
  // that openlr and osm are based on different graphs, the score of junction should be increased.
  double const junctionFactor = IsJunction(candidate) ? 1.1 : 1.0;

  double const distM = mercator::DistanceOnEarth(point, candidate);
  double const score = distM <= kMaxScoreDistM
                         ? kMaxScoreForDist * junctionFactor
                         : static_cast<double>(kMaxScoreForDist) * junctionFactor / (1.0 + distM - kMaxScoreDistM);

  CHECK_GREATER_OR_EQUAL(score, 0.0, ());
  CHECK_LESS_OR_EQUAL(score, static_cast<double>(kMaxScoreForDist) * junctionFactor, ());
  return static_cast<Score>(score);
}
}  // namespace openlr
