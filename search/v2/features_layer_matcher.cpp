#include "search/v2/features_layer_matcher.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

namespace search
{
namespace v2
{
FeaturesLayerMatcher::FeaturesLayerMatcher(Index & index, MwmSet::MwmId const & mwmId,
                                           MwmValue & value, FeaturesVector const & featuresVector,
                                           my::Cancellable const & cancellable)
  : m_mwmId(mwmId)
  , m_reverseGeocoder(index)
  , m_houseToStreetTable(HouseToStreetTable::Load(value))
  , m_featuresVector(featuresVector)
  , m_loader(value, featuresVector, scales::GetUpperScale(), ReverseGeocoder::kLookupRadiusM)
  , m_cancellable(cancellable)
{
  ASSERT(m_houseToStreetTable.get(), ("Can't load HouseToStreetTable"));
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature)
{
  auto const it = m_matchingStreetsCache.find(houseId);
  if (it != m_matchingStreetsCache.cend())
    return it->second;

  auto const & streets = GetNearbyStreets(houseId, houseFeature);
  uint32_t const streetIndex = m_houseToStreetTable->Get(houseId);

  uint32_t streetId = kInvalidId;
  if (streetIndex < streets.size() && streets[streetIndex].m_id.m_mwmId == m_mwmId)
    streetId = streets[streetIndex].m_id.m_index;
  m_matchingStreetsCache[houseId] = streetId;
  return streetId;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  auto const it = m_nearbyStreetsCache.find(featureId);
  if (it != m_nearbyStreetsCache.cend())
    return it->second;

  FeatureType feature;
  m_featuresVector.GetByIndex(featureId, feature);

  auto & streets = m_nearbyStreetsCache[featureId];
  m_reverseGeocoder.GetNearbyStreets(feature, streets);
  return streets;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(
    uint32_t featureId, FeatureType & feature)
{
  auto const it = m_nearbyStreetsCache.find(featureId);
  if (it != m_nearbyStreetsCache.cend())
    return it->second;

  auto & streets = m_nearbyStreetsCache[featureId];
  m_reverseGeocoder.GetNearbyStreets(feature, streets);
  return streets;
}
}  // namespace v2
}  // namespace search
