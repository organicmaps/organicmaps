#pragma once

#include "coding/reader.hpp"

namespace feature
{
struct DatSectionHeader
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    Latest = V0
  };

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK_EQUAL(m_version, Version::V0, ());
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_featuresOffset);
    WriteToSink(sink, m_featuresSize);
  }

  void Read(Reader & reader)
  {
    NonOwningReaderSource source(reader);
    m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V0), ());
    m_featuresOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_featuresSize = ReadPrimitiveFromSource<uint32_t>(source);
  }

  Version m_version = Version::Latest;
  // All offsets are relative to the start of the section (offset of header is zero).
  uint32_t m_featuresOffset = 0;
  uint32_t m_featuresSize = 0;
};

inline std::string DebugPrint(DatSectionHeader::Version v)
{
  CHECK(v == DatSectionHeader::Version::V0, ());
  return "V0";
}
}  // namespace feature
