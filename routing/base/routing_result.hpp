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

  std::vector<Vertex> m_path;
  Weight m_distance;
};
}  // namespace routing
