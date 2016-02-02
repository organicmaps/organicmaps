#include "search/v2/features_layer_matcher.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

namespace search
{
namespace v2
{

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

vector<FeaturesLayerMatcher::TStreet> const &
FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  auto entry = m_nearbyStreetsCache.Get(featureId);
  if (!entry.second)
    return entry.first;

  FeatureType feature;
  GetByIndex(featureId, feature);

  GetNearbyStreetsImpl(feature, entry.first);
  return entry.first;
}

vector<FeaturesLayerMatcher::TStreet> const &
FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId, FeatureType & feature)
{
  auto entry = m_nearbyStreetsCache.Get(featureId);
  if (!entry.second)
    return entry.first;

  GetNearbyStreetsImpl(feature, entry.first);
  return entry.first;
}

void FeaturesLayerMatcher::GetNearbyStreetsImpl(FeatureType & feature, vector<TStreet> & streets)
{
  m_reverseGeocoder.GetNearbyStreets(feature, streets);
  for (size_t i = 0; i < streets.size(); ++i)
  {
    if (streets[i].m_distanceMeters > ReverseGeocoder::kLookupRadiusM)
    {
      streets.resize(i);
      return;
    }
  }
}

uint32_t FeaturesLayerMatcher::GetMatchingStreetImpl(uint32_t houseId, FeatureType & houseFeature)
{
  auto const & streets = GetNearbyStreets(houseId, houseFeature);

  uint32_t index;
  if (m_houseToStreetTable->Get(houseId, index) && index < streets.size())
    return streets[index].m_id.m_index;

  // If there is no saved street for feature, assume that it's a nearest street if it's too close.
  if (!streets.empty() && streets[0].m_distanceMeters < 100.0)
    return streets[0].m_id.m_index;

  return kInvalidId;
}

}  // namespace v2
}  // namespace search
