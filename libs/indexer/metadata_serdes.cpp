#include "indexer/metadata_serdes.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include "defines.hpp"

#include <type_traits>

namespace indexer
{
using namespace std;

void MetadataDeserializer::Header::Read(Reader & reader)
{
  static_assert(is_same<underlying_type_t<Version>, uint8_t>::value, "");
  NonOwningReaderSource source(reader);
  m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_EQUAL(base::Underlying(m_version), base::Underlying(Version::V0), ());
  m_stringsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_stringsSize = ReadPrimitiveFromSource<uint32_t>(source);
  m_metadataMapOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_metadataMapSize = ReadPrimitiveFromSource<uint32_t>(source);
}

bool MetadataDeserializer::Get(uint32_t featureId, feature::MetadataBase & meta)
{
  MetaIds metaIds;
  if (!GetIds(featureId, metaIds))
    return false;

  lock_guard<mutex> guard(m_stringsMutex);
  for (auto const & id : metaIds)
  {
    CHECK_LESS_OR_EQUAL(id.second, m_strings.GetNumStrings(), ());
    meta.Set(id.first, m_strings.ExtractString(*m_stringsSubreader, id.second));
  }
  return true;
}

bool MetadataDeserializer::GetIds(uint32_t featureId, MetaIds & metaIds) const
{
  return m_map->GetThreadsafe(featureId, metaIds);
}

std::string MetadataDeserializer::GetMetaById(uint32_t id)
{
  lock_guard<mutex> guard(m_stringsMutex);
  return m_strings.ExtractString(*m_stringsSubreader, id);
}

// static
unique_ptr<MetadataDeserializer> MetadataDeserializer::Load(Reader & reader)
{
  auto deserializer = make_unique<MetadataDeserializer>();
  deserializer->m_version = Version::V0;

  Header header;
  header.Read(reader);

  deserializer->m_stringsSubreader = reader.CreateSubReader(header.m_stringsOffset, header.m_stringsSize);
  if (!deserializer->m_stringsSubreader)
    return {};
  deserializer->m_strings.InitializeIfNeeded(*deserializer->m_stringsSubreader);

  deserializer->m_mapSubreader = reader.CreateSubReader(header.m_metadataMapOffset, header.m_metadataMapSize);
  if (!deserializer->m_mapSubreader)
    return {};

  // Decodes block encoded by writeBlockCallback from MetadataBuilder::Freeze.
  auto const readBlockCallback = [&](NonOwningReaderSource & source, uint32_t blockSize, vector<MetaIds> & values)
  {
    values.resize(blockSize);
    for (size_t i = 0; i < blockSize && source.Size() > 0; ++i)
    {
      auto const size = ReadVarUint<uint32_t>(source);
      values[i].resize(size);
      CHECK_GREATER(size, 0, ());

      for (auto & value : values[i])
        value.first = ReadPrimitiveFromSource<uint8_t>(source);

      values[i][0].second = ReadVarUint<uint32_t>(source);
      for (size_t j = 1; j < values[i].size(); ++j)
        values[i][j].second = values[i][j - 1].second + ReadVarInt<int32_t>(source);
    }
  };

  deserializer->m_map = Map::Load(*deserializer->m_mapSubreader, readBlockCallback);
  if (!deserializer->m_map)
    return {};

  return deserializer;
}

// static
std::unique_ptr<MetadataDeserializer> MetadataDeserializer::Load(FilesContainerR const & cont)
{
  return Load(*cont.GetReader(METADATA_FILE_TAG).GetPtr());
}

// MetadataBuilder -----------------------------------------------------------------------------
void MetadataBuilder::Put(uint32_t featureId, feature::Metadata const & meta)
{
  MetadataDeserializer::MetaIds metaIds;
  meta.ForEach([&](feature::Metadata::EType type, std::string const & value)
  {
    uint32_t id = 0;
    auto const it = m_stringToId.find(value);
    if (it != m_stringToId.end())
    {
      id = it->second;
      CHECK_LESS_OR_EQUAL(id, m_stringToId.size(), ());
      CHECK_EQUAL(m_idToString.count(id), 1, ());
    }
    else
    {
      id = base::asserted_cast<uint32_t>(m_stringToId.size());
      m_stringToId[value] = id;
      CHECK(m_idToString.emplace(id, value).second, ());
      CHECK_EQUAL(m_idToString.size(), m_stringToId.size(), ());
    }
    metaIds.emplace_back(type, id);
  });

  m_builder.Put(featureId, metaIds);
}

void MetadataBuilder::Freeze(Writer & writer) const
{
  uint64_t const startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  MetadataDeserializer::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_stringsOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  {
    coding::BlockedTextStorageWriter<decltype(writer)> stringsWriter(writer, 1000 /* blockSize */);
    for (size_t i = 0; i < m_idToString.size(); ++i)
    {
      auto const it = m_idToString.find(base::asserted_cast<uint32_t>(i));
      CHECK(it != m_idToString.end(), ());
      stringsWriter.Append(it->second);
    }
    // stringsWriter destructor writes strings section index right after strings.
  }

  header.m_stringsSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_stringsOffset - startOffset);
  bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_metadataMapOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  auto const writeBlockCallback = [](auto & w, auto begin, auto end)
  {
    for (auto it = begin; it != end; ++it)
    {
      MetadataDeserializer::MetaIds const & metaIds = *it;
      CHECK_GREATER(metaIds.size(), 0, ());
      WriteVarUint(w, metaIds.size());
      for (auto const & kv : metaIds)
        WriteToSink(w, kv.first);

      WriteVarUint(w, metaIds[0].second);
      for (size_t i = 1; i < it->size(); ++i)
        WriteVarInt(w, static_cast<int32_t>(metaIds[i].second) - static_cast<int32_t>(metaIds[i - 1].second));
    }
  };
  m_builder.Freeze(writer, writeBlockCallback);

  header.m_metadataMapSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_metadataMapOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}
}  // namespace indexer
