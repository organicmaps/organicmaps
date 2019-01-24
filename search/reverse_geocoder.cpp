#include "reverse_geocoder.hpp"

#include "search/mwm_context.hpp"

#include "indexer/data_source.hpp"

#include "indexer/fake_feature_ids.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"

#include <functional>
#include <limits>

using namespace std;

namespace search
{
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
  return MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
}

void AddStreet(FeatureType & ft, m2::PointD const & center, bool includeSquaresAndSuburbs,
               vector<ReverseGeocoder::Street> & streets)
{
  bool const addAsStreet =
      ft.GetFeatureType() == feature::GEOM_LINE && ftypes::IsWayChecker::Instance()(ft);
  bool const isSquareOrSuburb =
      ftypes::IsSquareChecker::Instance()(ft) || ftypes::IsSuburbChecker::Instance()(ft);
  bool const addAsSquareOrSuburb = includeSquaresAndSuburbs && isSquareOrSuburb;

  if (!addAsStreet && !addAsSquareOrSuburb)
    return;

  string name;
  if (!ft.GetName(StringUtf8Multilang::kDefaultCode, name))
    return;

  ASSERT(!name.empty(), ());

  streets.emplace_back(ft.GetID(), feature::GetMinDistanceMeters(ft, center), name);
}
}  // namespace

ReverseGeocoder::ReverseGeocoder(DataSource const & dataSource) : m_dataSource(dataSource) {}

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

// static
boost::optional<uint32_t> ReverseGeocoder::GetMatchedStreetIndex(std::string const & keyName,
                                                                 vector<Street> const & streets)
{
  auto matchStreet = [&](bool ignoreStreetSynonyms) -> boost::optional<uint32_t> {
    // Find the exact match or the best match in kSimilarityTresholdPercent limit.
    uint32_t result;
    size_t minPercent = kSimilarityThresholdPercent + 1;

    auto const key = GetStreetNameAsKey(keyName, ignoreStreetSynonyms);
    for (auto const & street : streets)
    {
      strings::UniString const actual = GetStreetNameAsKey(street.m_name, ignoreStreetSynonyms);

      size_t const editDistance =
          strings::EditDistance(key.begin(), key.end(), actual.begin(), actual.end());

      if (editDistance == 0)
        return street.m_id.m_index;

      if (actual.empty())
        continue;

      size_t const percent = editDistance * 100 / actual.size();
      if (percent < minPercent)
      {
        result = street.m_id.m_index;
        minPercent = percent;
      }
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

string ReverseGeocoder::GetFeatureStreetName(FeatureType & ft) const
{
  Address addr;
  HouseTable table(m_dataSource);
  GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), false /* ignoreEdits */, addr);
  return addr.m_street.m_name;
}

string ReverseGeocoder::GetOriginalFeatureStreetName(FeatureType & ft) const
{
  Address addr;
  HouseTable table(m_dataSource);
  GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), true /* ignoreEdits */, addr);
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

  uint32_t streetId;
  HouseToStreetTable::StreetIdType type;
  if (!table.Get(bld.m_id, type, streetId))
    return false;

  switch (type)
  {
  case HouseToStreetTable::StreetIdType::Index:
  {
    vector<Street> streets;
    // Get streets without squares and suburbs for backward compatibility with data.
    GetNearbyStreetsWaysOnly(bld.m_id.m_mwmId, bld.m_center, streets);
    if (streetId < streets.size())
    {
      addr.m_building = bld;
      addr.m_street = streets[streetId];
      return true;
    }
    LOG(LWARNING, ("Out of bound street index", streetId, "for", bld.m_id));
    return false;
  }
  case HouseToStreetTable::StreetIdType::FeatureId:
  {
    FeatureID streetFeature(bld.m_id.m_mwmId, streetId);
    string streetName;
    double distance;
    m_dataSource.ReadFeature(
        [&](FeatureType & ft) {
          ft.GetName(StringUtf8Multilang::kDefaultCode, streetName);
          distance = feature::GetMinDistanceMeters(ft, bld.m_center);
        },
        streetFeature);
    CHECK(!streetName.empty(), ());
    addr.m_building = bld;
    addr.m_street = Street(streetFeature, distance, streetName);
    return true;
  }
  case HouseToStreetTable::StreetIdType::None:
  {
    // Prior call of table.Get() is expected to fail.
    UNREACHABLE();
  }
  }
  UNREACHABLE();
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
ReverseGeocoder::Building ReverseGeocoder::FromFeature(FeatureType & ft, double distMeters)
{
  return { ft.GetID(), distMeters, ft.GetHouseNumber(), feature::GetCenter(ft) };
}

bool ReverseGeocoder::HouseTable::Get(FeatureID const & fid,
                                      HouseToStreetTable::StreetIdType & type,
                                      uint32_t & streetIndex)
{
  if (feature::FakeFeatureIds::IsEditorCreatedFeature(fid.m_index))
    return false;

  if (!m_table || m_handle.GetId() != fid.m_mwmId)
  {
    m_handle = m_dataSource.GetMwmHandleById(fid.m_mwmId);
    if (!m_handle.IsAlive())
    {
      LOG(LWARNING, ("MWM", fid, "is dead"));
      return false;
    }
    m_table = search::HouseToStreetTable::Load(*m_handle.GetValue<MwmValue>());
  }

  type = m_table->GetStreetIdType();
  return m_table->Get(fid.m_index, streetIndex);
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
