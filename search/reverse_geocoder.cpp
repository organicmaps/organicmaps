#include "search/reverse_geocoder.hpp"

#include "search/city_finder.hpp"
#include "search/mwm_context.hpp"
#include "search/region_info_getter.hpp"

#include "storage/country_info_getter.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/data_source.hpp"
#include "indexer/fake_feature_ids.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>

namespace search
{
using namespace std;

namespace
{
size_t constexpr kSimilarityThresholdPercent = 10;
int constexpr kQueryScale = scales::GetUpperScale();
/// Max number of tries (nearest houses with housenumber) to check when getting point address.
size_t constexpr kMaxNumTriesToApproxAddress = 10;

using AppendStreet = function<void(FeatureType & ft)>;
using FillStreets =
    function<void(MwmSet::MwmHandle && handle, m2::RectD const & rect, AppendStreet && addStreet)>;

m2::RectD GetLookupRect(m2::PointD const & center, double radiusM)
{
  return mercator::RectByCenterXYAndSizeInMeters(center, radiusM);
}

void AddStreet(FeatureType & ft, m2::PointD const & center, bool includeSquaresAndSuburbs,
               vector<ReverseGeocoder::Street> & streets)
{
  bool const addAsStreet =
      ft.GetGeomType() == feature::GeomType::Line && ftypes::IsWayChecker::Instance()(ft);
  bool const isSquareOrSuburb =
      ftypes::IsSquareChecker::Instance()(ft) || ftypes::IsSuburbChecker::Instance()(ft);
  bool const addAsSquareOrSuburb = includeSquaresAndSuburbs && isSquareOrSuburb;

  if (!addAsStreet && !addAsSquareOrSuburb)
    return;

  string_view const name = ft.GetReadableName();
  if (!name.empty())
    streets.emplace_back(ft.GetID(), feature::GetMinDistanceMeters(ft, center), name, ft.GetNames());
}

// Following methods join only non-empty arguments in order with
// commas.
string Join(string const & s)
{
  return s;
}

template <typename... Args>
string Join(string const & s, Args &&... args)
{
  auto const tail = Join(forward<Args>(args)...);
  if (s.empty())
    return tail;
  if (tail.empty())
    return s;
  return s + ", " + tail;
}
}  // namespace

ReverseGeocoder::ReverseGeocoder(DataSource const & dataSource) : m_dataSource(dataSource) {}

// static
optional<uint32_t> ReverseGeocoder::GetMatchedStreetIndex(string_view keyName,
                                                          vector<Street> const & streets)
{
  auto matchStreet = [&](bool ignoreStreetSynonyms) -> optional<uint32_t>
  {
    // Find the exact match or the best match in kSimilarityTresholdPercent limit.
    uint32_t result;
    size_t minPercent = kSimilarityThresholdPercent + 1;

    auto const key = GetStreetNameAsKey(keyName, ignoreStreetSynonyms);
    for (auto const & street : streets)
    {
      bool fullMatchFound = false;
      street.m_multilangName.ForEach([&](int8_t /* langCode */, string_view name)
      {
        if (fullMatchFound)
          return;

        strings::UniString const actual = GetStreetNameAsKey(name, ignoreStreetSynonyms);

        size_t const editDistance =
            strings::EditDistance(key.begin(), key.end(), actual.begin(), actual.end());

        if (editDistance == 0)
        {
          result = street.m_id.m_index;
          fullMatchFound = true;
          return;
        }

        if (actual.empty())
          return;

        size_t const percent = editDistance * 100 / actual.size();
        if (percent < minPercent)
        {
          result = street.m_id.m_index;
          minPercent = percent;
        }
      });

      if (fullMatchFound)
        return result;
    }

    if (minPercent <= kSimilarityThresholdPercent)
      return result;
    return {};
  };

  auto result = matchStreet(false /* ignoreStreetSynonyms */);
  if (result)
    return result;
  return matchStreet(true /* ignoreStreetSynonyms */);
}

// static
void ReverseGeocoder::GetNearbyStreets(search::MwmContext & context, m2::PointD const & center,
                                       bool includeSquaresAndSuburbs, vector<Street> & streets)
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addStreet = [&](FeatureType & ft) {
    AddStreet(ft, center, includeSquaresAndSuburbs, streets);
  };

  context.ForEachFeature(rect, addStreet);
  sort(streets.begin(), streets.end(), base::LessBy(&Street::m_distanceMeters));
}

void ReverseGeocoder::GetNearbyStreets(MwmSet::MwmId const & id, m2::PointD const & center,
                                       vector<Street> & streets) const
{
  MwmSet::MwmHandle mwmHandle = m_dataSource.GetMwmHandleById(id);
  if (mwmHandle.IsAlive())
  {
    search::MwmContext context(move(mwmHandle));
    GetNearbyStreets(context, center, true /* includeSquaresAndSuburbs */, streets);
  }
}

