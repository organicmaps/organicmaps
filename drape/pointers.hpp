#pragma once

#include <cassert>

template <typename T>
class WeakPointer
{
public:
  WeakPointer()
    : m_p(NULL)
  {
  }

  WeakPointer(T * p)
    : m_p(p)
  {
  }

  bool IsNull() const
  {
    return m_p == NULL;
  }

  T * operator ->()
  {
    return m_p;
  }

  const T * operator ->() const
  {
    return m_p;
  }

  T * GetRaw()
  {
    return m_p;
  }

  const T * GetRaw() const
  {
    return m_p;
  }

  bool operator != (const WeakPointer<T> & other) const
  {
    return *m_p != *other.m_p;
  }

  bool operator < (const WeakPointer<T> & other) const
  {
    return *m_p < *other.m_p;
  }

private:
  T * m_p;
};

template <typename T>
class StrongPointer
{
public:
  StrongPointer()
    : m_p(NULL)
  {

  }

  StrongPointer(T * p)
    : m_p(p)
  {
  }

  ~StrongPointer()
  {
    //assert(m_p == NULL);
  }

  void Reset(T * p)
  {
    Destroy();
    m_p = p;
  }

  void Destroy()
  {
    delete m_p;
    m_p = NULL;
  }

  WeakPointer<T> GetWeakPointer()
  {
    return WeakPointer<T>(m_p);
  }

  const WeakPointer<T> GetWeakPointer() const
  {
    return WeakPointer<T>(m_p);
  }

  template <typename U>
  WeakPointer<U> GetWeakPointer()
  {
    return WeakPointer<U>(m_p);
  }

  bool IsNull()
  {
    return m_p == NULL;
  }

  T * operator ->()
  {
    return m_p;
  }

  const T * operator ->() const
  {
    return m_p;
  }

  T * GetRaw()
  {
    return m_p;
  }

  const T * GetRaw() const
  {
    return m_p;
  }

  bool operator != (const StrongPointer<T> & other) const
  {
    return *m_p != *other.m_p;
  }

  bool operator < (const StrongPointer<T> & other) const
  {
    return *m_p < *other.m_p;
  }

private:
  T * m_p;
};
