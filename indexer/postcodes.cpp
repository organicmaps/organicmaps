#include "indexer/postcodes.hpp"

#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <type_traits>

using namespace std;

namespace indexer
{
void Postcodes::Header::Read(Reader & reader)
{
  static_assert(is_same<underlying_type_t<Version>, uint8_t>::value, "");
  NonOwningReaderSource source(reader);
  m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_EQUAL(base::Underlying(m_version), base::Underlying(Version::V0), ());
  m_stringsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_stringsSize = ReadPrimitiveFromSource<uint32_t>(source);
  m_postcodesMapOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_postcodesMapSize = ReadPrimitiveFromSource<uint32_t>(source);
}

bool Postcodes::Get(uint32_t id, std::string & postcode)
{
  uint32_t postcodeId;
  if (!m_map->Get(id, postcodeId))
    return false;

  CHECK_LESS_OR_EQUAL(postcodeId, m_strings.GetNumStrings(), ());
  postcode = m_strings.ExtractString(*m_stringsSubreader, postcodeId);
  return true;
}

// static
unique_ptr<Postcodes> Postcodes::Load(Reader & reader)
{
  auto postcodes = make_unique<Postcodes>();
  postcodes->m_version = Version::V0;

  Header header;
  header.Read(reader);

  postcodes->m_stringsSubreader =
      reader.CreateSubReader(header.m_stringsOffset, header.m_stringsSize);
  if (!postcodes->m_stringsSubreader)
    return {};
  postcodes->m_strings.InitializeIfNeeded(*postcodes->m_stringsSubreader);

  postcodes->m_mapSubreader =
      reader.CreateSubReader(header.m_postcodesMapOffset, header.m_postcodesMapSize);
  if (!postcodes->m_mapSubreader)
    return {};

  // Decodes block encoded by writeBlockCallback from PostcodesBuilder::Freeze.
  auto const readBlockCallback = [&](NonOwningReaderSource & source, uint32_t blockSize,
                                     vector<uint32_t> & values) {
    // We may have some unused values it the tail of the last block but it's ok because
    // default block size is 64.
    values.resize(blockSize);
    for (size_t i = 0; i < blockSize && source.Size() > 0; ++i)
      values[i] = ReadVarUint<uint32_t>(source);
  };

  postcodes->m_map = Map::Load(*postcodes->m_mapSubreader, readBlockCallback);
  if (!postcodes->m_map)
    return {};

  return postcodes;
}

// PostcodesBuilder -----------------------------------------------------------------------------
void PostcodesBuilder::Put(uint32_t featureId, std::string const & postcode)
{
  uint32_t postcodeId = 0;
  auto const it = m_postcodeToId.find(postcode);
  if (it != m_postcodeToId.end())
  {
    postcodeId = it->second;
    CHECK_LESS_OR_EQUAL(postcodeId, m_postcodeToId.size(), ());
    CHECK_EQUAL(m_idToPostcode.count(postcodeId), 1, ());
  }
  else
  {
    postcodeId = base::asserted_cast<uint32_t>(m_postcodeToId.size());
    m_postcodeToId[postcode] = postcodeId;
    CHECK(m_idToPostcode.emplace(postcodeId, postcode).second, ());
    CHECK_EQUAL(m_idToPostcode.size(), m_postcodeToId.size(), ());
  }
  m_builder.Put(featureId, postcodeId);
}

void PostcodesBuilder::Freeze(Writer & writer) const
{
  size_t startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  Postcodes::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_stringsOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  {
    coding::BlockedTextStorageWriter<decltype(writer)> stringsWriter(writer, 1000 /* blockSize */);
    for (size_t i = 0; i < m_idToPostcode.size(); ++i)
    {
      auto const it = m_idToPostcode.find(base::asserted_cast<uint32_t>(i));
      CHECK(it != m_idToPostcode.end(), ());
      stringsWriter.Append(it->second);
    }
    // stringsWriter destructor writes strings section index right after strings.
  }

  header.m_stringsSize =
      base::asserted_cast<uint32_t>(writer.Pos() - header.m_stringsOffset - startOffset);
  bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_postcodesMapOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);

  auto const writeBlockCallback = [](auto & w, auto begin, auto end) {
    for (auto it = begin; it != end; ++it)
      WriteVarUint(w, *it);
  };
  m_builder.Freeze(writer, writeBlockCallback);

  header.m_postcodesMapSize =
      base::asserted_cast<uint32_t>(writer.Pos() - header.m_postcodesMapOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}
}  // namespace indexer
