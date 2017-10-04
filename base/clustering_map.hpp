#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace base
{
// Maps keys to lists of values, but allows to clusterize keys
// together, and to get all values from a cluster.
//
// NOTE: the map is NOT thread-safe.
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ClusteringMap
{
public:
  // Appends |value| to the list of values in the cluster
  // corresponding to |key|.
  //
  // Amortized complexity: O(log*(n) * F), where n is the total number
  // of keys in the map, F is the complexity of find in unordered_map.
  template <typename V>
  void Append(Key const & key, V && value)
  {
    auto & entry = GetRoot(key);
    entry.m_values.push_back(std::forward<V>(value));
  }

  // Unions clusters corresponding to |u| and |v|.
  //
  // Amortized complexity: O(log*(n) * F + log(m)), where n is the
  // total number of keys and m is the total number of values in the
  // map, F is the complexity of find in unordered_map.
  void Union(Key const & u, Key const & v)
  {
    auto & ru = GetRoot(u);
    auto & rv = GetRoot(v);
    if (ru.m_root == rv.m_root)
      return;

    if (ru.m_rank < rv.m_rank)
      Attach(rv /* root */, ru /* child */);
    else
      Attach(ru /* root */, rv /* child */);
  }

  // Returns all values from the cluster corresponding to |key|.
  //
  // Amortized complexity: O(log*(n) * F), where n is the total number
  // of keys in the map, F is the complexity of find in unordered map.
  std::vector<Value> const & Get(Key const & key)
  {
    auto const & entry = GetRoot(key);
    return entry.m_values;
  }

private:
  struct Entry
  {
    Key m_root;
    size_t m_rank = 0;
    std::vector<Value> m_values;
  };

  Entry & GetRoot(Key const & key)
  {
    auto & entry = GetEntry(key);
    if (entry.m_root == key)
      return entry;

    auto & root = GetRoot(entry.m_root);
    entry.m_root = root.m_root;
    return root;
  }

  void Attach(Entry & parent, Entry & child)
  {
    ASSERT_LESS_OR_EQUAL(child.m_rank, parent.m_rank, ());

    child.m_root = parent.m_root;
    if (child.m_rank == parent.m_rank)
      ++parent.m_rank;

    auto & pv = parent.m_values;
    auto & cv = child.m_values;
    if (pv.size() < cv.size())
      pv.swap(cv);
    pv.insert(pv.end(), cv.begin(), cv.end());
  }

  Entry & GetEntry(Key const & key)
  {
    auto it = m_table.find(key);
    if (it != m_table.end())
      return it->second;

    auto & entry = m_table[key];
    entry.m_root = key;
    return entry;
  }

  std::unordered_map<Key, Entry, Hash> m_table;
};
}  // namespace base
