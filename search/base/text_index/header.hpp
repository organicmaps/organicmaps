
#pragma once

#include "search/base/text_index/text_index.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>

namespace search_base
{
struct TextIndexHeader
{
  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());

    sink.Write(kHeaderMagic.data(), kHeaderMagic.size());
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_numTokens);
    WriteToSink(sink, m_dictPositionsOffset);
    WriteToSink(sink, m_dictWordsOffset);
    WriteToSink(sink, m_postingsStartsOffset);
    WriteToSink(sink, m_postingsListsOffset);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());

    std::string headerMagic(kHeaderMagic.size(), ' ');
    source.Read(&headerMagic[0], headerMagic.size());
    CHECK_EQUAL(headerMagic, kHeaderMagic, ());
    m_version = static_cast<TextIndexVersion>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());
    m_numTokens = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictPositionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictWordsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsStartsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsListsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  }

  static std::string const kHeaderMagic;
  TextIndexVersion m_version = TextIndexVersion::Latest;
  uint32_t m_numTokens = 0;
  uint32_t m_dictPositionsOffset = 0;
  uint32_t m_dictWordsOffset = 0;
  uint32_t m_postingsStartsOffset = 0;
  uint32_t m_postingsListsOffset = 0;
};
}  // namespace search_base
