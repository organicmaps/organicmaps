#pragma once

#include "base/logging.hpp"

#include <sstream>
#include <string>
#include <vector>

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
  template <class ContT> explicit AverageStats(ContT const & cont)
  {
    for (auto const & e : cont)
      (*this)(e);
  }

  void operator()(T const & x)
  {
    ++m_count;
    if (x > m_max)
      m_max = x;
    m_sum += x;
  }

  std::string ToString() const
  {
    std::ostringstream out;
    out << "N = " << m_count << "; Total = " << m_sum << "; Max = " << m_max << "; Avg = " << GetAverage();
    return out.str();
  }

  double GetAverage() const { return m_count == 0 ? 0.0 : m_sum / static_cast<double>(m_count); }
  uint32_t GetCount() const { return m_count; }

private:
  uint32_t m_count = 0;
  T m_max = 0;
  T m_sum = 0;
};

template <class T> class StatsCollector
{
  std::vector<std::pair<std::string, base::AverageStats<T>>> m_vec;

public:
  explicit StatsCollector(std::initializer_list<std::string> init)
  {
    for (auto & name : init)
      m_vec.push_back({std::move(name), {}});
  }
  ~StatsCollector()
  {
    for (auto const & e : m_vec)
      LOG_SHORT(LINFO, (e.first, ":", e.second.ToString()));
  }

  base::AverageStats<T> & Get(size_t i) { return m_vec[i].second; }
};

}  // namespace base
