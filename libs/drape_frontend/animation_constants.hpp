#pragma once

#include <chrono>

namespace df
{
using AnimationClock = std::chrono::steady_clock;

double constexpr kMaxAnimationTimeSec = 1.5;
double constexpr kStepAnimationTimeSec = 0.20;
double constexpr kCameraAnimationTimeSec = 0.20;
double constexpr kAutoZoomAnimationTimeSec = 1.2;
double constexpr kOverlayFadeTimeSec = 0.25;
}  // namespace df
