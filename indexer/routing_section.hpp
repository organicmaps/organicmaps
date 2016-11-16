#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/cstdint.hpp"

namespace feature
{
class RoutingSectionHeader final
{
public:
  RoutingSectionHeader() : m_version(0), m_reserved16(0), m_reserved32(0) {}

  uint16_t GetVersion() const { return m_version; }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserved16);
    WriteToSink(sink, m_reserved32);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    m_version = ReadPrimitiveFromSource<decltype(m_version)>(src);
    m_reserved16 = ReadPrimitiveFromSource<decltype(m_reserved16)>(src);
    m_reserved32 = ReadPrimitiveFromSource<decltype(m_reserved32)>(src);
  }

private:
  uint16_t m_version;
  uint16_t m_reserved16;
  uint32_t m_reserved32;
};

static_assert(sizeof(RoutingSectionHeader) == 8, "Wrong header size of routing section.");
}  // namespace feature
