#pragma once

#include "std/iterator.hpp"
#include "std/utility.hpp"

namespace my
{
namespace details
{
template <typename TCollection>
struct const_respective_iterator
{
  using hint = int;
  using type = typename TCollection::iterator;
};

template <typename TCollection>
struct const_respective_iterator<TCollection const>
{
  using hint = double;
  using type = typename TCollection::const_iterator;
};

template <typename TCollection>
using const_respective_iterator_t =
    typename const_respective_iterator<typename std::remove_reference<TCollection>::type>::type;
}  // namespace details

template <typename TCounter, typename TElement>
struct IndexedElement : std::pair<TCounter, TElement>
{
  using base = std::pair<TCounter, TElement>;
  using base::base;

  TCounter const index{base::first};
  TElement & item{base::second};
};

template <typename TCounter, typename TElement>
IndexedElement<TCounter, TElement> MakeIndexedElement(TElement && item, TCounter counter = {})
{
  return IndexedElement<TCounter, TElement>(counter, item);
}

template <typename TIterator, typename TCounter>
struct EnumeratingIterator
{
  using original_iterator = TIterator;
  using original_reference = typename std::iterator_traits<original_iterator>::reference;

  using difference_type = typename std::iterator_traits<original_iterator>::difference_type;
  using value_type = IndexedElement<TCounter, original_reference>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;

  EnumeratingIterator(original_iterator const it, TCounter const counter)
    : m_iterator(it), m_counter(counter)
  {
  }

  EnumeratingIterator & operator++()
  {
    ++m_iterator;
    ++m_counter;
    return *this;
  }

  value_type operator*() { return MakeIndexedElement(*m_iterator, m_counter); }
  bool operator==(EnumeratingIterator const & it) const { return m_iterator == it.m_iterator; }
  bool operator!=(EnumeratingIterator const & it) const { return !(*this == it); }
  original_iterator m_iterator;
  TCounter m_counter;
};

template <typename TIterator, typename TCounter>
struct EnumeratorWrapper
{
  using original_iterator = TIterator;
  using iterator = EnumeratingIterator<original_iterator, TCounter>;
  using value_type = typename std::iterator_traits<iterator>::value_type;

  EnumeratorWrapper(original_iterator const begin, original_iterator const end,
                    TCounter const countFrom)
    : m_begin(begin), m_end(end), m_countFrom(countFrom)
  {
  }

  iterator begin() { return {m_begin, m_countFrom}; }
  iterator end() { return {m_end, {}}; }
  original_iterator const m_begin;
  original_iterator const m_end;
  TCounter const m_countFrom;
};

template <typename TCollection, typename TCounter = size_t>
auto enumerate(TCollection && collection, TCounter const counter = {})
    -> EnumeratorWrapper<details::const_respective_iterator_t<TCollection>, TCounter>
{
  return {begin(collection), end(collection), counter};
}
}  // namespace my
