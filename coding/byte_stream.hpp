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

  const void * Ptr() const {
    return m_p;
  }

  void Advance(size_t size)
  {
    m_p += size;
  }
private:
  const unsigned char * m_p;
};

// TODO: Separate sink and rich write interface.
template <class TStorage = vector<unsigned char> > class PushBackByteSink
{
public:
  explicit PushBackByteSink(TStorage & storage)
    : m_Storage(storage), m_InitialStorageSize(storage.size())
  {
  }

  void Write(void const * p, size_t size)
  {
    m_Storage.insert(m_Storage.end(),
                     static_cast<unsigned char const *>(p),
                     static_cast<unsigned char const *>(p) + size);
  }

  size_t BytesWritten() const
  {
    return m_Storage.size() - m_InitialStorageSize;
  }
private:
  TStorage & m_Storage;
  size_t m_InitialStorageSize;
};
