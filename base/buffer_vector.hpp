#pragma once

#include "assert.hpp"

#include "../std/array.hpp"
#include "../std/vector.hpp"


namespace detail
{
  template <class T> void swap_adl_inner(T & r1, T & r2)
  {
    swap(r1, r2);
  }
}

template <class T, size_t N> class buffer_vector
{
  array<T, N> m_static;
  vector<T> m_dynamic;
  size_t m_size;

  void swap_elements(size_t n)
  {
    for (size_t i = 0; i < n; ++i)
      detail::swap_adl_inner(m_static[i], m_dynamic[i]);
  }

  // call before new size applied (in growing)
  void flush_to_dynamic()
  {
    if (m_size > 0 && m_size <= N)
    {
      if (m_dynamic.size() >= m_size)
        swap_elements(m_size);
      else
      {
        ASSERT ( m_dynamic.empty(), () );
        m_dynamic.clear();
        m_dynamic.insert(m_dynamic.end(), m_static.data(), m_static.data() + m_size);
      }
    }
  }

  // call before new size applied (in reducing)
  void flush_to_static(size_t n)
  {
    if (m_size > N)
    {
      ASSERT_LESS_OR_EQUAL ( n, N, () );
      swap_elements(n);
      m_dynamic.clear();
    }
  }

public:
  typedef T value_type;
  typedef T const & const_reference;
  typedef T & reference;

  buffer_vector() : m_size(0) {}
  buffer_vector(size_t n) : m_size(0)
  {
    resize(n);
  }

  void reserve(size_t n)
  {
    if (n > N)
      m_dynamic.reserve(n);
  }

  void resize(size_t n)
  {
    if (n > N)
    {
      m_dynamic.resize(n);
      flush_to_dynamic();
    }
    else
      flush_to_static(n);

    m_size = n;
  }

  void clear()
  {
    m_size = 0;
    m_dynamic.clear();
  }

  T const * data() const
  {
    return (m_size > N ? &m_dynamic[0] : m_static.data());
  }

  bool empty() const { return m_size == 0; }

  size_t size() const { return m_size; }

  T const & front() const
  {
    ASSERT ( !empty(), () );
    return (m_size > N ? m_dynamic.front() : m_static.front());
  }
  T const & back() const
  {
    ASSERT ( !empty(), () );
    return (m_size > N ? m_dynamic.back() : m_static[m_size-1]);
  }

  T const & operator[](size_t i) const
  {
    return (m_size > N ? m_dynamic[i] : m_static[i]); 
  }
  T & operator[](size_t i)
  {
    return (m_size > N ? m_dynamic[i] : m_static[i]); 
  }

  void push_back(T const & t)
  {
    if (m_size < N)
      m_static[m_size] = t;
    else
    {
      flush_to_dynamic();
      m_dynamic.push_back(t);
    }

    ++m_size;
   } 

  void swap(buffer_vector & rhs)
  {
    m_static.swap(rhs.m_static);
    m_dynamic.swap(rhs.m_dynamic);
    std::swap(m_size, rhs.m_size);
  }
};

template <class T, size_t N>
void swap(buffer_vector<T, N> & r1, buffer_vector<T, N> & r2) { r1.swap(r2); }

template <typename T, size_t N>
inline string debug_print(buffer_vector<T, N> const & v)
{
  return ::my::impl::DebugPrintSequence(v.data(), v.data() + v.size());
}
