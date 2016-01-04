#pragma once

#include "utils/projection.hpp"

namespace dp
{

namespace depth
{

float const POSITION_ACCURACY = minDepth + 1.0f;
float const MY_POSITION_MARK = maxDepth - 1.0f;

} // namespace depth

} // namespace dp
