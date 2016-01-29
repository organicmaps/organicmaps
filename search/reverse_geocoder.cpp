#include "reverse_geocoder.hpp"
#include "search_string_utils.hpp"

#include "search/v2/house_to_street_table.hpp"
#include "search/v2/mwm_context.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/stl_helpers.hpp"

#include "std/limits.hpp"

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

void ReverseGeocoder::GetNearbyStreets(MwmSet::MwmId const & id, m2::PointD const & center,
                                       vector<Street> & streets) const
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addStreet = [&](FeatureType & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE ||
        !ftypes::IsStreetChecker::Instance()(ft))
    {
      return;
    }

    string name;
    static int8_t const lang = StringUtf8Multilang::GetLangIndex("default");
    if (!ft.GetName(lang, name))
      return;

    ASSERT(!name.empty(), ());

    streets.push_back({ft.GetID(), feature::GetMinDistanceMeters(ft, center), name});
  };

  MwmSet::MwmHandle mwmHandle = m_index.GetMwmHandleById(id);
  if (mwmHandle.IsAlive())
  {
    search::v2::MwmContext(move(mwmHandle)).ForEachFeature(rect, addStreet);
    sort(streets.begin(), streets.end(), my::CompareBy(&Street::m_distanceMeters));
  }
}

void ReverseGeocoder::GetNearbyStreets(FeatureType & ft, vector<Street> & streets) const
{
  ASSERT(ft.GetID().IsValid(), ());
  GetNearbyStreets(ft.GetID().m_mwmId, feature::GetCenter(ft), streets);
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

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, Address & addr) const
{
  vector<Building> buildings;
  GetNearbyBuildings(center, buildings);

  vector<Street> streets;
  unique_ptr<search::v2::HouseToStreetTable> table;
  MwmSet::MwmHandle mwmHandle;

  int triesCount = 0;
  for (auto const & b : buildings)
  {
    if (!table || mwmHandle.GetId() != b.m_id.m_mwmId)
    {
      mwmHandle = m_index.GetMwmHandleById(b.m_id.m_mwmId);
      if (!mwmHandle.IsAlive())
        continue;
      table = search::v2::HouseToStreetTable::Load(*mwmHandle.GetValue<MwmValue>());
    }

    GetNearbyStreets(b.m_id.m_mwmId, b.m_center, streets);

    uint32_t ind;
    if (table->Get(b.m_id.m_index, ind) && ind < streets.size())
    {
      addr.m_building = b;
      addr.m_street = streets[ind];
      return;
    }

    // Do not analyze more than 5 houses to get exact address.
    if (++triesCount == 5)
      return;
  }
}

pair<vector<ReverseGeocoder::Street>, uint32_t>
ReverseGeocoder::GetNearbyFeatureStreets(FeatureType const & feature) const
{
  pair<vector<ReverseGeocoder::Street>, uint32_t> result;
  auto & streetIndex = result.second;
  streetIndex = numeric_limits<uint32_t>::max();

  FeatureID const fid = feature.GetID();
  MwmSet::MwmHandle const mwmHandle = m_index.GetMwmHandleById(fid.m_mwmId);
  if (!mwmHandle.IsAlive())
  {
    LOG(LWARNING, ("MWM for", feature, "is dead"));
    return result;
  }

  auto & streets = result.first;
  GetNearbyStreets(const_cast<FeatureType &>(feature), streets);

  unique_ptr<search::v2::HouseToStreetTable> const table =
      search::v2::HouseToStreetTable::Load(*mwmHandle.GetValue<MwmValue>());

  if (table->Get(fid.m_index, streetIndex) && streetIndex >= streets.size())
    LOG(LWARNING, ("Out of bound index", streetIndex, "for", feature));
  return result;
}

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, vector<Building> & buildings) const
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addBuilding = [&](FeatureType const & ft)
  {
    if (!ftypes::IsBuildingChecker::Instance()(ft))
      return;

    // Skip empty house numbers.
    string const number = ft.GetHouseNumber();
    if (number.empty())
      return;

    buildings.push_back({ft.GetID(), feature::GetMinDistanceMeters(ft, center),
                         number, feature::GetCenter(ft)});
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
