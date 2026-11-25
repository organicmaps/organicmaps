#pragma once

#include <chrono>

namespace df
{
using AnimationClock = std::chrono::steady_clock;

double constexpr kMaxAnimationTimeSec = 1.5;
double constexpr kOverlayFadeTimeSec = 0.25;
}  // namespace df
