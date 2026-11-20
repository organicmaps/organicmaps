#pragma once

#include <chrono>

namespace df
{
using AnimationClock = std::chrono::steady_clock;

double constexpr kMaxAnimationTimeSec = 0.5;
double constexpr kStepAnimationTimeSec = 0.16;
double constexpr kCameraAnimationTimeSec = 0.20;
double constexpr kAutoZoomAnimationTimeSec = 0.25;
double constexpr kOverlayAnimationTimeSec = 0.10;
}  // namespace df
