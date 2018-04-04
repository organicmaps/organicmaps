#include "testing/testing.hpp"

#include "base/fifo_cache.hpp"

#include <list>
#include <set>
#include <unordered_map>

using namespace std;

template <typename Key, typename Value>
class FifoCacheTest
{
public:
  FifoCacheTest(size_t capacity, typename FifoCache<Key, Value>::Loader const & loader)
      : m_cache(capacity, loader)
  {
  }

  Value const & GetValue(Key const & key) { return m_cache.GetValue(key); }
  unordered_map<Key, Value> const & GetMap() const { return m_cache.m_map; }
  list<Key> const & GetList() const { return m_cache.m_list; }

  bool IsValid() const
  {
    set<Key> listKeys(m_cache.m_list.cbegin(), m_cache.m_list.cend());
    set<Key> mapKeys;

    for (auto const & kv : m_cache.m_map)
      mapKeys.insert(kv.first);

    return listKeys == mapKeys;
  }

private:
  FifoCache<Key, Value> m_cache;
};

UNIT_TEST(FifoCache_Smoke)
{
  using Key = int;
  using Value = int;
  FifoCacheTest<Key, Value> cache(3 /* capacity */, [](Key k, Value & v) { v = k; } /* loader */);

  TEST_EQUAL(cache.GetValue(1), 1, ());
  TEST_EQUAL(cache.GetValue(2), 2, ());
  TEST_EQUAL(cache.GetValue(3), 3, ());
  TEST_EQUAL(cache.GetValue(4), 4, ());
  TEST_EQUAL(cache.GetValue(1), 1, ());
  TEST(cache.IsValid(), ());
}

UNIT_TEST(FifoCache)
{
  using Key = int;
  using Value = int;
  FifoCacheTest<Key, Value> cache(3 /* capacity */, [](Key k, Value & v) { v = k; } /* loader */);

  TEST_EQUAL(cache.GetValue(1), 1, ());
  TEST_EQUAL(cache.GetValue(3), 3, ());
  TEST_EQUAL(cache.GetValue(2), 2, ());
  TEST(cache.IsValid(), ());
  {
    unordered_map<Key, Value> expectedMap({{1 /* key */, 1 /* value */}, {2, 2}, {3, 3}});
    TEST_EQUAL(cache.GetMap(), expectedMap, ());
    list<Key> expectedList({2, 3, 1});
    TEST_EQUAL(cache.GetList(), expectedList, ());
  }

  TEST_EQUAL(cache.GetValue(7), 7, ());
  TEST(cache.IsValid(), ());
  {
    unordered_map<Key, Value> expectedMap({{7 /* key */, 7 /* value */}, {2, 2}, {3, 3}});
    TEST_EQUAL(cache.GetMap(), expectedMap, ());
    list<Key> expectedList({7, 2, 3});
    TEST_EQUAL(cache.GetList(), expectedList, ());
  }
}

UNIT_TEST(FifoCache_LoaderCalls)
{
  using Key = int;
  using Value = int;
  bool shouldLoadBeCalled = true;
  auto loader = [&shouldLoadBeCalled](Key k, Value & v) {
    TEST(shouldLoadBeCalled, ());
    v = k;
  };

  FifoCacheTest<Key, Value> cache(3 /* capacity */, loader);
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
  cache.GetValue(1);
  TEST(cache.IsValid(), ());
}
