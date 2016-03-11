#pragma once

#include "std/algorithm.hpp"
#include "std/vector.hpp"

namespace my
{
namespace impl
{
template <bool isField, typename T, typename C>
struct Comparer;

template<typename T, typename C>
struct Comparer<true, T, C>
{
  Comparer(T(C::*p)) : p_(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return lhs.*p_ < rhs.*p_; }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return lhs->*p_ < rhs->*p_;
  }

  T(C::*p_);
};

template<typename T, typename C>
struct Comparer<false, T, C>
{
  Comparer(T (C::*p)() const) : p_(p) {}

  inline bool operator()(C const & lhs, C const & rhs) const { return (lhs.*p_)() < (rhs.*p_)(); }

  inline bool operator()(C const * const lhs, C const * const rhs) const
  {
    return (lhs->*p_)() < (rhs->*p_)();
  }

  T(C::*p_)() const;
};
}  // namespace impl

// Sorts and removes duplicate entries from |v|.
template <typename T>
void SortUnique(std::vector<T> & v)
{
  sort(v.begin(), v.end());
  v.erase(unique(v.begin(), v.end()), v.end());
}

// Creates a comparer being able to compare two instances of class C
// (given by reference or pointer) by a field of C.  For example, to
// create comparer that is able to compare pairs of ints by second
// component, it's enough to call CompareBy(&pair<int, int>::second).
template <typename T, typename C>
impl::Comparer<true, T, C> CompareBy(T(C::*p))
{
  return impl::Comparer<true, T, C>(p);
}

template <typename T, typename C>
impl::Comparer<false, T, C> CompareBy(T (C::*p)() const)
{
  return impl::Comparer<false, T, C>(p);
}
}  // namespace my
