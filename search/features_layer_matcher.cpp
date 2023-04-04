#include "search/features_layer_matcher.hpp"

#include "search/house_to_street_table.hpp"
#include "search/reverse_geocoder.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include <string>

using namespace std;

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
  /// @todo But what if houseId was edited? Should we check it like in GetMatchingStreet(FeatureType) ?
  /// I think, we should implement more robust logic with invalidating all caches when editor was invoked.
  auto entry = m_matchingStreetsCache.Get(houseId);
  if (!entry.second)
    return entry.first;

  auto feature = GetByIndex(houseId);
  if (!feature)
    return kInvalidId;

  return GetMatchingStreet(*feature);
}

FeaturesLayerMatcher::Streets const & FeaturesLayerMatcher::GetNearbyStreets(FeatureType & feature)
{
  auto entry = m_nearbyStreetsCache.Get(feature.GetID().m_index);
  if (!entry.second)
    return entry.first;

  auto & streets = entry.first;
  m_reverseGeocoder.GetNearbyStreets(feature, streets);

  return streets;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(FeatureType & houseFeature)
{
  // Check if this feature is modified - the logic will be different.
  string streetName;
  bool const edited = osm::Editor::Instance().GetEditedFeatureStreet(houseFeature.GetID(), streetName);

  // Check the cached result value.
  auto entry = m_matchingStreetsCache.Get(houseFeature.GetID().m_index);
  if (!edited && !entry.second)
    return entry.first;

  uint32_t & result = entry.first;
  result = kInvalidId;

  FeatureID streetId;
  if (!edited && m_reverseGeocoder.GetOriginalStreetByHouse(houseFeature, streetId))
  {
    result = streetId.m_index;
    return result;
  }

  // Get nearby streets and calculate the resulting index.
  auto const & streets = GetNearbyStreets(houseFeature);

  if (edited)
  {
    auto const ret = find_if(streets.begin(), streets.end(),
                             [&streetName](Street const & st) { return st.m_name == streetName; });
    if (ret != streets.end())
    {
      result = ret->m_id.m_index;
      return result;
    }
  }

  // If there is no saved street for feature, assume that it's a nearest street if it's too close.
  if (!streets.empty() && streets[0].m_distanceMeters < kMaxApproxStreetDistanceM)
    result = streets[0].m_id.m_index;

  return result;
}
}  // namespace search
