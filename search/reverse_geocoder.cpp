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

#include "std/function.hpp"
#include "std/limits.hpp"

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

void AddStreet(FeatureType & ft, m2::PointD const & center,
               vector<ReverseGeocoder::Street> & streets)
{
  if (ft.GetFeatureType() != feature::GEOM_LINE || !ftypes::IsStreetChecker::Instance()(ft))
  {
    return;
  }

  string name;
  if (!ft.GetName(StringUtf8Multilang::kDefaultCode, name))
    return;

  ASSERT(!name.empty(), ());

  streets.emplace_back(ft.GetID(), feature::GetMinDistanceMeters(ft, center), name);
}

void GetNearbyStreetsImpl(DataSource const & source, MwmSet::MwmId const & id,
                          m2::PointD const & center, vector<ReverseGeocoder::Street> & streets,
                          FillStreets && fillStreets)
{
  m2::RectD const rect = GetLookupRect(center, ReverseGeocoder::kLookupRadiusM);
  MwmSet::MwmHandle mwmHandle = source.GetMwmHandleById(id);

  if (!mwmHandle.IsAlive())
    return;

  auto const addStreet = [&center, &streets](FeatureType & ft) { AddStreet(ft, center, streets); };

  fillStreets(move(mwmHandle), rect, addStreet);

  sort(streets.begin(), streets.end(), base::LessBy(&ReverseGeocoder::Street::m_distanceMeters));
}
}  // namespace

ReverseGeocoder::ReverseGeocoder(DataSource const & dataSource) : m_dataSource(dataSource) {}

// static
void ReverseGeocoder::GetNearbyStreets(search::MwmContext & context, m2::PointD const & center,
                                       vector<Street> & streets)
{
  m2::RectD const rect = GetLookupRect(center, kLookupRadiusM);

  auto const addStreet = [&center, &streets](FeatureType & ft) { AddStreet(ft, center, streets); };

  context.ForEachFeature(rect, addStreet);
  sort(streets.begin(), streets.end(), base::LessBy(&Street::m_distanceMeters));
}

void ReverseGeocoder::GetNearbyStreets(MwmSet::MwmId const & id, m2::PointD const & center,
                                       vector<Street> & streets) const
{
  auto const fillStreets = [](MwmSet::MwmHandle && handle, m2::RectD const & rect,
                              AppendStreet && addStreet)
  {
    search::MwmContext(move(handle)).ForEachFeature(rect, addStreet);
  };

  GetNearbyStreetsImpl(m_dataSource, id, center, streets, move(fillStreets));
}

void ReverseGeocoder::GetNearbyStreets(FeatureType & ft, vector<Street> & streets) const
{
  ASSERT(ft.GetID().IsValid(), ());
  GetNearbyStreets(ft.GetID().m_mwmId, feature::GetCenter(ft), streets);
}

void ReverseGeocoder::GetNearbyOriginalStreets(MwmSet::MwmId const & id, m2::PointD const & center,
                                               vector<Street> & streets) const
{
  auto const fillStreets = [](MwmSet::MwmHandle && handle, m2::RectD const & rect,
                              AppendStreet && addStreet)
  {
    search::MwmContext(move(handle)).ForEachOriginalFeature(rect, addStreet);
  };

  GetNearbyStreetsImpl(m_dataSource, id, center, streets, move(fillStreets));
}

// static
size_t ReverseGeocoder::GetMatchedStreetIndex(strings::UniString const & keyName,
                                              vector<Street> const & streets)
{
  // Find the exact match or the best match in kSimilarityTresholdPercent limit.
  size_t const count = streets.size();
  size_t result = count;
  size_t minPercent = kSimilarityThresholdPercent + 1;

  for (size_t i = 0; i < count; ++i)
  {
    strings::UniString const actual = GetStreetNameAsKey(streets[i].m_name);

    size_t const editDistance = strings::EditDistance(keyName.begin(), keyName.end(),
                                                      actual.begin(), actual.end());

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
ReverseGeocoder::GetNearbyFeatureStreets(FeatureType & ft) const
{
  pair<vector<ReverseGeocoder::Street>, uint32_t> result;

  GetNearbyStreets(ft, result.first);

  HouseTable table(m_dataSource);
  if (!table.Get(ft.GetID(), result.second))
    result.second = numeric_limits<uint32_t>::max();

  return result;
}

pair<vector<ReverseGeocoder::Street>, uint32_t>
ReverseGeocoder::GetNearbyOriginalFeatureStreets(FeatureType & ft) const
{
  pair<vector<ReverseGeocoder::Street>, uint32_t> result;

  ASSERT(ft.GetID().IsValid(), ());
  GetNearbyOriginalStreets(ft.GetID().m_mwmId, feature::GetCenter(ft), result.first);

  HouseTable table(m_dataSource);
  if (!table.Get(ft.GetID(), result.second))
    result.second = numeric_limits<uint32_t>::max();

  return result;
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
    if (GetNearbyAddress(table, b, addr) || (++triesCount == kMaxNumTriesToApproxAddress))
      break;
  }
}

bool ReverseGeocoder::GetExactAddress(FeatureType & ft, Address & addr) const
{
  if (ft.GetHouseNumber().empty())
    return false;
  HouseTable table(m_dataSource);
  return GetNearbyAddress(table, FromFeature(ft, 0.0 /* distMeters */), addr);
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

  uint32_t ind;
  if (!table.Get(bld.m_id, ind))
    return false;

  vector<Street> streets;
  GetNearbyStreets(bld.m_id.m_mwmId, bld.m_center, streets);
  if (ind < streets.size())
  {
    addr.m_building = bld;
    addr.m_street = streets[ind];
    return true;
  }
  else
  {
    LOG(LWARNING, ("Out of bound street index", ind, "for", bld.m_id));
    return false;
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
ReverseGeocoder::Building ReverseGeocoder::FromFeature(FeatureType & ft, double distMeters)
{
  return { ft.GetID(), distMeters, ft.GetHouseNumber(), feature::GetCenter(ft) };
}

bool ReverseGeocoder::HouseTable::Get(FeatureID const & fid, uint32_t & streetIndex)
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
