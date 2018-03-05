#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace my
{
namespace impl
{
// When isField is true, following functors operate on a
// pointers-to-field.  Otherwise, they operate on a
// pointers-to-const-method.
template <bool isField, typename T, typename C>
struct Less;

template <typename T, typename C>
struct Less<true, T, C>
{
  Less(T(C::*p)) : m_p(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return lhs.*m_p < rhs.*m_p; }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return lhs->*m_p < rhs->*m_p;
  }

  T(C::*m_p);
};

template <typename T, typename C>
struct Less<false, T, C>
{
  Less(T (C::*p)() const) : m_p(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return (lhs.*m_p)() < (rhs.*m_p)(); }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return (lhs->*m_p)() < (rhs->*m_p)();
  }

  T (C::*m_p)() const;
};

template <bool isField, typename T, typename C>
struct Equals;

template <typename T, typename C>
struct Equals<true, T, C>
{
  Equals(T(C::*p)) : m_p(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return lhs.*m_p == rhs.*m_p; }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return lhs->*m_p == rhs->*m_p;
  }

  T(C::*m_p);
};

template <typename T, typename C>
struct Equals<false, T, C>
{
  Equals(T (C::*p)() const) : m_p(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return (lhs.*m_p)() == (rhs.*m_p)(); }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return (lhs->*m_p)() == (rhs->*m_p)();
  }

  T (C::*m_p)() const;
};
}  // namespace impl

// Sorts and removes duplicate entries from |c|.
template <typename Cont>
void SortUnique(Cont & c)
{
  sort(c.begin(), c.end());
  c.erase(unique(c.begin(), c.end()), c.end());
}

// Sorts according to |less| and removes duplicate entries according to |equals| from |c|.
// Note. If several entries are equal according to |less| an arbitrary entry of them
// is left in |c| after a call of this function.
template <class Cont, typename Less, typename Equals>
void SortUnique(Cont & c, Less && less, Equals && equals)
{
  sort(c.begin(), c.end(), std::forward<Less>(less));
  c.erase(unique(c.begin(), c.end(), std::forward<Equals>(equals)), c.end());
}

template <class Cont, class Fn>
void EraseIf(Cont & c, Fn && fn)
{
  c.erase(remove_if(c.begin(), c.end(), std::forward<Fn>(fn)), c.end());
}

// Creates a comparer being able to compare two instances of class C
// (given by reference or pointer) by a field or const method of C.
// For example, to create comparer that is able to compare pairs of
// ints by second component, it's enough to call LessBy(&pair<int,
// int>::second).
template <typename T, typename C>
impl::Less<true, T, C> LessBy(T(C::*p))
{
  return impl::Less<true, T, C>(p);
}

template <typename T, typename C>
impl::Less<false, T, C> LessBy(T (C::*p)() const)
{
  return impl::Less<false, T, C>(p);
}

template <typename T, typename C>
impl::Equals<true, T, C> EqualsBy(T(C::*p))
{
  return impl::Equals<true, T, C>(p);
}

template <typename T, typename C>
impl::Equals<false, T, C> EqualsBy(T (C::*p)() const)
{
  return impl::Equals<false, T, C>(p);
}

template <typename T>
std::underlying_type_t<T> Key(T value)
{
  return static_cast<std::underlying_type_t<T>>(value);
}

// Use this if you want to make a functor whose first
// argument is ignored and the rest are forwarded to |fn|.
template <typename Fn>
class IgnoreFirstArgument
{
public:
  template <typename Gn>
  IgnoreFirstArgument(Gn && gn) : m_fn(std::forward<Gn>(gn)) {}

  template <typename Arg, typename... Args>
  std::result_of_t<Fn(Args &&...)> operator()(Arg && arg, Args &&... args)
  {
    return m_fn(std::forward<Args>(args)...);
  }

private:
  Fn m_fn;
};

template <typename Fn>
IgnoreFirstArgument<Fn> MakeIgnoreFirstArgument(Fn && fn)
{
  return IgnoreFirstArgument<Fn>(std::forward<Fn>(fn));
}
}  // namespace my
