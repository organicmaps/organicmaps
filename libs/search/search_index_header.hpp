#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

namespace search
{
struct SearchIndexHeader
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    V1 = 1,
    V2 = 2,
    Latest = V2
  };

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V2), ());
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_indexOffset);
    WriteToSink(sink, m_indexSize);
  }

  void Read(Reader & reader)
  {
    NonOwningReaderSource source(reader);
    m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V2), ());
    m_indexOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_indexSize = ReadPrimitiveFromSource<uint32_t>(source);
  }

  Version m_version = Version::Latest;
  // All offsets are relative to the start of the section (offset of header is zero).
  uint32_t m_indexOffset = 0;
  uint32_t m_indexSize = 0;
};
}  // namespace search
