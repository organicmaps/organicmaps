#pragma once

#include "std/array.hpp"

namespace dp
{
  float constexpr minDepth = -20000.0f;
  float constexpr maxDepth =  20000.0f;

  void MakeProjection(array<float, 16> & result, float left, float right, float bottom, float top);
} // namespace dp
