#pragma once

#include "kml/header_binary.hpp"
#include "kml/types.hpp"
#include "kml/visitors.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/sha1.hpp"
#include "coding/text_storage.hpp"

#include "platform/platform.hpp"

#include <string>
#include <vector>

namespace kml
{
namespace binary
{
enum class Version : uint8_t
{
  V0 = 0,
  V1 = 1, // 11th April 2018 (new Point2D storage, added deviceId, feature name -> custom name).
  V2 = 2, // 25th April 2018 (added serverId).
  V3 = 3, // 7th May 2018 (persistent feature types).
  Latest = V3
};

class SerializerKml
{
public:
  explicit SerializerKml(FileData & data);
  ~SerializerKml();

  void ClearCollectionIndex();

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    // Write format version.
    WriteToSink(sink, Version::Latest);

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
  void SerializeCategory(Sink & sink)
  {
    CategorySerializerVisitor<Sink> visitor(sink, kDoubleBits);
    visitor(m_data.m_categoryData);
  }

  template <typename Sink>
  void SerializeBookmarks(Sink & sink)
  {
    BookmarkSerializerVisitor<Sink> visitor(sink, kDoubleBits);
    visitor(m_data.m_bookmarksData);
  }

  template <typename Sink>
  void SerializeTracks(Sink & sink)
  {
    BookmarkSerializerVisitor<Sink> visitor(sink, kDoubleBits);
    visitor(m_data.m_tracksData);
  }

  // Serializes texts in a compressed storage with block access.
  template <typename Sink>
  void SerializeStrings(Sink & sink)
  {
    coding::BlockedTextStorageWriter<Sink> writer(sink, 200000 /* blockSize */);
    for (auto const & str : m_strings)
      writer.Append(str);
  }

private:
  FileData & m_data;
  std::vector<std::string> m_strings;
};

class DeserializerKml
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerKml(FileData & data);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    // Check version.
    NonOwningReaderSource source(reader);
    auto const v = ReadPrimitiveFromSource<Version>(source);

    if (v != Version::Latest && v != Version::V2)
      MYTHROW(DeserializeException, ("Incorrect file version."));

    // Read device id.
    {
      auto const sz = ReadVarUint<uint32_t>(source);
      m_data.m_deviceId.resize(sz);
      source.Read(&m_data.m_deviceId[0], sz);
    }

    // Read server id.
    {
      auto const sz = ReadVarUint<uint32_t>(source);
      m_data.m_serverId.resize(sz);
      source.Read(&m_data.m_serverId[0], sz);
    }

    // Read bits count in double number.
    m_doubleBits = ReadPrimitiveFromSource<uint8_t>(source);
    if (m_doubleBits == 0 || m_doubleBits > 32)
      MYTHROW(DeserializeException, ("Incorrect double bits count: ", m_doubleBits));

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());
    InitializeIfNeeded(*subReader);

    // Deserialize category.
    {
      auto categorySubReader = CreateCategorySubReader(*subReader);
      NonOwningReaderSource src(*categorySubReader);
      CategoryDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
      visitor(m_data.m_categoryData);
    }

    // Deserialize bookmarks.
    {
      auto bookmarkSubReader = CreateBookmarkSubReader(*subReader);
      NonOwningReaderSource src(*bookmarkSubReader);
      BookmarkDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
      visitor(m_data.m_bookmarksData);
    }

    // Migrate bookmarks.
    if (v == Version::V2)
    {
      for (auto & data : m_data.m_bookmarksData)
        data.m_featureTypes.clear();
    }

    // Deserialize tracks.
    {
      auto trackSubReader = CreateTrackSubReader(*subReader);
      NonOwningReaderSource src(*trackSubReader);
      BookmarkDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
      visitor(m_data.m_tracksData);
    }

    // Deserialize strings.
    {
      auto textsSubReader = CreateStringsSubReader(*subReader);
      coding::BlockedTextStorage<Reader> strings(*textsSubReader);
      DeserializedStringCollector<Reader> collector(strings);
      CollectorVisitor<decltype(collector)> visitor(collector);
      m_data.Visit(visitor);
      CollectorVisitor<decltype(collector)> clearVisitor(collector,
                                                         true /* clear index */);
      m_data.Visit(clearVisitor);
    }
  }

private:
  template <typename ReaderType>
  void InitializeIfNeeded(ReaderType const & reader)
  {
    if (m_initialized)
      return;

    NonOwningReaderSource source(reader);
    m_header.Deserialize(source);
    m_initialized = true;
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateSubReader(ReaderType const & reader,
                                          uint64_t startPos, uint64_t endPos)
  {
    ASSERT(m_initialized, ());
    ASSERT_GREATER_OR_EQUAL(endPos, startPos, ());
    auto const size = endPos - startPos;
    return reader.CreateSubReader(startPos, size);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateCategorySubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_categoryOffset, m_header.m_bookmarksOffset);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateBookmarkSubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_bookmarksOffset, m_header.m_tracksOffset);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateTrackSubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_tracksOffset, m_header.m_stringsOffset);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateStringsSubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_stringsOffset, m_header.m_eosOffset);
  }

  FileData & m_data;
  Header m_header;
  uint8_t m_doubleBits = 0;
  bool m_initialized = false;
};
}  // namespace binary
}  // namespace kml
