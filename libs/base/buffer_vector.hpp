#pragma once

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

// Calls swap() function using argument dependant lookup.
// // Do NOT override this function, but override swap() function instead!
template <typename T>
void Swap(T & a, T & b)
{
  using std::swap;
  swap(a, b);
}

template <class T, size_t N>
class buffer_vector
{
private:
  enum
  {
    USE_DYNAMIC = N + 1
  };
  // TODO (@gmoryes) consider std::aligned_storage
  T m_static[N];
  size_t m_size;
  std::vector<T> m_dynamic;

  constexpr bool IsDynamic() const { return m_size == USE_DYNAMIC; }

  constexpr void MoveStatic(buffer_vector & rhs) noexcept
  {
    static_assert(std::is_nothrow_move_assignable_v<T>);

    std::move(rhs.m_static, rhs.m_static + rhs.m_size, m_static);
  }

  constexpr void SetStaticSize(size_t newSize)
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      // Call destructors for old elements.
      for (size_t i = newSize; i < m_size; ++i)
        m_static[i] = T();
    }
    m_size = newSize;
  }

  static constexpr size_t SwitchCapacity() { return 3 * N / 2 + 1; }

public:
  typedef T value_type;
  typedef T const & const_reference;
  typedef T & reference;
  typedef size_t size_type;
  typedef T const * const_iterator;
  typedef T * iterator;

  constexpr buffer_vector() : m_size(0) {}
  constexpr explicit buffer_vector(size_t n) : m_size(0) { resize(n); }

  constexpr buffer_vector(std::initializer_list<T> init) : m_size(0)
  {
    assign(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()));
  }

  template <typename TIt>
  constexpr buffer_vector(TIt beg, TIt end) : m_size(0)
  {
    assign(beg, end);
  }

  constexpr buffer_vector(buffer_vector const &) = default;

  constexpr buffer_vector(buffer_vector && rhs) noexcept : m_size(rhs.m_size), m_dynamic(std::move(rhs.m_dynamic))
  {
    if (!IsDynamic())
      MoveStatic(rhs);

    rhs.m_size = 0;
  }

  constexpr buffer_vector & operator=(buffer_vector const & rhs) = default;

  constexpr buffer_vector & operator=(buffer_vector && rhs) noexcept
  {
    if (this != &rhs)
    {
      m_size = rhs.m_size;
      m_dynamic = std::move(rhs.m_dynamic);

      if (!IsDynamic())
        MoveStatic(rhs);

      rhs.m_size = 0;
    }
    return *this;
  }

  template <size_t M>
  constexpr void append(buffer_vector<value_type, M> const & v)
  {
    append(v.begin(), v.end());
  }

  template <typename TIt>
  constexpr void append(TIt beg, TIt end)
  {
    if (!IsDynamic())
    {
      size_t const newSize = std::distance(beg, end) + m_size;
      if (newSize <= N)
      {
        std::copy(beg, end, &m_static[m_size]);
        m_size = newSize;
        return;
      }
      else
        SwitchToDynamic(newSize);
    }

    m_dynamic.insert(m_dynamic.end(), beg, end);
  }

  constexpr void append(size_t count, T const & c)
  {
    if (!IsDynamic())
    {
      size_t const newSize = count + m_size;
      if (newSize <= N)
      {
        std::fill_n(&m_static[m_size], count, c);
        m_size = newSize;
        return;
      }
      else
        SwitchToDynamic(newSize);
    }

    m_dynamic.insert(m_dynamic.end(), count, c);
  }

  template <typename TIt>
  constexpr void assign(TIt beg, TIt end)
  {
    if (IsDynamic())
    {
      m_dynamic.assign(beg, end);
      return;
    }

    m_size = 0;
    append(beg, end);
  }

  constexpr void reserve(size_t n)
  {
    if (IsDynamic() || n > N)
      m_dynamic.reserve(n);
  }

  constexpr void resize(size_t n)
  {
    if (IsDynamic())
    {
      m_dynamic.resize(n);
      return;
    }

    if (n <= N)
    {
      /// @note Do not make value initialization intentionally!
      /// To speedup its primary usage (uint8_t or Point buffer).
      SetStaticSize(n);
    }
    else
    {
      SwitchToDynamic(n);
      m_dynamic.resize(n);
      ASSERT_EQUAL(m_dynamic.size(), n, ());
    }
  }

  constexpr void resize(size_t n, T c)
  {
    if (IsDynamic())
    {
      m_dynamic.resize(n, c);
      return;
    }

    if (n <= N)
    {
      if (n > m_size)
        std::fill_n(&m_static[m_size], n - m_size, c);

      SetStaticSize(n);
    }
    else
    {
      size_t const oldSize = m_size;
      SwitchToDynamic(n);
      m_dynamic.insert(m_dynamic.end(), n - oldSize, c);
      ASSERT_EQUAL(m_dynamic.size(), n, ());
    }
  }

  constexpr void clear()
  {
    if (IsDynamic())
    {
      m_dynamic.clear();
      return;
    }

    SetStaticSize(0);
  }

  /// @todo Here is some inconsistencies:
  /// - "data" method should return 0 if vector is empty;\n
  /// - potential memory overrun if m_dynamic is empty;\n
  /// The best way to fix this is to reset m_size from USE_DYNAMIC to 0 when vector becomes empty.
  /// But now I will just add some assertions to test memory overrun.
  //@{
  constexpr T const * data() const
  {
    if (IsDynamic())
      return m_dynamic.data();
    return &m_static[0];
  }

  constexpr T * data()
  {
    if (IsDynamic())
      return m_dynamic.data();

    return &m_static[0];
  }
  //@}

  constexpr T const * begin() const { return data(); }
  constexpr T const * cbegin() const { return data(); }
  constexpr T * begin() { return data(); }
  constexpr T const * end() const { return data() + size(); }
  constexpr T const * cend() const { return data() + size(); }
  constexpr T * end() { return data() + size(); }
  //@}

  constexpr bool empty() const { return (IsDynamic() ? m_dynamic.empty() : m_size == 0); }
  constexpr size_t size() const { return (IsDynamic() ? m_dynamic.size() : m_size); }

  constexpr T const & front() const
  {
    ASSERT(!empty(), ());
    return *begin();
  }

  constexpr T & front()
  {
    ASSERT(!empty(), ());
    return *begin();
  }

  constexpr T const & back() const
  {
    ASSERT(!empty(), ());
    return *(end() - 1);
  }

  constexpr T & back()
  {
    ASSERT(!empty(), ());
    return *(end() - 1);
  }

  constexpr T const & operator[](size_t i) const
  {
    ASSERT_LESS(i, size(), ());
    return *(begin() + i);
  }

  constexpr T & operator[](size_t i)
  {
    ASSERT_LESS(i, size(), ());
    return *(begin() + i);
  }

  constexpr void swap(buffer_vector & rhs) noexcept
  {
    m_dynamic.swap(rhs.m_dynamic);

    // Only swap static elements that are actually in use.
    bool const lhsDyn = IsDynamic();
    bool const rhsDyn = rhs.IsDynamic();
    if (!lhsDyn || !rhsDyn)
    {
      size_t const maxUsed = lhsDyn ? rhs.m_size : (rhsDyn ? m_size : std::max(m_size, rhs.m_size));
      for (size_t i = 0; i < maxUsed; ++i)
        Swap(m_static[i], rhs.m_static[i]);
    }

    Swap(m_size, rhs.m_size);
  }

  /// By value to be consistent with m_vec.push_back(m_vec[0]).
  constexpr void push_back(T t)
  {
    if (!IsDynamic())
    {
      if (m_size < N)
      {
        m_static[m_size] = std::move(t);
        ++m_size;  // keep basic exception safety here
        return;
      }
      else
        SwitchToDynamic(SwitchCapacity());
    }

    m_dynamic.push_back(std::move(t));
  }

  constexpr void pop_back()
  {
    if (IsDynamic())
    {
      m_dynamic.pop_back();
      return;
    }

    ASSERT_GREATER(m_size, 0, ());
    --m_size;
    m_static[m_size] = T();
  }

  template <class... Args>
  constexpr void emplace_back(Args &&... args)
  {
    if (IsDynamic())
    {
      m_dynamic.emplace_back(std::forward<Args>(args)...);
    }
    else if (m_size < N)
    {
      m_static[m_size] = value_type(std::forward<Args>(args)...);
      ++m_size;  // keep basic exception safety here
    }
    else
    {
      // Construct value first in case of reallocation and m_vec.emplace_back(m_vec[0]).
      value_type value(std::forward<Args>(args)...);
      SwitchToDynamic(SwitchCapacity());
      m_dynamic.push_back(std::move(value));
    }
  }

  template <typename TIt>
  constexpr void insert(const_iterator where, TIt beg, TIt end)
  {
    size_t const pos = base::asserted_cast<size_t>(where - data());
    ASSERT_LESS_OR_EQUAL(pos, size(), ());

    if (IsDynamic())
    {
      m_dynamic.insert(m_dynamic.begin() + pos, beg, end);
      return;
    }

    size_t const n = end - beg;
    if (m_size + n <= N)
    {
      if (pos != m_size)
        for (size_t i = m_size - 1; i >= pos && i < m_size; --i)
          Swap(m_static[i], m_static[i + n]);

      m_size += n;
      T * writableWhere = &m_static[0] + pos;
      ASSERT(where == writableWhere, ());
      while (beg != end)
        *(writableWhere++) = *(beg++);
    }
    else
    {
      SwitchToDynamic(m_size + n);
      m_dynamic.insert(m_dynamic.begin() + pos, beg, end);
    }
  }

  constexpr void insert(const_iterator where, value_type const & value) { insert(where, &value, &value + 1); }

  template <class Fn>
  constexpr void erase_if(Fn && fn)
  {
    iterator b = begin();
    iterator e = end();
    iterator i = std::remove_if(b, e, std::forward<Fn>(fn));
    if (i != e)
      resize(std::distance(b, i));
  }

  constexpr void erase(iterator first, iterator last)
  {
    if (first == last)
      return;

    auto const numToErase = std::distance(first, last);
    for (; first != end() - numToErase; ++first)
      Swap(*first, *(first + numToErase));
    resize(std::distance(begin(), first));
  }
  void erase(iterator it) { erase(it, it + 1); }

