#pragma once

#include "storage/country_tree.hpp"

#include "std/algorithm.hpp"
#include "std/unordered_map.hpp"

template <class K>
struct CountryTreeKeyHasher
{
  size_t operator()(K const & k) const { return m_hash(k); }
private:
  hash<string> m_hash;
};

template<>
struct CountryTreeKeyHasher<int>
{
  size_t operator()(int k) const { return m_hash(k); }
private:
  hash<int> m_hash;
};

/// This class is developed for using in Storage. It's a implementation of a tree with ability
/// of access to its nodes in constant time with the help of hash table.
/// It should be filled with AddAtDepth method.
/// This class is used in Storage and filled based on countries.txt (countries_migrate.txt).
/// While filling CountryTree nodes in countries.txt should be visited in DFS order.
template <class I, class T>
class CountryTreeFacade
{
  using TCountryTreeHashTable = unordered_multimap<I, CountryTree<T> *, CountryTreeKeyHasher<I>>;

  CountryTree<T> m_countryTree;
  TCountryTreeHashTable m_countryTreeHashTable;

public:
  CountryTreeFacade(T const & value = T(), CountryTree<T> * parent = nullptr)
    : m_countryTree(value, parent) {}

  /// @return reference is valid only up to the next tree structure modification
  T const & Value() const { return m_countryTree.Value(); }

  /// @return reference is valid only up to the next tree structure modification
  T & Value() { return m_countryTree.Value(); }

  /// @return reference is valid only up to the next tree structure modification
  T & AddAtDepth(int level, T const & value)
  {
    CountryTree<T> * const added = m_countryTree.AddAtDepth(level, value);
    ASSERT(added, ());
    m_countryTreeHashTable.insert(make_pair(value.Name(), added));
    return added->Value();
  }

  /// Deletes all children and makes tree empty
  void Clear() { m_countryTree.Clear(); }

  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  void Find(T const & value, vector<CountryTree<T> const *> & found) const
  {
    found.clear();

    if (IsEqual(value, m_countryTree.Value()))
      found.push_back(&m_countryTree);

    auto const range = m_countryTreeHashTable.equal_range(value.Name());
    auto const end = m_countryTreeHashTable.end();
    if (range.first == end && range.second == end)
      return;

    for_each(range.first, range.second,
        [&found](typename TCountryTreeHashTable::value_type const & node) { found.push_back(&*node.second); });
  }

  CountryTree<T> const * const FindFirst(T const & value) const
  {
    vector<CountryTree<T> const *> found;
    Find(value, found);
    if (found.empty())
      return nullptr;
    return found[0];
  }

  /// \brief Find only leaves.
  /// \note It's a termprary fucntion for compatablity with old countries.txt.
  /// When new countries.txt with unique ids will be added FindLeaf will be removed
  /// and Find will be used intead.
  /// @TODO(bykoianko) Remove this method on countries.txt update.
  CountryTree<T> const * const FindFirstLeaf(T const & value) const
  {
    vector<CountryTree<T> const *> found;
    Find(value, found);

    for (auto node : found)
    {
      if (node->ChildrenCount() == 0)
        return node;
    }
    return nullptr;
  }

  size_t ChildrenCount() const { return m_countryTree.ChildrenCount(); }

private:
  static bool IsEqual(T const & v1, T const & v2) { return !(v1 < v2) && !(v2 < v1); }
};
