#pragma once

#include "std/array.hpp"

namespace dp
{
  static float const minDepth = -20000.0f;
  static float const maxDepth =  20000.0f;

  void MakeProjection(array<float, 16> & result, float left, float right, float bottom, float top);
} // namespace dp
