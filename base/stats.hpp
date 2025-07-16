#pragma once

#include "base/logging.hpp"

#include <queue>
#include <sstream>
#include <string>
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

template <class T>
class StatsCollector
{
  std::vector<std::pair<std::string, AverageStats<T>>> m_vec;

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

  AverageStats<T> & Get(size_t i) { return m_vec[i].second; }
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
