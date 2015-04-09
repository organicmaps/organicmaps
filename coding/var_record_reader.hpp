#pragma once
#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "base/base.hpp"
#include "std/algorithm.hpp"
#include "std/vector.hpp"

inline uint32_t VarRecordSizeReaderVarint(ArrayByteSource & source)
{
  return ReadVarUint<uint32_t>(source);
}

inline uint32_t VarRecordSizeReaderFixed(ArrayByteSource & source)
{
  return ReadPrimitiveFromSource<uint32_t>(source);
}

// Efficiently reads records, encoded as [VarUint size] [Data] .. [VarUint size] [Data].
// If size of a record is less than expectedRecordSize, exactly 1 Reader.Read() call is made,
// otherwise exactly 2 Reader.Read() calls are made.
// Second template parameter is strategy for reading record size,
// either &VarRecordSizeReaderVarint or &VarRecordSizeReaderFixed.
template <class ReaderT, uint32_t (*VarRecordSizeReaderFn)(ArrayByteSource &)>
class VarRecordReader
{
public:
  VarRecordReader(ReaderT const & reader, uint32_t expectedRecordSize)
  : m_Reader(reader), m_ReaderSize(reader.Size()), m_ExpectedRecordSize(expectedRecordSize)
  {
    ASSERT_GREATER_OR_EQUAL(expectedRecordSize, 4, ());
  }

  uint64_t ReadRecord(uint64_t const pos, vector<char> & buffer, uint32_t & recordOffset, uint32_t & actualSize) const
  {
    ASSERT_LESS(pos, m_ReaderSize, ());
    uint32_t const initialSize = static_cast<uint32_t>(
        min(static_cast<uint64_t>(m_ExpectedRecordSize), m_ReaderSize - pos));

    if (buffer.size() < initialSize)
      buffer.resize(initialSize);

    m_Reader.Read(pos, &buffer[0], initialSize);
    ArrayByteSource source(&buffer[0]);
    uint32_t const recordSize = VarRecordSizeReaderFn(source);
    uint32_t const recordSizeSize = static_cast<uint32_t>(source.PtrC() - &buffer[0]);
    uint32_t const fullSize = recordSize + recordSizeSize;
    ASSERT_LESS_OR_EQUAL(pos + fullSize, m_ReaderSize, ());
    if (buffer.size() < fullSize)
      buffer.resize(fullSize);
    if (initialSize < fullSize)
      m_Reader.Read(pos + initialSize, &buffer[initialSize], fullSize - initialSize);

    recordOffset = recordSizeSize;
    actualSize = fullSize;
    return pos + fullSize;
  }

  template <typename F>
  void ForEachRecord(F const & f) const
  {
    uint64_t pos = 0;
    vector<char> buffer;
    while (pos < m_ReaderSize)
    {
      uint32_t offset = 0, size = 0;
      uint64_t nextPos = ReadRecord(pos, buffer, offset, size);
      // uint64_t -> uint32_t : assume that feature dat file not more than 4Gb
      f(static_cast<uint32_t>(pos), &buffer[offset], static_cast<uint32_t>(size - offset));
      pos = nextPos;
    }
    ASSERT_EQUAL(pos, m_ReaderSize, ());
  }

  bool IsEqual(string const & fName) const { return m_Reader.IsEqual(fName); }

protected:
  ReaderT m_Reader;
  uint64_t m_ReaderSize;
  uint32_t m_ExpectedRecordSize; // Expected size of a record.
};
