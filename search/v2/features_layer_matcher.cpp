#include "search/v2/features_layer_matcher.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

namespace search
{
namespace v2
{
FeaturesLayerMatcher::FeaturesLayerMatcher(Index & index, MwmSet::MwmId const & mwmId,
                                           MwmValue & value, FeaturesVector const & featuresVector)
  : m_mwmId(mwmId)
  , m_reverseGeocoder(index)
  , m_houseToStreetTable(HouseToStreetTable::Load(value))
  , m_featuresVector(featuresVector)
  , m_loader(value, featuresVector, scales::GetUpperScale(), ReverseGeocoder::kLookupRadiusM)
{
  ASSERT(m_houseToStreetTable.get(), ("Can't load HouseToStreetTable"));
}
}  // namespace v2
}  // namespace search
