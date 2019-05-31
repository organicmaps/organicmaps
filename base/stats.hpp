#pragma once

#include "base/base.hpp"

#include <sstream>
#include <string>

namespace base
{
template <typename T>
class NoopStats
{
public:
  NoopStats() {}
  void operator() (T const &) {}
  std::string GetStatsStr() const { return ""; }
};

template <typename T>
class AverageStats
{
public:
  AverageStats() = default;

  template <typename Iter>
  AverageStats(Iter begin, Iter end)
  {
    for (auto it = begin; it != end; ++it)
      operator()(*it);
  }

  void operator()(T const & x)
  {
    ++m_count;
    m_sum += x;
  }

  std::string GetStatsStr() const
  {
    std::ostringstream out;
    out << "N: " << m_count;
    if (m_count > 0)
      out << " Avg: " << GetAverage();
    return out.str();
  }

  double GetAverage() const { return m_count == 0 ? 0.0 : m_sum / static_cast<double>(m_count); }

  uint32_t GetCount() const { return m_count; }

private:
  uint32_t m_count = 0;
  double m_sum = 0.0;
};
}  // namespace base
