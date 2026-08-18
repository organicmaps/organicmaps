
#pragma once

#include "search/base/text_index/text_index.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string_view>

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

    // Read into a zero-terminated buffer: building a std::string_view from a (pointer, size)
    // pair makes gcc 14 evaluate the view's range constructor constraints, which fails here.
    char headerMagic[kHeaderMagic.size() + 1] = {};
    source.Read(headerMagic, kHeaderMagic.size());
    CHECK_EQUAL(kHeaderMagic, headerMagic, ());
    m_version = static_cast<TextIndexVersion>(ReadPrimitiveFromSource<uint8_t>(source));
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());
    m_numTokens = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictPositionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictWordsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsStartsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsListsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  }

  static constexpr std::string_view kHeaderMagic = "mapsmetextidx";
  TextIndexVersion m_version = TextIndexVersion::Latest;
  uint32_t m_numTokens = 0;
  uint32_t m_dictPositionsOffset = 0;
  uint32_t m_dictWordsOffset = 0;
  uint32_t m_postingsStartsOffset = 0;
  uint32_t m_postingsListsOffset = 0;
};
}  // namespace search_base
