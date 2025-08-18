#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

// Implementing this base class allows an object to be collection of objects.
template <typename T>
class CollectionBase
{
public:
  void Append(T const & collector) { m_collection.push_back(collector); }

  void AddCollection(CollectionBase<T> const & collection)
  {
    std::copy(std::begin(collection.m_collection), std::end(collection.m_collection), std::back_inserter(m_collection));
  }

  std::vector<T> const & GetCollection() const { return m_collection; }

  bool Empty() const { return m_collection.empty(); }

protected:
  std::vector<T> m_collection;
};
