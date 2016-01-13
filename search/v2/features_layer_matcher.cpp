#include "search/v2/features_layer_matcher.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

namespace search
{
namespace v2
{
// static
double const FeaturesLayerMatcher::kBuildingRadiusMeters = 50;

FeaturesLayerMatcher::FeaturesLayerMatcher(Index & index, MwmContext & context,
                                           my::Cancellable const & cancellable)
  : m_context(context)
  , m_reverseGeocoder(index)
  , m_houseToStreetTable(HouseToStreetTable::Load(m_context.m_value))
  , m_loader(context.m_value, context.m_vector, scales::GetUpperScale(),
             ReverseGeocoder::kLookupRadiusM)
  , m_cancellable(cancellable)
{
  ASSERT(m_houseToStreetTable.get(), ("Can't load HouseToStreetTable"));
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId)
{
  auto const it = m_matchingStreetsCache.find(houseId);
  if (it != m_matchingStreetsCache.cend())
    return it->second;

  FeatureType houseFeature;
  m_context.m_vector.GetByIndex(houseId, houseFeature);

  return GetMatchingStreetImpl(houseId, houseFeature);
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature)
{
  auto const it = m_matchingStreetsCache.find(houseId);
  if (it != m_matchingStreetsCache.cend())
    return it->second;

  return GetMatchingStreetImpl(houseId, houseFeature);
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  auto const it = m_nearbyStreetsCache.find(featureId);
  if (it != m_nearbyStreetsCache.cend())
    return it->second;

  FeatureType feature;
  m_context.m_vector.GetByIndex(featureId, feature);

  return GetNearbyStreetsImpl(featureId, feature);
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(
    uint32_t featureId, FeatureType & feature)
{
  auto const it = m_nearbyStreetsCache.find(featureId);
  if (it != m_nearbyStreetsCache.cend())
    return it->second;

  return GetNearbyStreetsImpl(featureId, feature);
}

uint32_t FeaturesLayerMatcher::GetMatchingStreetImpl(uint32_t houseId, FeatureType & houseFeature)
{
  auto const & streets = GetNearbyStreets(houseId, houseFeature);

  uint32_t streetId = kInvalidId;
  uint32_t streetIndex;
  if (!m_houseToStreetTable->Get(houseId, streetIndex))
    streetIndex = streets.size();

  if (streetIndex < streets.size() && streets[streetIndex].m_id.m_mwmId == m_context.m_id)
    streetId = streets[streetIndex].m_id.m_index;
  m_matchingStreetsCache[houseId] = streetId;
  return streetId;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreetsImpl(
    uint32_t featureId, FeatureType & feature)
{
  auto & streets = m_nearbyStreetsCache[featureId];
  m_reverseGeocoder.GetNearbyStreets(feature::GetCenter(feature), streets);
  return streets;
}
}  // namespace v2
}  // namespace search
