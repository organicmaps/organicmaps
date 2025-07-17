#pragma once

#include "coding/reader.hpp"
#include "coding/serdes_binary_header.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

namespace descriptions
{
struct HeaderV0
{
  template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    visitor(m_featuresOffset, "featuresOffset");
    visitor(m_langMetaOffset, "langMetaOffset");
    visitor(m_indexOffset, "indexOffset");
    visitor(m_stringsOffset, "stringsOffset");
    visitor(m_eosOffset, "eosOffset");
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    coding::binary::HeaderSerVisitor<Sink> visitor(sink);
    visitor(*this);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    coding::binary::HeaderDesVisitor<Source> visitor(source);
    visitor(*this);
  }

  uint64_t m_featuresOffset = 0;
  uint64_t m_langMetaOffset = 0;
  uint64_t m_indexOffset = 0;
  uint64_t m_stringsOffset = 0;
  uint64_t m_eosOffset = 0;  // End of section.
};
}  // namespace descriptions
