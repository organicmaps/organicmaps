#include "search/reverse_geocoder.hpp"

#include "search/city_finder.hpp"
#include "search/house_to_street_table.hpp"
#include "search/mwm_context.hpp"
#include "search/region_info_getter.hpp"
#include "search/street_vicinity_loader.hpp"

#include "storage/country_info_getter.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/data_source.hpp"
#include "indexer/fake_feature_ids.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <functional>

namespace search
{
using namespace std;

namespace
{
int constexpr kQueryScale = scales::GetUpperScale();
/// Max number of tries (nearest houses with housenumber) to check when getting point address.
size_t constexpr kMaxNumTriesToApproxAddress = 10;

using AppendStreet = function<void(FeatureType & ft)>;
using FillStreets = function<void(MwmSet::MwmHandle && handle, m2::RectD const & rect, AppendStreet && addStreet)>;

m2::RectD GetLookupRect(m2::PointD const & center, double radiusM)
{
  return mercator::RectByCenterXYAndSizeInMeters(center, radiusM);
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
  auto tail = Join(std::forward<Args>(args)...);
  if (s.empty())
    return tail;
  if (tail.empty())
    return s;
  return s + ", " + tail;
}

ReverseGeocoder::Building FromFeatureImpl(FeatureType & ft, std::string const & hn, double distMeters)
{
  return {ft.GetID(), distMeters, hn, feature::GetCenter(ft)};
}

std::string const & GetHouseNumber(FeatureType & ft)
{
  std::string const & hn = ft.GetHouseNumber();
  if (hn.empty() && ftypes::IsAddressInterpolChecker::Instance()(ft))
    return ft.GetRef();
  return hn;
}

}  // namespace

ReverseGeocoder::ReverseGeocoder(DataSource const & dataSource) : m_dataSource(dataSource) {}

template <class ObjT, class FilterT>
vector<ObjT> GetNearbyObjects(search::MwmContext & context, m2::PointD const & center, double radiusM,
                              FilterT && filter)
{
  vector<ObjT> objs;

  m2::RectD const rect = GetLookupRect(center, radiusM);
  context.ForEachFeature(rect, [&](FeatureType & ft)
  {
    if (filter(ft))
    {
      string_view const name = ft.GetReadableName();
      if (!name.empty())
        objs.emplace_back(ft.GetID(), feature::GetMinDistanceMeters(ft, center), name, ft.GetNames());
    }
  });

  sort(objs.begin(), objs.end(), base::LessBy(&ObjT::m_distanceMeters));
  return objs;
}

vector<ReverseGeocoder::Street> ReverseGeocoder::GetNearbyStreets(search::MwmContext & context,
                                                                  m2::PointD const & center, double radiusM)
{
  return GetNearbyObjects<Street>(context, center, radiusM,
                                  [](FeatureType & ft) { return StreetVicinityLoader::IsStreet(ft); });
}

vector<ReverseGeocoder::Street> ReverseGeocoder::GetNearbyStreets(MwmSet::MwmId const & id,
                                                                  m2::PointD const & center) const
{
  MwmSet::MwmHandle mwmHandle = m_dataSource.GetMwmHandleById(id);
  if (mwmHandle.IsAlive())
  {
    search::MwmContext context(std::move(mwmHandle));
    return GetNearbyStreets(context, center);
  }
  return {};
}

vector<ReverseGeocoder::Street> ReverseGeocoder::GetNearbyStreets(FeatureType & ft) const
{
  ASSERT(ft.GetID().IsValid(), ());
  return GetNearbyStreets(ft.GetID().m_mwmId, feature::GetCenter(ft));
}

std::vector<ReverseGeocoder::Place> ReverseGeocoder::GetNearbyPlaces(search::MwmContext & context,
                                                                     m2::PointD const & center, double radiusM)
{
  return GetNearbyObjects<Place>(context, center, radiusM, [](FeatureType & ft)
  {
    return (ftypes::IsLocalityChecker::Instance().GetType(ft) >= ftypes::LocalityType::City ||
            ftypes::IsSuburbChecker::Instance()(ft));
  });
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

  m_dataSource.ReadFeature([&](FeatureType & ft) { bld = FromFeature(ft, 0.0 /* distMeters */); }, fid);
  GetNearbyAddress(table, bld, true /* ignoreEdits */, addr);
  return addr.m_street.m_name;
}

bool ReverseGeocoder::GetOriginalStreetByHouse(FeatureType & house, FeatureID & streetId) const
{
  Address addr;
  HouseTable table(m_dataSource);

  // Ignore edits here, because this function is called once with edits check before.
  if (GetNearbyAddress(table, FromFeature(house, 0.0 /* distMeters */), true /* ignoreEdits */, addr))
  {
    streetId = addr.m_street.m_id;
    return true;
  }

  return false;
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, Address & addr) const
{
  GetNearbyAddress(center, kLookupRadiusM, addr);
}

void ReverseGeocoder::GetNearbyAddress(m2::PointD const & center, double maxDistanceM, Address & addr,
                                       bool placeAsStreet /* = false*/) const
{
  vector<Building> buildings;
  GetNearbyBuildings(center, maxDistanceM, buildings);

  HouseTable table(m_dataSource, placeAsStreet);
  size_t triesCount = 0;

  for (auto const & b : buildings)
  {
    // It's quite enough to analyze nearest kMaxNumTriesToApproxAddress houses for the exact nearby address.
    // When we can't guarantee suitable address for the point with distant houses.
    if (GetNearbyAddress(table, b, false /* ignoreEdits */, addr) || (++triesCount == kMaxNumTriesToApproxAddress))
      break;
  }
}

bool ReverseGeocoder::GetExactAddress(FeatureType & ft, Address & addr, bool placeAsStreet /* = false*/) const
{
  std::string const & hn = GetHouseNumber(ft);
  if (hn.empty())
    return false;

  HouseTable table(m_dataSource, placeAsStreet);
  return GetNearbyAddress(table, FromFeatureImpl(ft, hn, 0.0 /* distMeters */), false /* ignoreEdits */, addr);
}

bool ReverseGeocoder::GetExactAddress(FeatureID const & fid, Address & addr) const
{
  bool res;
  m_dataSource.ReadFeature([&](FeatureType & ft) { res = GetExactAddress(ft, addr, true /* placeAsStreet */); }, fid);
  return res;
}

bool ReverseGeocoder::GetNearbyAddress(HouseTable & table, Building const & bld, bool ignoreEdits, Address & addr) const
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
    // Not used since OM.
    //  case HouseToStreetTable::StreetIdType::Index:
    //  {
    //    vector<Street> streets;
    //    // Get streets without squares and suburbs for backward compatibility with data.
    //    GetNearbyStreetsWaysOnly(bld.m_id.m_mwmId, bld.m_center, streets);
    //    if (res->m_streetId < streets.size())
    //    {
    //      addr.m_building = bld;
    //      addr.m_street = streets[res->m_streetId];
    //      return true;
    //    }
    //    LOG(LWARNING, ("Out of bound street index", res->m_streetId, "for", bld.m_id));
    //    return false;
    //  }
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

void ReverseGeocoder::GetNearbyBuildings(m2::PointD const & center, double radius, vector<Building> & buildings) const
{
  auto const addBuilding = [&](FeatureType & ft)
  {
    std::string const & hn = GetHouseNumber(ft);
    if (hn.empty())
      return;

    auto const distance = feature::GetMinDistanceMeters(ft, center);
    if (distance <= radius)
      buildings.push_back(FromFeatureImpl(ft, hn, distance));
  };

  auto const stop = [&]() { return buildings.size() >= kMaxNumTriesToApproxAddress; };

  m_dataSource.ForClosestToPoint(addBuilding, stop, center, radius, kQueryScale);
  sort(buildings.begin(), buildings.end(), base::LessBy(&Building::m_distanceMeters));
}

// static
ReverseGeocoder::RegionAddress ReverseGeocoder::GetNearbyRegionAddress(m2::PointD const & center,
                                                                       storage::CountryInfoGetter const & infoGetter,
                                                                       CityFinder & cityFinder)
{
  RegionAddress addr;
  addr.m_featureId = cityFinder.GetCityFeatureID(center);
  if (!addr.m_featureId.IsValid() || addr.m_featureId.m_mwmId.GetInfo()->GetType() == MwmInfo::WORLD)
    addr.m_countryId = infoGetter.GetRegionCountryId(center);
  return addr;
}

string ReverseGeocoder::GetLocalizedRegionAddress(RegionAddress const & addr, RegionInfoGetter const & nameGetter) const
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
      RegionInfoGetter::NameBufferT nameParts;
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
  return FromFeatureImpl(ft, ft.GetHouseNumber(), distMeters);
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

  auto res = value->m_house2street->Get(fid.m_index);
  if (!res && m_placeAsStreet)
  {
    if (!value->m_house2place)
      value->m_house2place = LoadHouseToPlaceTable(*value);
    res = value->m_house2place->Get(fid.m_index);
  }
  return res;
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

}  // namespace search
