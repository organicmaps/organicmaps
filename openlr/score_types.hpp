#pragma once

#include "routing/road_graph.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

namespace openlr
{
using Edge = routing::Edge;
using EdgeVector = routing::RoadGraphBase::TEdgeVector;

using Score = uint32_t;

struct ScorePoint
{
  ScorePoint() = default;
  ScorePoint(Score score, m2::PointD const & point) : m_score(score), m_point(point) {}

  bool operator<(ScorePoint const & o) const
  {
    if (m_score != o.m_score)
      return m_score < o.m_score;
    return m_point < o.m_point;
  }

  bool operator==(ScorePoint const & o) const
  {
    return m_score == o.m_score && m_point == o.m_point;
  }

  Score m_score = 0;
  m2::PointD m_point;
};

using ScorePointVec = std::vector<ScorePoint>;

struct ScoreEdge
{
  ScoreEdge(Score score, Edge const & edge) : m_score(score), m_edge(edge) {}

  Score m_score;
  Edge m_edge;
};

using ScoreEdgeVec = std::vector<ScoreEdge>;

struct ScorePath
{
  ScorePath(Score score, EdgeVector && path) : m_score(score), m_path(move(path)) {}

  Score m_score;
  EdgeVector m_path;
};

using ScorePathVec = std::vector<ScorePath>;
}  // namespace openlr
