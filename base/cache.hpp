#include "base/assert.hpp"
#include "base/base.hpp"
#include "std/type_traits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace my
{
  // Simple cache that stores list of values.
  template <typename KeyT, typename ValueT> class Cache
  {
  private:
    Cache(Cache<KeyT, ValueT> const &);                             // Not implemented.
    Cache<KeyT, ValueT> & operator = (Cache<KeyT, ValueT> const &); // Not implemented.

  public:
    // @logCacheSize is pow of two for number of elements in cache.
    explicit Cache(uint32_t logCacheSize)
      : m_Cache(new Data[1 << logCacheSize]), m_HashMask((1 << logCacheSize) - 1)
    {
      STATIC_ASSERT((is_same<KeyT, uint32_t>::value ||
                     is_same<KeyT, uint64_t>::value));

      // We always use cache with static constant. So debug assert is enough here.
      ASSERT_GREATER ( logCacheSize, 0, () );
      ASSERT_GREATER ( m_HashMask, 0, () );
      ASSERT_LESS(logCacheSize, 32, ());

      Reset();
    }

    ~Cache()
    {
      delete [] m_Cache;
    }

    uint32_t GetCacheSize() const
    {
      return m_HashMask + 1;
    }

    // Find value by @key. If @key is found, returns reference to its value.
    // If @key is not found, some other key is removed and it's value is reused without
    // re-initialization. It's up to caller to re-initialize the new value if @found == false.
    // TODO: Return pair<ValueT *, bool> instead?
    ValueT & Find(KeyT const & key, bool & found)
    {
      Data & data = m_Cache[Index(key)];
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
      for (uint32_t i = 0; i <= m_HashMask; ++i)
        f(m_Cache[i].m_Value);
    }

    void Reset()
    {
      // Initialize m_Cache such, that Index(m_Cache[i].m_Key) != i.
      for (uint32_t i = 0; i <= m_HashMask; ++i)
      {
        KeyT & key = m_Cache[i].m_Key;
        for (key = 0; Index(key) == i; ++key) ;
      }
    }

  private:
    inline size_t Index(KeyT const & key) const
    {
      return static_cast<size_t>(Hash(key) & m_HashMask);
    }

    inline static uint32_t Hash(uint32_t x)
    {
      x = (x ^ 61) ^ (x >> 16);
      x = x + (x << 3);
      x = x ^ (x >> 4);
      x = x * 0x27d4eb2d;
      x = x ^ (x >> 15);
      return x;
    }
    inline static uint32_t Hash(uint64_t x)
    {
      return Hash(uint32_t(x) ^ uint32_t(x >> 32));
    }

    // TODO: Consider using separate arrays for keys and values, to save on padding.
    struct Data
    {
      Data() : m_Key(), m_Value() {}
      KeyT m_Key;
      ValueT m_Value;
    };

    Data * const m_Cache;
    uint32_t const m_HashMask;
  };

  // Simple cache that stores list of values and provides cache missing statistics.
  // CacheWithStat class has the same interface as Cache class
  template <typename TKey, typename TValue>
  class CacheWithStat
  {
  public:
    // @logCacheSize is pow of two for number of elements in cache.
    explicit CacheWithStat(uint32_t logCacheSize)
      : m_cache(logCacheSize), m_miss(0), m_access(0)
    {
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
}
