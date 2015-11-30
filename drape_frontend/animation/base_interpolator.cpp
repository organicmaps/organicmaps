#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolation_holder.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

namespace df
{

BaseInterpolator::BaseInterpolator(double duration, double delay)
  : m_elapsedTime(0.0)
  , m_duration(duration)
  , m_delay(delay)
{
  ASSERT(m_duration > 0.0, ());
  InterpolationHolder::Instance().RegisterInterpolator(this);
}

BaseInterpolator::~BaseInterpolator()
{
  InterpolationHolder::Instance().DeregisterInterpolator(this);
}

bool BaseInterpolator::IsFinished() const
{
  return m_elapsedTime > (m_duration + m_delay);
}

void BaseInterpolator::Advance(double elapsedSeconds)
{
  m_elapsedTime += elapsedSeconds;
}

double BaseInterpolator::GetT() const
{
  if (IsFinished())
    return 1.0;

  return max(m_elapsedTime - m_delay, 0.0) / m_duration;
}

double BaseInterpolator::GetElapsedTime() const
{
  return m_elapsedTime;
}

double BaseInterpolator::GetDuration() const
{
  return m_duration;
}

}
