#pragma once

#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

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
  virtual std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const = 0;

  void ReadAsString(std::string & s) const;

  // Reads the contents of this Reader to a vector of 8-bit bytes.
  // Similar to ReadAsString but makes no assumptions about the char type.
  std::vector<uint8_t> ReadAsBytes() const;

  static bool IsEqual(std::string const & name1, std::string const & name2);
};

// Reader from memory.
template <bool WithExceptions>
class MemReaderTemplate : public Reader
{
public:
  // Construct from block of memory.
  MemReaderTemplate(void const * pData, size_t size)
    : m_pData(static_cast<char const *>(pData)), m_size(size)
  {
  }

  uint64_t Size() const override { return m_size; }

  void Read(uint64_t pos, void * p, size_t size) const override
  {
    AssertPosAndSize(pos, size);
    memcpy(p, m_pData + pos, size);
  }

  MemReaderTemplate SubReader(uint64_t pos, uint64_t size) const
  {
    AssertPosAndSize(pos, size);
    return MemReaderTemplate(m_pData + pos, static_cast<size_t>(size));
  }

  std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override
  {
    AssertPosAndSize(pos, size);
    return std::make_unique<MemReaderTemplate>(m_pData + pos, static_cast<size_t>(size));
  }

private:
  bool GoodPosAndSize(uint64_t pos, uint64_t size) const
  {
    // In case of 32 bit system, when sizeof(size_t) == 4.
    return (pos + size <= Size() && size <= std::numeric_limits<size_t>::max());
  }

  void AssertPosAndSize(uint64_t pos, uint64_t size) const
  {
    if constexpr (WithExceptions)
    {
      if (!GoodPosAndSize(pos, size))
        MYTHROW(Reader::SizeException, (pos, size, Size()));
    }
    else
    {
      ASSERT(GoodPosAndSize(pos, size), (pos, size, Size()));
    }
  }

  char const * m_pData;
  size_t m_size;
};

using MemReader = MemReaderTemplate<false>;
using MemReaderWithExceptions = MemReaderTemplate<true>;

// Reader wrapper to hold the pointer to a polymorfic reader.
// Common use: ReaderSource<ReaderPtr<Reader> >.
// Note! It takes the ownership of Reader.
template <class TReader>
class ReaderPtr
{
protected:
  std::shared_ptr<TReader> m_p;

public:
  template <typename TReaderDerived>
  ReaderPtr(std::unique_ptr<TReaderDerived> p) : m_p(move(p))
  {
  }

  uint64_t Size() const
  {
    return m_p->Size();
  }

  void Read(uint64_t pos, void * p, size_t size) const
  {
    m_p->Read(pos, p, size);
  }

  void ReadAsString(std::string & s) const { m_p->ReadAsString(s); }

  ReaderPtr<Reader> SubReader(uint64_t pos, uint64_t size) const
  {
    return {m_p->CreateSubReader(pos, size)};
  }

  TReader * GetPtr() const { return m_p.get(); }
};

// Model reader store file id as string.
class ModelReader : public Reader
{
  std::string m_name;

public:
  ModelReader(std::string const & name) : m_name(name) {}

  virtual std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override = 0;

  std::string const & GetName() const { return m_name; }
};

// Reader pointer class for data files.
class ModelReaderPtr : public ReaderPtr<ModelReader>
{
  using TBase = ReaderPtr<ModelReader>;

public:
  template <typename TReaderDerived>
  ModelReaderPtr(std::unique_ptr<TReaderDerived> p) : TBase(move(p))
  {
  }

  ModelReaderPtr SubReader(uint64_t pos, uint64_t size) const
  {
    return std::unique_ptr<ModelReader>(
        static_cast<ModelReader *>(m_p->CreateSubReader(pos, size).release()));
  }

  std::string const & GetName() const { return m_p->GetName(); }
};

/// Source that reads from a reader and holds Reader by non-owning reference.
/// No templates here allows to hide Deserialization functions in cpp.
class NonOwningReaderSource
{
public:
  /// @note Reader shouldn't change it's size during the source's lifetime.
  explicit NonOwningReaderSource(Reader const & reader)
  : m_reader(reader), m_pos(0), m_end(reader.Size())
  {}

  NonOwningReaderSource(Reader const & reader, uint64_t pos, uint64_t end)
  : m_reader(reader), m_pos(pos), m_end(end)
  {}

  void Read(void * p, size_t size)
  {
    m_reader.Read(m_pos, p, size);
    m_pos += size;
    CheckPosition();
  }

  void Skip(uint64_t size)
  {
    m_pos += size;
    CheckPosition();
  }

  uint64_t Pos() const { return m_pos; }

  uint64_t Size() const
  {
    CheckPosition();
    return m_end - m_pos;
  }

  void SetPosition(uint64_t pos)
  {
    m_pos = pos;
    CheckPosition();
  }

private:
  void CheckPosition() const
  {
    ASSERT_LESS_OR_EQUAL(m_pos, m_end, ());
  }

  Reader const & m_reader;
  uint64_t m_pos, m_end;
};

/// Source that reads from a reader and holds Reader by value.
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

  /// @todo We can avoid calling virtual Reader::SubReader and creating unique_ptr here
  /// by simply making "ReaderSource ReaderSource::SubSource(pos, end)" and storing "ReaderSource::m_end"
  /// like I did in NonOwningReaderSource. Unfortunatelly, it needs a lot of efforts in refactoring.
  /// @{
  TReader SubReader(uint64_t size)
  {
    uint64_t const pos = m_pos;
    Skip(size);
    return m_reader.SubReader(pos, size);
  }

  TReader SubReader() { return SubReader(Size()); }

  std::unique_ptr<Reader> CreateSubReader(uint64_t size)
  {
    uint64_t const pos = m_pos;
    Skip(size);
    return m_reader.CreateSubReader(pos, size);
  }

  std::unique_ptr<Reader> CreateSubReader() { return CreateSubReader(Size()); }
  /// @}

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
  static_assert(std::is_trivially_copyable<TPrimitive>::value, "");
#endif
  TPrimitive primitive;
  ReadFromPos(reader, pos, &primitive, sizeof(primitive));
  return SwapIfBigEndianMacroBased(primitive);
}

template <typename TPrimitive, class TSource>
TPrimitive ReadPrimitiveFromSource(TSource & source)
{
#ifndef OMIM_OS_LINUX
  static_assert(std::is_trivially_copyable<TPrimitive>::value, "");
#endif
  TPrimitive primitive;
  source.Read(&primitive, sizeof(primitive));
  return SwapIfBigEndianMacroBased(primitive);
}

template <typename TPrimitive, typename TSource>
void ReadPrimitiveFromSource(TSource & source, TPrimitive & primitive)
{
  primitive = ReadPrimitiveFromSource<TPrimitive, TSource>(source);
}
