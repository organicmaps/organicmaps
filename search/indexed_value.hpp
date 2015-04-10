#pragma once

#include "std/bind.hpp"
#include "std/algorithm.hpp"
#include "std/vector.hpp"
#include "std/limits.hpp"


namespace search
{

/// Intrusive class to implement multi-index for some user-defined type.
template <size_t N> class IndexedValueBase
{
protected:
  static size_t const SIZE = N;
  size_t m_ind[N];

public:
  IndexedValueBase()
  {
    for (size_t i = 0; i < N; ++i)
      m_ind[i] = numeric_limits<size_t>::max();
  }

  void SetIndex(size_t i, size_t v) { m_ind[i] = v; }

  void SortIndex()
  {
    sort(m_ind, m_ind + N);
  }

  static bool Less(IndexedValueBase const & r1, IndexedValueBase const & r2)
  {
    for (size_t i = 0; i < N; ++i)
    {
      if (r1.m_ind[i] != r2.m_ind[i])
        return (r1.m_ind[i] < r2.m_ind[i]);
    }

    return false;
  }
};

template <class T, class CompFactory>
void SortByIndexedValue(vector<T> & vec, CompFactory factory)
{
  for (size_t i = 0; i < CompFactory::SIZE; ++i)
  {
    typename CompFactory::CompT comp = factory.Get(i);

    // sort by needed criteria
    sort(vec.begin(), vec.end(), comp);

    // assign ranks
    size_t rank = 0;
    for (size_t j = 0; j < vec.size(); ++j)
    {
      if (j > 0 && comp(vec[j-1], vec[j]))
        ++rank;

      vec[j].SetIndex(i, rank);
    }
  }

  // prepare combined criteria
  for_each(vec.begin(), vec.end(), [] (IndexedValueBase<CompFactory::SIZE> & p) { p.SortIndex(); });

  // sort results according to combined criteria
  sort(vec.begin(), vec.end(), &IndexedValueBase<CompFactory::SIZE>::Less);
}

}
