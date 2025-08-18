#pragma once

#include "openlr/graph.hpp"
#include "openlr/score_types.hpp"
#include "openlr/stats.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace openlr
{
class ScoreCandidatePointsGetter
{
public:
  ScoreCandidatePointsGetter(size_t maxJunctionCandidates, size_t maxProjectionCandidates,
                             DataSource const & dataSource, Graph & graph)
    : m_maxJunctionCandidates(maxJunctionCandidates)
    , m_maxProjectionCandidates(maxProjectionCandidates)
    , m_dataSource(dataSource)
    , m_graph(graph)
  {}

  void GetEdgeCandidates(m2::PointD const & p, bool isLastPoint, ScoreEdgeVec & edges)
  {
    GetJunctionPointCandidates(p, isLastPoint, edges);
    EnrichWithProjectionPoints(p, edges);
  }

private:
  void GetJunctionPointCandidates(m2::PointD const & p, bool isLastPoint, ScoreEdgeVec & edgeCandidates);
  void EnrichWithProjectionPoints(m2::PointD const & p, ScoreEdgeVec & edgeCandidates);

  /// \returns true if |p| is a junction and false otherwise.
  bool IsJunction(m2::PointD const & p);
  Score GetScoreByDistance(m2::PointD const & point, m2::PointD const & candidate);

  size_t const m_maxJunctionCandidates;
  size_t const m_maxProjectionCandidates;

  DataSource const & m_dataSource;
  Graph & m_graph;
};
}  // namespace openlr
