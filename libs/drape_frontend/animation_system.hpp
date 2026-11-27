#pragma once

#include "drape_frontend/animation/arrow_animation.hpp"
#include "drape_frontend/screen_animations.hpp"

#include "base/macros.hpp"

#include <optional>
#include <utility>

namespace df
{
class AnimationSystem
{
public:
  AnimationSystem() = default;
  DISALLOW_COPY_AND_MOVE(AnimationSystem);

  bool HasAnimations() const { return m_map.has_value() || m_arrow.has_value(); }
  bool IsCameraMoving() const { return m_map && !m_map->IsFinished(); }
  bool HasMapAnimation(MapAnimation::Type type) const { return m_map && m_map->GetType() == type; }
  MapAnimation const * GetMapAnimation() const { return m_map ? &*m_map : nullptr; }
  double GetRemainingScaleTime(AnimationClock::time_point now) const
  {
    return m_map ? m_map->GetRemainingScaleTime(now) : 0.0;
  }

  std::optional<ScreenBase> GetScreen(ScreenBase const & currentScreen) const;
  ScreenBase GetTargetScreen(ScreenBase const & currentScreen) const;
  std::optional<ArrowPose> GetArrowPose() const;

  void StartMapAnimation(MapAnimation && animation) { m_map.emplace(std::move(animation)); }
  bool RetargetMove(ScreenBase const & endScreen, AnimationClock::time_point now)
  {
    return m_map && m_map->Retarget(endScreen, now);
  }
  void HoldArrowPose(ArrowPose const & pose) { m_heldArrowPose = pose; }
  void ReleaseArrowHold() { m_heldArrowPose.reset(); }
  void AnimateArrowTo(ArrowPose const & target, AnimationClock::time_point requestedAt, double moveDuration,
                      double rotateDuration);
  void CancelMapAnimation() { m_map.reset(); }
  void CancelArrowAnimation();
  void Advance(AnimationClock::time_point now);

private:
  std::optional<MapAnimation> m_map;
  std::optional<ArrowAnimation> m_arrow;
  std::optional<ArrowPose> m_heldArrowPose;
};
}  // namespace df
