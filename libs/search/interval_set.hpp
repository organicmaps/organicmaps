#pragma once

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

// todo(@m) Move to search/base?
namespace search
{
// This class represents a set of disjoint intervals in the form
// [begin, end).  Note that neighbour intervals are always coalesced,
// so while [0, 1), [1, 2) and [2, 3) are disjoint, after addition to
// the set they will be stored as a single [0, 3).
template <typename Elem>
class IntervalSet
{
public:
  using Interval = std::pair<Elem, Elem>;

  // Adds an |interval| to the set, coalescing adjacent intervals if needed.
  //
  // Complexity: O(num of intervals intersecting with |interval| +
  // log(total number of intervals)).
  void Add(Interval const & interval);

  // Subtracts set from an |interval| and appends result to
  // |difference|.
  //
  // Complexity: O(num of intervals intersecting with |interval| +
  // log(total number of intervals)).
  void SubtractFrom(Interval const & interval, std::vector<Interval> & difference) const;

  // Returns all elements of the set as a set of intervals.
  //
  // Complexity: O(1).
  inline std::set<Interval> const & Elems() const { return m_intervals; }

private:
  using Iterator = typename std::set<Interval>::iterator;

  // Calculates range of intervals that have non-empty intersection with a given |interval|.
  void Cover(Interval const & interval, Iterator & begin, Iterator & end) const;

  // This is a set of disjoint intervals.
  std::set<Interval> m_intervals;
};

template <typename Elem>
void IntervalSet<Elem>::Add(Interval const & interval)
{
  // Skips empty intervals.
  if (interval.first == interval.second)
    return;

  Iterator begin;
  Iterator end;
  Cover(interval, begin, end);

  Elem from = interval.first;
  Elem to = interval.second;

  // Updates |from| and |to| in accordance with corner intervals (if any).
  if (begin != end)
  {
    if (begin->first < from)
      from = begin->first;

    auto last = end;
    --last;
    if (last->second > to)
      to = last->second;
  }

  // Now all elements [from, to) can be added to the set as a single
  // interval which will replace all intervals in [begin, end). But
  // note that it can be possible to merge new interval with its
  // neighbors, so following code checks it.
  if (begin != m_intervals.begin())
  {
    auto prevBegin = begin;
    --prevBegin;
    if (prevBegin->second == from)
    {
      begin = prevBegin;
      from = prevBegin->first;
    }
  }
  if (end != m_intervals.end() && end->first == to)
  {
    to = end->second;
    ++end;
  }

  m_intervals.erase(begin, end);
  m_intervals.emplace(from, to);
}

template <typename Elem>
void IntervalSet<Elem>::SubtractFrom(Interval const & interval, std::vector<Interval> & difference) const
{
  Iterator begin;
  Iterator end;

  Cover(interval, begin, end);

  Elem from = interval.first;
  Elem const to = interval.second;

  for (auto it = begin; it != end && from < to; ++it)
  {
    if (it->first > from)
    {
      difference.emplace_back(from, it->first);
      from = it->second;
    }
    else
    {
      from = std::max(from, it->second);
    }
  }

  if (from < to)
    difference.emplace_back(from, to);
}

template <typename Elem>
void IntervalSet<Elem>::Cover(Interval const & interval, Iterator & begin, Iterator & end) const
{
  Elem const & from = interval.first;
  Elem const & to = interval.second;

  begin = m_intervals.lower_bound(std::make_pair(from, from));
  if (begin != m_intervals.begin())
  {
    auto prev = begin;
    --prev;
    if (prev->second > from)
      begin = prev;
  }

  end = m_intervals.lower_bound(std::make_pair(to, to));
}
}  // namespace search
