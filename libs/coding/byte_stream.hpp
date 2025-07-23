#pragma once

#include "base/base.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

class ArrayByteSource
{
public:
  explicit ArrayByteSource(void const * p) : m_p(static_cast<uint8_t const *>(p)) {}

  uint8_t ReadByte() { return *m_p++; }

  void Read(void * ptr, size_t size)
  {
    memcpy(ptr, m_p, size);
    m_p += size;
  }

  void const * Ptr() const { return m_p; }
  uint8_t const * PtrUint8() const { return m_p; }

  void Advance(size_t size) { m_p += size; }

private:
  uint8_t const * m_p;
};

template <class StorageT>
class PushBackByteSink
{
public:
  explicit PushBackByteSink(StorageT & storage) : m_Storage(storage) {}

  void Write(void const * p, size_t size)
  {
    // assume input buffer as buffer of bytes
    uint8_t const * pp = static_cast<uint8_t const *>(p);
    m_Storage.insert(m_Storage.end(), pp, pp + size);
  }

  size_t Pos() const { return m_Storage.size(); }

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
