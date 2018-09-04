#pragma once

#include "base/base.hpp"

#include <cstddef>
#include <cstring>

class ArrayByteSource
{
public:
  explicit ArrayByteSource(const void * p) : m_p(static_cast<const unsigned char *>(p)) {}

  unsigned char ReadByte()
  {
    return *m_p++;
  }

  void Read(void * ptr, size_t size)
  {
    memcpy(ptr, m_p, size);
    m_p += size;
  }

  inline void const * Ptr() const { return m_p; }
  inline unsigned char const * PtrUC() const { return m_p; }
  inline char const * PtrC() const { return static_cast<char const *>(Ptr()); }

  void Advance(size_t size)
  {
    m_p += size;
  }

private:
  unsigned char const * m_p;
};

template <class StorageT> class PushBackByteSink
{
public:
  explicit PushBackByteSink(StorageT & storage)
    : m_Storage(storage)
  {
  }

  void Write(void const * p, size_t size)
  {
    // assume input buffer as buffer of bytes
    unsigned char const * pp = static_cast<unsigned char const *>(p);
    m_Storage.insert(m_Storage.end(), pp, pp + size);
  }

  size_t Pos() const
  {
    return m_Storage.size();
  }
private:
  StorageT & m_Storage;
};

class CountingSink
{
public:
  CountingSink() : m_Count(0) {}
  inline void Write(void const *, size_t size) { m_Count += size; }
  inline size_t GetCount() const { return m_Count; }
private:
  size_t m_Count;
};
