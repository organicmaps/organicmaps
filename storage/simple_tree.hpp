#pragma once

#include "std/vector.hpp"
#include "std/algorithm.hpp"

template <class T>
class SimpleTree
{
  typedef std::vector<SimpleTree<T> > internal_container_type;

  T m_value;
  internal_container_type m_siblings;

public:
  SimpleTree(T const & value = T()) : m_value(value)
  {
  }

  /// @return reference is valid only up to the next tree structure modification
  T const & Value() const
  {
    return m_value;
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
    while (level-- > 0 && !node->m_siblings.empty())
      node = &node->m_siblings.back();
    return node->Add(value);
  }

  /// @return reference is valid only up to the next tree structure modification
  T & Add(T const & value)
  {
    m_siblings.push_back(SimpleTree(value));
    return m_siblings.back().Value();
  }

  /// Deletes all children and makes tree empty
  void Clear()
  {
    m_siblings.clear();
  }

  bool operator<(SimpleTree<T> const & other) const
  {
    return Value() < other.Value();
  }

  /// sorts siblings independently on each level by default
  void Sort(bool onlySiblings = false)
  {
    std::sort(m_siblings.begin(), m_siblings.end());
    if (!onlySiblings)
      for (typename internal_container_type::iterator it = m_siblings.begin(); it != m_siblings.end(); ++it)
        it->Sort(false);
  }

  SimpleTree<T> const & operator[](size_t index) const
  {
    return m_siblings.at(index);
  }

  size_t SiblingsCount() const
  {
    return m_siblings.size();
  }

  template <class TFunctor>
  void ForEachSibling(TFunctor & f)
  {
    for (typename internal_container_type::iterator it = m_siblings.begin(); it != m_siblings.end(); ++it)
      f(*it);
  }

  template <class TFunctor>
  void ForEachSibling(TFunctor & f) const
  {
    for (typename internal_container_type::const_iterator it = m_siblings.begin(); it != m_siblings.end(); ++it)
      f(*it);
  }

  template <class TFunctor>
  void ForEachChildren(TFunctor & f)
  {
    for (typename internal_container_type::iterator it = m_siblings.begin(); it != m_siblings.end(); ++it)
    {
      f(*it);
      it->ForEachChildren(f);
    }
  }

  template <class TFunctor>
  void ForEachChildren(TFunctor & f) const
  {
    for (typename internal_container_type::const_iterator it = m_siblings.begin(); it != m_siblings.end(); ++it)
    {
      f(*it);
      it->ForEachChildren(f);
    }
  }
};
