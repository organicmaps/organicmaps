#pragma once

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/base.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Reads records, encoded as [VarUint size] [Data] .. [VarUint size] [Data].
template <class ReaderT>
class VarRecordReader
{
public:
  VarRecordReader(ReaderT const & reader) : m_reader(reader), m_readerSize(reader.Size()) {}

  std::vector<char> ReadRecord(uint64_t const pos) const
  {
    ASSERT_LESS(pos, m_readerSize, ());
    ReaderSource source(m_reader);
    source.Skip(pos);
    uint32_t const recordSize = ReadVarUint<uint32_t>(source);
    std::vector<char> buffer(recordSize);
    source.Read(buffer.data(), recordSize);
    return buffer;
  }

  void ForEachRecord(std::function<void(uint32_t, std::vector<char> &&)> const & f) const
  {
    uint64_t pos = 0;
    ReaderSource source(m_reader);
    while (pos < m_readerSize)
    {
      uint32_t const recordSize = ReadVarUint<uint32_t>(source);
      std::vector<char> buffer(recordSize);
      source.Read(buffer.data(), recordSize);
      f(static_cast<uint32_t>(pos), std::move(buffer));
      pos = source.Pos();
    }
    ASSERT_EQUAL(pos, m_ReaderSize, ());
  }

protected:
  ReaderT m_reader;
  uint64_t m_readerSize;
};
