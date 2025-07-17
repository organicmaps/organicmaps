#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/follow_animation.hpp"
#include "drape_frontend/animation/linear_animation.hpp"
#include "drape_frontend/animation/parabolic_animation.hpp"
#include "drape_frontend/animation/scale_animation.hpp"
#include "drape_frontend/animation/sequence_animation.hpp"
#include "drape_frontend/animation_constants.hpp"
#include "drape_frontend/screen_operations.hpp"

namespace df
{

/*
 * PARABOLIC ANIMATION TUNING PARAMETERS
 * ====================================
 * 
 * This system creates smooth parabolic animations for location button taps.
 * The animation zooms out while panning, then zooms back in, creating a ballistic trajectory.
 * 
 * All key parameters are defined as named constants below for easy tuning:
 * 
 * 1. DISTANCE THRESHOLD - kDistanceThresholdViewports
 *    - Current: 5.0 viewport sizes
 *    - Controls when to skip animation and jump immediately for very long distances
 *    - Lower = more animations, Higher = more immediate jumps
 *    - Used to prevent disorienting animations for intercontinental jumps
 * 
 * 2. ZOOM LEVEL THRESHOLD - kZoomLevelThreshold
 *    - Current: 0.0001 scale units (higher scale = more zoomed out)
 *    - Determines when to use significant vs minimal zoom-out during animation
 *    - Lower = more restrictive (less zoom-out), Higher = more zoom-out
 *    - Prevents unnecessary zoom-out when already viewing city/regional level
 * 
 * 3. SIGNIFICANT ZOOM-OUT FACTORS - kSignificantZoomOut*
 *    - Base: 1.2x, Growth: 0.6x, Max: 1.8x (multiply scale to zoom out)
 *    - Used when very zoomed in (street/building level detail)
 *    - Creates pronounced parabolic effect for local navigation
 *    - Higher values = more dramatic zoom-out during pan
 * 
 * 4. MINIMAL ZOOM-OUT FACTORS - kMinimalZoomOut*
 *    - Base: 1.02x, Growth: 0.08x, Max: 1.15x
 *    - Used when already zoomed out (city/regional level)
 *    - Subtle zoom-out maintains parabolic feel without disorientation
 *    - Keep low to avoid unnecessary zoom-out when context already visible
 * 
 * 5. ANIMATION DURATION - kAnimationDuration
 *    - Current: 1.0 second fixed duration
 *    - Controls overall animation speed and responsiveness
 *    - Shorter = snappier, Longer = more relaxed feel
 * 
 * 6. TIMING CURVE - Search: "ParabolicAnimation::GetT" in parabolic_animation.cpp
 *    - Current: Quadratic ease-out for smooth deceleration
 *    - Controls acceleration/deceleration feel throughout animation
 *    - Ease-out provides natural, comfortable motion ending
 * 
 * 7. LOGARITHMIC DISTANCE SCALING - kLogScalingFactor
 *    - Current: 4.0 (logarithm base for distance scaling)
 *    - Smooth logarithmic curve prevents abrupt zoom-out changes
 *    - Provides proportional zoom-out based on travel distance
 *    - Higher values = more gradual scaling, Lower = more aggressive scaling
 */
namespace parabolic_animation
{
  // Distance threshold: skip animation for movements beyond this many viewport sizes
  double constexpr kDistanceThresholdViewports = 5.0;
  
  // Zoom level threshold: use different zoom-out behavior above/below this scale
  // Higher scale = more zoomed out, so small values = very zoomed in
  double constexpr kZoomLevelThreshold = 0.0001;
  
  // Animation duration in seconds (matches kParabolicAnimationDuration in parabolic_animation.hpp)
  double constexpr kAnimationDuration = kParabolicAnimationDuration;
  
  // Significant zoom-out factors (used when very zoomed in)
  double constexpr kSignificantZoomOutBase = 1.2;
  double constexpr kSignificantZoomOutGrowth = 0.6;
  double constexpr kSignificantZoomOutMax = 1.8;
  
  // Minimal zoom-out factors (used when already zoomed out)
  double constexpr kMinimalZoomOutBase = 1.02;
  double constexpr kMinimalZoomOutGrowth = 0.08;
  double constexpr kMinimalZoomOutMax = 1.15;
  
