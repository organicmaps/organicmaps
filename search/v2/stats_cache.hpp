#pragma once
#include "base/logging.hpp"

#include "std/unordered_map.hpp"


namespace search
{
namespace v2
{

template <class TKey, class TValue>
class StatsCache
{
  unordered_map<TKey, TValue> m_map;
  size_t m_count, m_new, m_emptyCount;
  char const * m_name;

public:
  explicit StatsCache(char const * name) : m_count(0), m_new(0), m_emptyCount(0), m_name(name) {}

  pair<TValue &, bool> Get(TKey const & key)
  {
    auto r = m_map.insert(make_pair(key, TValue()));

    ++m_count;
    if (r.second)
      ++m_new;

    return pair<TValue &, bool>(r.first->second, r.second);
  }

  void Clear()
  {
    m_map.clear();
    m_count = m_new = 0;
  }

  void FinishQuery()
  {
    if (m_count != 0)
    {
      LOG(LDEBUG, ("Cache", m_name, "Queries =", m_count, "From cache =", m_count - m_new, "Added =", m_new));
      m_count = m_new = 0;
    }
    else if (++m_emptyCount > 5)
    {
      Clear();
      m_emptyCount = 0;
    }
  }
};

} // namespace v2
} // namespace search
