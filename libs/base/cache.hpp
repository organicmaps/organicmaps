#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/macros.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace base
{
// Simple cache that stores list of values.
template <typename KeyT, typename ValueT>
class Cache
{
  DISALLOW_COPY(Cache);

public:
  Cache() = default;
  Cache(Cache && r) = default;

  /// @param[in] logCacheSize is pow of two for number of elements in cache.
  explicit Cache(uint32_t logCacheSize) { Init(logCacheSize); }

  /// @param[in] logCacheSize is pow of two for number of elements in cache.
  void Init(uint32_t logCacheSize)
  {
    ASSERT(logCacheSize > 0 && logCacheSize < 32, (logCacheSize));
    static_assert((std::is_same<KeyT, uint32_t>::value || std::is_same<KeyT, uint64_t>::value), "");

    m_cache.reset(new Data[1 << logCacheSize]);
    m_hashMask = (1 << logCacheSize) - 1;

    Reset();
  }

  uint32_t GetCacheSize() const { return m_hashMask + 1; }

  // Find value by @key. If @key is found, returns reference to its value.
  // If @key is not found, some other key is removed and it's value is reused without
  // re-initialization. It's up to caller to re-initialize the new value if @found == false.
  // TODO: Return pair<ValueT *, bool> instead?
  ValueT & Find(KeyT const & key, bool & found)
  {
    Data & data = m_cache[Index(key)];
    if (data.m_Key == key)
    {
      found = true;
    }
    else
    {
      found = false;
      data.m_Key = key;
    }
    return data.m_Value;
  }

  template <typename F>
  void ForEachValue(F && f)
  {
    for (uint32_t i = 0; i <= m_hashMask; ++i)
      f(m_cache[i].m_Value);
  }

  void Reset()
  {
    // Initialize m_Cache such, that Index(m_Cache[i].m_Key) != i.
    for (uint32_t i = 0; i <= m_hashMask; ++i)
    {
      KeyT & key = m_cache[i].m_Key;
      for (key = 0; Index(key) == i; ++key)
        ;
    }
  }

private:
  size_t Index(KeyT const & key) const { return static_cast<size_t>(Hash(key) & m_hashMask); }

  static uint32_t Hash(uint32_t x)
  {
    x = (x ^ 61) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2d;
    x = x ^ (x >> 15);
    return x;
  }

  static uint32_t Hash(uint64_t x) { return Hash(uint32_t(x) ^ uint32_t(x >> 32)); }

  // TODO: Consider using separate arrays for keys and values, to save on padding.
  struct Data
  {
    Data() : m_Key(), m_Value() {}
    KeyT m_Key;
    ValueT m_Value;
  };

  std::unique_ptr<Data[]> m_cache;
  uint32_t m_hashMask;
};

// Simple cache that stores list of values and provides cache missing statistics.
// CacheWithStat class has the same interface as Cache class
template <typename TKey, typename TValue>
class CacheWithStat
{
public:
  CacheWithStat() = default;

  explicit CacheWithStat(uint32_t logCacheSize) : m_cache(logCacheSize), m_miss(0), m_access(0) {}

  /// @param[in] logCacheSize is pow of two for number of elements in cache.
  void Init(uint32_t logCacheSize)
  {
    m_cache.Init(logCacheSize);
    m_miss = m_access = 0;
  }

  double GetCacheMiss() const
  {
    if (m_access == 0)
      return 0.0;
    return static_cast<double>(m_miss) / static_cast<double>(m_access);
  }

  uint32_t GetCacheSize() const { return m_cache.GetCacheSize(); }

  TValue & Find(TKey const & key, bool & found)
  {
    ++m_access;
    TValue & res = m_cache.Find(key, found);
    if (!found)
      ++m_miss;
    return res;
  }

  template <typename F>
  void ForEachValue(F && f)
  {
    m_cache.ForEachValue(std::forward<F>(f));
  }

  void Reset()
  {
    m_cache.Reset();
    m_access = 0;
    m_miss = 0;
  }

private:
  Cache<TKey, TValue> m_cache;
  uint64_t m_miss;
  uint64_t m_access;
};
}  // namespace base
