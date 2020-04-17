#include "search/house_to_street_table.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/files_container.hpp"
#include "coding/fixed_bits_ddvector.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <vector>

using namespace std;

namespace search
{
namespace
{
class Fixed3BitsTable : public HouseToStreetTable
{
public:
  using Vector = FixedBitsDDVector<3, ModelReaderPtr>;

  explicit Fixed3BitsTable(MwmValue const & value)
    : m_vector(Vector::Create(value.m_cont.GetReader(SEARCH_ADDRESS_FILE_TAG)))
  {
    ASSERT(m_vector.get(), ("Can't instantiate Fixed3BitsDDVector."));
  }

  // HouseToStreetTable overrides:
  bool Get(uint32_t houseId, uint32_t & streetIndex) const override
  {
    return m_vector->Get(houseId, streetIndex);
  }

  StreetIdType GetStreetIdType() const override { return StreetIdType::Index; }

private:
  unique_ptr<Vector> m_vector;
};

class EliasFanoMap : public HouseToStreetTable
{
public:
  using Map = MapUint32ToValue<uint32_t>;

  explicit EliasFanoMap(unique_ptr<Reader> && reader) : m_reader(move(reader))
  {
    ASSERT(m_reader, ());
    auto readBlockCallback = [](auto & source, uint32_t blockSize, vector<uint32_t> & values) {
      CHECK_GREATER(blockSize, 0, ());
      values.resize(blockSize);
      values[0] = ReadVarUint<uint32_t>(source);

      for (size_t i = 1; i < blockSize && source.Size() > 0; ++i)
      {
        // Feature ids for all real features are less than numeric_limits<int32_t>::max()
        // so we can use delta coding with int32_t difference type.
        auto const delta = ReadVarInt<int32_t>(source);
        values[i] = base::asserted_cast<uint32_t>(values[i - 1] + delta);
      }
    };

    m_map = Map::Load(*m_reader, readBlockCallback);
    ASSERT(m_map.get(), ("Can't instantiate MapUint32ToValue<uint32_t>."));
  }

  // HouseToStreetTable overrides:
  bool Get(uint32_t houseId, uint32_t & streetIndex) const override
  {
    return m_map->Get(houseId, streetIndex);
  }

  StreetIdType GetStreetIdType() const override { return StreetIdType::FeatureId; }

private:
  unique_ptr<Reader> m_reader;
  unique_ptr<Map> m_map;
};

class DummyTable : public HouseToStreetTable
{
public:
  // HouseToStreetTable overrides:
  bool Get(uint32_t /* houseId */, uint32_t & /* streetIndex */) const override { return false; }
  StreetIdType GetStreetIdType() const override { return StreetIdType::None; }
};
}  // namespace

// HouseToStreetTable::Header ---------------------------------------------------------------------
void HouseToStreetTable::Header::Read(Reader & reader)
{
  NonOwningReaderSource source(reader);
  m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V2), ());
  m_tableOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_tableSize = ReadPrimitiveFromSource<uint32_t>(source);
}

// HouseToStreetTable ------------------------------------------------------------------------------
// static
unique_ptr<HouseToStreetTable> HouseToStreetTable::Load(MwmValue const & value)
{
  version::MwmTraits traits(value.GetMwmVersion());
  auto const format = traits.GetHouseToStreetTableFormat();

  unique_ptr<HouseToStreetTable> result;

  try
  {
    if (format == version::MwmTraits::HouseToStreetTableFormat::Fixed3BitsDDVector)
    {
      result = make_unique<Fixed3BitsTable>(value);
    }
    if (format == version::MwmTraits::HouseToStreetTableFormat::EliasFanoMap)
    {
      FilesContainerR::TReader reader = value.m_cont.GetReader(SEARCH_ADDRESS_FILE_TAG);
      ASSERT(reader.GetPtr(), ("Can't get", SEARCH_ADDRESS_FILE_TAG, "section reader."));
      auto subreader = reader.GetPtr()->CreateSubReader(0, reader.Size());
      CHECK(subreader, ());
      result = make_unique<EliasFanoMap>(move(subreader));
    }
    if (format == version::MwmTraits::HouseToStreetTableFormat::HouseToStreetTableWithHeader)
    {
      FilesContainerR::TReader reader = value.m_cont.GetReader(SEARCH_ADDRESS_FILE_TAG);
      ASSERT(reader.GetPtr(), ("Can't get", SEARCH_ADDRESS_FILE_TAG, "section reader."));

      Header header;
      header.Read(*reader.GetPtr());
      CHECK(header.m_version == Version::V2, (base::Underlying(header.m_version)));

      auto subreader = reader.GetPtr()->CreateSubReader(header.m_tableOffset, header.m_tableSize);
      CHECK(subreader, ());
      result = make_unique<EliasFanoMap>(move(subreader));
    }
  }
  catch (Reader::OpenException const & ex)
  {
    LOG(LWARNING, (ex.Msg()));
  }

  if (!result)
    result = make_unique<DummyTable>();
  return result;
}

// HouseToStreetTableBuilder -----------------------------------------------------------------------
void HouseToStreetTableBuilder::Put(uint32_t houseId, uint32_t streetId)
{
  m_builder.Put(houseId, streetId);
}

void HouseToStreetTableBuilder::Freeze(Writer & writer) const
{
  size_t const startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  HouseToStreetTable::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = static_cast<uint64_t>(writer.Pos());
  coding::WritePadding(writer, bytesWritten);

  // Each street id is encoded as delta from some prediction.
  // First street id in the block encoded as VarUint, all other street ids in the block
  // encoded as VarInt delta from previous id
  auto const writeBlockCallback = [](auto & w, auto begin, auto end) {
    CHECK(begin != end, ("MapUint32ToValueBuilder should guarantee begin != end."));
    WriteVarUint(w, *begin);
    auto prevIt = begin;
    for (auto it = begin + 1; it != end; ++it)
    {
      int32_t const delta = base::asserted_cast<int32_t>(*it) - *prevIt;
      WriteVarInt(w, delta);
      prevIt = it;
    }
  };

  header.m_tableOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  m_builder.Freeze(writer, writeBlockCallback);
  header.m_tableSize =
      base::asserted_cast<uint32_t>(writer.Pos() - header.m_tableOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}
}  // namespace search
