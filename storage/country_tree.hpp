#pragma once

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

/// \brief This class is developed for using in Storage. It's a implementation of a tree with
/// ability
/// of access to its nodes in expected constant time with the help of hash table.
/// It should be filled with AddAtDepth method.
/// This class is used in Storage and filled based on countries.txt (countries_migrate.txt).
/// While filling CountryTree nodes in countries.txt should be visited in DFS order.
/// \param TKey is a type of keys to look for |TValue| in |m_countryTreeHashTable|.
/// \param TValue is a type of values which are saved in |m_countryTree| nodes.
template <class TKey, class TValue>
class CountryTree
{
public:
  /// This class is developed for using in CountryTree. It's a implementation of a tree.
  /// It should be filled with AddAtDepth method.
  /// This class is used in filled based on countries.txt (countries_migrate.txt).
  /// While filling Node nodes in countries.txt should be visited in DFS order.
  class Node
  {
    TValue m_value;

    /// \brief m_children contains all first generation descendants of the node.
    /// Note. Once created the order of elements of |m_children| should not be changed.
    /// See implementation of AddAtDepth and Add methods for details.
    vector<unique_ptr<Node>> m_children;
    Node * m_parent;

    /// @return reference is valid only up to the next tree structure modification
    Node * Add(TValue const & value)
    {
      m_children.emplace_back(make_unique<Node>(value, this));
      return m_children.back().get();
    }

  public:
    Node(TValue const & value = TValue(), Node * parent = nullptr)
      : m_value(value), m_parent(parent)
    {
    }

    /// @return reference is valid only up to the next tree structure modification
    TValue const & Value() const { return m_value; }

    /// @return reference is valid only up to the next tree structure modification
    TValue & Value() { return m_value; }

    /// @return reference is valid only up to the next tree structure modification
    Node * AddAtDepth(int level, TValue const & value)
    {
      Node * node = this;
      while (level-- > 0 && !node->m_children.empty())
        node = node->m_children.back().get();
      ASSERT_EQUAL(level, -1, ());
      return node->Add(value);
    }

    /// Deletes all children and makes tree empty
    void Clear() { m_children.clear(); }

    bool operator<(Node const & other) const { return Value() < other.Value(); }

    bool HasParent() const { return m_parent != nullptr; }

    Node const & Parent() const
    {
      CHECK(HasParent(), ());
      return *m_parent;
    }

    Node const & Child(size_t index) const
    {
      ASSERT_LESS(index, m_children.size(), ());
      return *m_children[index];
    }

    size_t ChildrenCount() const { return m_children.size(); }

    /// \brief Calls functor f for each first generation descendant of the node.
    template <class TFunctor>
    void ForEachChild(TFunctor && f)
    {
      for (auto & child : m_children)
        f(*child);
    }

    template <class TFunctor>
    void ForEachChild(TFunctor && f) const
    {
      for (auto const & child : m_children)
        f(*child);
    }

    /// \brief Calls functor f for all nodes (add descendant) in the tree.
    template <class TFunctor>
    void ForEachDescendant(TFunctor && f)
    {
      for (auto & child : m_children)
      {
        f(*child);
        child->ForEachDescendant(f);
      }
    }

    template <class TFunctor>
    void ForEachDescendant(TFunctor && f) const
    {
      for (auto const & child : m_children)
      {
        f(*child);
        child->ForEachDescendant(f);
      }
    }

    template <class TFunctor>
    void ForEachInSubtree(TFunctor && f)
    {
      f(*this);
      for (auto & child : m_children)
        child->ForEachInSubtree(f);
    }

    template <class TFunctor>
    void ForEachInSubtree(TFunctor && f) const
    {
      f(*this);
      for (auto const & child : m_children)
        child->ForEachInSubtree(f);
    }

    template <class TFunctor>
    void ForEachAncestorExceptForTheRoot(TFunctor && f)
    {
      if (m_parent == nullptr || m_parent->m_parent == nullptr)
        return;
      f(*m_parent);
      m_parent->ForEachAncestorExceptForTheRoot(f);
    }

    template <class TFunctor>
    void ForEachAncestorExceptForTheRoot(TFunctor && f) const
    {
      if (m_parent == nullptr || m_parent->m_parent == nullptr)
        return;
      f(*m_parent);
      m_parent->ForEachAncestorExceptForTheRoot(f);
    }
  };

private:
  using TCountryTreeHashTable = unordered_multimap<TKey, Node *>;

public:
  CountryTree(TValue const & value = TValue(), Node * parent = nullptr)
    : m_countryTree(make_unique<Node>(value, parent))
  {
  }

  Node const & GetRoot() const { return *m_countryTree; }
  Node & GetRoot() { return *m_countryTree; }

  /// @return reference is valid only up to the next tree structure modification
  TValue & AddAtDepth(int level, TValue const & value)
  {
    Node * const added = m_countryTree->AddAtDepth(level, value);
    ASSERT(added, ());
    m_countryTreeHashTable.insert(make_pair(value.Name(), added));
    return added->Value();
  }

  /// Deletes all children and makes tree empty
  void Clear() { m_countryTree->Clear(); }

  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  void Find(TValue const & value, vector<Node const *> & found) const
  {
    found.clear();

    if (IsEqual(value, m_countryTree->Value()))
      found.push_back(m_countryTree.get());

    auto const range = m_countryTreeHashTable.equal_range(value.Name());
    auto const end = m_countryTreeHashTable.end();
    if (range.first == end && range.second == end)
      return;

    for_each(range.first, range.second,
             [&found](typename TCountryTreeHashTable::value_type const & node)
    {
      found.push_back(&*node.second);
    });
  }

  Node const * const FindFirst(TValue const & value) const
  {
    vector<Node const *> found;
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
  Node const * const FindFirstLeaf(TValue const & value) const
  {
    vector<Node const *> found;
    Find(value, found);

    for (auto node : found)
    {
      if (node->ChildrenCount() == 0)
        return node;
    }
    return nullptr;
  }

  size_t ChildrenCount() const { return m_countryTree->ChildrenCount(); }

private:
  static bool IsEqual(TValue const & v1, TValue const & v2) { return !(v1 < v2) && !(v2 < v1); }

  /// @TODO(bykoianko) The root of the tree currently is processed in a special way.
  /// It's never deleted. Because of it it's necessary to work with the root in a special way.
  /// See SetCountriesContainerAttrs method for example. And CountryTree::Clear() method
  /// does not delete the root. It should be fixed.
  unique_ptr<Node> m_countryTree;
  TCountryTreeHashTable m_countryTreeHashTable;
};
