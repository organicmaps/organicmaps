#pragma once

#include "drape/drape_global.hpp"

#include <array>

namespace dp
{
/// @todo: asymmetric values lead to in-range polygons being clipped still, might be a bug in projection matrix?
float constexpr kMinDepth = -25000.0f;
float constexpr kMaxDepth = 25000.0f;

std::array<float, 16> MakeProjection(dp::ApiVersion apiVersion, float left, float right,
                                     float bottom, float top);
}  // namespace dp
