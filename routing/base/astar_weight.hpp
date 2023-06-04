#pragma once

#include "base/assert.hpp"

#include <limits>

namespace routing
{
template <typename Weight>
constexpr Weight GetAStarWeightZero();

template <>
constexpr double GetAStarWeightZero<double>()
{
  return 0.0;
}

// Precision of comparison weights.
template <typename Weight>
constexpr Weight GetAStarWeightEpsilon()
{
  UNREACHABLE();
}

template <>
constexpr double GetAStarWeightEpsilon<double>()
{
  return 1e-6;
}

template <typename Weight>
constexpr Weight GetAStarWeightMax();

template <>
constexpr double GetAStarWeightMax<double>()
{
  return std::numeric_limits<double>::max();
}

}  // namespace routing
