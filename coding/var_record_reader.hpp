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
  VarRecordReader(ReaderT const & reader) : m_reader(reader) {}

  std::vector<uint8_t> ReadRecord(uint64_t const pos) const
  {
    ReaderSource source(m_reader);
    ASSERT_LESS(pos, source.Size(), ());
    source.Skip(pos);
    uint32_t const recordSize = ReadVarUint<uint32_t>(source);
    std::vector<uint8_t> buffer(recordSize);
    source.Read(buffer.data(), recordSize);
    return buffer;
  }

  void ForEachRecord(std::function<void(uint32_t, std::vector<uint8_t> &&)> const & f) const
  {
    ReaderSource source(m_reader);
    while (source.Size() > 0)
    {
      auto const pos = source.Pos();
      uint32_t const recordSize = ReadVarUint<uint32_t>(source);
      std::vector<uint8_t> buffer(recordSize);
      source.Read(buffer.data(), recordSize);
      f(static_cast<uint32_t>(pos), std::move(buffer));
    }
  }

protected:
  ReaderT m_reader;
};
