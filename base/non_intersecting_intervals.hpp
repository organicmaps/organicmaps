#pragma once

#include "base/assert.hpp"

#include <algorithm>
#include <set>

namespace base
{
///\brief Data structure that maintains a set of non-intersecting intervals.
///       A new interval may only be added to the set if it does not intersect any
///       of the present intervals, thus the choice which intervals to keep is made
///       in a greedy online fashion with no lookahead.
template <typename T>
class NonIntersectingIntervals
{
public:
  /// \brief Adds new interval to set if it doesn't intersect with any that has been already added.
  /// \return true if there are no such intervals, that intersect the [left, right] interval.
  bool AddInterval(T left, T right);
  bool Intersects(T left, T right) const;

private:
  struct Interval
  {
    Interval(T left, T right);

    struct LessByLeftEnd
    {
      bool operator()(Interval const & lhs, Interval const & rhs) const;
    };

    bool Intersects(Interval const & rhs) const;

    T m_left{};
    T m_right{};
  };

  std::set<Interval, typename Interval::LessByLeftEnd> m_leftEnds;
};

template <typename T>
bool NonIntersectingIntervals<T>::Intersects(T left, T right) const
{
  Interval interval(left, right);
  auto it = m_leftEnds.lower_bound(interval);
  if (it != m_leftEnds.end() && interval.Intersects(*it))
    return true;

  if (it != m_leftEnds.begin() && interval.Intersects(*std::prev(it)))
    return true;

  return false;
}

template <typename T>
bool NonIntersectingIntervals<T>::AddInterval(T left, T right)
{
  if (Intersects(left, right))
    return false;

  m_leftEnds.emplace(left, right);
  return true;
}

template <typename T>
NonIntersectingIntervals<T>::Interval::Interval(T left, T right) : m_left(left)
                                                                 , m_right(right)
{
  CHECK_LESS_OR_EQUAL(left, right, ());
}

template <typename T>
bool NonIntersectingIntervals<T>::Interval::LessByLeftEnd::operator()(Interval const & lhs, Interval const & rhs) const
{
  return lhs.m_left < rhs.m_left;
}

template <typename T>
bool NonIntersectingIntervals<T>::Interval::Intersects(Interval const & rhs) const
{
  return std::max(m_left, rhs.m_left) <= std::min(m_right, rhs.m_right);
}
}  // namespace base
