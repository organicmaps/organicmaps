#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <utility>

template <class WriterT>
class VarSerialVectorWriter
{
public:
  // Actually, it is possible to refactor to accept maxPossibleSize (i.e. capacity) instead of size.
  VarSerialVectorWriter(WriterT & writer, uint32_t size) : m_writer(writer), m_size(size), m_recordNumber(0)
  {
    WriteToSink(m_writer, size);
    m_sizesOffset = m_writer.Pos();
    m_writer.Seek(GetDataStartPos());
  }

  ~VarSerialVectorWriter() { CHECK_EQUAL(m_recordNumber, m_size, ()); }

  void FinishRecord()
  {
    CHECK_LESS(m_recordNumber, m_size, ());

    uint64_t const pos = m_writer.Pos();
    uint64_t const recordSize64 = pos - GetDataStartPos();
    uint32_t const recordSize = static_cast<uint32_t>(recordSize64);
    CHECK_EQUAL(recordSize, recordSize64, ());

    m_writer.Seek(m_sizesOffset + m_recordNumber * sizeof(uint32_t));
    WriteToSink(m_writer, recordSize);
    m_writer.Seek(pos);

    ++m_recordNumber;
  }

private:
  uint64_t GetDataStartPos() const { return m_sizesOffset + m_size * sizeof(uint32_t); }

private:
  WriterT & m_writer;
  uint64_t m_sizesOffset;
  uint32_t m_size;
  uint32_t m_recordNumber;
};

template <class ReaderT>
class VarSerialVectorReader
{
public:
  template <typename SourceT>
  explicit VarSerialVectorReader(SourceT & source)
    :

    /// Reading this code can blow your mind :)
    /// Initialization (and declaration below) order of m_offsetsReader and m_dataReader matters!!!
    /// @see ReaderSource::SubReader - it's not constant.
    /// @todo Do this stuff in a better way.
    m_size(ReadPrimitiveFromSource<uint32_t>(source))
    , m_offsetsReader(source.SubReader(m_size * sizeof(uint32_t)))
    , m_dataReader(source.SubReader())
  {}

  /// Used for unit tests only. Dont't do COW in production code.
  std::string Read(uint32_t i) const
  {
    std::pair<uint32_t, uint32_t> const posAsize = GetPosAndSize(i);

    std::string result;
    result.resize(posAsize.second);
    ReadFromPos(m_dataReader, posAsize.first, (void *)result.data(), posAsize.second);
    return result;
  }

  ReaderT SubReader(uint32_t i) const
  {
    std::pair<uint32_t, uint32_t> const posAsize = GetPosAndSize(i);
    return m_dataReader.SubReader(posAsize.first, posAsize.second);
  }

  uint64_t Size() const { return m_size; }

private:
  std::pair<uint32_t, uint32_t> GetPosAndSize(uint32_t i) const
  {
    uint32_t const begin = i == 0 ? 0 : ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, (i - 1) * sizeof(uint32_t));
    uint32_t const end = ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, i * sizeof(uint32_t));

    ASSERT_LESS_OR_EQUAL(begin, end, ());
    return std::make_pair(begin, end - begin);
  }

private:
  /// @note Do NOT change declaration order.
  uint64_t m_size;
  ReaderT m_offsetsReader;
  ReaderT m_dataReader;
};
