#pragma once

#include "base/assert.hpp"
#include "base/mutex.hpp"

#include "std/map.hpp"
#include "std/typeinfo.hpp"

namespace dp
{

class PointerTracker
{
public:
  ~PointerTracker();

  template <typename T>
  void Ref(T * p, bool needDestroyCheck)
  {
    threads::MutexGuard g(m_mutex);
    if (p == nullptr)
      return;

    map_t::iterator it = m_countMap.find(p);
    if (it == m_countMap.end())
    {
      m_countMap.insert(make_pair((void *)p, make_pair(1, typeid(p).name())));
      if (needDestroyCheck)
        m_alivePointers.insert(p);
    }
    else
      it->second.first++;
  }

  void Deref(void * p);
  void Destroy(void * p);

private:
  typedef map<void *, pair<int, string> > map_t;
  map_t m_countMap;
  typedef set<void *> alive_pointers_t;
  alive_pointers_t m_alivePointers;
  threads::Mutex m_mutex;
};

#define DISABLE_DEBUG_PRT_TRACKING

#if defined(DEBUG) && !defined(DISABLE_DEBUG_PRT_TRACKING)
  #define CHECK_POINTERS
#endif

#if defined(CHECK_POINTERS)
  extern PointerTracker g_tracker;

  #define REF_POINTER(p, c) g_tracker.Ref(p, c)
  #define DEREF_POINTER(p) g_tracker.Deref(p)
  #define DESTROY_POINTER(p) g_tracker.Destroy(p)

  #define DECLARE_CHECK bool m_checkOnDestroy
  #define DECLARE_CHECK_GET bool IsCheckOnDestroy() const { return m_checkOnDestroy; }
  #define DECLARE_CHECK_SET void SetCheckOnDestroy(bool doCheck) { m_checkOnDestroy = doCheck; }
  #define SET_CHECK_FLAG(x) SetCheckOnDestroy(x)
  #define GET_CHECK_FLAG(x) (x).IsCheckOnDestroy()
  #define ASSERT_CHECK_FLAG(x) ASSERT(GET_CHECK_FLAG(x), ())
#else
  #define REF_POINTER(p, c)
  #define DEREF_POINTER(p)
  #define DESTROY_POINTER(p)

  #define DECLARE_CHECK
  #define DECLARE_CHECK_GET
  #define DECLARE_CHECK_SET
  #define SET_CHECK_FLAG(x)
  #define GET_CHECK_FLAG(x) false
  #define ASSERT_CHECK_FLAG(x)
#endif

template <typename T>
class DrapePointer
{
public:
  DrapePointer() : m_p(nullptr) { SET_CHECK_FLAG(true); }

  bool operator==(DrapePointer<T> const & other) const
  {
    return m_p == other.m_p;
  }

protected:
  DrapePointer(T * p, bool needDestroyedCheck = true)
    : m_p(p)
  {
    SET_CHECK_FLAG(needDestroyedCheck);
    REF_POINTER(m_p, GET_CHECK_FLAG(*this));
  }

  DrapePointer(DrapePointer<T> const & other) : m_p(nullptr)
  {
    SET_CHECK_FLAG(GET_CHECK_FLAG(other));
    Reset(other.GetNonConstRaw());
  }

  DrapePointer<T> & operator=(DrapePointer<T> const & other)
  {
    SET_CHECK_FLAG(GET_CHECK_FLAG(other));
    Reset(other.GetNonConstRaw());
    return *this;
  }

  void Destroy()
  {
    DESTROY_POINTER(m_p);
    delete m_p;
    DEREF_POINTER(m_p);
    m_p = nullptr;
  }

  void Reset(T * p)
  {
    ResetImpl(p);
  }

  T * GetRaw()                { return m_p; }
  T const * GetRaw() const    { return m_p; }
  T * GetNonConstRaw() const  { return m_p; }

  DECLARE_CHECK_GET;
  DECLARE_CHECK_SET;

  // Need to be const for copy constructor and assigment operator of TransfromPointer
  void SetToNull() const
  {
    ResetImpl(nullptr);
  }

private:
  void ResetImpl(T * p) const
  {
    DEREF_POINTER(m_p);
    m_p = p;
    REF_POINTER(m_p, GET_CHECK_FLAG(*this));
  }

private:
  // Mutable for Move method
  mutable T * m_p;
  DECLARE_CHECK;
};

template <typename T> class MasterPointer;

template <typename T>
class TransferPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> base_t;
public:
  TransferPointer(TransferPointer<T> const & other)
    : base_t(other)
  {
    ASSERT_CHECK_FLAG(other);
    other.SetToNull();
  }

