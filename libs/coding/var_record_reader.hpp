#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include <string>
#include <vector>

// Reads records, encoded as [VarUint size] [Data] .. [VarUint size] [Data].
template <class ReaderT>
class VarRecordReader
{
public:
  explicit VarRecordReader(ReaderT const & reader) : m_reader(reader) {}

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

  template <class FnT> void ForEachRecord(FnT && fn) const
  {
    ReaderSource source(m_reader);
    while (source.Size() > 0)
    {
      auto const pos = source.Pos();
      uint32_t const recordSize = ReadVarUint<uint32_t>(source);
      std::vector<uint8_t> buffer(recordSize);
      source.Read(buffer.data(), recordSize);
      fn(static_cast<uint32_t>(pos), std::move(buffer));
    }
  }

private:
  ReaderT m_reader;
};
