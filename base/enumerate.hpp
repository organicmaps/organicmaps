#pragma once

#include "std/iterator.hpp"
#include "std/utility.hpp"

namespace my
{
namespace details
{
template <typename TCollection>
struct ConstRespectiveIterator
{
  using type = typename TCollection::iterator;
};

template <typename TCollection>
struct ConstRespectiveIterator<TCollection const>
{
  using type = typename TCollection::const_iterator;
};

template <typename TCollection>
using ConstRespectiveIteratorT =
    typename ConstRespectiveIterator<typename std::remove_reference<TCollection>::type>::type;
}  // namespace details

template <typename TIndex, typename TElement>
struct IndexedElement : std::pair<TIndex, TElement>
{
  using TBase = std::pair<TIndex, TElement>;
  using TBase::TBase;

  TIndex const index{TBase::first};
  TElement & item{TBase::second};
};

template <typename TIndex, typename TElement>
IndexedElement<TIndex, TElement> MakeIndexedElement(TElement && item, TIndex counter = {})
{
  return IndexedElement<TIndex, TElement>(counter, item);
}

template <typename TIterator, typename TIndex>
struct EnumeratingIterator
{
  using original_iterator = TIterator;
  using original_reference = typename std::iterator_traits<original_iterator>::reference;

  using difference_type = typename std::iterator_traits<original_iterator>::difference_type;
  using value_type = IndexedElement<TIndex, original_reference>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;

  EnumeratingIterator(original_iterator const it, TIndex const counter)
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
  TIndex m_counter;
};

template <typename TIterator, typename TIndex>
struct EnumeratorWrapper
{
  using original_iterator = TIterator;
  using iterator = EnumeratingIterator<original_iterator, TIndex>;
  using value_type = typename std::iterator_traits<iterator>::value_type;

  EnumeratorWrapper(original_iterator const begin, original_iterator const end,
                    TIndex const countFrom)
    : m_begin(begin), m_end(end), m_countFrom(countFrom)
  {
  }

  iterator begin() { return {m_begin, m_countFrom}; }
  iterator end() { return {m_end, {}}; }
  original_iterator const m_begin;
  original_iterator const m_end;
  TIndex const m_countFrom;
};

template <typename TCollection, typename TIndex = size_t>
auto Enumerate(TCollection && collection, TIndex const counter = {})
    -> EnumeratorWrapper<details::ConstRespectiveIteratorT<TCollection>, TIndex>
{
  return {begin(collection), end(collection), counter};
}
}  // namespace my
