#pragma once

#include "routing/base/astar_weight.hpp"

#include <vector>

namespace routing
{
template <typename Vertex, typename Weight>
struct RoutingResult final
{
  RoutingResult() : m_distance(GetAStarWeightZero<Weight>()) {}

  void Clear()
  {
    m_path.clear();
    m_distance = GetAStarWeightZero<Weight>();
  }

  bool Empty() const { return m_path.empty(); }

  std::vector<Vertex> m_path;
  Weight m_distance;

  struct LessWeight
  {
    bool operator()(RoutingResult const & l, RoutingResult const & r) const { return l.m_distance < r.m_distance; }
  };
};
}  // namespace routing
