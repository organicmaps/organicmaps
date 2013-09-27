#pragma once

#include "../std/memcpy.hpp"

template <typename T>
class ReferencePoiner
{
public:
  ReferencePoiner()
    : m_p(NULL)
  {
  }

  ReferencePoiner(T * p)
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

  bool operator != (const ReferencePoiner<T> & other) const
  {
    return *m_p != *other.m_p;
  }

  bool operator < (const ReferencePoiner<T> & other) const
  {
    return *m_p < *other.m_p;
  }

private:
  T * m_p;
};

template <typename T>
class OwnedPointer
{
public:
  OwnedPointer()
    : m_p(NULL)
  {
  }

  OwnedPointer(T * p)
    : m_p(p)
  {
  }

  ~OwnedPointer()
  {
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

  ReferencePoiner<T> GetWeakPointer()
  {
    return ReferencePoiner<T>(m_p);
  }

  const ReferencePoiner<T> GetWeakPointer() const
  {
    return ReferencePoiner<T>(m_p);
  }

  template <typename U>
  ReferencePoiner<U> GetWeakPointer()
  {
    return ReferencePoiner<U>(m_p);
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

  bool operator != (const OwnedPointer<T> & other) const
  {
    return *m_p != *other.m_p;
  }

  bool operator < (const OwnedPointer<T> & other) const
  {
    return *m_p < *other.m_p;
  }

private:
  T * m_p;
};
