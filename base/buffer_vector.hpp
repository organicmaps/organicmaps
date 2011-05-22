#pragma once

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"
#include "../base/assert.hpp"
#include "../base/swap.hpp"

template <class T, size_t N> class buffer_vector
{
private:
  enum { USE_DYNAMIC = N + 1 };
  T m_static[N];
  size_t m_size;
  vector<T> m_dynamic;

public:
  typedef T value_type;
  typedef T const & const_reference;
  typedef T & reference;
  typedef size_t size_type;

  buffer_vector() : m_size(0) {}
  explicit buffer_vector(size_t n, T c = T()) : m_size(0)
  {
    resize(n, c);
  }

  void reserve(size_t n)
  {
    if (m_size == USE_DYNAMIC || n > N)
      m_dynamic.reserve(n);
  }

  void resize(size_t n, T c = T())
  {
    if (m_size == USE_DYNAMIC)
      m_dynamic.resize(n);
    else
    {
      if (n <= N)
      {
        for (size_t i = m_size; i < n; ++i)
          m_static[i] = c;
        m_size = n;
      }
      else
      {
        m_dynamic.reserve(n);
        size_t const oldSize = m_size;
        SwitchToDynamic();
        m_dynamic.insert(m_dynamic.end(), n - oldSize, c);
        ASSERT_EQUAL(m_dynamic.size(), n, ());
      }
    }
  }

  void clear()
  {
    if (m_size == USE_DYNAMIC)
      m_dynamic.clear();
    else
      m_size = 0;
  }

  T const * data() const { return m_size == USE_DYNAMIC ? &m_dynamic[0] : &m_static[0]; }
  T       * data()       { return m_size == USE_DYNAMIC ? &m_dynamic[0] : &m_static[0]; }

  T const * begin() const { return data(); }
  T       * begin()       { return data(); }
  T const * end() const { return data() + size(); }
  T       * end()       { return data() + size(); }

  bool empty() const { return m_size == USE_DYNAMIC ? m_dynamic.empty() : m_size == 0; }
  size_t size() const { return m_size == USE_DYNAMIC ? m_dynamic.size() : m_size; }

  T const & front() const
  {
    ASSERT(!empty(), ());
    return *begin();
  }
  T & front()
  {
    ASSERT(!empty(), ());
    return *begin();
  }
  T const & back() const
  {
    ASSERT(!empty(), ());
    return *(end() - 1);
  }
  T & back()
  {
    ASSERT(!empty(), ());
    return *(end() - 1);
  }

  T const & operator[](size_t i) const
  {
    ASSERT_LESS(i, size(), ());
    return *(begin() + i);
  }
  T & operator[](size_t i)
  {
    ASSERT_LESS(i, size(), ());
    return *(begin() + i);
  }

  void swap(buffer_vector<T, N> & rhs)
  {
    m_dynamic.swap(rhs.m_dynamic);
    Swap(m_size, rhs.m_size);
    for (size_t i = 0; i < N; ++i)
      Swap(m_static[i], rhs.m_static[i]);
  }

  void push_back(T const & t)
  {
    if (m_size == USE_DYNAMIC)
      m_dynamic.push_back(t);
    else
    {
      if (m_size < N)
        m_static[m_size++] = t;
      else
      {
        ASSERT_EQUAL(m_size, N, ());
        m_dynamic.reserve(N + 1);
        SwitchToDynamic();
        m_dynamic.push_back(t);
        ASSERT_EQUAL(m_dynamic.size(), N + 1, ());
      }
    }
  }

  void pop_back()
  {
    if (m_size == USE_DYNAMIC)
      m_dynamic.pop_back();
    else
    {
      ASSERT_GREATER(m_size, 0, ());
      --m_size;
    }
  }

  template <typename IterT> void insert(T const * where, IterT beg, IterT end)
  {
    ptrdiff_t const pos = where - data();
    ASSERT_GREATER_OR_EQUAL(pos, 0, ());
    ASSERT_LESS_OR_EQUAL(pos, static_cast<ptrdiff_t>(size()), ());

    if (m_size == USE_DYNAMIC)
      m_dynamic.insert(m_dynamic.begin() + pos, beg, end);
    else
    {
      size_t const n = end - beg;
      if (m_size + n <= N)
      {
        if (pos != m_size)
          for (ptrdiff_t i = m_size - 1; i >= pos; --i)
            Swap(m_static[i], m_static[i + n]);

        m_size += n;
        T * writableWhere = &m_static[0] + pos;
        ASSERT_EQUAL(where, writableWhere, ());
        while (beg != end)
          *(writableWhere++) = *(beg++);
      }
      else
      {
        m_dynamic.reserve(m_size + n);
        SwitchToDynamic();
        m_dynamic.insert(m_dynamic.begin() + pos, beg, end);
        ASSERT_EQUAL(m_dynamic.size(), m_dynamic.capacity(), ());
      }
    }
  }

private:
  void SwitchToDynamic()
  {
    ASSERT_NOT_EQUAL(m_size, static_cast<size_t>(USE_DYNAMIC), ());
    ASSERT_EQUAL(m_dynamic.size(), 0, ());
    m_dynamic.insert(m_dynamic.end(), m_size, T());
    for (size_t i = 0; i < m_size; ++i)
      Swap(m_static[i], m_dynamic[i]);
    m_size = USE_DYNAMIC;
  }
};

template <class T, size_t N>
void swap(buffer_vector<T, N> & r1, buffer_vector<T, N> & r2) { r1.swap(r2); }

template <typename T, size_t N>
inline string debug_print(buffer_vector<T, N> const & v)
{
  return ::my::impl::DebugPrintSequence(v.data(), v.data() + v.size());
}
