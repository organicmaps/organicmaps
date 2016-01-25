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

FeaturesLayerMatcher::FeaturesLayerMatcher(Index & index, my::Cancellable const & cancellable)
  : m_context(nullptr)
  , m_reverseGeocoder(index)
  , m_nearbyStreetsCache("FeatureToNearbyStreets")
  , m_matchingStreetsCache("BuildingToStreet")
  , m_loader(scales::GetUpperScale(), ReverseGeocoder::kLookupRadiusM)
  , m_cancellable(cancellable)
{
}

void FeaturesLayerMatcher::SetContext(MwmContext * context)
{
  ASSERT(context, ());
  if (m_context == context)
    return;

  m_context = context;
  m_houseToStreetTable = HouseToStreetTable::Load(m_context->m_value);
  ASSERT(m_houseToStreetTable, ());
  m_loader.SetContext(context);
}

void FeaturesLayerMatcher::OnQueryFinished()
{
  m_nearbyStreetsCache.ClearIfNeeded();
  m_matchingStreetsCache.ClearIfNeeded();
  m_loader.OnQueryFinished();
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId)
{
  auto entry = m_matchingStreetsCache.Get(houseId);
  if (!entry.second)
    return entry.first;

  FeatureType houseFeature;
  GetByIndex(houseId, houseFeature);

  entry.first = GetMatchingStreetImpl(houseId, houseFeature);
  return entry.first;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature)
{
  auto entry = m_matchingStreetsCache.Get(houseId);
  if (!entry.second)
    return entry.first;

  entry.first = GetMatchingStreetImpl(houseId, houseFeature);
  return entry.first;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  auto entry = m_nearbyStreetsCache.Get(featureId);
  if (!entry.second)
    return entry.first;

  FeatureType feature;
  GetByIndex(featureId, feature);

  m_reverseGeocoder.GetNearbyStreets(feature::GetCenter(feature), entry.first);
  return entry.first;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(
    uint32_t featureId, FeatureType & feature)
{
  auto entry = m_nearbyStreetsCache.Get(featureId);
  if (!entry.second)
    return entry.first;

  m_reverseGeocoder.GetNearbyStreets(feature::GetCenter(feature), entry.first);
  return entry.first;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreetImpl(uint32_t houseId, FeatureType & houseFeature)
{
  auto const & streets = GetNearbyStreets(houseId, houseFeature);

  uint32_t streetId = kInvalidId;
  uint32_t streetIndex;
  if (!m_houseToStreetTable->Get(houseId, streetIndex))
    streetIndex = streets.size();

  if (streetIndex < streets.size() && streets[streetIndex].m_id.m_mwmId == m_context->m_id)
    streetId = streets[streetIndex].m_id.m_index;
  return streetId;
}

}  // namespace v2
}  // namespace search
