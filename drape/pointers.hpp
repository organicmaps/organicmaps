#pragma once

#include "base/assert.hpp"
#include "base/mutex.hpp"

#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/typeinfo.hpp"

//#define TRACK_POINTERS

/// This class tracks usage of drape_ptr's and ref_ptr's
class DpPointerTracker
{
public:
  static DpPointerTracker & Instance();

  template <typename T>
  void RefPtr(T * refPtr)
  {
    RefPtrNamed(static_cast<void*>(refPtr), typeid(refPtr).name());
  }

  void DerefPtr(void * p);

  void DestroyPtr(void * p);

private:
  DpPointerTracker() = default;
  ~DpPointerTracker();

  void RefPtrNamed(void * refPtr, string const & name);

  typedef map<void *, pair<int, string> > TAlivePointers;
  TAlivePointers m_alivePointers;
  mutex m_mutex;
};

// Custom deleter for unique_ptr
class DpPointerDeleter
{
public:
  template <typename T>
  void operator()(T * p)
  {
    DpPointerTracker::Instance().DestroyPtr(p);
    delete p;
  }
};

#if defined(TRACK_POINTERS)
template<typename T> using drape_ptr = unique_ptr<T, DpPointerDeleter>;
#else
template<typename T> using drape_ptr = unique_ptr<T>;
#endif

template <typename T, typename... Args>
drape_ptr<T> make_unique_dp(Args &&... args)
{
  return drape_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
class ref_ptr
{
public:
  ref_ptr()
    : m_ptr(nullptr), m_isOwnerUnique(false)
  {}

  ref_ptr(T * ptr, bool isOwnerUnique = false)
    : m_ptr(ptr), m_isOwnerUnique(isOwnerUnique)
  {
#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif
  }

  ref_ptr(ref_ptr const & rhs)
    : m_ptr(rhs.m_ptr), m_isOwnerUnique(rhs.m_isOwnerUnique)
  {
#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif
  }

  ref_ptr(ref_ptr && rhs)
  {
    m_ptr = rhs.m_ptr;
    rhs.m_ptr = nullptr;

    m_isOwnerUnique = rhs.m_isOwnerUnique;
    rhs.m_isOwnerUnique = false;
  }

  ~ref_ptr()
  {
#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().DerefPtr(m_ptr);
#endif
    m_ptr = nullptr;
  }

  T * operator->() const { return m_ptr; }

  template<typename TResult>
  operator ref_ptr<TResult>() const
  {
    STATIC_ASSERT(is_base_of<TResult, T>::value || is_base_of<T, TResult>::value ||
                  is_void<T>::value || is_void<TResult>::value);
    return ref_ptr<TResult>(static_cast<TResult *>(m_ptr), m_isOwnerUnique);
  }

  operator bool() const { return m_ptr != nullptr; }

  bool operator==(ref_ptr const & rhs) const { return m_ptr == rhs.m_ptr; }

  bool operator==(T * rhs) const { return m_ptr == rhs; }

  bool operator!=(ref_ptr const & rhs) const { return !operator==(rhs); }

  bool operator!=(T * rhs) const { return !operator==(rhs); }

  bool operator<(ref_ptr const & rhs) const { return m_ptr < rhs.m_ptr; }

  template<typename TResult, class = typename enable_if<!is_void<TResult>::value>::type>
  TResult & operator*() const
  {
    return *m_ptr;
  }

  ref_ptr & operator=(ref_ptr const & rhs)
  {
    if (this == &rhs)
      return *this;

#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().DerefPtr(m_ptr);
#endif

    m_ptr = rhs.m_ptr;
    m_isOwnerUnique = rhs.m_isOwnerUnique;

#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif

    return *this;
  }

  ref_ptr & operator=(ref_ptr && rhs)
  {
    if (this == &rhs)
      return *this;

#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().DerefPtr(m_ptr);
#endif

    m_ptr = rhs.m_ptr;
    rhs.m_ptr = nullptr;

    m_isOwnerUnique = rhs.m_isOwnerUnique;
    rhs.m_isOwnerUnique = false;

    return *this;
  }

  T * get() const { return m_ptr; }

private:
  T* m_ptr;
  bool m_isOwnerUnique;

  template <typename TResult>
  friend inline string DebugPrint(ref_ptr<TResult> const & v);
};

template <typename T>
inline string DebugPrint(ref_ptr<T> const & v)
{
  return DebugPrint(v.m_ptr);
}

template <typename T>
ref_ptr<T> make_ref(drape_ptr<T> const & drapePtr)
{
  return ref_ptr<T>(drapePtr.get(), true);
}

template <typename T>
ref_ptr<T> make_ref(T* ptr)
{
  return ref_ptr<T>(ptr, false);
}
