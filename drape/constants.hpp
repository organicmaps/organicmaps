#pragma once

#include "utils/projection.hpp"

namespace dp
{

namespace depth
{

float constexpr POSITION_ACCURACY = minDepth + 1.0f;
float constexpr MY_POSITION_MARK = maxDepth - 1.0f;

} // namespace depth

namespace displacement
{

int constexpr kDefaultMode = 0x1;
int constexpr kHotelMode = 0x2;

int constexpr kAllModes = kDefaultMode | kHotelMode;

} // namespace displacement

} // namespace dp