  // Logarithmic scaling factor for distance-based zoom-out
  double constexpr kLogScalingFactor = 4.0;
}

std::string const kPrettyMoveAnim = "PrettyMove";
std::string const kPrettyFollowAnim = "PrettyFollow";
std::string const kParabolicFollowAnim = "ParabolicFollow";
std::string const kParallelFollowAnim = "ParallelFollow";
std::string const kParallelLinearAnim = "ParallelLinear";

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  return GetPrettyMoveAnimation(startScreen, startScreen.GetScale(), endScreen.GetScale(),
                                startScreen.GetOrg(), endScreen.GetOrg());
}

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen,
                                                    m2::AnyRectD const & startRect, m2::AnyRectD const & endRect)
{
  double const startScale = CalculateScale(screen.PixelRect(), startRect.GetLocalRect());
  double const endScale = CalculateScale(screen.PixelRect(), endRect.GetLocalRect());

  return GetPrettyMoveAnimation(screen, startScale, endScale, startRect.GlobalCenter(), endRect.GlobalCenter());
}

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                    m2::PointD const & startPt, m2::PointD const & endPt)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(startPt, endPt, screen.PixelRectIn3d(), startScale);
  ASSERT_GREATER(moveDuration, 0.0, ());

  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyMoveAnim);

  auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
  zoomOutAnim->SetScale(startScale, startScale * scaleFactor);
  zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  //TODO (in future): Pass fixed duration instead of screen.
  auto moveAnim = make_unique_dp<MapLinearAnimation>();
  moveAnim->SetMove(startPt, endPt, screen);
  moveAnim->SetMaxDuration(kMaxAnimationTimeSec);

  auto zoomInAnim = make_unique_dp<MapLinearAnimation>();
  zoomInAnim->SetScale(startScale * scaleFactor, endScale);
  zoomInAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  sequenceAnim->AddAnimation(std::move(zoomOutAnim));
  sequenceAnim->AddAnimation(std::move(moveAnim));
  sequenceAnim->AddAnimation(std::move(zoomInAnim));
  return sequenceAnim;
}

drape_ptr<SequenceAnimation> GetPrettyFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos, double targetScale,
                                                      double targetAngle, m2::PointD const & endPixelPos)
{
  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyFollowAnim);

  m2::RectD const viewportRect = startScreen.PixelRectIn3d();

  ScreenBase tmp = startScreen;
  tmp.SetAngle(targetAngle);
  tmp.MatchGandP3d(userPos, viewportRect.Center());

  double const moveDuration = PositionInterpolator::GetMoveDuration(startScreen.GetOrg(), tmp.GetOrg(), startScreen);
  ASSERT_GREATER(moveDuration, 0.0, ());

  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  tmp = startScreen;

  if (moveDuration > 0.0)
  {
    tmp.SetScale(startScreen.GetScale() * scaleFactor);

    auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
    zoomOutAnim->SetScale(startScreen.GetScale(), tmp.GetScale());
    zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);
    sequenceAnim->AddAnimation(std::move(zoomOutAnim));

    tmp.MatchGandP3d(userPos, viewportRect.Center());

    auto moveAnim = make_unique_dp<MapLinearAnimation>();
    moveAnim->SetMove(startScreen.GetOrg(), tmp.GetOrg(), viewportRect, tmp.GetScale());
    moveAnim->SetMaxDuration(kMaxAnimationTimeSec);
    sequenceAnim->AddAnimation(std::move(moveAnim));
  }

  auto followAnim = make_unique_dp<MapFollowAnimation>(tmp, userPos, endPixelPos,
                                                       targetScale, targetAngle,
                                                       false /* isAutoZoom */);
  followAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);
  sequenceAnim->AddAnimation(std::move(followAnim));
  return sequenceAnim;
}

