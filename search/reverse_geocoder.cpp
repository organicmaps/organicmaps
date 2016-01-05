#include "reverse_geocoder.hpp"
#include "search_string_utils.hpp"

#include "search/v2/house_to_street_table.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/stl_helpers.hpp"

namespace search
{
namespace
{
size_t constexpr kSimilarityThresholdPercent = 10;
int const kQueryScale = scales::GetUpperScale();
} // namespace

// static
double const ReverseGeocoder::kLookupRadiusM = 500.0;

ReverseGeocoder::ReverseGeocoder(Index const & index) : m_index(index) {}

void ReverseGeocoder::GetNearbyStreets(FeatureType const & addrFt, vector<Street> & streets)
{
  GetNearbyStreets(feature::GetCenter(addrFt), streets);
}

void ReverseGeocoder::GetNearbyStreets(m2::PointD const & center, vector<Street> & streets)
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addStreet = [&](FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE)
      return;

    if (!ftypes::IsStreetChecker::Instance()(ft))
      return;

    string name;
    static int8_t const lang = StringUtf8Multilang::GetLangIndex("default");
    if (!ft.GetName(lang, name))
      return;

    ASSERT(!name.empty(), ());

    double const distanceM = feature::GetMinDistanceMeters(ft, center);
    if (distanceM > kLookupRadiusM)
      return;

    streets.push_back({ft.GetID(), distanceM, name});
  };

  m_index.ForEachInRect(addStreet, rect, kQueryScale);
  sort(streets.begin(), streets.end(), my::CompareBy(&Street::m_distanceMeters));
}

// static
size_t ReverseGeocoder::GetMatchedStreetIndex(string const & keyName,
                                              vector<Street> const & streets)
{
  strings::UniString const expected = strings::MakeUniString(keyName);

  // Find the exact match or the best match in kSimilarityTresholdPercent limit.
  size_t const count = streets.size();
  size_t result = count;
  size_t minPercent = kSimilarityThresholdPercent + 1;

  for (size_t i = 0; i < count; ++i)
  {
    string key;
    search::GetStreetNameAsKey(streets[i].m_name, key);
    strings::UniString const actual = strings::MakeUniString(key);

    size_t const editDistance =
        strings::EditDistance(expected.begin(), expected.end(), actual.begin(), actual.end());

    if (editDistance == 0)
      return i;

    if (actual.empty())
      continue;

    size_t const percent = editDistance * 100 / actual.size();
    if (percent < minPercent)
    {
      result = i;
      minPercent = percent;
    }
  }

  return result;
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, Address & addr)
{
  vector<Building> buildings;
  GetNearbyBuildings(center, buildings);

  vector<Street> streets;
  unique_ptr<search::v2::HouseToStreetTable> table;
  MwmSet::MwmHandle mwmHandle;

  for (auto const & b : buildings)
  {
    if (!table || mwmHandle.GetId() != b.m_id.m_mwmId)
    {
      mwmHandle = m_index.GetMwmHandleById(b.m_id.m_mwmId);
      if (!mwmHandle.IsAlive())
        continue;
      table = search::v2::HouseToStreetTable::Load(*mwmHandle.GetValue<MwmValue>());
    }

    GetNearbyStreets(b.m_center, streets);

    uint32_t const ind = table->Get(b.m_id.m_index);
    if (ind < streets.size())
    {
      addr.m_building = b;
      addr.m_street = streets[ind];
      return;
    }
  }
}

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, vector<Building> & buildings)
{
  GetNearbyBuildings(center, kLookupRadiusM, buildings);
}

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, double radiusM,
                                         vector<Building> & buildings)
{
  // Seems like a copy-paste here of the GetNearbyStreets function.
  // Trying to factor out common logic will cause many variables logic.

  m2::RectD const rect = GetLookupRect(center, radiusM);

  auto const addBuilding = [&](FeatureType const & ft)
  {
    if (!ftypes::IsBuildingChecker::Instance()(ft))
      return;

    // Skip empty house numbers.
    string const number = ft.GetHouseNumber();
    if (number.empty())
      return;

    double const distanceM = feature::GetMinDistanceMeters(ft, center);
    if (distanceM > radiusM)
      return;

    buildings.push_back({ft.GetID(), distanceM, number, feature::GetCenter(ft)});
  };

  m_index.ForEachInRect(addBuilding, rect, kQueryScale);
  sort(buildings.begin(), buildings.end(), my::CompareBy(&Building::m_distanceMeters));
}

// static
m2::RectD ReverseGeocoder::GetLookupRect(m2::PointD const & center, double radiusM)
{
  return MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
}
} // namespace search
