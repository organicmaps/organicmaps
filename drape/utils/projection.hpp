#pragma once

#include "drape/drape_global.hpp"

#include <array>

namespace dp
{
float constexpr kMinDepth = -20000.0f;
float constexpr kMaxDepth = 20000.0f;

std::array<float, 16> MakeProjection(dp::ApiVersion apiVersion, float left, float right,
                                     float bottom, float top);
}  // namespace dp
