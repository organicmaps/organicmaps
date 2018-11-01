#include "search/features_layer_matcher.hpp"

#include "search/house_to_street_table.hpp"
#include "search/reverse_geocoder.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

namespace search
{
/// Max distance from house to street where we do search matching
/// even if there is no exact street written for this house.
int constexpr kMaxApproxStreetDistanceM = 100;

FeaturesLayerMatcher::FeaturesLayerMatcher(DataSource const & dataSource,
                                           base::Cancellable const & cancellable)
  : m_context(nullptr)
  , m_postcodes(nullptr)
  , m_reverseGeocoder(dataSource)
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
  m_loader.SetContext(context);
}

void FeaturesLayerMatcher::SetPostcodes(CBV const * postcodes)
{
  m_postcodes = postcodes;
}

void FeaturesLayerMatcher::OnQueryFinished()
{
  m_nearbyStreetsCache.ClearIfNeeded();
  m_matchingStreetsCache.ClearIfNeeded();
  m_loader.OnQueryFinished();
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId)
{
  FeatureType feature;
  return GetMatchingStreetImpl(houseId, feature);
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature)
{
  return GetMatchingStreetImpl(houseId, houseFeature);
}

FeaturesLayerMatcher::TStreets const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId)
{
  FeatureType feature;
  return GetNearbyStreetsImpl(featureId, feature);
}

FeaturesLayerMatcher::TStreets const & FeaturesLayerMatcher::GetNearbyStreets(uint32_t featureId,
                                                                              FeatureType & feature)
{
  return GetNearbyStreetsImpl(featureId, feature);
}

FeaturesLayerMatcher::TStreets const & FeaturesLayerMatcher::GetNearbyStreetsImpl(
    uint32_t featureId, FeatureType & feature)
{
  static FeaturesLayerMatcher::TStreets const kEmptyStreets;

  auto entry = m_nearbyStreetsCache.Get(featureId);
  if (!entry.second)
    return entry.first;

  if (!feature.GetID().IsValid() && !GetByIndex(featureId, feature))
    return kEmptyStreets;

  auto & streets = entry.first;
  m_reverseGeocoder.GetNearbyStreets(feature, streets);
  for (size_t i = 0; i < streets.size(); ++i)
  {
    if (streets[i].m_distanceMeters > ReverseGeocoder::kLookupRadiusM)
    {
      streets.resize(i);
      break;
    }
  }

  return streets;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreetImpl(uint32_t houseId, FeatureType & houseFeature)
{
  // Check if this feature is modified - the logic will be different.
  string streetName;
  bool const edited =
      osm::Editor::Instance().GetEditedFeatureStreet(houseFeature.GetID(), streetName);

  // Check the cached result value.
  auto entry = m_matchingStreetsCache.Get(houseId);
  if (!edited && !entry.second)
    return entry.first;

  // Load feature if needed.
  if (!houseFeature.GetID().IsValid() && !GetByIndex(houseId, houseFeature))
    return kInvalidId;

  // Get nearby streets and calculate the resulting index.
  auto const & streets = GetNearbyStreets(houseId, houseFeature);
  uint32_t & result = entry.first;
  result = kInvalidId;

  if (edited)
  {
    auto const ret = find_if(streets.begin(), streets.end(), [&streetName](TStreet const & st)
                             {
                               return st.m_name == streetName;
                             });
    if (ret != streets.end())
      result = ret->m_id.m_index;
  }
  else
  {
    uint32_t index;
    if (m_context->GetStreetIndex(houseId, index) && index < streets.size())
      result = streets[index].m_id.m_index;
  }

  // If there is no saved street for feature, assume that it's a nearest street if it's too close.
  if (result == kInvalidId && !streets.empty() &&
      streets[0].m_distanceMeters < kMaxApproxStreetDistanceM)
  {
    result = streets[0].m_id.m_index;
  }

  return result;
}
}  // namespace search
