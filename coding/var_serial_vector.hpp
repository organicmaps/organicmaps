#pragma once

#include "endianness.hpp"
#include "reader.hpp"
#include "write_to_sink.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"
#include "../std/scoped_ptr.hpp"


template <typename ItT, typename TDstStream>
void WriteVarSerialVector(ItT begin, ItT end, TDstStream & dst)
{
  vector<uint32_t> offsets;
  uint32_t offset = 0;
  for (ItT it = begin; it != end; ++it)
  {
    offset += it->size() * sizeof((*it)[0]);
    offsets.push_back(offset);
  }

  WriteToSink(dst, static_cast<uint32_t>(end - begin));

  for (size_t i = 0; i < offsets.size(); ++i)
    WriteToSink(dst, offsets[i]);

  for (ItT it = begin; it != end; ++it)
  {
    typename ItT::value_type const & v = *it;
    if (!v.empty())
      dst.Write(&v[0], v.size() * sizeof(v[0]));
  }
}

template <class WriterT> class VarSerialVectorWriter
{
public:
  // Actually, it is possible to refactor to accept maxPossibleSize (i.e. capacity) instead of size.
  VarSerialVectorWriter(WriterT & writer, uint32_t size)
    : m_Writer(writer), m_Size(size), m_RecordNumber(0)
  {
    WriteToSink(m_Writer, size);
    m_SizesOffset = m_Writer.Pos();
    m_Writer.Seek(m_SizesOffset + size * 4);
  }

  ~VarSerialVectorWriter()
  {
    CHECK_EQUAL(m_RecordNumber, m_Size, ());
  }

  void FinishRecord()
  {
    CHECK_LESS(m_RecordNumber, m_Size, ());
    uint64_t const pos = m_Writer.Pos();
    uint64_t recordSize64 = pos - m_SizesOffset - m_Size * 4;
    uint32_t recordSize = static_cast<uint32_t>(recordSize64);
    CHECK_EQUAL(recordSize, recordSize64, ());
    m_Writer.Seek(m_SizesOffset + m_RecordNumber * 4);
    WriteToSink(m_Writer, recordSize);
    m_Writer.Seek(pos);
    ++m_RecordNumber;
  }

private:
  WriterT & m_Writer;
  uint64_t m_SizesOffset;
  uint32_t m_Size;
  uint32_t m_RecordNumber;
};

template <class ReaderT>
class VarSerialVectorReader
{
public:
  template <typename SourceT>
  explicit VarSerialVectorReader(SourceT & source) :

  /// Reading this code can blow your mind :)
  /// Initialization (and declaration below) order of m_offsetsReader and m_dataReader matters!!!
  /// @see ReaderSource::SubReader - it's not constant.
  /// @todo Do this stuff in a better way.
  m_size(ReadPrimitiveFromSource<uint32_t>(source)),
  m_offsetsReader(source.SubReader(m_size * sizeof(uint32_t))),
  m_dataReader(source.SubReader())
  {
  }

  string Read(uint64_t i) const
  {
    size_t begin =
        i == 0 ? 0 : ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, (i - 1) * sizeof(uint32_t));
    size_t end = ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, i * sizeof(uint32_t));

    string result;
    result.resize(end - begin);
    ReadFromPos(m_dataReader, begin, (void *)result.data(), end - begin);
    return result;
  }

  ReaderT SubReader(uint64_t i) const
  {
    size_t begin =
        i == 0 ? 0 : ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, (i - 1) * sizeof(uint32_t));
    size_t end = ReadPrimitiveFromPos<uint32_t>(m_offsetsReader, i * sizeof(uint32_t));

    return m_dataReader.SubReader(begin, end - begin);
  }

  uint64_t Size() const
  {
    return m_size;
  }

private:

  /// @note Do NOT change declaration order.
  uint64_t m_size;
  ReaderT m_offsetsReader;
  ReaderT m_dataReader;
};
