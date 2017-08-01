#pragma once

#include "utils/projection.hpp"

namespace dp
{
namespace depth
{
float constexpr kMyPositionMarkDepth = maxDepth - 1.0f;
}  // namespace depth

namespace displacement
{
int constexpr kDefaultMode = 0x1;
int constexpr kHotelMode = 0x2;
}  // namespace displacement

uint32_t constexpr kScreenPixelRectExtension = 75; // in pixels.
}  // namespace dp
