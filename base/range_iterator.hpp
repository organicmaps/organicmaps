#include "std/iterator.hpp"
#include "std/type_traits.hpp"

namespace base
{
template <typename TCounter>
struct RangeIterator
{
  explicit RangeIterator(TCounter const current):
      m_current(current)
  {
  }

  RangeIterator & operator++() { ++m_current; return *this; }
  RangeIterator & operator--() { --m_current; return *this; }

  bool operator==(RangeIterator const & it) const { return m_current == it.m_current; }
  bool operator!=(RangeIterator const & it) const { return !(*this == it); }

  TCounter operator*() const { return m_current; }

  TCounter m_current;
};
} // namespace base

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
} // namespace std

namespace base
{
template <typename TCounter, bool forward>
struct RangeWrapper
{

  using value_type = typename std::remove_cv<TCounter>::type;
  using iterator_base = RangeIterator<value_type>;
  using iterator = typename std::conditional<forward,
                                             iterator_base,
                                             std::reverse_iterator<iterator_base>>::type;

  RangeWrapper(TCounter const from, TCounter const to):
      m_begin(from),
      m_end(to)
  {
  }

  iterator begin() const { return iterator(iterator_base(m_begin)); }
  iterator end() const { return iterator(iterator_base(m_end)); }

  value_type const m_begin;
  value_type const m_end;
};

template <typename TCounter>
RangeWrapper<TCounter, true> range(TCounter const to)
{
  return {{}, to};
}

template <typename TCounter>
RangeWrapper<TCounter, true> range(TCounter const from, TCounter const to)
{
  return {from, to};
}

template <typename TCounter>
RangeWrapper<TCounter, false> reverse_range(TCounter const to)
{
  return {to, {}};
}

template <typename TCounter>
RangeWrapper<TCounter, false> reverse_range(TCounter const from, TCounter const to)
{
  return {to, from};
}

template <typename TCounter>
RangeIterator<TCounter> MakeRangeIterator(TCounter const counter)
{
  return RangeIterator<TCounter>(counter);
}
} // namespace base
