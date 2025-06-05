#pragma once

#include "utils/projection.hpp"

namespace dp
{
namespace depth
{
float constexpr kMyPositionMarkDepth = kMaxDepth - 1.0f;
}  // namespace depth

uint32_t constexpr kScreenPixelRectExtension = 75;  // in pixels.
}  // namespace dp
