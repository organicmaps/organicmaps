#pragma once
#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/memcpy.hpp"


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

  inline const void * Ptr() const { return m_p; }
  inline const unsigned char * PtrUC() const { return m_p; }
  inline const char * PtrC() const { return static_cast<char const *>(Ptr()); }

  void Advance(size_t size)
  {
    m_p += size;
  }

private:
  const unsigned char * m_p;
};

template <class StorageT> class PushBackByteSink
{
public:
  explicit PushBackByteSink(StorageT & storage)
    : m_Storage(storage)//, m_InitialStorageSize(storage.size())
  {
  }

  void Write(void const * p, size_t size)
  {
    // assume input buffer as buffer of bytes
    unsigned char const * pp = static_cast<unsigned char const *>(p);
    m_Storage.append(pp, pp + size);
  }

  size_t Pos() const
  {
    return m_Storage.size();
  }
private:
  StorageT & m_Storage;
};

template <typename T> class PushBackByteSink<vector<T> >
{
public:
  explicit PushBackByteSink(vector<T> & storage)
    : m_Storage(storage)//, m_InitialStorageSize(storage.size())
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
  vector<T> & m_Storage;
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
