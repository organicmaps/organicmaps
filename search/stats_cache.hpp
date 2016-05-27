#pragma once
#include "base/logging.hpp"

#include "std/unordered_map.hpp"
#include "std/utility.hpp"

namespace search
{
template <class TKey, class TValue>
class Cache
{
  unordered_map<TKey, TValue> m_map;

  /// query statistics
  size_t m_accesses;
  size_t m_misses;

  size_t m_emptyQueriesCount;  /// empty queries count at a row
  string m_name;               /// cache name for print functions

public:
  explicit Cache(string const & name)
    : m_accesses(0), m_misses(0), m_emptyQueriesCount(0), m_name(name)
  {
  }

  pair<TValue &, bool> Get(TKey const & key)
  {
    auto r = m_map.insert(make_pair(key, TValue()));

    ++m_accesses;
    if (r.second)
      ++m_misses;

    return pair<TValue &, bool>(r.first->second, r.second);
  }

  void Clear()
  {
    m_map.clear();
    m_accesses = m_misses = 0;
    m_emptyQueriesCount = 0;
  }

  /// Called at the end of every search query.
  void ClearIfNeeded()
  {
    if (m_accesses != 0)
    {
      LOG(LDEBUG, ("Cache", m_name, "Queries =", m_accesses, "From cache =", m_accesses - m_misses,
                   "Added =", m_misses));
      m_accesses = m_misses = 0;
      m_emptyQueriesCount = 0;
    }
    else if (++m_emptyQueriesCount > 5)
    {
      LOG(LDEBUG, ("Clearing cache", m_name));
      Clear();
    }
  }
};

}  // namespace search
