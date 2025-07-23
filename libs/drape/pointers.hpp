#pragma once

#include "drape/drape_diagnostics.hpp"

#include "base/assert.hpp"

#include <memory>

#if defined(TRACK_POINTERS)
#include <map>
#include <mutex>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

// This class tracks usage of drape_ptr's and ref_ptr's
class DpPointerTracker
{
public:
  typedef std::map<void *, std::pair<int, std::string>> TAlivePointers;

  static DpPointerTracker & Instance();

  template <typename T>
  void RefPtr(T * refPtr)
  {
    RefPtrNamed(static_cast<void *>(refPtr), typeid(refPtr).name());
  }

  void DerefPtr(void * p);

  void DestroyPtr(void * p);

  TAlivePointers const & GetAlivePointers() const;

private:
  DpPointerTracker() = default;
  ~DpPointerTracker();

  void RefPtrNamed(void * refPtr, std::string const & name);

  TAlivePointers m_alivePointers;
  std::mutex m_mutex;
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

template <typename T>
using drape_ptr = std::unique_ptr<T, DpPointerDeleter>;
#else
template <typename T>
using drape_ptr = std::unique_ptr<T>;
#endif

template <typename T, typename... Args>
drape_ptr<T> make_unique_dp(Args &&... args)
{
  return drape_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class ref_ptr
{
public:
  ref_ptr() noexcept : m_ptr(nullptr)
  {
#if defined(TRACK_POINTERS)
    m_isOwnerUnique = false;
#endif
  }

#if defined(TRACK_POINTERS)
  ref_ptr(T * ptr, bool isOwnerUnique = false) noexcept
#else
  ref_ptr(T * ptr) noexcept
#endif
    : m_ptr(ptr)
  {
#if defined(TRACK_POINTERS)
    m_isOwnerUnique = isOwnerUnique;
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif
  }

  ref_ptr(ref_ptr const & rhs) noexcept : m_ptr(rhs.m_ptr)
  {
#if defined(TRACK_POINTERS)
    m_isOwnerUnique = rhs.m_isOwnerUnique;
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif
  }

  ref_ptr(ref_ptr && rhs) noexcept
  {
    m_ptr = rhs.m_ptr;
    rhs.m_ptr = nullptr;

#if defined(TRACK_POINTERS)
    m_isOwnerUnique = rhs.m_isOwnerUnique;
    rhs.m_isOwnerUnique = false;
#endif
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

  template <typename TResult>
  operator ref_ptr<TResult>() const
  {
    TResult * res;

    if constexpr (std::is_base_of<TResult, T>::value || std::is_void<TResult>::value)
      res = m_ptr;
    else
    {
      /// @todo I'd prefer separate down_cast<ref_ptr<TResult>> function, but the codebase already relies on it.
      static_assert(std::is_base_of<T, TResult>::value);
      res = static_cast<TResult *>(m_ptr);
      ASSERT_EQUAL(dynamic_cast<TResult *>(m_ptr), res, ("Avoid multiple inheritance"));
    }

#if defined(TRACK_POINTERS)
    return ref_ptr<TResult>(res, m_isOwnerUnique);
#else
    return ref_ptr<TResult>(res);
#endif
  }

  explicit operator bool() const { return m_ptr != nullptr; }

  bool operator==(ref_ptr const & rhs) const { return m_ptr == rhs.m_ptr; }

  bool operator==(T * rhs) const { return m_ptr == rhs; }

  bool operator!=(ref_ptr const & rhs) const { return !operator==(rhs); }

  bool operator!=(T * rhs) const { return !operator==(rhs); }

  bool operator<(ref_ptr const & rhs) const { return m_ptr < rhs.m_ptr; }

  template <typename TResult, typename = std::enable_if_t<!std::is_void<TResult>::value>>
  TResult & operator*() const
  {
    return *m_ptr;
  }

  ref_ptr & operator=(ref_ptr const & rhs) noexcept
  {
    if (this == &rhs)
      return *this;

#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().DerefPtr(m_ptr);
#endif

    m_ptr = rhs.m_ptr;

#if defined(TRACK_POINTERS)
    m_isOwnerUnique = rhs.m_isOwnerUnique;
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().RefPtr(m_ptr);
#endif

    return *this;
  }

  ref_ptr & operator=(ref_ptr && rhs) noexcept
  {
    if (this == &rhs)
      return *this;

#if defined(TRACK_POINTERS)
    if (m_isOwnerUnique)
      DpPointerTracker::Instance().DerefPtr(m_ptr);
#endif

    m_ptr = rhs.m_ptr;
    rhs.m_ptr = nullptr;

#if defined(TRACK_POINTERS)
    m_isOwnerUnique = rhs.m_isOwnerUnique;
    rhs.m_isOwnerUnique = false;
#endif

    return *this;
  }

  T * get() const { return m_ptr; }

private:
  T * m_ptr;
#if defined(TRACK_POINTERS)
  bool m_isOwnerUnique;
#endif

  template <typename TResult>
  friend inline std::string DebugPrint(ref_ptr<TResult> const & v);
};

template <typename T>
inline std::string DebugPrint(ref_ptr<T> const & v)
{
  return DebugPrint(v.m_ptr);
}

template <typename T>
ref_ptr<T> make_ref(drape_ptr<T> const & drapePtr)
{
#if defined(TRACK_POINTERS)
  return ref_ptr<T>(drapePtr.get(), true /* isOwnerUnique */);
#else
  return ref_ptr<T>(drapePtr.get());
#endif
}

template <typename T>
ref_ptr<T> make_ref(T * ptr)
{
#if defined(TRACK_POINTERS)
  return ref_ptr<T>(ptr, false /* isOwnerUnique */);
#else
  return ref_ptr<T>(ptr);
#endif
}
