#pragma once

#include "../../std/string.hpp"
#include "../../std/utility.hpp"

namespace bench
{
  struct Result
  {
    double m_time;
    size_t m_count;

    Result() : m_time(0), m_count(0) {}

    void Add(double t)
    {
      m_time += t;
      ++m_count;
    }

    void Add(Result const & r)
    {
      m_time += r.m_time;
      m_count += r.m_count;
    }
  };

  struct AllResult
  {
    Result m_all, m_reading;

    void Print(bool perFrame) const;
  };

  /// @param[in] count number of times to run benchmark
  AllResult RunFeaturesLoadingBenchmark(
      string const & file, size_t count, pair<int, int> scaleR);
}
