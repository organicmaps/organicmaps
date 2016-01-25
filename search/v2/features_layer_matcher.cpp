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
  , m_nearbyStreetsCache("F2NS")
  , m_matchingStreetsCache("B2S")
  , m_loader(scales::GetUpperScale(), ReverseGeocoder::kLookupRadiusM)
  , m_cancellable(cancellable)
{
}

void FeaturesLayerMatcher::InitContext(MwmContext * context)
{
  m_context = context;
  m_houseToStreetTable = HouseToStreetTable::Load(m_context->m_value);
  ASSERT(m_houseToStreetTable, ());
  m_loader.InitContext(context);
}

void FeaturesLayerMatcher::FinishQuery()
{
  m_nearbyStreetsCache.FinishQuery();
  m_matchingStreetsCache.FinishQuery();
  m_loader.FinishQuery();
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId)
{
  auto r = m_matchingStreetsCache.Get(houseId);
  if (!r.second)
    return r.first;

  FeatureType houseFeature;
  GetByIndex(houseId, houseFeature);

  r.first = GetMatchingStreetImpl(houseId, houseFeature);
  return r.first;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature)
{
  auto r = m_matchingStreetsCache.Get(houseId);
  if (!r.second)
    return r.first;

  r.first = GetMatchingStreetImpl(houseId, houseFeature);
  return r.first;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  auto r = m_nearbyStreetsCache.Get(featureId);
  if (!r.second)
    return r.first;

  FeatureType feature;
  GetByIndex(featureId, feature);

  m_reverseGeocoder.GetNearbyStreets(feature::GetCenter(feature), r.first);
  return r.first;
}

vector<ReverseGeocoder::Street> const & FeaturesLayerMatcher::GetNearbyStreets(
    uint32_t featureId, FeatureType & feature)
{
  auto r = m_nearbyStreetsCache.Get(featureId);
  if (!r.second)
    return r.first;

  m_reverseGeocoder.GetNearbyStreets(feature::GetCenter(feature), r.first);
  return r.first;
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
