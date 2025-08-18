#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <unordered_map>

/// \brief Implementation of cache with least recently used replacement policy.
template <typename Key, typename Value>
class LruCache
{
  template <typename K, typename V>
  friend class LruCacheTest;
  template <typename K, typename V>
  friend class LruCacheKeyAgeTest;

public:
  /// \param maxCacheSize Maximum size of the cache in number of items. It should be one or greater.
  /// \param loader Function which is called if it's necessary to load a new item for the cache.
  /// For the same |key| should be loaded the same |value|.
  explicit LruCache(size_t maxCacheSize) : m_maxCacheSize(maxCacheSize) { CHECK_GREATER(maxCacheSize, 0, ()); }

  // Find value by @key. If @key is found, returns reference to its value.
  Value & Find(Key const & key, bool & found)
  {
    auto const it = m_cache.find(key);
    if (it != m_cache.cend())
    {
      m_keyAge.UpdateAge(key);
      found = true;
      return it->second;
    }

    if (m_cache.size() >= m_maxCacheSize)
    {
      m_cache.erase(m_keyAge.GetLruKey());
      m_keyAge.RemoveLru();
    }

    m_keyAge.InsertKey(key);
    Value & value = m_cache[key];
    found = false;
    return value;
  }

  void Clear()
  {
    m_cache.clear();
    m_keyAge.Clear();
  }

  /// \brief Checks for coherence class params.
  /// \note It's a time consumption method and should be called for tests only.
  bool IsValidForTesting() const
  {
    if (!m_keyAge.IsValidForTesting())
      return false;

    for (auto const & kv : m_cache)
      if (!m_keyAge.IsKeyValidForTesting(kv.first))
        return false;

    return true;
  }

private:
  /// \brief This class support cross mapping from age to key and for key to age.
  /// It lets effectively get least recently used key (key with minimum value of age)
  /// and find key age by its value to update the key age.
  /// \note Size of |m_ageToKey| and |m_ageToKey| should be the same.
  /// All keys of |m_ageToKey| should be values of |m_ageToKey| and on the contrary
  /// all keys of |m_ageToKey| should be values of |m_ageToKey|.
  /// \note Ages should be unique for all keys.
  class KeyAge
  {
    template <typename K, typename V>
    friend class LruCacheKeyAgeTest;

  public:
    void Clear()
    {
      m_age = 0;
      m_ageToKey.clear();
      m_keyToAge.clear();
    }

    /// \brief Increments |m_age| and insert key to |m_ageToKey| and |m_keyToAge|.
    /// \note This method should be used only if there's no |key| in |m_ageToKey| and |m_keyToAge|.
    void InsertKey(Key const & key)
    {
      ++m_age;
      m_ageToKey[m_age] = key;
      m_keyToAge[key] = m_age;
    }

    /// \brief Increments |m_age| and updates key age in |m_ageToKey| and |m_keyToAge|.
    /// \note This method should be used only if there's |key| in |m_ageToKey| and |m_keyToAge|.
    void UpdateAge(Key const & key)
    {
      ++m_age;
      auto keyToAgeIt = m_keyToAge.find(key);
      CHECK(keyToAgeIt != m_keyToAge.end(), ());
      // Removing former age.
      size_t const removed = m_ageToKey.erase(keyToAgeIt->second);
      CHECK_EQUAL(removed, 1, ());
      // Putting new age.
      m_ageToKey[m_age] = key;
      keyToAgeIt->second = m_age;
    }

    /// \returns Least recently used key without updating the age.
    /// \note |m_ageToKey| and |m_keyToAge| shouldn't be empty.
    Key const & GetLruKey() const
    {
      CHECK(!m_ageToKey.empty(), ());
      // The smaller age the older item.
      return m_ageToKey.cbegin()->second;
    }

    void RemoveLru()
    {
      Key const & lru = GetLruKey();
      size_t const removed = m_keyToAge.erase(lru);
      CHECK_EQUAL(removed, 1, ());
      m_ageToKey.erase(m_ageToKey.begin());
    }

    /// \brief Checks for coherence class params.
    /// \note It's a time consumption method and should be called for tests only.
    bool IsValidForTesting() const
    {
      for (auto const & kv : m_ageToKey)
      {
        if (kv.first > m_age)
          return false;
        if (m_keyToAge.find(kv.second) == m_keyToAge.cend())
          return false;
      }
      for (auto const & kv : m_keyToAge)
      {
        if (kv.second > m_age)
          return false;
        if (m_ageToKey.find(kv.second) == m_ageToKey.cend())
          return false;
      }
      return true;
    }

    /// \returns true if |key| and its age are contained in |m_keyToAge| and |m_keyToAge|.
    /// \note It's a time consumption method and should be called for tests only.
    bool IsKeyValidForTesting(Key const & key) const
    {
      auto const keyToAgeId = m_keyToAge.find(key);
      if (keyToAgeId == m_keyToAge.cend())
        return false;
      auto const ageToKeyIt = m_ageToKey.find(keyToAgeId->second);
      if (ageToKeyIt == m_ageToKey.cend())
        return false;
      return key == ageToKeyIt->second;
    }

  private:
    size_t m_age = 0;
    std::map<size_t, Key> m_ageToKey;
    std::unordered_map<Key, size_t> m_keyToAge;
  };

  size_t const m_maxCacheSize;
  std::unordered_map<Key, Value> m_cache;
  KeyAge m_keyAge;
};
