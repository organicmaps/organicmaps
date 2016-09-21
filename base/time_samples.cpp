#include "base/time_samples.hpp"

#include "std/cmath.hpp"

namespace my
{
void TimeSamples::Add(double s)
{
  m_s += s;
  m_s2 += s * s;
  ++m_total;
}

double TimeSamples::GetMean() const { return m_total == 0 ? 0.0 : m_s / m_total; }

double TimeSamples::GetSd() const
{
  if (m_total < 2)
    return 0.0;
  return std::max((m_s2 - m_s * m_s / static_cast<double>(m_total)) / (m_total - 1), 0.0);
}

double TimeSamples::GetVar() const { return sqrt(GetSd()); }
}  // namespace my
