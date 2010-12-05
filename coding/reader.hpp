#pragma once
#include "endianness.hpp"
#include "source.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"
#include "../base/macros.hpp"
#include "../std/string.hpp"
#include "../std/memcpy.hpp"

// Base class for random-access Reader. Not thread-safe.
class Reader
{
public:
  DECLARE_EXCEPTION(Exception, RootException);
  DECLARE_EXCEPTION(OpenException, Exception);
  DECLARE_EXCEPTION(ReadException, Exception);

  virtual ~Reader() {}
  virtual uint64_t Size() const = 0;
  virtual void Read(uint64_t pos, void * p, size_t size) const = 0;
  virtual Reader * CreateSubReader(uint64_t pos, uint64_t size) const = 0;
};

// Reader from memory.
class MemReader : public Reader
{
public:
  MemReader() {} // TODO: Remove MemReader().

  // Construct from block of memory.
  MemReader(void const * pData, size_t size)
    : m_pData(static_cast<char const *>(pData)), m_Size(size)
  {
  }

  inline uint64_t Size() const
  {
    return m_Size;
  }

  inline void Read(uint64_t pos, void * p, size_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    memcpy(p, m_pData + pos, size);
  }

  inline MemReader SubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    return MemReader(m_pData + pos, static_cast<size_t>(size));
  }

  inline MemReader * CreateSubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    return new MemReader(m_pData + pos, static_cast<size_t>(size));
  }

private:
  char const * m_pData;
  size_t m_Size;
};

// Source that readers from a reader.
template <typename ReaderT>
class ReaderSource
{
public:
  typedef ReaderT ReaderType;

  ReaderSource(ReaderT const & reader) : m_reader(reader), m_pos(0)
  {
  }

  void Read(void * p, size_t size)
  {
    if (m_pos + size > m_reader.Size())
    {
      size_t remainingSize = static_cast<size_t>(m_reader.Size() - m_pos);
      m_reader.Read(m_pos, p, remainingSize);
      m_pos = m_reader.Size();
      MYTHROW1(SourceOutOfBoundsException, remainingSize, ());
    }
    else
    {
      m_reader.Read(m_pos, p, size);
      m_pos += size;
    }
  }

  void Skip(uint64_t size)
  {
    m_pos += size;
  }

  uint64_t Pos() const
  {
    return m_pos;
  }

  ReaderT const & Reader() const
  {
    return m_reader;
  }

  ReaderT SubReader(uint64_t size)
  {
    uint64_t pos = m_pos;
    m_pos += size;
    return m_reader.SubReader(pos, size);
  }

  ReaderT SubReader()
  {
    return SubReader(m_reader.Size() - m_pos);
  }

private:
  ReaderT m_reader;
  uint64_t m_pos;
};

template <class ReaderT> inline
void ReadFromPos(ReaderT const & reader, uint64_t pos, void * p, size_t size)
{
  reader.Read(pos, p, size);
}

template <typename PrimitiveT, class ReaderT> inline
PrimitiveT ReadPrimitiveFromPos(ReaderT const & reader, uint64_t pos)
{
  PrimitiveT primitive;
  ReadFromPos(reader, pos, &primitive, sizeof(primitive));
  return SwapIfBigEndian(primitive);
}

template <typename PrimitiveT, class TSource> inline
PrimitiveT ReadPrimitiveFromSource(TSource & source)
{
  PrimitiveT primitive;
  source.Read(&primitive, sizeof(primitive));
  return SwapIfBigEndian(primitive);
}
