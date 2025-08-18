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
    V1,  // 2025.06, get some free bits in Feature::Header2
    Latest = V1
  };

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_featuresOffset);
    WriteToSink(sink, m_featuresSize);
  }

  template <typename Source>
  void Read(Source & source)
  {
    m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK(Version::V0 <= m_version && m_version <= Version::Latest, (m_version));

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
  switch (v)
  {
  case DatSectionHeader::Version::V0: return "V0";
  default: return "Latest(V1)";
  }
}
}  // namespace feature
