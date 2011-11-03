#pragma once

#include "threaded_list.hpp"
#include "logging.hpp"
#include "../std/bind.hpp"

struct BasePoolElemFactory
{
  string m_resName;
  size_t m_elemSize;
  BasePoolElemFactory(char const * resName, size_t elemSize);

  char const * ResName() const;
  size_t ElemSize() const;
};

template <typename TElem, typename TElemFactory>
struct BasePoolTraits
{
  TElemFactory m_factory;

  BasePoolTraits(TElemFactory const & factory) : m_factory(factory)
  {}

  void Free(ThreadedList<TElem> & elems, TElem const & elem)
  {
    elems.PushBack(elem);
  }
};

template <typename TElem, typename TElemFactory>
struct FixedSizePoolTraits : BasePoolTraits<TElem, TElemFactory>
{
  typedef BasePoolTraits<TElem, TElemFactory> base_t;
  size_t m_count;
  bool m_isAllocated;

  FixedSizePoolTraits(TElemFactory const & factory, size_t count)
    : base_t(factory),
      m_count(count),
      m_isAllocated(false)
  {}

  TElem const Reserve(ThreadedList<TElem> & elems)
  {
    if (!m_isAllocated)
    {
      m_isAllocated = true;

      LOG(LDEBUG, ("allocating ", base_t::m_factory.ElemSize() * m_count, "bytes for ", base_t::m_factory.ResName()));

      for (size_t i = 0; i < m_count; ++i)
        elems.PushBack(base_t::m_factory.Create());
    }

    return elems.Front(true);
  }
};

template <typename TElem, typename TElemFactory>
struct AllocateOnDemandPoolTraits : BasePoolTraits<TElem, TElemFactory>
{
  typedef BasePoolTraits<TElem, TElemFactory> base_t;

  size_t m_poolSize;
  AllocateOnDemandPoolTraits(TElemFactory const & factory, size_t )
    : base_t(factory),
      m_poolSize(0)
  {}

  void ReserveImpl(list<TElem> & l, TElem & elem)
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

  TElem const Reserve(ThreadedList<TElem> & elems)
  {
    TElem elem;
    elems.ProcessList(bind(&AllocateOnDemandPoolTraits<TElem, TElemFactory>::ReserveImpl, this, _1, ref(elem)));
    return elem;
  }
};

// This class tracks OpenGL resources allocation in
// a multithreaded environment.
template <typename TElem, typename TPoolTraits>
class ResourcePool
{
private:

  TPoolTraits m_traits;
  ThreadedList<TElem> m_pool;

public:

  ResourcePool(TPoolTraits const & traits)
    : m_traits(traits)
  {
    /// quick trick to perform lazy initialization
    /// on the same thread the pool was created.
    Free(Reserve());
  }

  TElem const Reserve()
  {
    return m_traits.Reserve(m_pool);
  }

  void Free(TElem const & elem)
  {
    m_traits.Free(m_pool, elem);
  }

  size_t Size() const
  {
    return m_pool.Size();
  }

  void EnterForeground()
  {}

  void EnterBackground()
  {}

  void Cancel()
  {
    m_pool.Cancel();
  }

  bool IsCancelled() const
  {
    return m_pool.IsCancelled();
  }
};
