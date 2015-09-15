#pragma once

#include "geometry/point2d.hpp"

#include "base/base.hpp"

#include "std/limits.hpp"

class Index;

namespace routing
{
uint8_t constexpr kNoSpeedCamera = numeric_limits<uint8_t>::max();
uint8_t CheckCameraInPoint(m2::PointD const & point, Index const & index);
}  // namespace routing
