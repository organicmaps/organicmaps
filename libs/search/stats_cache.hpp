#pragma once

#include "base/logging.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace search
{
template <class Key, class Value>
class Cache
{
  std::unordered_map<Key, Value> m_map;

  /// query statistics
  size_t m_accesses;
  size_t m_misses;

  size_t m_emptyQueriesCount;  /// empty queries count at a row
  std::string m_name;          /// cache name for print functions

public:
  explicit Cache(std::string const & name) : m_accesses(0), m_misses(0), m_emptyQueriesCount(0), m_name(name) {}

  std::pair<Value &, bool> Get(Key const & key)
  {
    auto r = m_map.insert(std::make_pair(key, Value()));

    ++m_accesses;
    if (r.second)
      ++m_misses;

    return std::pair<Value &, bool>(r.first->second, r.second);
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
      LOG(LDEBUG,
          ("Cache", m_name, "Queries =", m_accesses, "From cache =", m_accesses - m_misses, "Added =", m_misses));
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