void ReverseGeocoder::GetNearbyStreets(FeatureType & ft, vector<Street> & streets) const
{
  ASSERT(ft.GetID().IsValid(), ());
  GetNearbyStreets(ft.GetID().m_mwmId, feature::GetCenter(ft), streets);
}

void ReverseGeocoder::GetNearbyStreetsWaysOnly(MwmSet::MwmId const & id, m2::PointD const & center,
                                               vector<Street> & streets) const
{
  MwmSet::MwmHandle mwmHandle = m_dataSource.GetMwmHandleById(id);
  if (mwmHandle.IsAlive())
  {
    search::MwmContext context(move(mwmHandle));
    GetNearbyStreets(context, center, false /* includeSquaresAndSuburbs */, streets);
  }
}

string ReverseGeocoder::GetFeatureStreetName(FeatureType & ft) const
{
  Address addr;
  HouseTable table(m_dataSource);
  GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), false /* ignoreEdits */, addr);
  return addr.m_street.m_name;
}

string ReverseGeocoder::GetOriginalFeatureStreetName(FeatureID const & fid) const
{
  Address addr;
  HouseTable table(m_dataSource);
  Building bld;

  m_dataSource.ReadFeature([&](FeatureType & ft) { bld = FromFeature(ft, 0.0 /* distMeters */); },
                           fid);
  GetNearbyAddress(table, bld, true /* ignoreEdits */, addr);
  return addr.m_street.m_name;
}

bool ReverseGeocoder::GetStreetByHouse(FeatureType & house, FeatureID & streetId) const
{
  Address addr;
  HouseTable table(m_dataSource);
  if (GetNearbyAddress(table, FromFeature(house, 0.0 /* distMeters */), false /* ignoreEdits */, addr))
  {
    streetId = addr.m_street.m_id;
    return true;
  }

  return false;
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, Address & addr) const
{
  return GetNearbyAddress(center, kLookupRadiusM, addr);
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, double maxDistanceM,
                                       Address & addr) const
{
  vector<Building> buildings;
  GetNearbyBuildings(center, maxDistanceM, buildings);

  HouseTable table(m_dataSource);
  size_t triesCount = 0;

  for (auto const & b : buildings)
  {
    // It's quite enough to analize nearest kMaxNumTriesToApproxAddress houses for the exact nearby address.
    // When we can't guarantee suitable address for the point with distant houses.
    if (GetNearbyAddress(table, b, false /* ignoreEdits */, addr) ||
        (++triesCount == kMaxNumTriesToApproxAddress))
      break;
  }
}

bool ReverseGeocoder::GetExactAddress(FeatureType & ft, Address & addr) const
{
  if (ft.GetHouseNumber().empty())
    return false;
  HouseTable table(m_dataSource);
  return GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), false /* ignoreEdits */,
                          addr);
}

bool ReverseGeocoder::GetExactAddress(FeatureID const & fid, Address & addr) const
{
  bool res;
  m_dataSource.ReadFeature([&](FeatureType & ft) { res = GetExactAddress(ft, addr); }, fid);
  return res;
}

bool ReverseGeocoder::GetNearbyAddress(HouseTable & table, Building const & bld, bool ignoreEdits,
                                       Address & addr) const
{
  string street;
  if (!ignoreEdits && osm::Editor::Instance().GetEditedFeatureStreet(bld.m_id, street))
  {
    addr.m_building = bld;
    addr.m_street.m_name = street;
    return true;
  }

  auto const res = table.Get(bld.m_id);
  if (!res)
    return false;

  switch (res->m_type)
  {
  case HouseToStreetTable::StreetIdType::Index:
  {
    vector<Street> streets;
    // Get streets without squares and suburbs for backward compatibility with data.
    GetNearbyStreetsWaysOnly(bld.m_id.m_mwmId, bld.m_center, streets);
    if (res->m_streetId < streets.size())
    {
      addr.m_building = bld;
      addr.m_street = streets[res->m_streetId];
      return true;
    }
    LOG(LWARNING, ("Out of bound street index", res->m_streetId, "for", bld.m_id));
    return false;
  }
  case HouseToStreetTable::StreetIdType::FeatureId:
  {
    FeatureID streetFeature(bld.m_id.m_mwmId, res->m_streetId);
    CHECK(bld.m_id.m_mwmId.IsAlive(), (bld.m_id.m_mwmId));
    m_dataSource.ReadFeature([&bld, &addr](FeatureType & ft)
    {
      double distance = feature::GetMinDistanceMeters(ft, bld.m_center);
      addr.m_street = Street(ft.GetID(), distance, ft.GetReadableName(), ft.GetNames());
    }, streetFeature);

    CHECK(!addr.m_street.m_multilangName.IsEmpty(), (bld.m_id.m_mwmId, res->m_streetId));
    addr.m_building = bld;
    return true;
  }
  default:
  {
    // Prior call of table.Get() is expected to fail.
    UNREACHABLE();
  }
  }
}

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, double radius,
                                         vector<Building> & buildings) const
{
  auto const addBuilding = [&](FeatureType & ft) {
    auto const distance = feature::GetMinDistanceMeters(ft, center);
    if (!ft.GetHouseNumber().empty() && distance <= radius)
      buildings.push_back(FromFeature(ft, distance));
  };

  auto const stop = [&]() { return buildings.size() >= kMaxNumTriesToApproxAddress; };

  m_dataSource.ForClosestToPoint(addBuilding, stop, center, radius, kQueryScale);
  sort(buildings.begin(), buildings.end(), base::LessBy(&Building::m_distanceMeters));
}

