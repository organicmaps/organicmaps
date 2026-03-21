#include "testing/testing.hpp"

#include "base/clustering_map.hpp"
#include "base/internal/message.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
template <typename T>
std::vector<T> Sort(std::vector<T> vs)
{
  std::sort(vs.begin(), vs.end());
  return vs;
}

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ClusteringMapAdapter
{
public:
  struct Cluster
  {
    Cluster(Key const & key, Value const & value) : m_keys({key}), m_values({value}) {}

    Cluster(std::vector<Key> const & keys, std::vector<Value> const & values) : m_keys(keys), m_values(values)
    {
      std::sort(m_keys.begin(), m_keys.end());
      std::sort(m_values.begin(), m_values.end());
    }

    bool operator<(Cluster const & rhs) const
    {
      if (m_keys != rhs.m_keys)
        return m_keys < rhs.m_keys;
      return m_values < rhs.m_values;
    }

    bool operator==(Cluster const & rhs) const { return m_keys == rhs.m_keys && m_values == rhs.m_values; }

    friend std::string DebugPrint(Cluster const & cluster)
    {
      std::ostringstream os;
      os << "Cluster [";
      os << "keys: " << ::DebugPrint(cluster.m_keys) << ", ";
      os << "values: " << ::DebugPrint(cluster.m_values);
      os << "]";
      return os.str();
    }

    std::vector<Key> m_keys;
    std::vector<Value> m_values;
  };

  template <typename V>
  void Append(Key const & key, V && value)
  {
    m_m.Append(key, std::forward<V>(value));
  }

  void Union(Key const & u, Key const & v) { m_m.Union(u, v); }

  std::vector<Value> Get(Key const & key) { return Sort(m_m.Get(key)); }

  std::vector<Cluster> Clusters()
  {
    std::vector<Cluster> clusters;

    m_m.ForEachCluster([&](std::vector<Key> const & keys, std::vector<Value> const & values)
    { clusters.emplace_back(keys, values); });
    std::sort(clusters.begin(), clusters.end());
    return clusters;
  }

private:
  base::ClusteringMap<Key, Value, Hash> m_m;
};

UNIT_TEST(ClusteringMap_Smoke)
{
  {
    ClusteringMapAdapter<int, std::string> m;
    TEST(m.Get(0).empty(), ());
    TEST(m.Get(1).empty(), ());

    m.Union(0, 1);
    TEST(m.Get(0).empty(), ());
    TEST(m.Get(1).empty(), ());
  }

  {
    ClusteringMapAdapter<int, std::string> m;
    m.Append(0, "Hello");
    m.Append(1, "World!");

    TEST_EQUAL(m.Get(0), std::vector<std::string>({"Hello"}), ());
    TEST_EQUAL(m.Get(1), std::vector<std::string>({"World!"}), ());

    m.Union(0, 1);
    TEST_EQUAL(m.Get(0), std::vector<std::string>({"Hello", "World!"}), ());
    TEST_EQUAL(m.Get(1), std::vector<std::string>({"Hello", "World!"}), ());

    m.Append(2, "alpha");
    m.Append(3, "beta");
    m.Append(4, "gamma");

    TEST_EQUAL(m.Get(2), std::vector<std::string>({"alpha"}), ());
    TEST_EQUAL(m.Get(3), std::vector<std::string>({"beta"}), ());
    TEST_EQUAL(m.Get(4), std::vector<std::string>({"gamma"}), ());

    m.Union(2, 3);
    m.Union(3, 4);

    TEST_EQUAL(m.Get(2), std::vector<std::string>({"alpha", "beta", "gamma"}), ());
    TEST_EQUAL(m.Get(3), std::vector<std::string>({"alpha", "beta", "gamma"}), ());
    TEST_EQUAL(m.Get(4), std::vector<std::string>({"alpha", "beta", "gamma"}), ());

    TEST_EQUAL(m.Get(5), std::vector<std::string>(), ());
    m.Union(2, 5);
    TEST_EQUAL(m.Get(5), std::vector<std::string>({"alpha", "beta", "gamma"}), ());
  }
}

UNIT_TEST(ClusteringMap_ForEach)
{
  using Map = ClusteringMapAdapter<int, std::string>;
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
      std::vector<Cluster> const expected = {
          {Cluster{0, "Hello"}, Cluster{1, "World!"}, Cluster{2, "alpha"}, Cluster{3, "beta"}, Cluster{4, "gamma"}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(0, 1);
    {
      std::vector<Cluster> const expected = {
          {Cluster{{0, 1}, {"Hello", "World!"}}, Cluster{2, "alpha"}, Cluster{3, "beta"}, Cluster{4, "gamma"}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(2, 3);
    m.Union(3, 4);
    {
      std::vector<Cluster> const expected = {
          {Cluster{{0, 1}, {"Hello", "World!"}}, Cluster{{2, 3, 4}, {"alpha", "beta", "gamma"}}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }

    m.Union(0, 3);
    {
      std::vector<Cluster> const expected = {{Cluster{{0, 1, 2, 3, 4}, {"Hello", "World!", "alpha", "beta", "gamma"}}}};
      TEST_EQUAL(expected, m.Clusters(), ());
    }
  }
}
}  // namespace
