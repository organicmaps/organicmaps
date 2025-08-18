#include "search/features_layer_matcher.hpp"

#include "search/house_to_street_table.hpp"
#include "search/reverse_geocoder.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include <string>

namespace search
{
using namespace std;

/// Max distance from house to street where we do search matching
/// even if there is no exact street written for this house.
int constexpr kMaxApproxStreetDistanceM = 100;

FeaturesLayerMatcher::FeaturesLayerMatcher(DataSource const & dataSource, base::Cancellable const & cancellable)
  : m_context(nullptr)
  , m_postcodes(nullptr)
  , m_reverseGeocoder(dataSource)
  , m_nearbyStreetsCache("FeatureToNearbyStreets")
  , m_matchingStreetsCache("BuildingToStreet")
  , m_place2address("PlaceToAddresses")
  , m_loader(scales::GetUpperScale(), ReverseGeocoder::kLookupRadiusM)
  , m_cancellable(cancellable)
{}

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
  m_place2address.ClearIfNeeded();

  m_loader.OnQueryFinished();
}

std::vector<uint32_t> const & FeaturesLayerMatcher::GetPlaceAddrFeatures(uint32_t placeId,
                                                                         std::function<CBV()> const & fn)
{
  ASSERT(fn, ());

  auto const res = m_place2address.Get(placeId);
  if (res.second)
  {
    auto & value = m_context->m_value;
    if (!value.m_house2place)
      value.m_house2place = LoadHouseToPlaceTable(value);

    fn().ForEach([&](uint32_t fid)
    {
      auto const r = value.m_house2place->Get(fid);
      if (r && r->m_streetId == placeId)
        res.first.push_back(fid);
    });

    ASSERT(base::IsSortedAndUnique(res.first), ());
  }
  return res.first;
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(FeatureID const & houseId)
{
  std::unique_ptr<FeatureType> feature;
  return GetMatchingStreetImpl(houseId, [&]()
  {
    feature = GetByIndex(houseId.m_index);
    return feature.get();
  });
}

uint32_t FeaturesLayerMatcher::GetMatchingStreet(FeatureType & feature)
{
  return GetMatchingStreetImpl(feature.GetID(), [&]() { return &feature; });
}

FeaturesLayerMatcher::Streets const & FeaturesLayerMatcher::GetNearbyStreets(FeatureType & feature)
{
  auto entry = m_nearbyStreetsCache.Get(feature.GetID().m_index);
  if (!entry.second)
    return entry.first;

  entry.first = m_reverseGeocoder.GetNearbyStreets(feature);
  return entry.first;
}

template <class FeatureGetterT>
uint32_t FeaturesLayerMatcher::GetMatchingStreetImpl(FeatureID const & id, FeatureGetterT && getter)
{
  // Check if this feature is modified - the logic will be different.
  string streetName;
  bool const edited = osm::Editor::Instance().GetEditedFeatureStreet(id, streetName);

  // Check the cached result value.
  auto entry = m_matchingStreetsCache.Get(id.m_index);
  if (!edited && !entry.second)
    return entry.first;

  uint32_t & result = entry.first;
  result = kInvalidId;

  FeatureType * pFeature = getter();
  if (pFeature == nullptr)
    return result;

  FeatureID streetId;
  if (!edited && m_reverseGeocoder.GetOriginalStreetByHouse(*pFeature, streetId))
  {
    result = streetId.m_index;
    return result;
  }

  // Get nearby streets and calculate the resulting index.
  auto const & streets = GetNearbyStreets(*pFeature);

  if (edited)
  {
    auto const ret =
        find_if(streets.begin(), streets.end(), [&streetName](Street const & st) { return st.m_name == streetName; });
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