private:
  void SwitchToDynamic(size_t toReserve)
  {
    ASSERT_NOT_EQUAL(m_size, static_cast<size_t>(USE_DYNAMIC), ());
    ASSERT_EQUAL(m_dynamic.size(), 0, ());

    m_dynamic.reserve(toReserve);
    if constexpr (std::is_trivially_constructible_v<T>)
    {
      // Use range move (memcpy) if cheap resize.
      m_dynamic.resize(m_size);
      std::move(m_static, m_static + m_size, m_dynamic.begin());
    }
    else
    {
      for (size_t i = 0; i < m_size; ++i)
        m_dynamic.push_back(std::move(m_static[i]));
    }

    m_size = USE_DYNAMIC;
  }
};

template <class T, size_t N>
void swap(buffer_vector<T, N> & r1, buffer_vector<T, N> & r2)
{
  r1.swap(r2);
}

template <typename T, size_t N>
std::string DebugPrint(buffer_vector<T, N> const & v)
{
  return DebugPrintSequence(v.data(), v.data() + v.size());
}

template <typename T, size_t N1, size_t N2>
bool operator==(buffer_vector<T, N1> const & v1, buffer_vector<T, N2> const & v2)
{
  return (v1.size() == v2.size() && std::equal(v1.begin(), v1.end(), v2.begin()));
}

