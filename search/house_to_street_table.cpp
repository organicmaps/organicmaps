#include "search/house_to_street_table.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/fixed_bits_ddvector.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <vector>

#include "defines.hpp"

using namespace std;

namespace search
{
namespace
{
class Fixed3BitsTable : public HouseToStreetTable
{
public:
  using Vector = FixedBitsDDVector<3, ModelReaderPtr>;

  explicit Fixed3BitsTable(MwmValue & value)
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

  explicit EliasFanoMap(MwmValue & value) : m_reader(unique_ptr<ModelReader>())
  {
    auto const readBlockCallback = [&](NonOwningReaderSource & source, uint32_t blockSize,
                                       vector<uint32_t> & values) {
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

    m_reader = value.m_cont.GetReader(SEARCH_ADDRESS_FILE_TAG);
    ASSERT(m_reader.GetPtr(), ("Can't get", SEARCH_ADDRESS_FILE_TAG, "section reader."));

    m_map = Map::Load(*m_reader.GetPtr(), readBlockCallback);
    ASSERT(m_map.get(), ("Can't instantiate MapUint32ToValue<uint32_t>."));
  }

  // HouseToStreetTable overrides:
  bool Get(uint32_t houseId, uint32_t & streetIndex) const override
  {
    return m_map->Get(houseId, streetIndex);
  }

  StreetIdType GetStreetIdType() const override { return StreetIdType::FeatureId; }

private:
  FilesContainerR::TReader m_reader;
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

unique_ptr<HouseToStreetTable> HouseToStreetTable::Load(MwmValue & value)
{
  version::MwmTraits traits(value.GetMwmVersion());
  auto const format = traits.GetHouseToStreetTableFormat();

  unique_ptr<HouseToStreetTable> result;

  try
  {
    if (format == version::MwmTraits::HouseToStreetTableFormat::Fixed3BitsDDVector)
      result = make_unique<Fixed3BitsTable>(value);
    if (format == version::MwmTraits::HouseToStreetTableFormat::EliasFanoMap)
      result = make_unique<EliasFanoMap>(value);
  }
  catch (Reader::OpenException const & ex)
  {
    LOG(LWARNING, (ex.Msg()));
  }

  if (!result)
    result = make_unique<DummyTable>();
  return result;
}

}  // namespace search
