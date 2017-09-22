#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

namespace routing
{
namespace transit
{
struct TransitHeader final
{
  TransitHeader() { Reset(); }
  TransitHeader(uint16_t version, uint32_t gatesOffset, uint32_t edgesOffset,
                uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                uint32_t networksOffset, uint32_t endOffset);
  void Reset();
  bool IsEqualForTesting(TransitHeader const & header) const;

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserve);
    WriteToSink(sink, m_gatesOffset);
    WriteToSink(sink, m_edgesOffset);
    WriteToSink(sink, m_transfersOffset);
    WriteToSink(sink, m_linesOffset);
    WriteToSink(sink, m_shapesOffset);
    WriteToSink(sink, m_networksOffset);
    WriteToSink(sink, m_endOffset);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    m_version = ReadPrimitiveFromSource<uint16_t>(src);
    m_reserve = ReadPrimitiveFromSource<uint16_t>(src);
    m_gatesOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_edgesOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_transfersOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_linesOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_shapesOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_networksOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_endOffset = ReadPrimitiveFromSource<uint32_t>(src);
  }

  uint16_t m_version;
  uint16_t m_reserve;
  uint32_t m_gatesOffset;
  uint32_t m_edgesOffset;
  uint32_t m_transfersOffset;
  uint32_t m_linesOffset;
  uint32_t m_shapesOffset;
  uint32_t m_networksOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(TransitHeader) == 32, "Wrong header size of transit section.");

string DebugPrint(TransitHeader const & turnItem);
}  // namespace transit
}  // namespace routing
