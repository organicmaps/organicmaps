#pragma once
#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include "std/cstring.hpp"
#include "std/shared_array.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

// Base class for random-access Reader. Not thread-safe.
class Reader
{
public:
  DECLARE_EXCEPTION(Exception, RootException);
  DECLARE_EXCEPTION(OpenException, Exception);
  DECLARE_EXCEPTION(SizeException, Exception);
  DECLARE_EXCEPTION(ReadException, Exception);
  DECLARE_EXCEPTION(TooManyFilesException, Exception);

  virtual ~Reader() {}
  virtual uint64_t Size() const = 0;
  virtual void Read(uint64_t pos, void * p, size_t size) const = 0;
  virtual unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const = 0;

  void ReadAsString(string & s) const;

  static bool IsEqual(string const & name1, string const & name2);
};

// Reader from memory.
class MemReader : public Reader
{
  bool AssertPosAndSize(uint64_t pos, uint64_t size) const;

public:
  // Construct from block of memory.
  MemReader(void const * pData, size_t size)
    : m_pData(static_cast<char const *>(pData)), m_size(size)
  {
  }

  inline uint64_t Size() const override
  {
    return m_size;
  }

  inline void Read(uint64_t pos, void * p, size_t size) const override
  {
    ASSERT ( AssertPosAndSize(pos, size), () );
    memcpy(p, m_pData + pos, size);
  }

  inline MemReader SubReader(uint64_t pos, uint64_t size) const
  {
    ASSERT ( AssertPosAndSize(pos, size), () );
    return MemReader(m_pData + pos, static_cast<size_t>(size));
  }

  inline unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override
  {
    ASSERT ( AssertPosAndSize(pos, size), () );
    return make_unique<MemReader>(m_pData + pos, static_cast<size_t>(size));
  }

private:
  char const * m_pData;
  size_t m_size;
};

// Reader wrapper to hold the pointer to a polymorfic reader.
// Common use: ReaderSource<ReaderPtr<Reader> >.
// Note! It takes the ownership of Reader.
template <class TReader>
class ReaderPtr
{
protected:
  shared_ptr<TReader> m_p;

public:
  template <typename TReaderDerived>
  ReaderPtr(unique_ptr<TReaderDerived> p) : m_p(move(p)) {}

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

  TReader * GetPtr() const { return m_p.get(); }
};

// Model reader store file id as string.
class ModelReader : public Reader
{
  string m_name;

public:
  ModelReader(string const & name) : m_name(name) {}

  virtual unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override = 0;

  inline string const & GetName() const { return m_name; }
};

// Reader pointer class for data files.
class ModelReaderPtr : public ReaderPtr<ModelReader>
{
  using TBase = ReaderPtr<ModelReader>;

public:
  template <typename TReaderDerived>
  ModelReaderPtr(unique_ptr<TReaderDerived> p) : TBase(move(p)) {}

  inline ModelReaderPtr SubReader(uint64_t pos, uint64_t size) const
  {
    return unique_ptr<ModelReader>(static_cast<ModelReader *>(m_p->CreateSubReader(pos, size).release()));
  }

  inline string const & GetName() const { return m_p->GetName(); }
};

// Source that reads from a reader.
template <typename TReader>
class ReaderSource
{
public:
  using ReaderType = TReader;

  ReaderSource(TReader const & reader) : m_reader(reader), m_pos(0) {}

  void Read(void * p, size_t size)
  {
    m_reader.Read(m_pos, p, size);
    m_pos += size;
  }

  void Skip(uint64_t size)
  {
    m_pos += size;
    ASSERT ( AssertPosition(), () );
  }

  uint64_t Pos() const
  {
    return m_pos;
  }

  uint64_t Size() const
  {
    ASSERT ( AssertPosition(), () );
    return (m_reader.Size() - m_pos);
  }

  TReader SubReader(uint64_t size)
  {
    uint64_t const pos = m_pos;
    Skip(size);
    return m_reader.SubReader(pos, size);
  }

  TReader SubReader() { return SubReader(Size()); }

private:
  bool AssertPosition() const
  {
    bool const ret = (m_pos <= m_reader.Size());
    ASSERT ( ret, (m_pos, m_reader.Size()) );
    return ret;
  }

  TReader m_reader;
  uint64_t m_pos;
};

template <class TReader>
inline void ReadFromPos(TReader const & reader, uint64_t pos, void * p, size_t size)
{
  reader.Read(pos, p, size);
}

template <typename TPrimitive, class TReader>
inline TPrimitive ReadPrimitiveFromPos(TReader const & reader, uint64_t pos)
{
#ifndef OMIM_OS_LINUX
  static_assert(is_trivially_copyable<TPrimitive>::value, "");
#endif
  TPrimitive primitive;
  ReadFromPos(reader, pos, &primitive, sizeof(primitive));
  return SwapIfBigEndian(primitive);
}

template <typename TPrimitive, class TSource>
TPrimitive ReadPrimitiveFromSource(TSource & source)
{
#ifndef OMIM_OS_LINUX
  static_assert(is_trivially_copyable<TPrimitive>::value, "");
#endif
  TPrimitive primitive;
  source.Read(&primitive, sizeof(primitive));
  return SwapIfBigEndian(primitive);
}
