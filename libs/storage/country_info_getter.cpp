#include "storage/country_info_getter.hpp"

#include "storage/country_decl.hpp"

#include "indexer/feature.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect_intersect.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/logging.hpp"

#include <boost/polygon/polygon.hpp>

namespace storage
{
struct CountryInfoReader::Coverage
{
  std::vector<CountryId> m_countries;
  struct Polygon
  {
    m2::RegionD m_outer;
    std::vector<m2::RegionD> m_holes;
  };
  std::vector<Polygon> m_polygons;
  std::vector<std::pair<m2::PointD, m2::PointD>> m_edges;
  m4::Tree<size_t> m_index;

  bool Contains(m2::RectD const & rect) const
  {
    bool boundary = false;
    m_index.ForEachInRect(rect, [&](size_t i)
    {
      auto [a, b] = m_edges[i];
      boundary = boundary || m2::Intersect(rect, a, b);
    });
    if (boundary)
      return false;
    auto const point = rect.Center();
    return std::any_of(m_polygons.begin(), m_polygons.end(), [&](auto const & polygon)
    {
      return polygon.m_outer.Contains(point) && std::none_of(polygon.m_holes.begin(), polygon.m_holes.end(),
                                                             [&](auto const & hole) { return hole.Contains(point); });
    });
  }
};

// CountryInfoGetterBase ---------------------------------------------------------------------------
CountryId CountryInfoGetterBase::GetRegionCountryId(m2::PointD const & pt) const
{
  RegionId const id = FindFirstCountry(pt);
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
  m2::PointD const wrapped(mercator::WrapX(pt.x), pt.y);
  for (size_t id = 0; id < m_countries.size(); ++id)
    if (BelongsToRegion(wrapped, id))
      return id;

  return kInvalidId;
}

// CountryInfoGetter -------------------------------------------------------------------------------
std::vector<CountryId> CountryInfoGetter::GetRegionsCountryIdByRect(m2::RectD rect, bool rough) const
{
  // Wrap input rect into canonical range for further IsIntersectOrInside and IsIntersectedByRegion calls
  // (which handle rects crossing the antimeridian).
  rect = mercator::WrapRectX(rect);

  std::vector<CountryId> result;
  for (size_t id = 0; id < m_countries.size(); ++id)
  {
    auto const res = CountryDef::IsIntersectOrInside(rect, m_countries[id].m_rect);
    if (res == CountryDef::Overlap::NONE)
      continue;

    if (res == CountryDef::Overlap::INSIDE || rough || IsIntersectedByRegion(rect, id))
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
  RegionId const id = FindFirstCountry(pt);
  if (id != kInvalidId)
    GetRegionInfo(m_countries[id].m_countryId, info);
}

void CountryInfoGetter::GetRegionInfo(CountryId const & countryId, CountryInfo & info) const
{
  info.m_name = countryId;
  CountryInfo::FileName2FullName(info.m_name);
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
  if (it == m_countryIndex.end())
  {
    // World files and obsolete country names may be absent from the current border index.
    return mercator::Bounds::FullRect();
  }
  ASSERT_LESS(it->second, m_countries.size(), ());
  return m_countries[it->second].m_rect;
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
  std::lock_guard coverageLock(m_coverageMutex);
  m_coverage.reset();
  std::lock_guard lock(m_readerMutex);

  m_polyCache.ForEachValue([](std::vector<m2::RegionD> & v) { std::vector<m2::RegionD>().swap(v); });
  m_polyCache.Reset();

  m_trgCache.ForEachValue([](std::vector<m2::PointD> & v) { std::vector<m2::PointD>().swap(v); });
  m_trgCache.Reset();
}

template <class Fn>
auto CountryInfoReader::WithRegion(RegionId id, Fn && fn) const
{
  std::lock_guard lock(m_readerMutex);

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

bool CountryInfoReader::IsRectCoveredByRegion(CountryId const & countryId, m2::RectD const & rect) const
{
  return IsRectCoveredByRegions({countryId}, rect);
}

bool CountryInfoReader::IsRectCoveredByRegions(std::vector<CountryId> const & countries, m2::RectD const & rect) const
{
  if (countries.empty())
    return false;
  if (!mercator::Bounds::FullRect().IsRectInside(rect))
    return false;
  using Coord = int32_t;
  using namespace boost::polygon;
  std::lock_guard lock(m_coverageMutex);
  if (!m_coverage || m_coverage->m_countries != countries)
  {
    auto prepared = std::make_shared<Coverage>();
    prepared->m_countries = countries;
    polygon_set_data<Coord> covered;
    for (auto const & country : countries)
    {
      auto const id = GetRegionId(country);
      if (id == kInvalidId)
        return false;
      WithRegion(id, [&](std::vector<m2::RegionD> const & regions)
      {
        for (auto const & region : regions)
        {
          std::vector<point_data<Coord>> points;
          for (auto const & point : region.Data())
          {
            auto const encoded = PointDToPointU(point, kPointCoordBits);
            points.emplace_back(encoded.x, encoded.y);
          }
          polygon_data<Coord> polygon;
          set_points(polygon, points.begin(), points.end());
          covered.insert(polygon);
        }
      });
    }
    std::vector<polygon_with_holes_data<Coord>> polygons;
    covered.get(polygons);
    auto const ring = [&](auto const & polygon)
    {
      std::vector<m2::PointD> points;
      for (auto it = begin_points(polygon); it != end_points(polygon); ++it)
        points.emplace_back(x(*it), y(*it));
      for (size_t i = 0; i < points.size(); ++i)
      {
        auto const & a = points[i];
        auto const & b = points[(i + 1) % points.size()];
        prepared->m_index.Add(prepared->m_edges.size(), m2::RectD(a, b));
        prepared->m_edges.emplace_back(a, b);
      }
      return m2::RegionD(std::move(points));
    };
    for (auto const & polygon : polygons)
    {
      Coverage::Polygon result{ring(polygon), {}};
      for (auto hole = begin_holes(polygon); hole != end_holes(polygon); ++hole)
        result.m_holes.push_back(ring(*hole));
      prepared->m_polygons.push_back(std::move(result));
    }
    prepared->m_index.Optimize();
    m_coverage = std::move(prepared);
  }
  // Cache the union, including holes, and index its remaining boundary once.
  // Shared region seams disappear before per-segment coverage queries. Round
  // outwards by one packed coordinate unit, not a map-simplification buffer:
  // that buffer can cross a border even when the requested water is covered.
  auto const low = PointDToPointU(rect.LeftBottom(), kPointCoordBits);
  auto const high = PointDToPointU(rect.RightTop(), kPointCoordBits);
  return m_coverage->Contains(m2::RectD(static_cast<double>(low.x) - 1, static_cast<double>(low.y) - 1,
                                        static_cast<double>(high.x) + 1, static_cast<double>(high.y) + 1));
}
void CountryInfoReader::GetTriangles(RegionId id, FeatureType & ft) const
{
  std::lock_guard lock(m_readerMutex);

  bool isFound = false;
  auto & trgs = m_trgCache.Find(static_cast<uint32_t>(id), isFound);

  if (!isFound)
  {
    ReaderSource<ModelReaderPtr> src(m_reader.GetReader("t" + std::to_string(id)));
    trgs.clear();
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
  return WithRegion(id, [&rect](std::vector<m2::RegionD> const & regions)
  {
    bool intersects = false;
    mercator::ForEachRectWrapped(rect, [&](m2::RectD const & part)
    {
      for (auto const & region : regions)
      {
        if (intersects)
          return;
        ASSERT(region.IsValid(), ());
        // Containment in either direction can have no boundary intersections.
        if (part.IsPointInside(region.Data().front()) || region.Contains(part.Center()))
        {
          intersects = true;
          return;
        }
        part.ForEachSide([&](m2::PointD const & p1, m2::PointD const & p2)
        {
          if (!intersects)
          {
            m2::PointD result;
            intersects = region.FindIntersection(p1, p2, result);
          }
        });
      }
    });
    return intersects;
  });
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
  m_countryIndex[country.m_countryId] = m_countries.size();
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
