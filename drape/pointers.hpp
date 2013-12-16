#pragma once

#include "../base/assert.hpp"

#ifdef DEBUG
  #include "../std/map.hpp"

  template <typename T>
  class PointerTracker
  {
  public:
    ~PointerTracker()
    {
      ASSERT(m_countMap.empty(), ());
    }

    void Ref(void * p)
    {
      if (p == NULL)
        return;

      ++m_countMap[p];
    }

    void Deref(void * p)
    {
      if (p == NULL)
        return;

      map_t::iterator it = m_countMap.find(p);
      ASSERT(it != m_countMap.end(), ());
      ASSERT(it->second > 0, ());

      if (--it->second == 0)
        m_countMap.erase(it);
    }

    void Destroy(void * p)
    {
      if (p == NULL)
        return;

      map_t::iterator it = m_countMap.find(p);
      ASSERT(it != m_countMap.end(), ());
      ASSERT(it->second == 1, ());
    }

  private:
    typedef map<void *, int> map_t;
    map_t m_countMap;
  };

  #define DECLARE_TRACKER \
    static PointerTracker<T> m_tracker
  #define IMPLEMENT_TRACKER template<typename T> PointerTracker<T> DrapePointer<T>::m_tracker
  #define REF_POINTER(p) m_tracker.Ref(p)
  #define DEREF_POINTER(p) m_tracker.Deref(p)
  #define DESTROY_POINTER(p) m_tracker.Destroy(p)
#else
  #define DECLARE_TRACKER
  #define REF_POINTER(p)
  #define DEREF_POINTER(p)
  #define DESTROY_POINTER(p)
  #define IMPLEMENT_TRACKER
#endif

template <typename T>
class DrapePointer
{
public:
  DrapePointer()      : m_p(NULL) {}

protected:
  DrapePointer(T * p) : m_p(p)
  {
    REF_POINTER(m_p);
  }

  DrapePointer(DrapePointer<T> const & other)
  {
    m_p = other.m_p;
    REF_POINTER(m_p);
  }

  void Destroy()
  {
    DESTROY_POINTER(m_p);
    delete m_p;
    DEREF_POINTER(m_p);
    m_p = NULL;
  }

  void Reset(T * p)
  {
    DEREF_POINTER(m_p);
    m_p = p;
    REF_POINTER(m_p);
  }

  T * GetRaw()                { return m_p; }
  T const * GetRaw() const    { return m_p; }
  T * GetNonConstRaw() const  { return m_p; }

private:
  DrapePointer<T> & operator = (DrapePointer<T> const & /*other*/) { ASSERT(false, ()); return *this; }

private:
  T * m_p;
  DECLARE_TRACKER;
};

IMPLEMENT_TRACKER;

template <typename T> class MasterPointer;

template <typename T>
class TransferPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> base_t;
public:
  TransferPointer(const TransferPointer<T> & other) : base_t(other) {}

  TransferPointer<T> & operator= (TransferPointer<T> const & other)
  {
    base_t::Reset(other.GetNonConstRaw());
    return *this;
  }

  void Destroy() { base_t::Destroy(); }

private:
  friend class MasterPointer<T>;
  TransferPointer() {}
  TransferPointer(T * p) : base_t(p) {}
};

template <typename T> class RefPointer;
template <typename T> RefPointer<T> MakeStackRefPointer(T * p);

template<typename T>
class RefPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> base_t;
public:
  RefPointer() : base_t() {}
  RefPointer(const RefPointer<T> & p) : base_t(p.GetNonConstRaw()) {}

  template <typename Y>
  RefPointer(const RefPointer<Y> & p) : base_t(p.GetNonConstRaw()) {}
  ~RefPointer() { base_t::Reset(NULL); }

  RefPointer & operator = (const RefPointer<T> & other)
  {
    base_t::Reset(other.GetNonConstRaw());
    return *this;
  }

  bool IsContentLess(RefPointer<T> const & other) const
  {
    return *GetRaw() < *other.GetRaw();
  }

  bool IsNull() const          { return base_t::GetRaw() == NULL; }
  T * operator->()             { return base_t::GetRaw(); }
  T const * operator->() const { return base_t::GetRaw(); }
  T * GetRaw()                 { return base_t::GetRaw(); }
  T const * GetRaw() const     { return base_t::GetRaw(); }

private:
  template <typename Y> friend class RefPointer;
  friend class MasterPointer<T>;
  friend RefPointer<T> MakeStackRefPointer<T>(T *);
  RefPointer(T * p) : base_t(p) {}
};

template <typename T>
RefPointer<T> MakeStackRefPointer(T * p)
{
  return RefPointer<T>(p);
}

template <typename T>
class MasterPointer : public DrapePointer<T>
{
  typedef DrapePointer<T> base_t;
public:
  MasterPointer() : base_t() {}
  MasterPointer(T * p) : base_t(p) {}
  MasterPointer(const MasterPointer<T> & other)
  {
    ASSERT(GetRaw() == NULL, ());
    base_t::Reset(other.GetNonConstRaw());
  }

  MasterPointer(TransferPointer<T> & transferPointer)
  {
    Reset(transferPointer.GetRaw());
    transferPointer.Reset(NULL);
  }

  ~MasterPointer()
  {
    base_t::Reset(NULL);
  }

  MasterPointer<T> & operator= (const MasterPointer<T> & other)
  {
    ASSERT(GetRaw() == NULL, ());
    base_t::Reset(other.GetNonConstRaw());
    return *this;
  }

  RefPointer<T> GetRefPointer() const
  {
    return RefPointer<T>(base_t::GetNonConstRaw());
  }

  TransferPointer<T> Move()
  {
    TransferPointer<T> result(GetRaw());
    base_t::Reset(NULL);
    return result;
  }

  void Destroy()
  {
    Reset(NULL);
  }

  void Reset(T * p)
  {
    base_t::Destroy();
    base_t::Reset(p);
  }

  bool IsNull() const          { return base_t::GetRaw() == NULL; }
  T * operator->()             { return base_t::GetRaw(); }
  T const * operator->() const { return base_t::GetRaw(); }
  T * GetRaw()                 { return base_t::GetRaw(); }
  T const * GetRaw() const     { return base_t::GetRaw(); }
};

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
