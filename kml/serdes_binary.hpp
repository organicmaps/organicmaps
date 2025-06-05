#pragma once

#include "kml/header_binary.hpp"
#include "kml/types.hpp"
#include "kml/types_v3.hpp"
#include "kml/types_v6.hpp"
#include "kml/types_v7.hpp"
#include "kml/types_v8.hpp"
#include "kml/types_v8mm.hpp"
#include "kml/types_v9mm.hpp"
#include "kml/visitors.hpp"

#include "coding/text_storage.hpp"

#include <string>
#include <vector>

namespace kml
{
namespace binary
{
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

  template <typename Sink>
  void SerializeCompilations(Sink & sink)
  {
    CategorySerializerVisitor<Sink> visitor(sink, kDoubleBits);
    visitor(m_data.m_compilationsData);
  }

  // Serializes texts in a compressed storage with block access.
  template <typename Sink>
  void SerializeStrings(Sink & sink)
  {
    coding::BlockedTextStorageWriter<Sink> writer(sink, 200000 /* blockSize */);
    for (auto const & str : m_strings)
      writer.Append(str);
  }

protected:
  FileData & m_data;
  std::vector<std::string> m_strings;
};

template <typename T, typename = void>
struct HasCompilationsData : std::false_type
{};

template <typename T>
struct HasCompilationsData<T, std::void_t<decltype(T::m_compilationsData)>>
  : std::is_same<decltype(T::m_compilationsData), std::vector<CategoryData>>
{};

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
    m_header.m_version = ReadPrimitiveFromSource<Version>(source);

    if (m_header.m_version != Version::V2 && m_header.m_version != Version::V3 && m_header.m_version != Version::V4 &&
        m_header.m_version != Version::V5 && m_header.m_version != Version::V6 && m_header.m_version != Version::V7 &&
        m_header.m_version != Version::V8 && m_header.m_version != Version::V9)
    {
      MYTHROW(DeserializeException, ("Incorrect file version."));
    }

