#include "storage/country_info_getter.hpp"

#include "storage/country_decl.hpp"

#include "indexer/feature.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/region2d.hpp"

#include "base/logging.hpp"

namespace storage
{

// CountryInfoGetterBase ---------------------------------------------------------------------------
CountryId CountryInfoGetterBase::GetRegionCountryId(m2::PointD const & pt) const
{
  // Wrap X to [-180, 180] so extended coordinates (past the antimeridian) map to
  // the correct country. Country boundaries and polygons are in canonical coordinates.
  m2::PointD const wrapped(mercator::WrapX(pt.x), pt.y);
  RegionId const id = FindFirstCountry(wrapped);
  return id == kInvalidId ? kInvalidCountryId : m_countries[id].m_countryId;
}

bool CountryInfoGetterBase::BelongsToAnyRegion(m2::PointD const & pt, RegionIdVec const & regions) const
{
  m2::PointD const wrapped(mercator::WrapX(pt.x), pt.y);
  for (auto const & id : regions)
    if (BelongsToRegion(wrapped, id))
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
std::vector<CountryId> CountryInfoGetter::GetRegionsCountryIdByRect(m2::RectD rect, bool rough) const
{
  // Normalize extended rect X into canonical range. IsRectOverlap already handles +-360 shifts,
  // but exact polygon checks (IsIntersectedByRegion) require canonical coordinates.
  if (rect.minX() >= mercator::Bounds::kMaxX || rect.maxX() <= mercator::Bounds::kMinX)
  {
    double const startX = mercator::WrapX(rect.minX());
    rect = m2::RectD(startX, rect.minY(), startX + rect.SizeX(), rect.maxY());
  }

  std::vector<CountryId> result;
  for (size_t id = 0; id < m_countries.size(); ++id)
  {
    if (!m_countries[id].IsRectOverlap(rect))
      continue;
    if (rect.IsRectInside(m_countries[id].m_rect) || rough || IsIntersectedByRegion(rect, id))
      result.push_back(m_countries[id].m_countryId);
  }
  return result;
}

void CountryInfoGetter::GetRegionsCountryId(m2::PointD const & pt, CountriesVec & closestCoutryIds,
                                            double lookupRadiusM) const
{
  closestCoutryIds.clear();

  m2::PointD const wrapped(mercator::WrapX(pt.x), pt.y);
  m2::RectD const lookupRect = mercator::RectByCenterXYAndSizeInMeters(wrapped, lookupRadiusM);

  for (size_t id = 0; id < m_countries.size(); ++id)
    if (m_countries[id].IsRectOverlap(lookupRect) && IsCloseEnough(id, wrapped, lookupRadiusM))
      closestCoutryIds.emplace_back(m_countries[id].m_countryId);
}

void CountryInfoGetter::GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const
{
  m2::PointD const wrapped(mercator::WrapX(pt.x), pt.y);
  RegionId const id = FindFirstCountry(wrapped);
  if (id != kInvalidId)
    GetRegionInfo(m_countries[id].m_countryId, info);
}

void CountryInfoGetter::GetRegionInfo(CountryId const & countryId, CountryInfo & info) const
{
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
    CountryInfoReader * result = new CountryInfoReader(platform.GetReader(PACKED_POLYGONS_FILE));
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
  ReaderSource<ModelReaderPtr> src(m_reader.GetReader(std::to_string(id)));

  uint32_t const count = ReadVarUint<uint32_t>(src);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result.emplace_back(std::move(points));
  }
  return result;
}

CountryInfoReader::CountryInfoReader(ModelReaderPtr polyR)
  : m_reader(polyR)
  , m_polyCache(3 /* logCacheSize */)
  , m_trgCache(6 /* logCacheSize */)
{
  ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
  rw::Read(src, m_countries);

  m_countryIndex.reserve(m_countries.size());
  for (RegionId i = 0; i < m_countries.size(); ++i)
    m_countryIndex[m_countries[i].m_countryId] = i;
}

void CountryInfoReader::ClearCachesImpl() const
{
  {
    std::lock_guard lock(m_polyMutex);

    m_polyCache.ForEachValue([](std::vector<m2::RegionD> & v) { std::vector<m2::RegionD>().swap(v); });
    m_polyCache.Reset();
  }

  {
    std::lock_guard lock(m_trgMutex);

    m_trgCache.ForEachValue([](std::vector<m2::PointD> & v) { std::vector<m2::PointD>().swap(v); });
    m_trgCache.Reset();
  }
}

template <class Fn>
auto CountryInfoReader::WithRegion(RegionId id, Fn && fn) const
{
  std::lock_guard lock(m_polyMutex);

  bool isFound = false;
  auto & regions = m_polyCache.Find(static_cast<uint32_t>(id), isFound);

  if (!isFound)
    regions = LoadRegionsFromDisk(id);

  return fn(regions);
}

bool CountryInfoReader::HasRegionTriangles() const
{
  return m_reader.IsExist("t0");
}

void CountryInfoReader::GetTriangles(RegionId id, FeatureType & ft) const
{
  std::lock_guard lock(m_trgMutex);

  bool isFound = false;
  auto & trgs = m_trgCache.Find(static_cast<uint32_t>(id), isFound);

  if (!isFound)
  {
    ReaderSource<ModelReaderPtr> src(m_reader.GetReader("t" + std::to_string(id)));
    serial::LoadOuterTriangles(src, serial::GeometryCodingParams(), trgs);
  }

  ft.SetTriangles(trgs);
}

bool CountryInfoReader::BelongsToRegion(m2::PointD const & pt, RegionId id) const
{
  if (!m_countries[id].IsPointInsideRect(pt))
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
}

void CountryInfoGetterForTesting::GetMatchedRegions(std::string const & affiliation, RegionIdVec & regions) const
{
  for (size_t i = 0; i < m_countries.size(); ++i)
  {
    auto const & id = m_countries[i].m_countryId;
    size_t const pos = id.find(affiliation);
    if (pos != std::string::npos && (pos == 0 || id[pos - 1] == '_'))
      regions.push_back(i);
  }
}

void CountryInfoGetterForTesting::ClearCachesImpl() const {}

bool CountryInfoGetterForTesting::BelongsToRegion(m2::PointD const & pt, RegionId id) const
{
  CHECK_LESS(id, m_countries.size(), ());
  return m_countries[id].IsPointInsideRect(pt);
}

bool CountryInfoGetterForTesting::IsIntersectedByRegion(m2::RectD const & rect, RegionId id) const
{
  CHECK_LESS(id, m_countries.size(), ());
  return m_countries[id].IsRectOverlap(rect);
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
