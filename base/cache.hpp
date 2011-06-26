#include "assert.hpp"
#include "base.hpp"
#include "../std/type_traits.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

namespace my
{
  // Simple Cache that stores list of most recently used values.
  template <typename KeyT, typename ValueT> class Cache
  {
  private:
    Cache(Cache<KeyT, ValueT> const &);                             // Not implemented.
    Cache<KeyT, ValueT> & operator = (Cache<KeyT, ValueT> const &); // Not implemented.
  public:
    // Create cache with maximum size @maxCachedObjects.
    explicit Cache(uint32_t logCacheSize)
      : m_Cache(new Data[1 << logCacheSize]), m_HashMask((1 << logCacheSize) - 1)
    {
      STATIC_ASSERT((is_same<KeyT, uint32_t>::value ||
                     is_same<KeyT, uint64_t>::value));
      CHECK_GREATER(logCacheSize, 1, ());
      CHECK_LESS(logCacheSize, 32, ());
      uint32_t const cacheSize = 1 << logCacheSize;
      // Initialize m_Cache such, that (Hash(m_Cache[i].m_Key) & m_HashMask) != i.
      for (uint32_t i = 0; i < cacheSize; ++i)
        for (m_Cache[i].m_Key = 0; (Hash(m_Cache[i].m_Key) & m_HashMask) == i; ++m_Cache[i].m_Key) ;
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
      Data & data = m_Cache[Hash(key) & m_HashMask];
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

    bool HasKey(KeyT const & key)
    {
      Data & data = m_Cache[Hash(key) & m_HashMask];
      return data.m_Key == key;
    }

    template <typename F>
    void ForEachValue(F f)
    {
      for (uint32_t i = 0; i <= m_HashMask; ++i)
        f(m_Cache[i].m_Value);
    }

  private:
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
}
