#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

namespace base
{
// Maps keys to lists of values, but allows to combine keys into
// clusters and to get all values from a cluster.
//
// NOTE: this class is NOT thread-safe.
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ClusteringMap
{
public:
  // In complexity specifications below:
  // * n is the total number of keys in the map
  // * m is the total number of values in the map
  // * α() is the inverse Ackermann function
  // * F is the complexity of find() in unordered_map
  //
  // Also, it's assumed that complexity to compare two keys is O(1).

  // Appends |value| to the list of values in the cluster
  // corresponding to |key|.
  //
  // Amortized complexity: O(α(n) * F).
  template <typename V>
  void Append(Key const & key, V && value)
  {
    auto & entry = GetRoot(key);
    entry.m_values.push_back(std::forward<V>(value));
  }

  void Delete(Key const & key) { m_table.erase(key); }

  // Unions clusters corresponding to |u| and |v|.
  //
  // Amortized complexity: O(α(n) * F + log(m)).
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
  // Amortized complexity: O(α(n) * F).
  std::vector<Value> const & Get(Key const & key)
  {
    auto const & entry = GetRoot(key);
    return entry.m_values;
  }

  // Complexity: O(n * log(n)).
  template <typename Fn>
  void ForEachCluster(Fn && fn)
  {
    struct EntryWithKey
    {
      EntryWithKey(Entry const * entry, Key const & key) : m_entry(entry), m_key(key) {}

      bool operator<(EntryWithKey const & rhs) const { return m_entry->m_root < rhs.m_entry->m_root; }

      Entry const * m_entry;
      Key m_key;
    };

    std::vector<EntryWithKey> eks;
    for (auto const & kv : m_table)
    {
      auto const & key = kv.first;
      auto const & entry = GetRoot(key);
      eks.emplace_back(&entry, key);
    }
    std::sort(eks.begin(), eks.end());

    size_t i = 0;
    while (i < eks.size())
    {
      std::vector<Key> keys;
      keys.push_back(eks[i].m_key);

      size_t j = i + 1;
      while (j < eks.size() && eks[i].m_entry->m_root == eks[j].m_entry->m_root)
      {
        keys.push_back(eks[j].m_key);
        ++j;
      }

      fn(keys, eks[i].m_entry->m_values);

      i = j;
    }
  }

  void Clear() { m_table.clear(); }

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

  Entry & GetEntry(Key const & key)
  {
    auto it = m_table.find(key);
    if (it != m_table.end())
      return it->second;

    auto & entry = m_table[key];
    entry.m_root = key;
    return entry;
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
    std::move(cv.begin(), cv.end(), std::back_inserter(pv));
    cv.clear();
    cv.shrink_to_fit();
  }

  std::unordered_map<Key, Entry, Hash> m_table;
};
}  // namespace base
