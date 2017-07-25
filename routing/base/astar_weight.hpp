#pragma once

#include <limits>

namespace routing
{
template <typename WeightType>
constexpr WeightType GetAStarWeightZero();

template <>
constexpr double GetAStarWeightZero<double>()
{
  return 0.0;
}

// Precision of comparison weights.
template <typename WeightType>
constexpr WeightType GetAStarWeightEpsilon();

template <>
constexpr double GetAStarWeightEpsilon<double>()
{
  return 1e-6;
}

template <typename WeightType>
constexpr WeightType GetAStarWeightMax();

template <>
constexpr double GetAStarWeightMax<double>()
{
  return std::numeric_limits<double>::max();
}

}  // namespace routing
