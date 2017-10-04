#include "testing/testing.hpp"

#include "base/clustering_map.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace base;
using namespace std;

namespace
{
template <typename T>
vector<T> Sort(vector<T> vs)
{
  sort(vs.begin(), vs.end());
  return vs;
}

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ClusteringMapAdapter
{
public:
  template <typename V>
  void Append(Key const & key, V && value)
  {
    m_m.Append(key, std::forward<V>(value));
  }

  void Union(Key const & u, Key const & v) { m_m.Union(u, v); }

  std::vector<Value> Get(Key const & key) { return Sort(m_m.Get(key)); }

private:
  ClusteringMap<Key, Value, Hash> m_m;
};

UNIT_TEST(ClusteringMap_Smoke)
{
  {
    ClusteringMapAdapter<int, string> m;
    TEST(m.Get(0).empty(), ());
    TEST(m.Get(1).empty(), ());

    m.Union(0, 1);
    TEST(m.Get(0).empty(), ());
    TEST(m.Get(1).empty(), ());
  }

  {
    ClusteringMapAdapter<int, string> m;
    m.Append(0, "Hello");
    m.Append(1, "World!");

    TEST_EQUAL(m.Get(0), vector<string>({"Hello"}), ());
    TEST_EQUAL(m.Get(1), vector<string>({"World!"}), ());

    m.Union(0, 1);
    TEST_EQUAL(m.Get(0), vector<string>({"Hello", "World!"}), ());
    TEST_EQUAL(m.Get(1), vector<string>({"Hello", "World!"}), ());

    m.Append(2, "alpha");
    m.Append(3, "beta");
    m.Append(4, "gamma");

    TEST_EQUAL(m.Get(2), vector<string>({"alpha"}), ());
    TEST_EQUAL(m.Get(3), vector<string>({"beta"}), ());
    TEST_EQUAL(m.Get(4), vector<string>({"gamma"}), ());

    m.Union(2, 3);
    m.Union(3, 4);

    TEST_EQUAL(m.Get(2), vector<string>({"alpha", "beta", "gamma"}), ());
    TEST_EQUAL(m.Get(3), vector<string>({"alpha", "beta", "gamma"}), ());
    TEST_EQUAL(m.Get(4), vector<string>({"alpha", "beta", "gamma"}), ());

    TEST_EQUAL(m.Get(5), vector<string>(), ());
    m.Union(2, 5);
    TEST_EQUAL(m.Get(5), vector<string>({"alpha", "beta", "gamma"}), ());
  }
}
}  // namespace
