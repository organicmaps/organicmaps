#pragma once

#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/text_storage.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/stl_helpers.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace indexer
{
class Postcodes
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
      CHECK_EQUAL(base::Underlying(m_version), base::Underlying(Version::V0), ());
      WriteToSink(sink, static_cast<uint8_t>(m_version));
      WriteToSink(sink, m_stringsOffset);
      WriteToSink(sink, m_stringsSize);
      WriteToSink(sink, m_postcodesMapOffset);
      WriteToSink(sink, m_postcodesMapSize);
    }

    void Read(Reader & reader);

    Version m_version = Version::Latest;
    // All offsets are relative to the start of the section (offset of header is zero).
    uint32_t m_stringsOffset = 0;
    uint32_t m_stringsSize = 0;
    uint32_t m_postcodesMapOffset = 0;
    uint32_t m_postcodesMapSize = 0;
  };

  static std::unique_ptr<Postcodes> Load(Reader & reader);

  // Tries to get |postcode| of the feature with id |featureId|. Returns false if table
  // does not have entry for the feature.
  WARN_UNUSED_RESULT bool Get(uint32_t featureId, std::string & postcode);

private:
  using Map = MapUint32ToValue<uint32_t>;

  std::unique_ptr<Reader> m_stringsSubreader;
  coding::BlockedTextStorageReader m_strings;
  std::unique_ptr<Map> m_map;
  std::unique_ptr<Reader> m_mapSubreader;
  Version m_version = Version::Latest;
};

class PostcodesBuilder
{
public:
  void Put(uint32_t featureId, std::string const & postcode);
  void Freeze(Writer & writer) const;

private:
  std::unordered_map<std::string, uint32_t> m_postcodeToId;
  std::unordered_map<uint32_t, std::string> m_idToPostcode;
  MapUint32ToValueBuilder<uint32_t> m_builder;
};
}  // namespace indexer
