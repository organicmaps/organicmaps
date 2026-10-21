#pragma once

#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace base
{

template <typename T>
class NoopStats
{
public:
  NoopStats() {}
  void operator()(T const &) {}
  std::string GetStatsStr() const { return ""; }
};

template <typename T>
class AverageStats
{
public:
  AverageStats() = default;
  template <class ContT>
  explicit AverageStats(ContT const & cont)
  {
    for (auto const & e : cont)
      (*this)(e);
  }

  void operator()(T const & x)
  {
    if (m_count == 0)
      m_min = m_max = x;
    else
    {
      m_min = std::min(m_min, x);
      m_max = std::max(m_max, x);
    }
    ++m_count;
    m_sum += x;
  }

  void Merge(AverageStats const & other)
  {
    if (other.m_count == 0)
      return;
    if (m_count == 0)
    {
      *this = other;
      return;
    }
    m_min = std::min(m_min, other.m_min);
    m_max = std::max(m_max, other.m_max);
    m_sum += other.m_sum;
    m_count += other.m_count;
  }

  std::string ToString() const
  {
    std::ostringstream out;
    out << "N = " << m_count << "; Total = " << m_sum << "; Min = " << m_min << "; Max = " << m_max
        << "; Avg = " << GetAverage();
    return out.str();
  }

  double GetAverage() const { return m_count == 0 ? 0.0 : m_sum / static_cast<double>(m_count); }
  uint64_t GetCount() const { return m_count; }
  T GetMin() const { return m_min; }
  T GetMax() const { return m_max; }
  T GetTotal() const { return m_sum; }

private:
  uint64_t m_count = 0;
  T m_min = 0;
  T m_max = 0;
  T m_sum = 0;
};

template <class T>
class StatsCollector
{
public:
  using Map = std::map<std::string, AverageStats<T>, std::less<>>;

  StatsCollector() = default;
  ~StatsCollector()
  {
    for (auto const & [name, stats] : GetStats())
      LOG_SHORT(LINFO, (name, ":", stats.ToString()));
  }

  void Add(std::string_view name, T value)
  {
    std::lock_guard lock(m_mutex);
    GetStatsImpl(name)(value);
  }

  void Add(std::string_view name, AverageStats<T> const & stats)
  {
    std::lock_guard lock(m_mutex);
    GetStatsImpl(name).Merge(stats);
  }

  Map GetStats() const
  {
    std::lock_guard lock(m_mutex);
    return m_stats;
  }

private:
  AverageStats<T> & GetStatsImpl(std::string_view name)
  {
    auto const it = m_stats.find(name);
    if (it != m_stats.end())
      return it->second;
    return m_stats.try_emplace(std::string(name)).first->second;
  }

  mutable std::mutex m_mutex;
  Map m_stats;
};

template <class Key>
class TopStatsCounter
{
  std::unordered_map<Key, size_t> m_data;

public:
  void Add(Key const & key) { ++m_data[key]; }

  void PrintTop(size_t count) const
  {
    ASSERT(count > 0, ());

    using PtrT = std::pair<Key const, size_t> const *;
    struct GreaterNumber
    {
      bool operator()(PtrT l, PtrT r) const { return l->second > r->second; }
    };

    std::priority_queue<PtrT, std::vector<PtrT>, GreaterNumber> queue;

    for (auto const & p : m_data)
    {
      if (queue.size() >= count)
      {
        if (queue.top()->second >= p.second)
          continue;
        queue.pop();
      }

      queue.push(&p);
    }

    std::vector<PtrT> vec;
    vec.reserve(count);

    while (!queue.empty())
    {
      vec.push_back(queue.top());
      queue.pop();
    }

    for (auto i = vec.rbegin(); i != vec.rend(); ++i)
    {
      PtrT const p = *i;
      LOG_SHORT(LINFO, (p->first, ":", p->second));
    }
  }
};

}  // namespace base
