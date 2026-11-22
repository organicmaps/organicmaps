#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/animation/opacity_animation.hpp"

#include "base/assert.hpp"

namespace df
{
InterpolationHolder & InterpolationHolder::Instance()
{
  static InterpolationHolder holder;
  return holder;
}

bool InterpolationHolder::IsActive() const
{
  return !m_interpolations.empty();
}

void InterpolationHolder::Advance(AnimationClock::time_point now)
{
  auto iter = m_interpolations.begin();
  while (iter != m_interpolations.end())
  {
    (*iter)->Advance(now);
    if ((*iter)->IsFinished())
      iter = m_interpolations.erase(iter);
    else
      ++iter;
  }
}

InterpolationHolder::~InterpolationHolder()
{
  ASSERT(m_interpolations.empty(), ());
}

void InterpolationHolder::Register(OpacityAnimation * interpolator)
{
  VERIFY(m_interpolations.insert(interpolator).second, ());
}

void InterpolationHolder::Deregister(OpacityAnimation * interpolator)
{
  m_interpolations.erase(interpolator);
}
}  // namespace df
