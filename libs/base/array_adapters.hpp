#pragma once

#include "base/assert.hpp"

namespace detail
{
template <class T, class TPtr>
class array_impl
{
protected:
  TPtr m_p;
  size_t m_size;

public:
  typedef T value_type;
  typedef T const & const_reference;
  typedef T & reference;

  array_impl(TPtr p, size_t sz) : m_p(p), m_size(sz) {}

  T const & operator[](size_t i) const
  {
    ASSERT_LESS(i, m_size, ());
    return m_p[i];
  }

  T const & back() const
  {
    ASSERT_GREATER(m_size, 0, ());
    return m_p[m_size - 1];
  }

  size_t size() const { return m_size; }

  bool empty() const { return (m_size == 0); }
};
}  // namespace detail

template <class T>
class array_read : public detail::array_impl<T, T const *>
{
public:
  array_read(T const * p, size_t sz) : detail::array_impl<T, T const *>(p, sz) {}
};

template <class TCont>
array_read<typename TCont::value_type> make_read_adapter(TCont const & cont)
{
  return array_read<typename TCont::value_type>(cont.empty() ? 0 : &cont[0], cont.size());
}

template <class T>
class array_write : public detail::array_impl<T, T *>
{
  size_t m_capacity;

  typedef detail::array_impl<T, T *> base_t;

public:
  template <class TCont>
  explicit array_write(TCont & cont)
    : detail::array_impl<T, T *>(cont.empty() ? 0 : &cont[0], 0)
    , m_capacity(cont.size())
  {}

  void push_back(T const & t)
  {
    ASSERT_LESS(base_t::m_size, m_capacity, ());
    base_t::m_p[base_t::m_size++] = t;
  }
};
