#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>

namespace my
{
using StringIL = std::initializer_list<char const *>;
}  // namespace my

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
    m_Container.insert(end(m_Container), t);
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
          std::swap(*beg, *end);
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
  return IsSorted(beg, end, std::less<typename std::iterator_traits<IterT>::value_type>());
}

template <class IterT> inline bool IsSortedAndUnique(IterT beg, IterT end)
{
  return IsSortedAndUnique(beg, end, std::less<typename std::iterator_traits<IterT>::value_type>());
}

struct DeleteFunctor
{
  template <typename T> void operator() (T const * p) const
  {
    delete p;
  }
};

namespace impl
{
  template <class TContainer, class TDeletor> class DeleteRangeFunctor
  {
    TContainer & m_cont;
    TDeletor m_deletor;

  public:
    DeleteRangeFunctor(TContainer & cont, TDeletor const & deletor)
      : m_cont(cont), m_deletor(deletor)
    {
    }

    void operator() ()
    {
      for_each(m_cont.begin(), m_cont.end(), m_deletor);
      m_cont.clear();
    }
  };
}

template <class TContainer, class TDeletor>
impl::DeleteRangeFunctor<TContainer, TDeletor>
GetRangeDeletor(TContainer & cont, TDeletor const & deletor)
{
  return impl::DeleteRangeFunctor<TContainer, TDeletor>(cont, deletor);
}

template <class TContainer, class TDeletor>
void DeleteRange(TContainer & cont, TDeletor const & deletor)
{
  (void)GetRangeDeletor(cont, deletor)();
}

struct IdFunctor
{
  template <typename T> inline T operator () (T const & x) const
  {
    return x;
  }
};

template <class T> struct EqualFunctor
{
  T const & m_t;
  explicit EqualFunctor(T const & t) : m_t(t) {}
  inline bool operator() (T const & t) const { return (t == m_t); }
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

template <class IterT1, class IterT2, class InsertIterT>
void AccumulateIntervals1With2(IterT1 b1, IterT1 e1, IterT2 b2, IterT2 e2, InsertIterT res)
{
  //typedef typename iterator_traits<InsertIterT>::value_type T;
  typedef typename std::iterator_traits<IterT1>::value_type T;

  T prev;
  bool validPrev = false;

  while (b1 != e1 || b2 != e2)
  {
    // Try to continue previous range.
    if (validPrev)
    {
      // add b1 range to prev if needed
      if (b1 != e1 && b1->first < prev.second)
      {
        // correct only second if needed
        if (prev.second < b1->second)
          prev.second = b1->second;
        ++b1;
        continue;
      }

      // add b2 range to prev if needed
      if (b2 != e2 && b2->first < prev.second)
      {
        // check that intervals are overlapped
        if (prev.first < b2->second)
        {
          // correct first and second if needed
          if (b2->first < prev.first)
            prev.first = b2->first;
          if (prev.second < b2->second)
            prev.second = b2->second;
        }

        ++b2;
        continue;
      }

      // if nothing to add - push to results
      *res++ = prev;
      validPrev = false;
    }

    if (b1 != e1)
    {
      // start new range
      prev = *b1++;
      validPrev = true;
    }
    else
    {
      // go to exit
      break;
    }
  }

  if (validPrev)
    *res++ = prev;
}
