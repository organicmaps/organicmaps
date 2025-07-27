#include "search/house_to_street_table.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/files_container.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <vector>

namespace search
{
using namespace std;

namespace
{
class EliasFanoMap : public HouseToStreetTable
{
public:
  using Map = MapUint32ToValue<uint32_t>;

  explicit EliasFanoMap(unique_ptr<Reader> && reader) : m_reader(std::move(reader))
  {
    ASSERT(m_reader, ());
    auto readBlockCallback = [](auto & source, uint32_t blockSize, vector<uint32_t> & values)
    {
      values.resize(blockSize);
      values[0] = ReadVarUint<uint32_t>(source);

      for (size_t i = 1; i < blockSize && source.Size() > 0; ++i)
      {
        // Feature ids for all real features are less than numeric_limits<int32_t>::max()
        // so we can use delta coding with int32_t difference type.
        values[i] = base::asserted_cast<uint32_t>(values[i - 1] + ReadVarInt<int32_t>(source));
      }
    };

    m_map = Map::Load(*m_reader, readBlockCallback);
    ASSERT(m_map.get(), ());
  }

  // HouseToStreetTable overrides:
  std::optional<Result> Get(uint32_t houseId) const override
  {
    uint32_t fID;
    if (!m_map->Get(houseId, fID))
      return {};
    return {{fID, StreetIdType::FeatureId}};
  }

private:
  unique_ptr<Reader> m_reader;
  unique_ptr<Map> m_map;
};

class DummyTable : public HouseToStreetTable
{
public:
  // HouseToStreetTable overrides:
  std::optional<Result> Get(uint32_t /* houseId */) const override { return {}; }
};

unique_ptr<HouseToStreetTable> LoadHouseTableImpl(MwmValue const & value, std::string const & tag)
{
  unique_ptr<HouseToStreetTable> result;

  try
  {
    auto const format = version::MwmTraits(value.GetMwmVersion()).GetHouseToStreetTableFormat();
    CHECK_EQUAL(format, version::MwmTraits::HouseToStreetTableFormat::HouseToStreetTableWithHeader, ());

    FilesContainerR::TReader reader = value.m_cont.GetReader(tag);

    HouseToStreetTable::Header header;
    ReaderSource source(reader);
    header.Read(source);
    CHECK(header.m_version == HouseToStreetTable::Version::V2, ());

    auto subreader = reader.GetPtr()->CreateSubReader(header.m_tableOffset, header.m_tableSize);
    CHECK(subreader, ());
    result = make_unique<EliasFanoMap>(std::move(subreader));
  }
  catch (Reader::OpenException const & ex)
  {
    LOG(LERROR, (ex.Msg()));
  }

  if (!result)
    result = make_unique<DummyTable>();
  return result;
}
}  // namespace

std::unique_ptr<HouseToStreetTable> LoadHouseToStreetTable(MwmValue const & value)
{
  return LoadHouseTableImpl(value, FEATURE2STREET_FILE_TAG);
}

std::unique_ptr<HouseToStreetTable> LoadHouseToPlaceTable(MwmValue const & value)
{
  return LoadHouseTableImpl(value, FEATURE2PLACE_FILE_TAG);
}

// HouseToStreetTableBuilder -----------------------------------------------------------------------
void HouseToStreetTableBuilder::Put(uint32_t houseId, uint32_t streetId)
{
  m_builder.Put(houseId, streetId);
}

void HouseToStreetTableBuilder::Freeze(Writer & writer) const
{
  uint64_t const startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  HouseToStreetTable::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  // Each street id is encoded as delta from some prediction.
  // First street id in the block encoded as VarUint, all other street ids in the block
  // encoded as VarInt delta from previous id
  auto const writeBlockCallback = [](auto & w, auto begin, auto end)
  {
    CHECK(begin != end, ());
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
  header.m_tableSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_tableOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}
}  // namespace search
