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

pair<vector<ReverseGeocoder::Street>, uint32_t>
ReverseGeocoder::GetNearbyFeatureStreets(FeatureType const & ft) const
{
  pair<vector<ReverseGeocoder::Street>, uint32_t> result;

  GetNearbyStreets(const_cast<FeatureType &>(ft), result.first);

  HouseTable table;
  if (!table.Get(m_index, ft.GetID(), result.first, result.second))
    result.second = numeric_limits<uint32_t>::max();

  return result;
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, Address & addr) const
{
  vector<Building> buildings;
  GetNearbyBuildings(center, buildings);

  HouseTable table;
  int triesCount = 0;

  for (auto const & b : buildings)
  {
    if (GetNearbyAddress(table, b, addr) || (++triesCount == 5))
      break;
  }
}

void ReverseGeocoder::GetNearbyAddress(FeatureType & ft, Address & addr) const
{
  HouseTable table;
  (void)GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), addr);
}

bool ReverseGeocoder::GetNearbyAddress(HouseTable & table, Building const & bld,
                                       Address & addr) const
{
  string street;
  if (osm::Editor::Instance().GetEditedFeatureStreet(bld.m_id, street))
  {
    addr.m_building = bld;
    addr.m_street.m_name = street;
    return true;
  }

  vector<Street> streets;
  GetNearbyStreets(bld.m_id.m_mwmId, bld.m_center, streets);

  uint32_t ind;
  if (table.Get(m_index, bld.m_id, streets, ind))
  {
    addr.m_building = bld;
    addr.m_street = streets[ind];
    return true;
  }

  return false;
}

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, vector<Building> & buildings) const
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addBuilding = [&](FeatureType & ft)
  {
    if (!ft.GetHouseNumber().empty())
      buildings.push_back(FromFeature(ft, feature::GetMinDistanceMeters(ft, center)));
  };

  m_index.ForEachInRect(addBuilding, rect, kQueryScale);
  sort(buildings.begin(), buildings.end(), my::CompareBy(&Building::m_distanceMeters));
}

// static
ReverseGeocoder::Building ReverseGeocoder::FromFeature(FeatureType & ft, double distMeters)
{
  return { ft.GetID(), distMeters, ft.GetHouseNumber(), feature::GetCenter(ft) };
}

// static
m2::RectD ReverseGeocoder::GetLookupRect(m2::PointD const & center, double radiusM)
{
  return MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
}

bool ReverseGeocoder::HouseTable::Get(Index const & index, FeatureID fId,
                                      vector<Street> const & streets, uint32_t & stIndex)
{
  if (!m_table || m_mwmHandle.GetId() != fId.m_mwmId)
  {
    m_mwmHandle = index.GetMwmHandleById(fId.m_mwmId);
    if (!m_mwmHandle.IsAlive())
    {
      LOG(LWARNING, ("MWM", fId, "is dead"));
      return false;
    }
    m_table = search::v2::HouseToStreetTable::Load(*m_mwmHandle.GetValue<MwmValue>());
  }

  if (!m_table->Get(fId.m_index, stIndex))
    return false;

  if (stIndex >= streets.size())
  {
    LOG(LWARNING, ("Out of bound index", stIndex, "for", fId));
    return false;
  }

  return true;
}

} // namespace search