// static
ReverseGeocoder::RegionAddress ReverseGeocoder::GetNearbyRegionAddress(
    m2::PointD const & center, storage::CountryInfoGetter const & infoGetter,
    CityFinder & cityFinder)
{
  RegionAddress addr;
  addr.m_featureId = cityFinder.GetCityFeatureID(center);
  if (!addr.m_featureId.IsValid() ||
      addr.m_featureId.m_mwmId.GetInfo()->GetType() == MwmInfo::WORLD)
  {
    addr.m_countryId = infoGetter.GetRegionCountryId(center);
  }
  return addr;
}

string ReverseGeocoder::GetLocalizedRegionAddress(RegionAddress const & addr,
                                                  RegionInfoGetter const & nameGetter) const
{
  if (!addr.IsValid())
    return {};

  string addrStr;
  if (addr.m_featureId.IsValid())
  {
    m_dataSource.ReadFeature([&addrStr](FeatureType & ft) { addrStr = ft.GetReadableName(); }, addr.m_featureId);

    auto const countryName = addr.GetCountryName();
    if (!countryName.empty())
    {
      vector<string> nameParts;
      nameGetter.GetLocalizedFullName(countryName, nameParts);
      nameParts.insert(nameParts.begin(), std::move(addrStr));
      nameParts.erase(unique(nameParts.begin(), nameParts.end()), nameParts.end());
      addrStr = strings::JoinStrings(nameParts, ", ");
    }
  }
  else
  {
    ASSERT(storage::IsCountryIdValid(addr.m_countryId), ());
    addrStr = nameGetter.GetLocalizedFullName(addr.m_countryId);
  }

  return addrStr;
}

// static
ReverseGeocoder::Building ReverseGeocoder::FromFeature(FeatureType & ft, double distMeters)
{
  return { ft.GetID(), distMeters, ft.GetHouseNumber(), feature::GetCenter(ft) };
}

std::optional<HouseToStreetTable::Result> ReverseGeocoder::HouseTable::Get(FeatureID const & fid)
{
  if (feature::FakeFeatureIds::IsEditorCreatedFeature(fid.m_index))
    return {};

  if (m_handle.GetId() != fid.m_mwmId)
  {
    auto handle = m_dataSource.GetMwmHandleById(fid.m_mwmId);
    if (!handle.IsAlive())
    {
      LOG(LWARNING, ("MWM", fid, "is dead"));
      return {};
    }
    m_handle = std::move(handle);
  }

  auto value = m_handle.GetValue();
  if (!value->m_house2street)
    value->m_house2street = LoadHouseToStreetTable(*value);

  return value->m_house2street->Get(fid.m_index);
}

string ReverseGeocoder::Address::FormatAddress() const
{
  // Check whether we can format address according to the query type
  // and actual address distance.

  // TODO (@m, @y): we can add "Near" prefix here in future according
  // to the distance.
  if (m_building.m_distanceMeters > 200.0)
    return {};

  return Join(m_street.m_name, m_building.m_name);
}

bool ReverseGeocoder::RegionAddress::IsValid() const
{
  return storage::IsCountryIdValid(m_countryId) || m_featureId.IsValid();
}

string ReverseGeocoder::RegionAddress::GetCountryName() const
{
  if (m_featureId.IsValid() && m_featureId.m_mwmId.GetInfo()->GetType() != MwmInfo::WORLD)
    return m_featureId.m_mwmId.GetInfo()->GetCountryName();
  return m_countryId;
}

bool ReverseGeocoder::RegionAddress::operator==(RegionAddress const & rhs) const
{
  return m_countryId == rhs.m_countryId && m_featureId == rhs.m_featureId;
}

bool ReverseGeocoder::RegionAddress::operator!=(RegionAddress const & rhs) const
{
  return !(*this == rhs);
}

bool ReverseGeocoder::RegionAddress::operator<(RegionAddress const & rhs) const
{
  if (m_countryId != rhs.m_countryId)
    return m_countryId < rhs.m_countryId;
  return m_featureId < rhs.m_featureId;
}

string DebugPrint(ReverseGeocoder::Object const & obj)
{
  return obj.m_name;
}

string DebugPrint(ReverseGeocoder::Address const & addr)
{
  return "{ " + DebugPrint(addr.m_building) + ", " + DebugPrint(addr.m_street) + " }";
}

} // namespace search
