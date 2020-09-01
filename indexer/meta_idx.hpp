#pragma once

#include "coding/map_uint32_to_val.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class FilesContainerR;
class Reader;
class Writer;

namespace feature
{
// Used for backward compatibility with mwm v10 only.
class MetadataIndex
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    Latest = V0
  };

  struct Header
  {
    template <typename Sink>
    void Serialize(Sink & sink) const
    {
      CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V0), ());
      WriteToSink(sink, static_cast<uint8_t>(m_version));
      WriteToSink(sink, m_indexOffset);
      WriteToSink(sink, m_indexSize);
    }

    void Read(Reader & reader);

    Version m_version = Version::Latest;
    // All offsets are relative to the start of the section (offset of header is zero).
    uint32_t m_indexOffset = 0;
    uint32_t m_indexSize = 0;
  };

  // Tries to get |offset| for metadata for feature with |id|. Returns false if table does not have
  // entry for the feature.
  WARN_UNUSED_RESULT bool Get(uint32_t id, uint32_t & offset) const;

  uint64_t Count() const { return m_map->Count(); };

  static std::unique_ptr<MetadataIndex> Load(Reader & reader);

private:
  using Map = MapUint32ToValue<uint32_t>;

  bool Init(std::unique_ptr<Reader> reader);

  std::unique_ptr<Map> m_map;
  std::unique_ptr<Reader> m_indexSubreader;
};

std::string DebugPrint(MetadataIndex::Version v);
}  // namespace feature
