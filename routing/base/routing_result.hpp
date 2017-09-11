#pragma once

#include "routing/base/astar_weight.hpp"

#include <vector>

namespace routing
{
template <typename TVertexType, typename TWeightType>
struct RoutingResult
{
  std::vector<TVertexType> path;
  TWeightType distance;
  RoutingResult() : distance(GetAStarWeightZero<TWeightType>()) {}
  void Clear()
  {
    path.clear();
    distance = GetAStarWeightZero<TWeightType>();
  }
};
}  // namespace routing
