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
}  // namespace indexer
