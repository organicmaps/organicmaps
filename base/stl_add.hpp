#pragma once
#include "../std/functional.hpp"
#include "../std/iterator.hpp"

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

struct NoopFunctor
{
  template <typename T> void operator () (T const & value) const
  {
    UNUSED_VALUE(value);
  }
};
