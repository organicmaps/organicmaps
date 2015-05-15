#pragma once

#include "base/thread.hpp"
#include "base/threaded_list.hpp"
#include "base/logging.hpp"
#include "base/assert.hpp"

#include "std/bind.hpp"
#include "std/unique_ptr.hpp"

//#define ALLOCATED_COUNT

struct BasePoolElemFactory
{
  string m_resName;
  size_t m_elemSize;
  size_t m_batchSize;

  BasePoolElemFactory(char const * resName, size_t elemSize, size_t batchSize);

  size_t BatchSize() const;
  char const * ResName() const;
  size_t ElemSize() const;
};

/// basic traits maintains a list of free resources.
template <typename TElem, typename TElemFactory>
struct BasePoolTraits
{
  TElemFactory m_factory;
  ThreadedList<TElem> m_pool;
  bool m_IsDebugging;
  threads::ThreadID m_MainThreadID;

  typedef TElem elem_t;

  BasePoolTraits(TElemFactory const & factory)
    : m_factory(factory),
      m_IsDebugging(false),
      m_MainThreadID(threads::GetCurrentThreadID())
  {
    m_pool.SetName(factory.ResName());
  }

  virtual ~BasePoolTraits()
  {}

  virtual void Init()
  {
    Free(Reserve());
  }

  virtual void Free(TElem const & elem)
  {
    m_pool.PushBack(elem);
  }

  virtual TElem const Reserve()
  {
    return m_pool.Front(true);
  }

  virtual size_t Size() const
  {
    return m_pool.Size();
  }

  virtual void Cancel()
  {
    m_pool.Cancel();
  }

  virtual bool IsCancelled() const
  {
    return m_pool.IsCancelled();
  }

  virtual void UpdateState()
  {
  }

  char const * ResName() const
  {
    return m_factory.ResName();
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
  int m_maxFreePoolSize;

  SeparateFreePoolTraits(TElemFactory const & factory)
    : base_t(factory), m_maxFreePoolSize(0)
  {}

  void Free(elem_t const & elem)
  {
    m_freePool.PushBack(elem);
/*    if (base_t::m_IsDebugging)
    {
      int oldMaxFreePoolSize = m_maxFreePoolSize;
      m_maxFreePoolSize = max(m_maxFreePoolSize, (int)m_freePool.Size());
      if (oldMaxFreePoolSize != m_maxFreePoolSize)
        LOG(LINFO, (base_t::m_pool.GetName(), "freePool maximum size has reached", m_maxFreePoolSize, "elements"));
    }*/
  }

  void UpdateStateImpl(list<elem_t> & l)
  {
    for (typename list<elem_t>::const_iterator it = l.begin();
         it != l.end();
         ++it)
    {
      base_t::m_factory.BeforeMerge(*it);
      base_t::m_pool.PushBack(*it);
    }

//    if ((base_t::m_IsDebugging) && (!base_t::m_pool.GetName().empty()))
//      LOG(LINFO, ("pool for", base_t::m_pool.GetName(), "has", base_t::m_pool.Size(), "elements"));

    l.clear();
  }

  void UpdateState()
  {
    m_freePool.ProcessList([this] (list<elem_t> & l) { UpdateStateImpl(l); });
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

    return base_t::Reserve();
  }
};

/// This traits allocates resources on demand.
template <typename TElemFactory, typename TBase>
struct AllocateOnDemandMultiThreadedPoolTraits : TBase
{
  typedef TBase base_t;
  typedef typename base_t::elem_t elem_t;
  typedef AllocateOnDemandMultiThreadedPoolTraits<TElemFactory, base_t> self_t;

#if defined(ALLOCATED_COUNT)
  size_t m_poolSize;
#endif
  AllocateOnDemandMultiThreadedPoolTraits(TElemFactory const & factory, size_t )
    : base_t(factory)
#if defined(ALLOCATED_COUNT)
    , m_poolSize(0)
#endif
  {}

