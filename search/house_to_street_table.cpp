#include "search/house_to_street_table.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/fixed_bits_ddvector.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"

#include "defines.hpp"

namespace search
{
namespace
{
class Fixed3BitsTable : public HouseToStreetTable
{
public:
  using TVector = FixedBitsDDVector<3, ModelReaderPtr>;

  Fixed3BitsTable(MwmValue & value)
    : m_vector(TVector::Create(value.m_cont.GetReader(SEARCH_ADDRESS_FILE_TAG)))
  {
    ASSERT(m_vector.get(), ("Can't instantiate FixedBitsDDVector."));
  }

  // HouseToStreetTable overrides:
  bool Get(uint32_t houseId, uint32_t & streetIndex) const override
  {
    return m_vector->Get(houseId, streetIndex);
  }

private:
  unique_ptr<TVector> m_vector;
};

class DummyTable : public HouseToStreetTable
{
public:
  // HouseToStreetTable overrides:
  bool Get(uint32_t /* houseId */, uint32_t & /* streetIndex */) const override { return false; }
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
      result.reset(new Fixed3BitsTable(value));
  }
  catch (Reader::OpenException const & ex)
  {
    LOG(LWARNING, (ex.Msg()));
  }

  if (!result)
    result.reset(new DummyTable());
  return result;
}

}  // namespace search
