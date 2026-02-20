#include "testing/testing.hpp"

#include "base/lru_cache.hpp"

#include <cstddef>
#include <functional>

template <typename Key, typename Value>
class LruCacheTest
{
public:
  using Loader = std::function<void(Key const & key, Value & value)>;

  LruCacheTest(size_t maxCacheSize, Loader const & loader) : m_cache(maxCacheSize), m_loader(loader) {}

  Value const & GetValue(Key const & key)
  {
    bool found;
    auto & value = m_cache.Find(key, found);
    if (!found)
      m_loader(key, value);
    return value;
  }

  bool IsValid() const { return m_cache.IsValidForTesting(); }

private:
  LruCache<Key, Value> m_cache;
  Loader m_loader;
};

UNIT_TEST(LruCacheSmokeTest)
{
  using Key = int;
  using Value = int;

  {
    LruCacheTest<Key, Value> cache(1 /* maxCacheSize */, [](Key k, Value & v) { v = 1; } /* loader */);
    TEST_EQUAL(cache.GetValue(1), 1, ());
    TEST_EQUAL(cache.GetValue(2), 1, ());
    TEST_EQUAL(cache.GetValue(3), 1, ());
    TEST_EQUAL(cache.GetValue(4), 1, ());
    TEST_EQUAL(cache.GetValue(1), 1, ());
    TEST(cache.IsValid(), ());
  }

  {
    LruCacheTest<Key, Value> cache(3 /* maxCacheSize */, [](Key k, Value & v) { v = k; } /* loader */);
    TEST_EQUAL(cache.GetValue(1), 1, ());
    TEST_EQUAL(cache.GetValue(2), 2, ());
    TEST_EQUAL(cache.GetValue(2), 2, ());
    TEST_EQUAL(cache.GetValue(5), 5, ());
    TEST_EQUAL(cache.GetValue(1), 1, ());
    TEST(cache.IsValid(), ());
  }
}

UNIT_TEST(LruCacheLoaderCallsTest)
{
  using Key = int;
  using Value = int;
  bool shouldLoadBeCalled = true;
  auto loader = [&shouldLoadBeCalled](Key k, Value & v)
  {
    TEST(shouldLoadBeCalled, ());
    v = k;
  };

  LruCacheTest<Key, Value> cache(3 /* maxCacheSize */, loader);
  TEST(cache.IsValid(), ());
  cache.GetValue(1);
  cache.GetValue(2);
  cache.GetValue(3);
  TEST(cache.IsValid(), ());
  shouldLoadBeCalled = false;
  cache.GetValue(3);
  cache.GetValue(2);
  cache.GetValue(1);
  TEST(cache.IsValid(), ());
  shouldLoadBeCalled = true;
  cache.GetValue(4);
  TEST(cache.IsValid(), ());
  shouldLoadBeCalled = false;
  cache.GetValue(1);
  TEST(cache.IsValid(), ());
}

UNIT_TEST(LruCacheEvictionOrder)
{
  using Key = int;
  using Value = int;
  // Verify that the LRU element is actually the one evicted.
  LruCacheTest<Key, Value> cache(2 /* maxCacheSize */, [](Key k, Value & v) { v = k; });
  cache.GetValue(1);  // cache: [1]
  cache.GetValue(2);  // cache: [2, 1]
  cache.GetValue(1);  // cache: [1, 2] — 1 refreshed to front
  TEST(cache.IsValid(), ());

  // Adding 3 should evict 2 (the LRU), not 1.
  bool loaderCalled = false;
  auto checkLoader = [&loaderCalled](Key k, Value & v)
  {
    loaderCalled = true;
    v = k;
  };

  LruCacheTest<Key, Value> cache2(2, checkLoader);
  cache2.GetValue(1);
  cache2.GetValue(2);
  cache2.GetValue(1);  // refresh 1
  loaderCalled = false;
  cache2.GetValue(1);  // should be cached
  TEST(!loaderCalled, ());
  cache2.GetValue(3);  // evicts 2
  TEST(loaderCalled, ());
  loaderCalled = false;
  cache2.GetValue(1);  // still cached
  TEST(!loaderCalled, ());
  cache2.GetValue(2);  // was evicted, must reload
  TEST(loaderCalled, ());
  TEST(cache2.IsValid(), ());
}
