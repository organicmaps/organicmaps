#pragma once

#include "kml/serdes_binary.hpp"
#include "kml/types.hpp"
#include "kml/types_v8.hpp"
#include "kml/visitors.hpp"

#include <string>
#include <vector>

namespace kml::binary
{
// This class generates KMB files in format V8.
// The only difference between V8 and V9 (Latest) is bookmarks structure.
class SerializerKmlV8 : public SerializerKml
{
public:
  explicit SerializerKmlV8(FileData & data) : SerializerKml(data) {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    // Write format version.
    WriteToSink(sink, Version::V8);

    // Write device id.
    {
      auto const sz = static_cast<uint32_t>(m_data.m_deviceId.size());
      WriteVarUint(sink, sz);
      sink.Write(m_data.m_deviceId.data(), sz);
    }

    // Write server id.
    {
      auto const sz = static_cast<uint32_t>(m_data.m_serverId.size());
      WriteVarUint(sink, sz);
      sink.Write(m_data.m_serverId.data(), sz);
    }

    // Write bits count in double number.
    WriteToSink(sink, kDoubleBits);

    auto const startPos = sink.Pos();

    // Reserve place for the header.
    Header header;
    WriteZeroesToSink(sink, header.Size());

    // Serialize category.
    header.m_categoryOffset = sink.Pos() - startPos;
    SerializeCategory(sink);

    // Serialize bookmarks.
    header.m_bookmarksOffset = sink.Pos() - startPos;
    SerializeBookmarks(sink);

    // Serialize tracks.
    header.m_tracksOffset = sink.Pos() - startPos;
    SerializeTracks(sink);

    // Serialize compilations.
    header.m_compilationsOffset = sink.Pos() - startPos;
    SerializeCompilations(sink);

    // Serialize strings.
    header.m_stringsOffset = sink.Pos() - startPos;
    SerializeStrings(sink);

    // Fill header.
    header.m_eosOffset = sink.Pos() - startPos;
    sink.Seek(startPos);
    header.Serialize(sink);
    sink.Seek(startPos + header.m_eosOffset);
  }

  template <typename Sink>
  void SerializeBookmarks(Sink & sink)
  {
    BookmarkSerializerVisitor<Sink> visitor(sink, kDoubleBits);
    // Downgrade bookmark format from Latest to V8
    std::vector<BookmarkDataV8> bookmarksDataV8;
    bookmarksDataV8.reserve(m_data.m_bookmarksData.size());
    for (BookmarkData & bm : m_data.m_bookmarksData)
      bookmarksDataV8.push_back(BookmarkDataV8(bm));
    visitor(bookmarksDataV8);
  }
};
}  // namespace kml::binary
