#pragma once
#include "endianness.hpp"
#include "source.hpp"

#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"
#include "../base/macros.hpp"

#include "../std/shared_array.hpp"
#include "../std/shared_ptr.hpp"
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

  void ReadAsString(string & s) const;

  static bool IsEqual(string const & name1, string const & name2);
};

// Reader from memory.
class MemReader : public Reader
{
public:
  // Construct from block of memory.
  MemReader(void const * pData, size_t size)
    : m_pData(static_cast<char const *>(pData)), m_size(size)
  {
  }

  inline uint64_t Size() const
  {
    return m_size;
  }

  inline void Read(uint64_t pos, void * p, size_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    memcpy(p, m_pData + pos, size);
  }

  inline MemReader SubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    ASSERT_LESS_OR_EQUAL(size, static_cast<size_t>(-1), ());
    return MemReader(m_pData + pos, static_cast<size_t>(size));
  }

  inline MemReader * CreateSubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    ASSERT_LESS_OR_EQUAL(size, static_cast<size_t>(-1), ());
    return new MemReader(m_pData + pos, static_cast<size_t>(size));
  }

private:
  char const * m_pData;
  size_t m_size;
};

class SharedMemReader
{
public:
  explicit SharedMemReader(size_t size) : m_data(new char[size]), m_offset(0), m_size(size) {}

  inline char * Data() { return m_data.get() + m_offset; }
  inline char const * Data() const { return m_data.get() + m_offset; }
  inline uint64_t Size() const { return m_size; }

  inline void Read(uint64_t pos, void * p, size_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    memcpy(p, Data() + pos, size);
  }

  inline SharedMemReader SubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    ASSERT_LESS_OR_EQUAL(size, static_cast<size_t>(-1), ());
    return SharedMemReader(m_data, static_cast<size_t>(pos), static_cast<size_t>(size));
  }

  inline SharedMemReader * CreateSubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
    ASSERT_LESS_OR_EQUAL(size, static_cast<size_t>(-1), ());
    return new SharedMemReader(m_data, static_cast<size_t>(pos), static_cast<size_t>(size));
  }

private:
  SharedMemReader(shared_array<char> const & data, size_t offset, size_t size)
    : m_data(data), m_offset(offset), m_size(size) {}

  shared_array<char> m_data;
  size_t m_offset;
  size_t m_size;
};

// Reader wrapper to hold the pointer to a polymorfic reader.
// Common use: ReaderSource<ReaderPtr<Reader> >.
// Note! It takes the ownership of Reader.
template <class ReaderT> class ReaderPtr
{
protected:
  shared_ptr<ReaderT> m_p;

public:
  ReaderPtr(ReaderT * p) : m_p(p) {}

  uint64_t Size() const
  {
    return m_p->Size();
  }

  void Read(uint64_t pos, void * p, size_t size) const
  {
    m_p->Read(pos, p, size);
  }

  void ReadAsString(string & s) const
  {
    m_p->ReadAsString(s);
  }
};

// Model reader store file id as string.
class ModelReader : public Reader
{
  string m_name;

public:
  ModelReader(string const & name) : m_name(name) {}

  virtual ModelReader * CreateSubReader(uint64_t pos, uint64_t size) const = 0;

  bool IsEqual(string const & name) const;

  string GetName() const { return m_name; }
};

// Reader pointer class for data files.
class ModelReaderPtr : public ReaderPtr<ModelReader>
{
  typedef ReaderPtr<ModelReader> base_type;

public:
  ModelReaderPtr(ModelReader * p) : base_type(p) {}

  inline ModelReaderPtr SubReader(uint64_t pos, uint64_t size) const
  {
    return m_p->CreateSubReader(pos, size);
  }
};


// Source that reads from a reader.
template <typename ReaderT> class ReaderSource
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