drape_ptr<Animation> GetParabolicFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos, double targetScale,
                                                          double targetAngle, m2::PointD const & endPixelPos)
{
  // See parabolic_animation namespace above for tuning parameters

  // Calculate the end screen state
  ScreenBase endScreen = startScreen;
  endScreen.SetAngle(targetAngle);
  endScreen.SetScale(targetScale);
  endScreen.MatchGandP3d(userPos, endPixelPos);

  // Calculate distance-based parabolic height
  double const distance = startScreen.GetOrg().Length(endScreen.GetOrg());
  double const currentScale = startScreen.GetScale();
  
  // Convert distance to screen pixels to determine appropriate zoom-out
  double const pixelDistance = distance / currentScale;
  double const viewportSize = std::min(startScreen.PixelRectIn3d().SizeX(), startScreen.PixelRectIn3d().SizeY());
  
  // Distance threshold: if movement is too far, skip animation
  if (pixelDistance > viewportSize * parabolic_animation::kDistanceThresholdViewports)
  {
    // Return null to indicate no animation should be used (immediate jump)
    return nullptr;
  }
  
  // Calculate zoom-out factor based on distance and current zoom level
  double const normalizedDistance = pixelDistance / viewportSize;
  double zoomOutFactor;
  
  // Higher scale values = more zoomed out, so we check if current scale is small (zoomed in)
  if (currentScale < parabolic_animation::kZoomLevelThreshold)
  {
    // We're zoomed in enough to benefit from moderate zoom-out during pan
    // Logarithmic curve: starts at base, grows smoothly, caps at max
    zoomOutFactor = parabolic_animation::kSignificantZoomOutBase + 
                    parabolic_animation::kSignificantZoomOutGrowth * 
                    std::log(1.0 + normalizedDistance) / std::log(parabolic_animation::kLogScalingFactor);
    zoomOutFactor = std::min(zoomOutFactor, parabolic_animation::kSignificantZoomOutMax);
  }
  else
  {
    // When already zoomed out, do minimal zoom-out for parabolic effect
    // This ensures we always have zoom-out -> zoom-in, never just zoom-in
    zoomOutFactor = parabolic_animation::kMinimalZoomOutBase + 
                    parabolic_animation::kMinimalZoomOutGrowth * 
                    std::log(1.0 + normalizedDistance) / std::log(parabolic_animation::kLogScalingFactor);
    zoomOutFactor = std::min(zoomOutFactor, parabolic_animation::kMinimalZoomOutMax);
  }

  double const baseScale = std::max(startScreen.GetScale(), targetScale);
  // Use a reasonable zoom-out factor - MULTIPLY to zoom out (higher scale = more zoomed out)
  double const peakScale = baseScale * zoomOutFactor;

  // Create the parabolic animation that combines all transformations
  auto anim = make_unique_dp<ParabolicAnimation>(startScreen,
                                                 startScreen.GetOrg(), endScreen.GetOrg(),
                                                 startScreen.GetScale(), targetScale,
                                                 startScreen.GetAngle(), targetAngle,
                                                 peakScale);
  return anim;
}

drape_ptr<MapLinearAnimation> GetRectAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  auto anim = make_unique_dp<MapLinearAnimation>();

  anim->SetRotate(startScreen.GetAngle(), endScreen.GetAngle());
  anim->SetMove(startScreen.GetOrg(), endScreen.GetOrg(),
                startScreen.PixelRectIn3d(), (startScreen.GetScale() + endScreen.GetScale()) / 2.0);
  anim->SetScale(startScreen.GetScale(), endScreen.GetScale());
  anim->SetMaxScaleDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapLinearAnimation> GetSetRectAnimation(ScreenBase const & screen,
                                                  m2::AnyRectD const & startRect, m2::AnyRectD const & endRect)
{
  auto anim = make_unique_dp<MapLinearAnimation>();

  double const startScale = CalculateScale(screen.PixelRect(), startRect.GetLocalRect());
  double const endScale = CalculateScale(screen.PixelRect(), endRect.GetLocalRect());

  anim->SetRotate(startRect.Angle().val(), endRect.Angle().val());
  anim->SetMove(startRect.GlobalCenter(), endRect.GlobalCenter(), screen);
  anim->SetScale(startScale, endScale);
  anim->SetMaxScaleDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapFollowAnimation> GetFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                 double targetScale, double targetAngle, m2::PointD const & endPixelPos,
                                                 bool isAutoZoom)
{
  auto anim = make_unique_dp<MapFollowAnimation>(startScreen, userPos, endPixelPos, targetScale, targetAngle, isAutoZoom);
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapScaleAnimation> GetScaleAnimation(ScreenBase const & startScreen, m2::PointD pxScaleCenter,
                                               m2::PointD glbScaleCenter, double factor)
{
  ScreenBase endScreen = startScreen;
  ApplyScale(pxScaleCenter, factor, endScreen);

  auto anim = make_unique_dp<MapScaleAnimation>(startScreen.GetScale(), endScreen.GetScale(),
                                                glbScaleCenter, pxScaleCenter);
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

} // namespace df
