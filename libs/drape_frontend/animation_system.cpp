#include "drape_frontend/animation_system.hpp"

#include "geometry/mercator.hpp"

namespace df
{
namespace
{
void AdvanceAnimation(auto & animation, AnimationClock::time_point now)
{
  if (!animation)
    return;

  // Keep the endpoint for the frame on which it is first reached.
  if (animation->IsFinished())
    animation.reset();
  else
    animation->Advance(now);
}
}  // namespace

void AnimationSystem::Advance(AnimationClock::time_point now)
{
  AdvanceAnimation(m_map, now);
  // A queued target must start from the last frame of the previous animation.
  if (m_heldArrowPose && m_arrow && m_arrow->IsFinished())
    m_heldArrowPose = m_arrow->GetPose();
  AdvanceAnimation(m_arrow, now);
}

std::optional<ScreenBase> AnimationSystem::GetScreen(ScreenBase const & currentScreen) const
{
  if (!m_map)
    return std::nullopt;

  std::optional<ScreenBase> screen(std::in_place, currentScreen);
  m_map->Apply(*screen);
  return screen;
}

ScreenBase AnimationSystem::GetTargetScreen(ScreenBase const & currentScreen) const
{
  ScreenBase screen = currentScreen;
  // New input starts from the visible position during a fling.
  if (m_map && m_map->GetType() != MapAnimation::Type::Fling)
    m_map->ApplyTarget(screen);
  return screen;
}

std::optional<ArrowPose> AnimationSystem::GetArrowPose() const
{
  if (m_arrow)
    return m_arrow->GetPose();
  return m_heldArrowPose;
}

void AnimationSystem::AnimateArrowTo(ArrowPose const & target, AnimationClock::time_point requestedAt,
                                     double moveDuration, double rotateDuration)
{
  if (moveDuration == 0.0 && rotateDuration == 0.0)
  {
    CancelArrowAnimation();
    return;
  }

  // The arrow animates from the pose that its owner held or from the running animation.
  auto start = GetArrowPose();
  ASSERT(start, ());
  ReleaseArrowHold();
  start->m_position.x = mercator::NearestWrapX(start->m_position.x, target.m_position.x);
  m_arrow.emplace(*start, target, requestedAt, moveDuration, rotateDuration);
}

void AnimationSystem::CancelArrowAnimation()
{
  m_arrow.reset();
  ReleaseArrowHold();
}
}  // namespace df
