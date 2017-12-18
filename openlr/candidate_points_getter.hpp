#pragma once

#include "openlr/graph.hpp"
#include "openlr/stats.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace openlr
{
class CandidatePointsGetter
{
  using MwmIdByPointFn = std::function<MwmSet::MwmId(m2::PointD const &)>;

public:
  CandidatePointsGetter(size_t const maxJunctionCandidates, size_t const maxProjectionCandidates,
                        Index const & index, MwmIdByPointFn const & mwmIdByPointFn, Graph & graph,
                        v2::Stats & stat)
    : m_maxJunctionCandidates(maxJunctionCandidates)
    , m_maxProjectionCandidates(maxProjectionCandidates)
    , m_index(index)
    , m_mwmIdByPointFn(mwmIdByPointFn)
    , m_graph(graph)
    , m_stat(stat)
  {
  }

  void GetCandidatePoints(m2::PointD const & p, std::vector<m2::PointD> & candidates)
  {
    GetJunctionPointCandidates(p, candidates);
    EnrichWithProjectionPoints(p, candidates);
  }

private:
  void GetJunctionPointCandidates(m2::PointD const & p, std::vector<m2::PointD> & candidates);
  void EnrichWithProjectionPoints(m2::PointD const & p, std::vector<m2::PointD> & candidates);

  size_t const m_maxJunctionCandidates;
  size_t const m_maxProjectionCandidates;

  Index const & m_index;
  Graph & m_graph;
  v2::Stats & m_stat;
  MwmIdByPointFn m_mwmIdByPointFn;
};
}  // namespace openlr
