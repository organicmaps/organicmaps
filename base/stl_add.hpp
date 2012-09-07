#pragma once
#include "../std/functional.hpp"
#include "../std/iterator.hpp"
#include "../std/map.hpp"

template <class ContainerT> class BackInsertFunctor
{
  ContainerT & m_Container;
public:
  explicit BackInsertFunctor(ContainerT & container) : m_Container(container)
  {
  }
  void operator() (typename ContainerT::value_type const & t) const
  {
    m_Container.push_back(t);
  }
};

template <class ContainerT>
BackInsertFunctor<ContainerT> MakeBackInsertFunctor(ContainerT & container)
{
  return BackInsertFunctor<ContainerT>(container);
}

template <class ContainerT> class InsertFunctor
{
  ContainerT & m_Container;
public:
  explicit InsertFunctor(ContainerT & container) : m_Container(container)
  {
  }
  void operator() (typename ContainerT::value_type const & t) const
  {
    m_Container.insert(t);
  }
};

template <class ContainerT>
InsertFunctor<ContainerT> MakeInsertFunctor(ContainerT & container)
{
  return InsertFunctor<ContainerT>(container);
}

template <class IterT, class CompareT> inline bool IsSorted(IterT beg, IterT end, CompareT comp)
{
  if (beg == end)
    return true;
  IterT prev = beg;
  for (++beg; beg != end; ++beg, ++prev)
  {
    if (comp(*beg, *prev))
      return false;
  }
  return true;
}

template <class IterT, class CompareT>
inline bool IsSortedAndUnique(IterT beg, IterT end, CompareT comp)
{
  if (beg == end)
    return true;
  IterT prev = beg;
  for (++beg; beg != end; ++beg, ++prev)
  {
    if (!comp(*prev, *beg))
      return false;
  }
  return true;
}

template <class IterT, class CompareT>
IterT RemoveIfKeepValid(IterT beg, IterT end, CompareT comp)
{
  while (beg != end)
  {
    if (comp(*beg))
    {
      while (beg != --end)
      {
        if (!comp(*end))
        {
          swap(*beg, *end);
          ++beg;
          break;
        }
      }
    }
    else
      ++beg;
  }

  return end;
}


template <class IterT> inline bool IsSorted(IterT beg, IterT end)
{
  return IsSorted(beg, end, less<typename iterator_traits<IterT>::value_type>());
}

template <class IterT> inline bool IsSortedAndUnique(IterT beg, IterT end)
{
  return IsSortedAndUnique(beg, end, less<typename iterator_traits<IterT>::value_type>());
}

struct DeleteFunctor
{
  template <typename T> void operator() (T const * p) const
  {
    delete p;
  }
};

template <class TContainer> class DeleteRangeGuard
{
  TContainer & m_cont;
public:
  DeleteRangeGuard(TContainer & cont) : m_cont(cont) {}
  ~DeleteRangeGuard()
  {
    for_each(m_cont.begin(), m_cont.end(), DeleteFunctor());
    m_cont.clear();
  }
};

struct NoopFunctor
{
  template <typename T> void operator () (T const &) const
  {
  }
};

struct IdFunctor
{
  template <typename T> inline T operator () (T const & x) const
  {
    return x;
  }
};

template <typename IterT> IterT NextIterInCycle(IterT it, IterT beg, IterT end)
{
  if (++it == end)
    return beg;
  return it;
}

template <typename IterT> IterT PrevIterInCycle(IterT it, IterT beg, IterT end)
{
  if (it == beg)
    it = end;
  return --it;
}

template <typename KeyT, typename ValueT, typename CompareT, typename AllocatorT>
ValueT ValueForKey(map<KeyT, ValueT, CompareT, AllocatorT> const & m, KeyT key, ValueT defaultV)
{
  typename map<KeyT, ValueT, CompareT, AllocatorT>::const_iterator const it = m.find(key);
  return (it == m.end() ? defaultV : it->second);
}
