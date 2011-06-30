#pragma once
#include "endianness.hpp"
#include "source.hpp"

#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"
#include "../base/macros.hpp"

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

  inline bool IsEqual(string const & name) const
  {
    return m_p->IsEqual(name);
  }

  inline bool IsEqual(ModelReaderPtr const & file) const
  {
    return m_p->IsEqual(file.m_p->GetName());
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
