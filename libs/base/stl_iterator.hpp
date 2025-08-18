#pragma once

#include <boost/iterator/iterator_facade.hpp>

namespace stl_iterator_detail
{
struct Dummy
{
  template <class T>
  Dummy & operator=(T const &)
  {
    return *this;
  }
};
}  // namespace stl_iterator_detail

class CounterIterator
  : public boost::iterator_facade<CounterIterator, stl_iterator_detail::Dummy, boost::forward_traversal_tag>
{
  size_t m_count;

public:
  CounterIterator() : m_count(0) {}
  size_t GetCount() const { return m_count; }

  stl_iterator_detail::Dummy & dereference() const
  {
    static stl_iterator_detail::Dummy dummy;
    return dummy;
  }
  void increment() { ++m_count; }
};
