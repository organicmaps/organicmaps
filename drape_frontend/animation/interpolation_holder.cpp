#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolation_holder.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace df
{

InterpolationHolder & InterpolationHolder::Instance()
{
  static InterpolationHolder holder;
  return holder;
}

bool InterpolationHolder::Advance(double elapsedSeconds)
{
  bool hasAnimations = !m_interpolations.empty();
  TInterpolatorSet::iterator iter = m_interpolations.begin();
  while (iter != m_interpolations.end())
  {
    (*iter)->Advance(elapsedSeconds);
    if ((*iter)->IsFinished())
      iter = m_interpolations.erase(iter);
    else
      ++iter;
  }

  return hasAnimations;
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

}