  void AllocateIfNeeded(list<elem_t> & l)
  {
    if (l.empty())
    {
#if defined(ALLOCATED_COUNT)
      m_poolSize += base_t::m_factory.BatchSize();
#endif
      for (unsigned i = 0; i < base_t::m_factory.BatchSize(); ++i)
        l.push_back(base_t::m_factory.Create());
    }
  }

  elem_t const Reserve()
  {
    elem_t res;
    base_t::m_pool.ProcessList([this, &res] (list<elem_t> & l) { AllocateAndReserve(l, res); });
    return res;
  }

  void AllocateAndReserve(list<elem_t> & l, elem_t & res)
  {
    AllocateIfNeeded(l);
    ASSERT ( !l.empty(), () );

    res = l.front();
    l.pop_front();
  }
};

template <typename TElemFactory, typename TBase>
struct AllocateOnDemandSingleThreadedPoolTraits : TBase
{
  typedef TBase base_t;
  typedef typename TBase::elem_t elem_t;
  typedef AllocateOnDemandSingleThreadedPoolTraits<TElemFactory, TBase> self_t;

#if defined(ALLOCATED_COUNT)
  size_t m_poolSize;
#endif

  AllocateOnDemandSingleThreadedPoolTraits(TElemFactory const & factory, size_t )
    : base_t(factory)
#if defined(ALLOCATED_COUNT)
    , m_poolSize(0)
#endif
  {}

  void Init()
  {}

  void AllocateIfNeeded(list<elem_t> & l)
  {
    if (l.empty())
    {
#if defined(ALLOCATED_COUNT)
      m_poolSize += base_t::m_factory.BatchSize();
#endif
      for (unsigned i = 0; i < base_t::m_factory.BatchSize(); ++i)
        l.push_back(base_t::m_factory.Create());
    }
  }

  elem_t const Reserve()
  {
    elem_t res;
    /// allocate resources if needed if we're on the main thread.
    if (threads::GetCurrentThreadID() == base_t::m_MainThreadID)
      base_t::m_pool.ProcessList([this, &res] (list<elem_t> & l) { AllocateAndReserve(l, res); });
    else
      res = base_t::Reserve();
    return res;
  }

  void AllocateAndReserve(list<elem_t> & l, elem_t & res)
  {
    AllocateIfNeeded(l);
    ASSERT ( !l.empty(), () );

    res = l.front();
    l.pop_front();
  }

  void UpdateState()
  {
    base_t::UpdateState();
    base_t::m_pool.ProcessList([this] (list<elem_t> & l) { AllocateIfNeeded(l); });
  }
};

/// resource pool interface
template <typename TElem>
class ResourcePool
{
public:
  virtual ~ResourcePool(){}
  virtual TElem const Reserve() = 0;
  virtual void Free(TElem const & elem) = 0;
  virtual size_t Size() const = 0;
  virtual void EnterForeground() = 0;
  virtual void EnterBackground() = 0;
  virtual void Cancel() = 0;
  virtual bool IsCancelled() const = 0;
  virtual void UpdateState() = 0;
  virtual void SetIsDebugging(bool flag) = 0;
  virtual char const * ResName() const = 0;
};

// This class tracks OpenGL resources allocation in
// a multithreaded environment.
template <typename TPoolTraits>
class ResourcePoolImpl : public ResourcePool<typename TPoolTraits::elem_t>
{
private:

  unique_ptr<TPoolTraits> const m_traits;

public:

  typedef typename TPoolTraits::elem_t elem_t;

  ResourcePoolImpl(TPoolTraits * traits)
    : m_traits(traits)
  {
    /// quick trick to perform lazy initialization
    /// on the same thread the pool was created.
    m_traits->Init();
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

  void UpdateState()
  {
    m_traits->UpdateState();
  }

  void SetIsDebugging(bool isDebugging)
  {
    m_traits->m_IsDebugging = isDebugging;
  }

  char const * ResName() const
  {
    return m_traits->ResName();
  }
};
