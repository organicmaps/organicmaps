#pragma once

#include "base/timer.hpp"

#include "std/cstdint.hpp"

namespace my
{
// This class can be used in measurements of code blocks performance.
// It can accumulate time samples, and can calculate mean time and
// standard deviation.
//
// *NOTE* This class is NOT thread-safe.
class TimeSamples final
{
public:
  void Add(double seconds);

  // Mean of the accumulated time samples.
  double GetMean() const;

  // Unbiased standard deviation of the accumulated time samples.
  double GetSD() const;

  // Unbiased variance of the accumulated time samples.
  double GetVar() const;

private:
  double m_sum = 0.0;
  double m_sum2 = 0.0;
  size_t m_total = 0;
};

// This class can be used as a single scoped time sample. It
// automatically adds time sample on destruction.
//
// *NOTE* This class is NOT thread-safe.
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
