#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/animation/base_interpolator.hpp"

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

void InterpolationHolder::Advance(double elapsedSeconds)
{
  auto iter = m_interpolations.begin();
  while (iter != m_interpolations.end())
  {
    (*iter)->Advance(elapsedSeconds);
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

void InterpolationHolder::RegisterInterpolator(BaseInterpolator * interpolator)
{
  VERIFY(m_interpolations.insert(interpolator).second, ());
}

void InterpolationHolder::DeregisterInterpolator(BaseInterpolator * interpolator)
{
  m_interpolations.erase(interpolator);
}
}  // namespace df