    ReadDeviceId(source);
    ReadServerId(source);
    ReadBitsCountInDouble(source);

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());
    InitializeIfNeeded(*subReader);

    switch (m_header.m_version)
    {
    case Version::Latest:
    {
      DeserializeFileData(subReader, m_data);
      break;
    }
    case Version::V8:
    {
      FileDataV8 dataV8;
      dataV8.m_deviceId = m_data.m_deviceId;
      dataV8.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV8);

      m_data = dataV8.ConvertToLatestVersion();
      break;
    }
    case Version::V8MM:
    {
      FileDataV8MM dataV8MM;
      dataV8MM.m_deviceId = m_data.m_deviceId;
      dataV8MM.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV8MM);

      m_data = dataV8MM.ConvertToLatestVersion();
      break;
    }
    case Version::V9MM:
    {
      FileDataV9MM dataV9MM;
      dataV9MM.m_deviceId = m_data.m_deviceId;
      dataV9MM.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV9MM);

      m_data = dataV9MM.ConvertToLatestVersion();
      break;
    }
    case Version::V7:
    {
      FileDataV7 dataV7;
      dataV7.m_deviceId = m_data.m_deviceId;
      dataV7.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV7);

      m_data = dataV7.ConvertToLatestVersion();
      break;
    }
    case Version::V6:
    case Version::V5:
    case Version::V4:
    {
      // NOTE: v.4, v.5 and v.6 are binary compatible.
      FileDataV6 dataV6;
      dataV6.m_deviceId = m_data.m_deviceId;
      dataV6.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV6);

      m_data = dataV6.ConvertToLatestVersion();
      break;
    }
    case Version::V3:
    case Version::V2:
    {
      // NOTE: v.2 and v.3 are binary compatible.
      FileDataV3 dataV3;
      dataV3.m_deviceId = m_data.m_deviceId;
      dataV3.m_serverId = m_data.m_serverId;
      DeserializeFileData(subReader, dataV3);

      // Migrate bookmarks (it's necessary ony for v.2).
      if (m_header.m_version == Version::V2)
        MigrateBookmarksV2(dataV3);

      m_data = dataV3.ConvertToLatestVersion();
      break;
    }
    default:
    {
      UNREACHABLE();
    }
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

    if (m_header.m_version == Version::V8 || m_header.m_version == Version::V9)
    {
      // Check if file has Opensource V8/V9 or MapsMe V8/V9 format.
      // Actual V8/V9 format has 6 offsets (uint64_t) in header. While V8MM/V9MM has 5 offsets.
      // It means that first section (usually categories) has offset 0x28 = 40 = 5 * 8.
      if (m_header.m_categoryOffset == 0x28 || m_header.m_bookmarksOffset == 0x28 || m_header.m_tracksOffset == 0x28 ||
          m_header.m_stringsOffset == 0x28 || m_header.m_compilationsOffset == 0x28)
      {
        m_header.m_version = (m_header.m_version == Version::V8 ? Version::V8MM : Version::V9MM);
        LOG(LINFO, ("KMB file has version", m_header.m_version));

        m_header.m_eosOffset = m_header.m_stringsOffset;
        m_header.m_stringsOffset = m_header.m_compilationsOffset;
      }
    }

    m_initialized = true;
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateSubReader(ReaderType const & reader, uint64_t startPos, uint64_t endPos)
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
    return CreateSubReader(reader, m_header.m_tracksOffset, m_header.m_compilationsOffset);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateCompilationsSubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_compilationsOffset, m_header.m_stringsOffset);
  }

  template <typename ReaderType>
  std::unique_ptr<Reader> CreateStringsSubReader(ReaderType const & reader)
  {
    return CreateSubReader(reader, m_header.m_stringsOffset, m_header.m_eosOffset);
  }

  void ReadDeviceId(NonOwningReaderSource & source)
  {
    auto const sz = ReadVarUint<uint32_t>(source);
    m_data.m_deviceId.resize(sz);
    source.Read(&m_data.m_deviceId[0], sz);
  }

  void ReadServerId(NonOwningReaderSource & source)
  {
    auto const sz = ReadVarUint<uint32_t>(source);
    m_data.m_serverId.resize(sz);
    source.Read(&m_data.m_serverId[0], sz);
  }

  void ReadBitsCountInDouble(NonOwningReaderSource & source)
  {
    m_doubleBits = ReadPrimitiveFromSource<uint8_t>(source);
    if (m_doubleBits == 0 || m_doubleBits > 32)
      MYTHROW(DeserializeException, ("Incorrect double bits count: ", m_doubleBits));
  }

  template <typename FileDataType>
  void DeserializeFileData(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    // Keep in mind - deserialization/serialization works in two stages:
    // - serialization/deserialization non-string members of structures;
    // - serialization/deserialization string members of structures.
    DeserializeCategory(subReader, data);
    DeserializeBookmarks(subReader, data);
    DeserializeTracks(subReader, data);
    if constexpr (HasCompilationsData<FileDataType>::value)
      DeserializeCompilations(subReader, data);
    DeserializeStrings(subReader, data);
  }

  template <typename FileDataType>
  void DeserializeCategory(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    auto categorySubReader = CreateCategorySubReader(*subReader);
    NonOwningReaderSource src(*categorySubReader);
    CategoryDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
    visitor(data.m_categoryData);
  }

  template <typename FileDataType>
  void DeserializeBookmarks(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    auto bookmarkSubReader = CreateBookmarkSubReader(*subReader);
    NonOwningReaderSource src(*bookmarkSubReader);
    BookmarkDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
    visitor(data.m_bookmarksData);
  }

  void MigrateBookmarksV2(FileDataV3 & data)
  {
    for (auto & d : data.m_bookmarksData)
      d.m_featureTypes.clear();
  }

  template <typename FileDataType>
  void DeserializeTracks(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    auto trackSubReader = CreateTrackSubReader(*subReader);
    NonOwningReaderSource src(*trackSubReader);
    BookmarkDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
    visitor(data.m_tracksData);
  }

  template <typename FileDataType>
  void DeserializeCompilations(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    auto compilationsSubReader = CreateCompilationsSubReader(*subReader);
    NonOwningReaderSource src(*compilationsSubReader);
    CategoryDeserializerVisitor<decltype(src)> visitor(src, m_doubleBits);
    visitor(data.m_compilationsData);
  }

  template <typename FileDataType>
  void DeserializeStrings(std::unique_ptr<Reader> & subReader, FileDataType & data)
  {
    auto textsSubReader = CreateStringsSubReader(*subReader);
    coding::BlockedTextStorage<Reader> strings(*textsSubReader);
    DeserializedStringCollector<Reader> collector(strings);
    CollectorVisitor<decltype(collector)> visitor(collector);
    data.Visit(visitor);
    CollectorVisitor<decltype(collector)> clearVisitor(collector, true /* clear index */);
    data.Visit(clearVisitor);
  }

  FileData & m_data;
  Header m_header;
  uint8_t m_doubleBits = 0;
  bool m_initialized = false;
};
}  // namespace binary
}  // namespace kml
