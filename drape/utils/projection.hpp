#pragma once

#include <array>

namespace dp
{
float constexpr minDepth = -20000.0f;
float constexpr maxDepth = 20000.0f;

void MakeProjection(std::array<float, 16> & result, float left, float right, float bottom, float top);
}  // namespace dp