template <typename T, size_t N1, size_t N2>
bool operator!=(buffer_vector<T, N1> const & v1, buffer_vector<T, N2> const & v2)
{
  return !(v1 == v2);
}

template <typename T, size_t N1, size_t N2>
bool operator<(buffer_vector<T, N1> const & v1, buffer_vector<T, N2> const & v2)
{
  return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end());
}

template <typename T, size_t N1, size_t N2>
bool operator>(buffer_vector<T, N1> const & v1, buffer_vector<T, N2> const & v2)
{
  return v2 < v1;
}
// TODO(AB): Use <=> operator.
// Used in:
// TrieChar const * const keyData = entry.GetKeyData();
// TTrieString key(keyData, keyData + entry.GetKeySize());
// using namespace std::rel_ops;  // ">=" for keys.
// CHECK_GREATER_OR_EQUAL(key, prevKey, (key, prevKey));
template <typename T, size_t N1, size_t N2>
bool operator>=(buffer_vector<T, N1> const & v1, buffer_vector<T, N2> const & v2)
{
  return !(v1 < v2);
}

namespace std
{
template <typename T, size_t N>
typename buffer_vector<T, N>::iterator begin(buffer_vector<T, N> & v)
{
  return v.begin();
}

template <typename T, size_t N>
typename buffer_vector<T, N>::const_iterator begin(buffer_vector<T, N> const & v)
{
  return v.begin();
}

template <typename T, size_t N>
typename buffer_vector<T, N>::const_iterator end(buffer_vector<T, N> const & v)
{
  return v.end();
}
}  // namespace std

template <class Dest, class Src>
void assign_range(Dest & dest, Src const & src)
{
  dest.assign(std::begin(src), std::end(src));
}
