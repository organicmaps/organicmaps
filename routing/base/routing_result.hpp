#pragma once

#include "routing/base/astar_weight.hpp"

#include <vector>

namespace routing
{
template <typename VertexType, typename WeightType>
struct RoutingResult final
{
  RoutingResult() : m_distance(GetAStarWeightZero<WeightType>()) {}

  void Clear()
  {
    m_path.clear();
    m_distance = GetAStarWeightZero<WeightType>();
  }

  std::vector<VertexType> m_path;
  WeightType m_distance;
};
}  // namespace routing
