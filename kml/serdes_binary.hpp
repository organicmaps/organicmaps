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
  Latest = V0
};

class SerializerKml
{
public:
  explicit SerializerKml(CategoryData & data);
  ~SerializerKml();

  void ClearCollectionIndex();

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    // Write format version.
    WriteToSink(sink, Version::Latest);

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
    CategorySerializerVisitor<Sink> visitor(sink);
    visitor(m_data);
  }

  template <typename Sink>
  void SerializeBookmarks(Sink & sink)
  {
    BookmarkSerializerVisitor<Sink> visitor(sink);
    visitor(m_data.m_bookmarksData);
  }

  template <typename Sink>
  void SerializeTracks(Sink & sink)
  {
    BookmarkSerializerVisitor<Sink> visitor(sink);
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
  CategoryData & m_data;
  std::vector<std::string> m_strings;
};

class DeserializerKml
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerKml(CategoryData & data);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    // Check version.
    NonOwningReaderSource source(reader);
    auto const v = ReadPrimitiveFromSource<Version>(source);
    if (v != Version::Latest)
      MYTHROW(DeserializeException, ("Incorrect file version."));

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());
    InitializeIfNeeded(*subReader);

    // Deserialize category.
    {
      auto categorySubReader = CreateCategorySubReader(*subReader);
      NonOwningReaderSource src(*categorySubReader);
      CategoryDeserializerVisitor<decltype(src)> visitor(src);
      visitor(m_data);
    }

    // Deserialize bookmarks.
    {
      auto bookmarkSubReader = CreateBookmarkSubReader(*subReader);
      NonOwningReaderSource src(*bookmarkSubReader);
      BookmarkDeserializerVisitor<decltype(src)> visitor(src);
      visitor(m_data.m_bookmarksData);
    }

    // Deserialize tracks.
    {
      auto trackSubReader = CreateTrackSubReader(*subReader);
      NonOwningReaderSource src(*trackSubReader);
      BookmarkDeserializerVisitor<decltype(src)> visitor(src);
      visitor(m_data.m_tracksData);
    }

    // Deserialize strings.
    {
      auto textsSubReader = CreateStringsSubReader(*subReader);
      coding::BlockedTextStorage<Reader> strings(*textsSubReader);
      DeserializedStringCollector<Reader> collector(strings);
      CollectorVisitor<decltype(collector)> visitor(collector);
      visitor(m_data);
      CollectorVisitor<decltype(collector)> clearVisitor(collector,
                                                         true /* clear index */);
      clearVisitor(m_data);
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

  CategoryData & m_data;
  Header m_header;
  bool m_initialized = false;
};
}  // namespace binary
}  // namespace kml
