#include "storage/country_info_getter.hpp"

#include "storage/country_decl.hpp"
#include "storage/country_tree.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/region2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace storage
{

// CountryInfoGetterBase ---------------------------------------------------------------------------
CountryId CountryInfoGetterBase::GetRegionCountryId(m2::PointD const & pt) const
{
  RegionId const id = FindFirstCountry(pt);
  return id == kInvalidId ? kInvalidCountryId : m_countries[id].m_countryId;
}

bool CountryInfoGetterBase::BelongsToAnyRegion(m2::PointD const & pt, RegionIdVec const & regions) const
{
  for (auto const & id : regions)
    if (BelongsToRegion(pt, id))
      return true;
  return false;
}

bool CountryInfoGetterBase::BelongsToAnyRegion(CountryId const & countryId, RegionIdVec const & regions) const
{
  for (auto const & id : regions)
    if (m_countries[id].m_countryId == countryId)
      return true;
  return false;
}

CountryInfoGetterBase::RegionId CountryInfoGetterBase::GetRegionId(CountryId const & countryId) const
{
  for (size_t id = 0; id < m_countries.size(); ++id)
    if (m_countries[id].m_countryId == countryId)
      return id;
  return kInvalidId;
}

CountryInfoGetterBase::RegionId CountryInfoGetterBase::FindFirstCountry(m2::PointD const & pt) const
{
  for (size_t id = 0; id < m_countries.size(); ++id)
    if (BelongsToRegion(pt, id))
      return id;

  return kInvalidId;
}

// CountryInfoGetter -------------------------------------------------------------------------------
std::vector<CountryId> CountryInfoGetter::GetRegionsCountryIdByRect(m2::RectD const & rect, bool rough) const
{
  std::vector<CountryId> result;
  for (size_t id = 0; id < m_countries.size(); ++id)
  {
    if (rect.IsRectInside(m_countries[id].m_rect))
    {
      result.push_back(m_countries[id].m_countryId);
    }
    else if (rect.IsIntersect(m_countries[id].m_rect))
    {
      if (rough || IsIntersectedByRegion(rect, id))
        result.push_back(m_countries[id].m_countryId);
    }
  }
  return result;
}

void CountryInfoGetter::GetRegionsCountryId(m2::PointD const & pt, CountriesVec & closestCoutryIds,
                                            double lookupRadiusM) const
{
  closestCoutryIds.clear();

  m2::RectD const lookupRect = mercator::RectByCenterXYAndSizeInMeters(pt, lookupRadiusM);

  for (size_t id = 0; id < m_countries.size(); ++id)
    if (m_countries[id].m_rect.IsIntersect(lookupRect) && IsCloseEnough(id, pt, lookupRadiusM))
      closestCoutryIds.emplace_back(m_countries[id].m_countryId);
}

void CountryInfoGetter::GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const
{
  RegionId const id = FindFirstCountry(pt);
  if (id != kInvalidId)
    GetRegionInfo(m_countries[id].m_countryId, info);
}

void CountryInfoGetter::GetRegionInfo(CountryId const & countryId, CountryInfo & info) const
{
  auto const it = m_idToInfo.find(countryId);
  if (it == m_idToInfo.end())
    return;

  info = it->second;
  if (info.m_name.empty())
    info.m_name = countryId;

  CountryInfo::FileName2FullName(info.m_name);
}

void CountryInfoGetter::CalcUSALimitRect(m2::RectD rects[3]) const
{
  auto fn = [&](CountryDef const & c)
  {
    if (c.m_countryId == "USA_Alaska")
      rects[1] = c.m_rect;
    else if (c.m_countryId == "USA_Hawaii")
      rects[2] = c.m_rect;
    else
      rects[0].Add(c.m_rect);
  };

  ForEachCountry("USA_", fn);
}

m2::RectD CountryInfoGetter::CalcLimitRect(std::string const & prefix) const
{
  m2::RectD rect;
  ForEachCountry(prefix, [&rect](CountryDef const & c) { rect.Add(c.m_rect); });
  return rect;
}

m2::RectD CountryInfoGetter::GetLimitRectForLeaf(CountryId const & leafCountryId) const
{
  auto const it = m_countryIndex.find(leafCountryId);
  if (it != m_countryIndex.end())
  {
    ASSERT_LESS(it->second, m_countries.size(), ());
    return m_countries[it->second].m_rect;
  }
  else
  {
    // Full rect for World files.
    return mercator::Bounds::FullRect();
  }
}

void CountryInfoGetter::GetMatchedRegions(std::string const & affiliation, RegionIdVec & regions) const
{
  // Once set, m_affiliations ptr is never changed (same as the content).
  ASSERT(m_affiliations, ());

  auto it = m_affiliations->find(affiliation);
  if (it == m_affiliations->end())
    return;

  for (RegionId i = 0; i < m_countries.size(); ++i)
    if (std::binary_search(it->second.begin(), it->second.end(), m_countries[i].m_countryId))
      regions.push_back(i);
}

void CountryInfoGetter::SetAffiliations(Affiliations const * affiliations)
{
  m_affiliations = affiliations;
}

template <class ToDo>
void CountryInfoGetter::ForEachCountry(std::string const & prefix, ToDo && toDo) const
{
  for (auto const & country : m_countries)
    if (country.m_countryId.starts_with(prefix))
      toDo(country);
}

// CountryInfoReader -------------------------------------------------------------------------------
// static
std::unique_ptr<CountryInfoReader> CountryInfoReader::CreateCountryInfoReader(Platform const & platform)
{
  try
  {
    CountryInfoReader * result =
        new CountryInfoReader(platform.GetReader(PACKED_POLYGONS_FILE), platform.GetReader(COUNTRIES_FILE));
    return std::unique_ptr<CountryInfoReader>(result);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't load needed resources for storage::CountryInfoGetter:", e.Msg()));
  }
  return {};
}

// static
std::unique_ptr<CountryInfoGetter> CountryInfoReader::CreateCountryInfoGetter(Platform const & platform)
{
  return CreateCountryInfoReader(platform);
}

std::vector<m2::RegionD> CountryInfoReader::LoadRegionsFromDisk(RegionId id) const
{
  std::vector<m2::RegionD> result;
  ReaderSource<ModelReaderPtr> src(m_reader.GetReader(strings::to_string(id)));

  uint32_t const count = ReadVarUint<uint32_t>(src);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result.emplace_back(std::move(points));
  }
  return result;
}

