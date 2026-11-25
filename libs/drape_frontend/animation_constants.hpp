#pragma once

#include <chrono>
#include <cstdint>

namespace df
{
using AnimationClock = std::chrono::steady_clock;

/// @name Camera. A move, a zoom and a rotation change linearly, each for its own time up to kMaxAnimationTimeSec.
/// @{
double constexpr kMaxAnimationTimeSec = 1.5;
// A move covers kMoveViewportsPerSec lengths of the shorter viewport side per second and takes at least
// kShortMoveTimeSec.
double constexpr kShortMoveTimeSec = 0.2;
double constexpr kMoveViewportsPerSec = 7.0;
// A zoom takes this time per unit of the ratio between the scales: x2 and x4 take 2 and 4 times as long.
double constexpr kZoomTimeSecPerScaleRatio = 0.1;
double constexpr kAutoZoomTimeSecPerScaleRatio = 0.6;
// Time to turn by 180 degrees.
double constexpr kHalfTurnTimeSec = 0.75;
// A move that takes longer than kMaxAnimationTimeSec zooms out by twice its ratio to the limit for at most
// kPrettyZoomTimeSec, moves for kMaxAnimationTimeSec and zooms in for at most kPrettyZoomTimeSec.
double constexpr kPrettyZoomTimeSec = kMaxAnimationTimeSec * 0.5;
/// @}

/// @name Fling.
/// @{
// The map travels kKineticAcceleration of the release speed in kKineticDuration, slowing down as
// exp(-kKineticFadeoff * t / kKineticDuration) until it stops.
double constexpr kKineticDuration = 1.5;
double constexpr kKineticFadeoff = 4.0;
double constexpr kKineticAcceleration = 0.4;
// Release speeds in pixels per second at visual scale 1: slower drags don't fling, and the speed limit grows from
// kKineticMaxSpeedStart at the minimum zoom to kKineticMaxSpeedEnd at the maximum one.
double constexpr kInstantVelocityThreshold = 200.0;
double constexpr kKineticMaxSpeedStart = 1000.0;
double constexpr kKineticMaxSpeedEnd = 5000.0;
// The release speed comes from the last kKineticTimeWindowSec of the drag, but at least kMinDragTimeSec.
double constexpr kKineticTimeWindowSec = 0.05;
double constexpr kMinDragTimeSec = 0.01;
// No fling for this time after a pinch.
uint64_t constexpr kKineticDelayMs = 500;
/// @}

/// @name Overlays and markers.
/// @{
double constexpr kOverlayFadeTimeSec = 0.25;
auto constexpr kCopyrightVisibleTime = std::chrono::seconds(10);
double constexpr kRulerFadeTimeSec = 0.4;
// A selection ring or a new bookmark grows to kPopUpOvershoot of its size at kPopUpOvershootTime of
// kPopUpTimeSec, shrinks to kPopUpUndershoot at kPopUpUndershootTime and settles at full size.
double constexpr kPopUpTimeSec = 0.25;
double constexpr kPopUpOvershoot = 1.3;
double constexpr kPopUpOvershootTime = 0.6;
double constexpr kPopUpUndershoot = 0.8;
double constexpr kPopUpUndershootTime = 0.85;
/// @}

/// @name Relations between the constants that the animations rely on.
/// @{
static_assert(0.0 < kPopUpOvershootTime && kPopUpOvershootTime < kPopUpUndershootTime && kPopUpUndershootTime < 1.0);
/// @}
}  // namespace df
