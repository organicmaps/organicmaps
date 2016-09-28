#include "base/time_samples.hpp"

#include "std/cmath.hpp"

namespace my
{
void TimeSamples::Add(double seconds)
{
  m_sum += seconds;
  m_sum2 += seconds * seconds;
  ++m_total;
}

double TimeSamples::GetMean() const { return m_total == 0 ? 0.0 : m_sum / m_total; }

double TimeSamples::GetSD() const
{
  if (m_total < 2)
    return 0.0;
  return std::max((m_sum2 - m_sum * m_sum / static_cast<double>(m_total)) / (m_total - 1), 0.0);
}

double TimeSamples::GetVar() const { return sqrt(GetSD()); }
}  // namespace my