CountryInfoReader::CountryInfoReader(ModelReaderPtr polyR, ModelReaderPtr countryR)
  : m_reader(polyR)
  , m_cache(3 /* logCacheSize */)

{
  ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
  rw::Read(src, m_countries);

  m_countryIndex.reserve(m_countries.size());
  for (RegionId i = 0; i < m_countries.size(); ++i)
    m_countryIndex[m_countries[i].m_countryId] = i;

  std::string buffer;
  countryR.ReadAsString(buffer);
  LoadCountryFile2CountryInfo(buffer, m_idToInfo);

  for (auto const & [k, v] : m_idToInfo)
    ASSERT_EQUAL(k, v.m_name, ());
}

void CountryInfoReader::ClearCachesImpl() const
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);

  m_cache.ForEachValue([](std::vector<m2::RegionD> & v) { std::vector<m2::RegionD>().swap(v); });
  m_cache.Reset();
}

template <class Fn>
auto CountryInfoReader::WithRegion(RegionId id, Fn && fn) const
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);

  bool isFound = false;
  auto & regions = m_cache.Find(static_cast<uint32_t>(id), isFound);

  if (!isFound)
    regions = LoadRegionsFromDisk(id);

  return fn(regions);
}

bool CountryInfoReader::BelongsToRegion(m2::PointD const & pt, RegionId id) const
{
  if (!m_countries[id].m_rect.IsPointInside(pt))
    return false;

  auto contains = [&pt](std::vector<m2::RegionD> const & regions)
  {
    for (auto const & region : regions)
      if (region.Contains(pt))
        return true;
    return false;
  };

  return WithRegion(id, contains);
}

bool CountryInfoReader::IsIntersectedByRegion(m2::RectD const & rect, RegionId id) const
{
  auto contains = [&rect](std::vector<m2::RegionD> const & regions)
  {
    for (auto const & region : regions)
    {
      bool isIntersect = false;
      rect.ForEachSide([&](m2::PointD const & p1, m2::PointD const & p2)
      {
        if (isIntersect)
          return;
        m2::PointD result;
        isIntersect = region.FindIntersection(p1, p2, result);
      });
      if (isIntersect)
        return true;
    }
    return false;
  };

  if (WithRegion(id, contains))
    return true;

  return BelongsToRegion(rect.Center(), id);
}

bool CountryInfoReader::IsCloseEnough(RegionId id, m2::PointD const & pt, double distance) const
{
  m2::RectD const lookupRect = mercator::RectByCenterXYAndSizeInMeters(pt, distance);
  auto isCloseEnough = [&](std::vector<m2::RegionD> const & regions)
  {
    for (auto const & region : regions)
      if (region.Contains(pt) || region.AtBorder(pt, lookupRect.SizeX() / 2))
        return true;
    return false;
  };

  return WithRegion(id, isCloseEnough);
}

// CountryInfoGetterForTesting ---------------------------------------------------------------------
CountryInfoGetterForTesting::CountryInfoGetterForTesting(std::vector<CountryDef> const & countries)
{
  for (auto const & country : countries)
    AddCountry(country);
}

void CountryInfoGetterForTesting::AddCountry(CountryDef const & country)
{
  m_countries.push_back(country);
  std::string const & name = country.m_countryId;
  m_idToInfo[name].m_name = name;
}

void CountryInfoGetterForTesting::GetMatchedRegions(std::string const & affiliation, RegionIdVec & regions) const
{
  for (size_t i = 0; i < m_countries.size(); ++i)
    if (m_countries[i].m_countryId == affiliation)
      regions.push_back(i);
}

void CountryInfoGetterForTesting::ClearCachesImpl() const {}

bool CountryInfoGetterForTesting::BelongsToRegion(m2::PointD const & pt, RegionId id) const
{
  CHECK_LESS(id, m_countries.size(), ());
  return m_countries[id].m_rect.IsPointInside(pt);
}

bool CountryInfoGetterForTesting::IsIntersectedByRegion(m2::RectD const & rect, RegionId id) const
{
  CHECK_LESS(id, m_countries.size(), ());
  return rect.IsIntersect(m_countries[id].m_rect);
}

bool CountryInfoGetterForTesting::IsCloseEnough(RegionId id, m2::PointD const & pt, double distance) const
{
  CHECK_LESS(id, m_countries.size(), ());

  m2::RegionD rgn;
  rgn.AddPoint(m_countries[id].m_rect.LeftTop());
  rgn.AddPoint(m_countries[id].m_rect.RightTop());
  rgn.AddPoint(m_countries[id].m_rect.RightBottom());
  rgn.AddPoint(m_countries[id].m_rect.LeftBottom());
  rgn.AddPoint(m_countries[id].m_rect.LeftTop());

  m2::RectD const lookupRect = mercator::RectByCenterXYAndSizeInMeters(pt, distance);
  return rgn.Contains(pt) || rgn.AtBorder(pt, lookupRect.SizeX() / 2);
}
}  // namespace storage
