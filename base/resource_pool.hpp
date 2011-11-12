#pragma once

#include "threaded_list.hpp"
#include "logging.hpp"
#include "../std/bind.hpp"
#include "../std/scoped_ptr.hpp"

struct BasePoolElemFactory
{
  string m_resName;
  size_t m_elemSize;
  BasePoolElemFactory(char const * resName, size_t elemSize);

  char const * ResName() const;
  size_t ElemSize() const;
};

/// basic traits maintains a list of free resources.
template <typename TElem, typename TElemFactory>
struct BasePoolTraits
{
  TElemFactory m_factory;
  ThreadedList<TElem> m_pool;

  typedef TElem elem_t;

  BasePoolTraits(TElemFactory const & factory)
    : m_factory(factory)
  {}

  void Free(TElem const & elem)
  {
    m_pool.PushBack(elem);
  }

  size_t Size() const
  {
    return m_pool.Size();
  }

  void Cancel()
  {
    m_pool.Cancel();
  }

  bool IsCancelled() const
  {
    return m_pool.IsCancelled();
  }
};

/// This traits stores the free elements in a separate pool and has
/// a separate method to merge them all into a main pool.
/// For example should be used for resources where a certain preparation operation
/// should be performed on main thread before returning resource
/// to a free pool(p.e. @see resource_manager.cpp StorageFactory)
template <typename TElemFactory, typename TBase>
struct SeparateFreePoolTraits : TBase
{
  typedef TBase base_t;
  typedef typename base_t::elem_t elem_t;

  ThreadedList<elem_t> m_freePool;

  SeparateFreePoolTraits(TElemFactory const & factory)
    : base_t(factory)
  {}

  void Free(elem_t const & elem)
  {
    m_freePool.PushBack(elem);
  }

  void MergeImpl(list<elem_t> & l)
  {
    for (typename list<elem_t>::const_iterator it = l.begin();
         it != l.end();
         ++it)
    {
      base_t::m_factory.BeforeMerge(*it);
      base_t::m_pool.PushBack(*it);
    }

    l.clear();
  }

  void Merge()
  {
    m_freePool.ProcessList(bind(&SeparateFreePoolTraits<TElemFactory, TBase>::MergeImpl, this, _1));
  }
};

/// This traits maintains a fixed-size of pre-allocated resources.
template <typename TElemFactory, typename TBase >
struct FixedSizePoolTraits : TBase
{
  typedef TBase base_t;
  typedef typename base_t::elem_t elem_t;

  size_t m_count;
  bool m_isAllocated;

  FixedSizePoolTraits(TElemFactory const & factory, size_t count)
    : base_t(factory),
      m_count(count),
      m_isAllocated(false)
  {}

  elem_t const Reserve()
  {
    if (!m_isAllocated)
    {
      m_isAllocated = true;

      LOG(LDEBUG, ("allocating ", base_t::m_factory.ElemSize() * m_count, "bytes for ", base_t::m_factory.ResName()));

      for (size_t i = 0; i < m_count; ++i)
        base_t::m_pool.PushBack(base_t::m_factory.Create());
    }

    return base_t::m_pool.Front(true);
  }
};

/// This traits allocates resources on demand.
template <typename TElemFactory, typename TBase>
struct AllocateOnDemandPoolTraits : TBase
{
  typedef TBase base_t;
  typedef typename base_t::elem_t elem_t;

  size_t m_poolSize;
  AllocateOnDemandPoolTraits(TElemFactory const & factory, size_t )
    : base_t(factory),
      m_poolSize(0)
  {}

  void ReserveImpl(list<elem_t> & l, elem_t & elem)
  {
    if (l.empty())
    {
      m_poolSize += base_t::m_factory.ElemSize();
      LOG(LDEBUG, ("allocating ", base_t::m_factory.ElemSize(), "bytes for ", base_t::m_factory.ResName(), " on-demand, poolSize=", m_poolSize));
      l.push_back(base_t::m_factory.Create());
    }

    elem = l.back();

    l.pop_back();
  }

  elem_t const Reserve()
  {
    elem_t elem;
    base_t::m_pool.ProcessList(bind(&AllocateOnDemandPoolTraits<elem_t, TElemFactory>::ReserveImpl, this, _1, ref(elem)));
    return elem;
  }
};

// This class tracks OpenGL resources allocation in
// a multithreaded environment.
template <typename TPoolTraits>
class ResourcePool
{
private:

  scoped_ptr<TPoolTraits> m_traits;

public:

  typedef typename TPoolTraits::elem_t elem_t;

  ResourcePool(TPoolTraits * traits)
    : m_traits(traits)
  {
    /// quick trick to perform lazy initialization
    /// on the same thread the pool was created.
    Free(Reserve());
  }

  elem_t const Reserve()
  {
    return m_traits->Reserve();
  }

  void Free(elem_t const & elem)
  {
    m_traits->Free(elem);
  }

  size_t Size() const
  {
    return m_traits->Size();
  }

  void EnterForeground()
  {}

  void EnterBackground()
  {}

  void Cancel()
  {
    return m_traits->Cancel();
  }

  bool IsCancelled() const
  {
    return m_traits->IsCancelled();
  }

  void Merge()
  {
    m_traits->Merge();
  }
};
