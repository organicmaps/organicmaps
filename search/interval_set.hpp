#pragma once

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace my
{
// This class represents a set of disjoint intervals in the form
// [begin, end).  Note that neighbour intervals are always coalesced,
// so while [0, 1), [1, 2) and [2, 3) are disjoint, after addition to
// the set they will be stored as a single [0, 3).
template <typename TElem>
class IntervalSet
{
public:
  using TInterval = pair<TElem, TElem>;

  // Adds an |interval| to the set, coalescing adjacent intervals if needed.
  //
  // Complexity: O(num of intervals intersecting with |interval| +
  // log(total number of intervals)).
  void Add(TInterval const & interval);

  // Subtracts set from an |interval| and appends result to
  // |difference|.
  //
  // Complexity: O(num of intervals intersecting with |interval| +
  // log(total number of intervals)).
  void SubtractFrom(TInterval const & interval, vector<TInterval> & difference) const;

  // Returns all elements of the set as a set of intervals.
  //
  // Complexity: O(1).
  inline set<TInterval> const & Elems() const { return m_intervals; }

private:
  using TIterator = typename set<TInterval>::iterator;

  // Calculates range of intervals that have non-empty intersection with a given |interval|.
  void Cover(TInterval const & interval, TIterator & begin, TIterator & end) const;

  // This is a set of disjoint intervals.
  set<TInterval> m_intervals;
};

template <typename TElem>
void IntervalSet<TElem>::Add(TInterval const & interval)
{
  // Skips empty intervals.
  if (interval.first == interval.second)
    return;

  TIterator begin;
  TIterator end;
  Cover(interval, begin, end);

  TElem from = interval.first;
  TElem to = interval.second;

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

template <typename TElem>
void IntervalSet<TElem>::SubtractFrom(TInterval const & interval,
                                      vector<TInterval> & difference) const
{
  TIterator begin;
  TIterator end;

  Cover(interval, begin, end);

  TElem from = interval.first;
  TElem const to = interval.second;

  for (auto it = begin; it != end && from < to; ++it)
  {
    if (it->first > from)
    {
      difference.emplace_back(from, it->first);
      from = it->second;
    }
    else
    {
      from = max(from, it->second);
    }
  }

  if (from < to)
    difference.emplace_back(from, to);
}

template <typename TElem>
void IntervalSet<TElem>::Cover(TInterval const & interval, TIterator & begin, TIterator & end) const
{
  TElem const & from = interval.first;
  TElem const & to = interval.second;

  begin = m_intervals.lower_bound(make_pair(from, from));
  if (begin != m_intervals.begin())
  {
    auto prev = begin;
    --prev;
    if (prev->second > from)
      begin = prev;
  }

  end = m_intervals.lower_bound(make_pair(to, to));
}
}  // namespace my
