#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <list>
#include <unordered_map>

/// \brief Implementation of cache with least recently used replacement policy.
/// All operations (Find, eviction) are O(1) amortized.
template <typename Key, typename Value>
class LruCache
{
  template <typename K, typename V>
  friend class LruCacheTest;

public:
  /// \param maxCacheSize Maximum size of the cache in number of items. It should be one or greater.
  explicit LruCache(size_t maxCacheSize) : m_maxCacheSize(maxCacheSize) { CHECK_GREATER(maxCacheSize, 0, ()); }

  // Find value by @key. If @key is found, returns reference to its value.
  Value & Find(Key const & key, bool & found)
  {
    auto const it = m_map.find(key);
    if (it != m_map.end())
    {
      // Move accessed element to front of LRU list (most recently used).
      m_lru.splice(m_lru.begin(), m_lru, it->second);
      found = true;
      return it->second->second;
    }

    if (m_map.size() >= m_maxCacheSize)
    {
      // Evict least recently used element (back of list).
      auto const & lruKey = m_lru.back().first;
      m_map.erase(lruKey);
      m_lru.pop_back();
    }

    m_lru.emplace_front(key, Value{});
    m_map.emplace(key, m_lru.begin());
    found = false;
    return m_lru.front().second;
  }

  void Clear()
  {
    m_map.clear();
    m_lru.clear();
  }

  /// \brief Checks for coherence class params.
  /// \note It's a time consumption method and should be called for tests only.
  bool IsValidForTesting() const
  {
    if (m_map.size() != m_lru.size())
      return false;

    for (auto it = m_lru.begin(); it != m_lru.end(); ++it)
    {
      auto const mapIt = m_map.find(it->first);
      if (mapIt == m_map.end())
        return false;
      if (mapIt->second != it)
        return false;
    }
    return true;
  }

private:
  using LruList = std::list<std::pair<Key, Value>>;
  using Map = std::unordered_map<Key, typename LruList::iterator>;

  size_t const m_maxCacheSize;
  LruList m_lru;
  Map m_map;
};
