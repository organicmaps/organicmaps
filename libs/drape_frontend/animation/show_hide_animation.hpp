#pragma once

#include "drape_frontend/animation/opacity_animation.hpp"

#include <optional>

namespace df
{
class ShowHideAnimation
{
public:
  ShowHideAnimation(bool visible, double duration) : m_duration(duration), m_visible(visible) {}

  void AnimateVisibility(bool visible);
  bool IsVisible() const { return m_visible || !IsFinished(); }
  bool IsFinished() const { return !m_fade || m_fade->IsFinished(); }
  double GetVisibility() const;

private:
  double const m_duration;
  bool m_visible;
  std::optional<OpacityAnimation> m_fade;
};
}  // namespace df
