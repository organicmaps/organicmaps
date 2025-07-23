#pragma once

#include "openlr/graph.hpp"
#include "openlr/stats.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace openlr
{
class CandidatePointsGetter
{
public:
  CandidatePointsGetter(size_t const maxJunctionCandidates, size_t const maxProjectionCandidates,
                        DataSource const & dataSource, Graph & graph)
    : m_maxJunctionCandidates(maxJunctionCandidates)
    , m_maxProjectionCandidates(maxProjectionCandidates)
    , m_dataSource(dataSource)
    , m_graph(graph)
  {}

  void GetCandidatePoints(m2::PointD const & p, std::vector<m2::PointD> & candidates)
  {
    FillJunctionPointCandidates(p, candidates);
    EnrichWithProjectionPoints(p, candidates);
  }

private:
  void FillJunctionPointCandidates(m2::PointD const & p, std::vector<m2::PointD> & candidates);
  void EnrichWithProjectionPoints(m2::PointD const & p, std::vector<m2::PointD> & candidates);

  size_t const m_maxJunctionCandidates;
  size_t const m_maxProjectionCandidates;

  DataSource const & m_dataSource;
  Graph & m_graph;
};
}  // namespace openlr
