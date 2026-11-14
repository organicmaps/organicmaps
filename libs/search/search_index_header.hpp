#pragma once

#include "search/search_index_values.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

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
    // The features of a trie node are a serialized compressed bit vector.
    V2 = 2,
    // The features of a trie node are an ids packet.
    V3 = 3,
    Latest = V3
  };

  using ListFormat = SingleValueSerializer<Uint64IndexValue>::ListFormat;

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::Latest), ());
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_indexOffset);
    WriteToSink(sink, m_indexSize);
  }

  void Read(Reader & reader)
  {
    NonOwningReaderSource source(reader);
    m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK(m_version == Version::V2 || m_version == Version::V3, (static_cast<uint32_t>(m_version)));
    m_indexOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_indexSize = ReadPrimitiveFromSource<uint32_t>(source);
  }

  // The serialization of the features of a trie node in the read version of the index.
  ListFormat GetListFormat() const
  {
    return m_version == Version::V2 ? ListFormat::CompressedBitVector : ListFormat::IdsPacket;
  }

  Version m_version = Version::Latest;
  // All offsets are relative to the start of the section (offset of header is zero).
  uint32_t m_indexOffset = 0;
  uint32_t m_indexSize = 0;
};
}  // namespace search
