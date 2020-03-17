#include "indexer/meta_idx.hpp"

#include "indexer/feature_processor.hpp"

#include "coding/endianness.hpp"
#include "coding/files_container.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <unordered_map>

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

using namespace std;

namespace feature
{
// MetadataIndex::Header ----------------------------------------------------------------------
void MetadataIndex::Header::Read(Reader & reader)
{
  NonOwningReaderSource source(reader);
  m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V0), ());
  m_indexOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_indexSize = ReadPrimitiveFromSource<uint32_t>(source);
}

// MetadataIndex ------------------------------------------------------------------------------
bool MetadataIndex::Get(uint32_t id, uint32_t & offset) const
{
  return m_map->GetThreadsafe(id, offset);
}

// static
unique_ptr<MetadataIndex> MetadataIndex::Load(Reader & reader)
{
  Header header;
  header.Read(reader);

  CHECK_EQUAL(header.m_version, MetadataIndex::Version::V0, ());

  auto subreader = reader.CreateSubReader(header.m_indexOffset, header.m_indexSize);
  if (!subreader)
    return {};

  auto table = make_unique<MetadataIndex>();
  if (!table->Init(move(subreader)))
    return {};

  return table;
}

bool MetadataIndex::Init(unique_ptr<Reader> reader)
{
  m_indexSubreader = move(reader);

  // Decodes block encoded by writeBlockCallback from MetadataIndexBuilder::Freeze.
  auto const readBlockCallback = [&](NonOwningReaderSource & source, uint32_t blockSize,
                                     vector<uint32_t> & values) {
    ASSERT_NOT_EQUAL(blockSize, 0, ());
    values.resize(blockSize);
    values[0] = ReadVarUint<uint32_t>(source);
    for (size_t i = 1; i < blockSize && source.Size() > 0; ++i)
    {
      auto const delta = ReadVarUint<uint32_t>(source);
      values[i] = values[i - 1] + delta;
    }
  };

  m_map = Map::Load(*m_indexSubreader, readBlockCallback);
  return m_map != nullptr;
}

// MetadataIndexBuilder -----------------------------------------------------------------------
void MetadataIndexBuilder::Put(uint32_t featureId, uint32_t offset)
{
  m_builder.Put(featureId, offset);
}

void MetadataIndexBuilder::Freeze(Writer & writer) const
{
  size_t startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  MetadataIndex::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  auto const writeBlockCallback = [](auto & w, auto begin, auto end) {
    WriteVarUint(w, *begin);
    auto prevIt = begin;
    for (auto it = begin + 1; it != end; ++it)
    {
      CHECK_GREATER_OR_EQUAL(*it, *prevIt, ());
      WriteVarUint(w, *it - *prevIt);
      prevIt = it;
    }
  };

  header.m_indexOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  m_builder.Freeze(writer, writeBlockCallback);
  header.m_indexSize =
      base::asserted_cast<uint32_t>(writer.Pos() - header.m_indexOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}

std::string DebugPrint(MetadataIndex::Version v)
{
  CHECK(v == MetadataIndex::Version::V0, (base::Underlying(v)));
  return "V0";
}
}  // namespace feature
