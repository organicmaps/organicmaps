#pragma once

#include "base/timer.hpp"

#include "std/cstdint.hpp"

namespace my
{
class TimeSamples final
{
public:
  void Add(double s);

  double GetMean() const;
  double GetSd() const;
  double GetVar() const;

private:
  double m_s = 0.0;
  double m_s2 = 0.0;
  size_t m_total = 0;
};

class ScopedTimeSample final
{
public:
  ScopedTimeSample(TimeSamples & ts) : m_ts(ts) {}
  ~ScopedTimeSample() { m_ts.Add(m_t.ElapsedSeconds()); }

private:
  TimeSamples & m_ts;
  my::Timer m_t;
};

}  // namespace my
