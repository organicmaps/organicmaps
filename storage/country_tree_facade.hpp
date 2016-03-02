#pragma once

#include "storage/country_tree.hpp"

#include "std/algorithm.hpp"
#include "std/unordered_map.hpp"

template <class K>
struct CountryTreeKeyHasher
{
  size_t operator()(K const & k) const { return m_hash(k.Name()); }
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

/// This class is developed for using in Storage. It's a implementation of a tree.
/// It should be filled with AddAtDepth method.
/// This class is used in Storage and filled based on countries.txt (countries_migrate.txt).
/// While filling CountryTree nodes in countries.txt should be visited in DFS order.
template <class T>
class CountryTreeFacade
{
  using TCountryTreeHashTable = unordered_multimap<T, shared_ptr<CountryTree<T>>, CountryTreeKeyHasher<T>>;

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
    shared_ptr<CountryTree<T>> const added = m_countryTree.AddAtDepth(level, value);
    m_countryTreeHashTable.insert(make_pair(value, added));
    return added->Value();
  }

  /// Deletes all children and makes tree empty
  void Clear() { m_countryTree.Clear(); }

  bool operator<(CountryTree<T> const & other) const { return m_countryTree < other; }

  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  void Find(T const & value, vector<CountryTree<T> const *> & found) const
  {
    found.clear();

    if (IsEqual(value, m_countryTree.Value()))
      found.push_back(&m_countryTree);

    auto const range = m_countryTreeHashTable.equal_range(value);
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

  bool HasParent() const { m_countryTree.HasParent(); }

  CountryTree<T> const & Parent() const { return m_countryTree.Parent(); }

  CountryTree<T> const & Child(size_t index) const { return m_countryTree.Child(index); }

  size_t ChildrenCount() const { return m_countryTree.ChildrenCount(); }

  /// \brief Calls functor f for each first generation descendant of the node.
  template <class TFunctor>
  void ForEachChild(TFunctor && f) { return m_countryTree.ForEachChild(f); }

  template <class TFunctor>
  void ForEachChild(TFunctor && f) const { return m_countryTree.ForEachChild(f); }

  /// \brief Calls functor f for all nodes (add descendant) in the tree.
  template <class TFunctor>
  void ForEachDescendant(TFunctor && f) { return m_countryTree.ForEachDescendant(f); }

  template <class TFunctor>
  void ForEachDescendant(TFunctor && f) const { return m_countryTree.ForEachDescendant(f); }

  template <class TFunctor>
  void ForEachInSubtree(TFunctor && f) { return m_countryTree.ForEachInSubtree(f); }

  template <class TFunctor>
  void ForEachInSubtree(TFunctor && f) const { return m_countryTree.ForEachInSubtree(f); }

  template <class TFunctor>
  void ForEachAncestorExceptForTheRoot(TFunctor && f) { return m_countryTree.ForEachAncestorExceptForTheRoot(f); }

  template <class TFunctor>
  void ForEachAncestorExceptForTheRoot(TFunctor && f) const { return m_countryTree.ForEachAncestorExceptForTheRoot(f); }

private:
  static bool IsEqual(T const & v1, T const & v2) { return !(v1 < v2) && !(v2 < v1); }
};