  TransferPointer<T> & operator=(TransferPointer<T> const & other)
  {
    ASSERT_CHECK_FLAG(other);
    base_t::operator =(other);
    other.SetToNull();
    return *this;
  }

  ~TransferPointer()
  {
    ASSERT(base_t::GetRaw() == nullptr, ());
    Destroy();
  }
  void Destroy() { base_t::Destroy(); }
  // IsNull need for test
  bool IsNull() { return base_t::GetRaw() == nullptr; }

private:
  friend class MasterPointer<T>;
  TransferPointer() {}
  explicit TransferPointer(T * p) : base_t(p) {}
};

template <typename T> class RefPointer;
template <typename T> RefPointer<T> MakeStackRefPointer(T * p);

template<typename T>
class RefPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> base_t;
public:
  RefPointer() : base_t() {}
  ~RefPointer() { base_t::Reset(nullptr); }

  template <typename Y>
  RefPointer(RefPointer<Y> const & p) : base_t(p.GetNonConstRaw(), GET_CHECK_FLAG(p)) {}

  bool IsContentLess(RefPointer<T> const & other) const { return *GetRaw() < *other.GetRaw(); }
  bool IsNull() const { return base_t::GetRaw() == nullptr; }
  T * operator->()             { return base_t::GetRaw(); }
  T const * operator->() const { return base_t::GetRaw(); }
  T * GetRaw()                 { return base_t::GetRaw(); }
  T const * GetRaw() const     { return base_t::GetRaw(); }

private:
  template <typename Y> friend class RefPointer;
  friend class MasterPointer<T>;
  friend RefPointer<T> MakeStackRefPointer<T>(T *);
  explicit RefPointer(T * p, bool needDestroyedCheck = true) : base_t(p, needDestroyedCheck) {}
};

template <typename T>
RefPointer<T> MakeStackRefPointer(T * p) { return RefPointer<T>(p, false); }

template <typename T>
RefPointer<void> StackVoidRef(T * p)
{
  return MakeStackRefPointer<void>(p);
}

template <typename T>
class MasterPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> TBase;

public:
  MasterPointer() : TBase() {}
  explicit MasterPointer(T * p) : TBase(p) {}
  explicit MasterPointer(TransferPointer<T> & transferPointer)
  {
    Reset(transferPointer.GetRaw());
    transferPointer.Reset(nullptr);
  }

  explicit MasterPointer(TransferPointer<T> && transferPointer)
  {
    Reset(transferPointer.GetRaw());
    transferPointer.Reset(nullptr);
  }

  ~MasterPointer()
  {
    TBase::Reset(nullptr);
  }

  RefPointer<T> GetRefPointer() const
  {
    return RefPointer<T>(TBase::GetNonConstRaw());
  }

  TransferPointer<T> Move()
  {
    TransferPointer<T> result(GetRaw());
    TBase::Reset(nullptr);
    return result;
  }

  void Destroy()
  {
    Reset(nullptr);
  }

  void Reset(T * p)
  {
    TBase::Destroy();
    TBase::Reset(p);
  }

  T * Release()
  {
    T * result = GetRaw();
    TBase::Reset(nullptr);
    return result;
  }

  bool IsNull() const { return TBase::GetRaw() == nullptr; }
  T * operator->() { return TBase::GetRaw(); }
  T const * operator->() const { return TBase::GetRaw(); }
  T * GetRaw() { return TBase::GetRaw(); }
  T const * GetRaw() const { return TBase::GetRaw(); }
};

template<typename T>
TransferPointer<T> MovePointer(T * p)
{
  return MasterPointer<T>(p).Move();
}

template <typename T>
T * NonConstGetter(dp::MasterPointer<T> & p)
{
  return p.GetRaw();
}

struct MasterPointerDeleter
{
  template <typename Y, typename T>
  void operator() (pair<Y, MasterPointer<T> > & value)
  {
    Destroy(value.second);
  }

  template <typename T>
  void operator() (MasterPointer<T> & value)
  {
    Destroy(value);
  }

private:
  template <typename T>
  void Destroy(MasterPointer<T> & value)
  {
    value.Destroy();
  }
};

} // namespace dp
