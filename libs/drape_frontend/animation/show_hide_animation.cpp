#include "drape_frontend/animation/show_hide_animation.hpp"

#include <cmath>

namespace df
{
void ShowHideAnimation::AnimateVisibility(bool visible)
{
  if (m_visible == visible)
    return;

  double const start = GetVisibility();
  m_visible = visible;
  double const end = m_visible ? 1.0 : 0.0;
  if (start == end)
    m_fade.reset();
  else
    m_fade.emplace(std::abs(end - start) * m_duration, start, end);
}

double ShowHideAnimation::GetVisibility() const
{
  if (m_fade)
    return m_fade->GetOpacity();
  return m_visible ? 1.0 : 0.0;
}
}  // namespace df
