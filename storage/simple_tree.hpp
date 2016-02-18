#pragma once

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

template <class T>
class SimpleTree
{
  T m_value;

  /// \brief m_children contains all first generation descendants of the node.
  /// Note. Once created the order of elements of |m_children| should not be changed.
  /// See implementation of AddAtDepth and Add methods for details.
  vector<SimpleTree<T>> m_children;
  SimpleTree<T> * m_parent;

  static bool IsEqual(T const & v1, T const & v2)
  {
    return !(v1 < v2) && !(v2 < v1);
  }

public:
  SimpleTree(T const & value = T(), SimpleTree<T> * parent = nullptr) : m_value(value), m_parent(parent)
  {
  }

  /// @return reference is valid only up to the next tree structure modification
  T const & Value() const
  {
    return m_value;
  }

  /// \brief Reserves child size vector. This method should be called once before filling
  /// children vector with correct |n| size to prevent m_children relocation while filling.
  void ReserveAtDepth(int level, size_t n)
  {
    SimpleTree<T> * node = this;
    while (level-- > 0 && !node->m_children.empty())
      node = &node->m_children.back();
    ASSERT_EQUAL(level, -1, ());
    return node->Reserve(n);
  }

  void Reserve(size_t n)
  {
    m_children.reserve(n);
  }

  /// @return reference is valid only up to the next tree structure modification
  T & Value()
  {
    return m_value;
  }

  /// @return reference is valid only up to the next tree structure modification
  T & AddAtDepth(int level, T const & value)
  {
    SimpleTree<T> * node = this;
    while (level-- > 0 && !node->m_children.empty())
      node = &node->m_children.back();
    ASSERT_EQUAL(level, -1, ());
    return node->Add(value);
  }

  /// @return reference is valid only up to the next tree structure modification
  T & Add(T const & value)
  {
    m_children.emplace_back(SimpleTree(value, this));
    return m_children.back().Value();
  }

  /// Deletes all children and makes tree empty
  void Clear()
  {
    m_children.clear();
  }

  bool operator<(SimpleTree<T> const & other) const
  {
    return Value() < other.Value();
  }

  /// \brief Checks all nodes in tree to find an equal one. If there're several equal nodes
  /// returns the first found.
  /// \returns a poiter item in the tree if found and nullptr otherwise.
  /// @TODO(bykoianko) The complexity of the method is O(n). But the structure (tree) is built on the start of the program
  /// and then actively used on run time. This method (and class) should be redesigned to make the function work faster.
  /// A hash table is being planned to use.
  void Find(T const & value, vector<SimpleTree<T> const *> & found) const
  {
    if (IsEqual(m_value, value))
      found.push_back(this);
    for (auto const & child : m_children)
      child.Find(value, found);
  }

  SimpleTree<T> const * const FindFirst(T const & value) const
  {
    if (IsEqual(m_value, value))
      return this;

    for (auto const & child : m_children)
    {
      SimpleTree<T> const * const found = child.FindFirst(value);
      if (found != nullptr)
        return found;
    }
    return nullptr;
  }

  /// \brief Find only leaves.
  /// \note It's a termprary fucntion for compatablity with old countries.txt.
  /// When new countries.txt with unique ids will be added FindLeaf will be removed
  /// and Find will be used intead.
  /// @TODO(bykoianko) Remove this method on countries.txt update.
  SimpleTree<T> const * const FindFirstLeaf(T const & value) const
  {
    if (IsEqual(m_value, value) && m_children.empty())
      return this; // It's a leaf.

    for (auto const & child : m_children)
    {
      SimpleTree<T> const * const found = child.FindFirstLeaf(value);
      if (found != nullptr)
        return found;
    }
    return nullptr;
  }

  bool HasParent() const { return m_parent != nullptr; }

  SimpleTree<T> const & Parent() const
  {
    CHECK(HasParent(), ());
    return *m_parent;
  }

  SimpleTree<T> const & Child(size_t index) const
  {
    ASSERT_LESS(index, m_children.size(), ());
    return m_children[index];
  }

  size_t ChildrenCount() const
  {
    return m_children.size();
  }

  /// \brief Calls functor f for each first generation descendant of the node.
  template <class TFunctor>
  void ForEachChild(TFunctor && f)
  {
    for (auto & child : m_children)
      f(child);
  }

  template <class TFunctor>
  void ForEachChild(TFunctor && f) const
  {
    for (auto const & child : m_children)
      f(child);
  }

  /// \brief Calls functor f for all nodes (add descendant) in the tree.
  template <class TFunctor>
  void ForEachDescendant(TFunctor && f)
  {
    for (auto & child : m_children)
    {
      f(child);
      child.ForEachDescendant(f);
    }
  }

  template <class TFunctor>
  void ForEachDescendant(TFunctor && f) const
  {
    for (auto const & child: m_children)
    {
      f(child);
      child.ForEachDescendant(f);
    }
  }

  template <class TFunctor>
  void ForEachInSubtree(TFunctor && f)
  {
    f(*this);
    for (auto & child: m_children)
      child.ForEachInSubtree(f);
  }

  template <class TFunctor>
  void ForEachInSubtree(TFunctor && f) const
  {
    f(*this);
    for (auto const & child: m_children)
      child.ForEachInSubtree(f);
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
