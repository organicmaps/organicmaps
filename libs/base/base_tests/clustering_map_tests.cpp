#include "testing/testing.hpp"

#include "base/clustering_map.hpp"
#include "base/internal/message.hpp"

#include <algorithm>
#include <sstream>
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

template <typename Key, typename Value, typename Hash = hash<Key>>
class ClusteringMapAdapter
{
public:
  struct Cluster
  {
    Cluster(Key const & key, Value const & value) : m_keys({key}), m_values({value}) {}

    Cluster(vector<Key> const & keys, vector<Value> const & values) : m_keys(keys), m_values(values)
    {
      sort(m_keys.begin(), m_keys.end());
      sort(m_values.begin(), m_values.end());
    }

    bool operator<(Cluster const & rhs) const
    {
      if (m_keys != rhs.m_keys)
        return m_keys < rhs.m_keys;
      return m_values < rhs.m_values;
    }

    bool operator==(Cluster const & rhs) const { return m_keys == rhs.m_keys && m_values == rhs.m_values; }

    friend string DebugPrint(Cluster const & cluster)
    {
      ostringstream os;
      os << "Cluster [";
      os << "keys: " << ::DebugPrint(cluster.m_keys) << ", ";
      os << "values: " << ::DebugPrint(cluster.m_values);
      os << "]";
      return os.str();
    }

    vector<Key> m_keys;
    vector<Value> m_values;
  };

  template <typename V>
  void Append(Key const & key, V && value)
  {
    m_m.Append(key, std::forward<V>(value));
  }

  void Union(Key const & u, Key const & v) { m_m.Union(u, v); }

  vector<Value> Get(Key const & key) { return Sort(m_m.Get(key)); }

  vector<Cluster> Clusters()
  {
    vector<Cluster> clusters;

    m_m.ForEachCluster([&](vector<Key> const & keys, vector<Value> const & values)
    { clusters.emplace_back(keys, values); });
    sort(clusters.begin(), clusters.end());
    return clusters;
  }

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

UNIT_TEST(ClusteringMap_ForEach)
{
  using Map = ClusteringMapAdapter<int, string>;
  using Cluster = Map::Cluster;

  {
    Map m;
    auto const clusters = m.Clusters();
    TEST(clusters.empty(), (clusters));
  }

  {
    Map m;
    m.Append(0, "Hello");
    m.Append(1, "World!");
    m.Append(2, "alpha");
    m.Append(3, "beta");
    m.Append(4, "gamma");

    {
      vector<Cluster> const expected = {
          {Cluster{0, "Hello"}, Cluster{1, "World!"}, Cluster{2, "alpha"}, Cluster{3, "beta"}, Cluster{4, "gamma"}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(0, 1);
    {
      vector<Cluster> const expected = {
          {Cluster{{0, 1}, {"Hello", "World!"}}, Cluster{2, "alpha"}, Cluster{3, "beta"}, Cluster{4, "gamma"}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(2, 3);
    m.Union(3, 4);
    {
      vector<Cluster> const expected = {
          {Cluster{{0, 1}, {"Hello", "World!"}}, Cluster{{2, 3, 4}, {"alpha", "beta", "gamma"}}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(0, 3);
    {
      vector<Cluster> const expected = {{Cluster{{0, 1, 2, 3, 4}, {"Hello", "World!", "alpha", "beta", "gamma"}}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }
  }
}
}  // namespace
