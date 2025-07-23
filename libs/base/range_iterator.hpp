#pragma once

#include <iterator>
#include <type_traits>

namespace base
{
// RagneItrator allows to write for loops as follows:
// for (auto const i : range(N))
//   ...
// for (auto const i : range(M, N))
//   ...
// And initialize stl containers like this:
// vector<int> mySequence(MakeRangeIterator(0), MakeRangeIterator(10));
template <typename TCounter>
struct RangeIterator
{
  explicit RangeIterator(TCounter const current) : m_current(current) {}
  RangeIterator & operator++()
  {
    ++m_current;
    return *this;
  }
  RangeIterator operator++(int) { return RangeIterator(m_current++); }
  RangeIterator & operator--()
  {
    --m_current;
    return *this;
  }
  RangeIterator operator--(int) { return RangeIterator(m_current--); }
  bool operator==(RangeIterator const & it) const { return m_current == it.m_current; }
  bool operator!=(RangeIterator const & it) const { return !(*this == it); }

  TCounter operator*() const { return m_current; }

  TCounter m_current;
};
}  // namespace base

namespace std
{
template <typename T>
struct iterator_traits<base::RangeIterator<T>>
{
  using difference_type = T;
  using value_type = T;
  using pointer = void;
  using reference = T;
  using iterator_category = std::bidirectional_iterator_tag;
};
}  // namespace std

namespace base
{
template <typename TCounter, bool forward>
struct RangeWrapper
{
  using value_type = std::remove_cv_t<TCounter>;
  using iterator_base = RangeIterator<value_type>;
  using iterator = std::conditional_t<forward, iterator_base, std::reverse_iterator<iterator_base>>;

  RangeWrapper(TCounter const from, TCounter const to) : m_begin(from), m_end(to) {}

  iterator begin() const { return iterator(iterator_base(m_begin)); }
  iterator end() const { return iterator(iterator_base(m_end)); }

  value_type const m_begin;
  value_type const m_end;
};

// Use this helper to iterate through 0 to `to'.
template <typename TCounter>
RangeWrapper<TCounter, true> UpTo(TCounter const to)
{
  return {{}, to};
}

// Use this helper to iterate through `from' to `to'.
template <typename TCounter>
RangeWrapper<TCounter, true> UpTo(TCounter const from, TCounter const to)
{
  return {from, to};
}

// Use this helper to iterate through `from' to 0.
template <typename TCounter>
RangeWrapper<TCounter, false> DownTo(TCounter const from)
{
  return {from, {}};
}

// Use this helper to iterate through `from' to `to'.
template <typename TCounter>
RangeWrapper<TCounter, false> DownTo(TCounter const from, TCounter const to)
{
  return {to, from};
}

template <typename TCounter>
RangeIterator<TCounter> MakeRangeIterator(TCounter const counter)
{
  return RangeIterator<TCounter>(counter);
}
}  // namespace base
