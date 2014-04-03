#include "intermediate_result.hpp"
#include "ftypes_matcher.hpp"
#include "geometry_utils.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/categories_holder.hpp"

#include "../geometry/angles.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"


namespace search
{

/// All constants in meters.
double const DIST_EQUAL_RESULTS = 100.0;
double const DIST_SAME_STREET = 5000.0;

namespace impl
{

template <class T> bool LessViewportDistanceT(T const & r1, T const & r2)
{
  if (r1.m_viewportDistance != r2.m_viewportDistance)
    return (r1.m_viewportDistance < r2.m_viewportDistance);

  if (r1.m_rank != r2.m_rank)
    return (r1.m_rank > r2.m_rank);

  return (r1.m_distanceFromViewportCenter < r2.m_distanceFromViewportCenter);
}

template <class T> bool LessRankT(T const & r1, T const & r2)
{
  if (r1.m_rank != r2.m_rank)
    return (r1.m_rank > r2.m_rank);

  if (r1.m_viewportDistance != r2.m_viewportDistance)
    return (r1.m_viewportDistance < r2.m_viewportDistance);

  if (r1.m_distance != r2.m_distance)
    return (r1.m_distance < r2.m_distance);

  return (r1.m_distanceFromViewportCenter < r2.m_distanceFromViewportCenter);
}

template <class T> bool LessDistanceT(T const & r1, T const & r2)
{
  if (r1.m_distance != r2.m_distance)
    return (r1.m_distance < r2.m_distance);

  if (r1.m_rank != r2.m_rank)
    return (r1.m_rank > r2.m_rank);

  return (r1.m_distanceFromViewportCenter < r2.m_distanceFromViewportCenter);
}

PreResult1::PreResult1(FeatureID const & fID, uint8_t rank, m2::PointD const & center,
                       m2::PointD const & pos, m2::RectD const & viewport, int8_t viewportID)
  : m_id(fID),
    m_center(center),
    m_rank(rank),
    m_viewportID(viewportID)
{
  ASSERT(m_id.IsValid(), ());

  CalcParams(viewport, pos);
}

PreResult1::PreResult1(m2::PointD const & center, m2::PointD const & pos, m2::RectD const & viewport)
  : m_center(center)
{
  CalcParams(viewport, pos);
}

namespace
{

void AssertValid(m2::PointD const & p)
{
  ASSERT ( my::between_s(-180.0, 180.0, p.x), (p.x) );
  ASSERT ( my::between_s(-180.0, 180.0, p.y), (p.y) );
}

}

void PreResult1::CalcParams(m2::RectD const & viewport, m2::PointD const & pos)
{
  AssertValid(m_center);

  // Check if point is valid (see Query::empty_pos_value).
  if (pos.x > -500 && pos.y > -500)
  {
    AssertValid(pos);
    m_distance = PointDistance(m_center, pos);
  }
  else
  {
    // empty distance
    m_distance = -1.0;
  }

  m_viewportDistance = ViewportDistance(viewport, m_center);
  m_distanceFromViewportCenter = PointDistance(m_center, viewport.Center());
}

bool PreResult1::LessRank(PreResult1 const & r1, PreResult1 const & r2)
{
  return LessRankT(r1, r2);
}

bool PreResult1::LessDistance(PreResult1 const & r1, PreResult1 const & r2)
{
  return LessDistanceT(r1, r2);
}

bool PreResult1::LessViewportDistance(PreResult1 const & r1, PreResult1 const & r2)
{
  return LessViewportDistanceT(r1, r2);
}


void PreResult2::CalcParams(m2::RectD const & viewport, m2::PointD const & pos)
{
  // dummy object to avoid copy-paste
  PreResult1 res(GetCenter(), pos, viewport);

  m_distance = res.m_distance;
  m_distanceFromViewportCenter = res.m_distanceFromViewportCenter;
  m_viewportDistance = res.m_viewportDistance;
}

PreResult2::PreResult2(FeatureType const & f, PreResult1 const * p,
                       m2::RectD const & viewport, m2::PointD const & pos,
                       string const & displayName, string const & fileName)
  : m_id(f.GetID()),
    m_types(f),
    m_str(displayName),
    m_resultType(RESULT_FEATURE),
    m_geomType(f.GetFeatureType())
{
  ASSERT(m_id.IsValid(), ());
  ASSERT(!m_types.Empty(), ());

  m_types.SortBySpec();

  m_rank = p ? p->GetRank() : 0;

  m2::PointD fCenter;
  if (p && f.GetFeatureType() != feature::GEOM_POINT)
  {
    // Optimization tip - use precalculated center point if possible.
    fCenter = p->GetCenter();
  }
  else
    fCenter = f.GetLimitRect(FeatureType::WORST_GEOMETRY).Center();

  m_region.SetParams(fileName, fCenter);
  CalcParams(viewport, pos);
}

PreResult2::PreResult2(m2::RectD const & viewport, m2::PointD const & pos, double lat, double lon)
  : m_str("(" + strings::to_string_dac(lat, 5) + ", " + strings::to_string_dac(lon, 5) + ")"),
    m_resultType(RESULT_LATLON),
    m_rank(255),
    m_geomType(feature::GEOM_UNDEFINED)
{
  m2::PointD const fCenter(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
  m_region.SetParams(string(), fCenter);
  CalcParams(viewport, pos);
}

PreResult2::PreResult2(string const & name, int penalty)
  : m_str(name), m_completionString(name + " "),

    // Categories should always be the first:
    m_distance(-1000.0),    // smallest distance :)
    m_distanceFromViewportCenter(-1000.0),
    m_resultType(RESULT_CATEGORY),
    m_rank(255),            // best rank
    m_viewportDistance(0),  // closest to viewport
    m_geomType(feature::GEOM_UNDEFINED)
{
}

namespace
{
  class SkipRegionInfo
  {
    static size_t const m_count = 2;
    uint32_t m_types[m_count];

  public:
    SkipRegionInfo()
    {
      char const * arr[][2] = {
        { "place", "continent" },
        { "place", "country" }
      };
      STATIC_ASSERT ( m_count == ARRAY_SIZE(arr) );

      Classificator const & c = classif();
      for (size_t i = 0; i < m_count; ++i)
        m_types[i] = c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2));
    }

    bool IsContinent(uint32_t t) const { return (m_types[0] == t); }
    bool IsCountry(uint32_t t) const { return (m_types[1] == t); }
  };
}

Result PreResult2::GenerateFinalResult(
                        storage::CountryInfoGetter const * pInfo,
                        CategoriesHolder const * pCat,
                        set<uint32_t> const * pTypes,
                        int8_t lang) const
{
  storage::CountryInfo info;

  uint32_t const type = GetBestType();

  static SkipRegionInfo checker;
  if (!checker.IsContinent(type))
  {
    m_region.GetRegion(pInfo, info);

    if (checker.IsCountry(type))
      info.m_name.clear();
  }

  switch (m_resultType)
  {
  case RESULT_FEATURE:
    return Result(m_id, GetCenter(), m_str, info.m_name, info.m_flag, GetFeatureType(pCat, pTypes, lang)
              #ifdef DEBUG
                  + ' ' + strings::to_string(static_cast<int>(m_rank))
              #endif
                  , type, m_distance);

  case RESULT_LATLON:
    return Result(GetCenter(), m_str, info.m_name, info.m_flag, m_distance);

  default:
    ASSERT_EQUAL ( m_resultType, RESULT_CATEGORY, () );
    return Result(m_str, m_completionString);
  }
}

bool PreResult2::LessRank(PreResult2 const & r1, PreResult2 const & r2)
{
  return LessRankT(r1, r2);
}

bool PreResult2::LessDistance(PreResult2 const & r1, PreResult2 const & r2)
{
  return LessDistanceT(r1, r2);
}

bool PreResult2::LessViewportDistance(PreResult2 const & r1, PreResult2 const & r2)
{
  return LessViewportDistanceT(r1, r2);
}

bool PreResult2::StrictEqualF::operator() (PreResult2 const & r) const
{
  if (m_r.m_resultType == r.m_resultType && m_r.m_resultType == RESULT_FEATURE)
  {
    if (m_r.m_str == r.m_str && m_r.GetBestType() == r.GetBestType())
      return (PointDistance(m_r.GetCenter(), r.GetCenter()) < DIST_EQUAL_RESULTS);
  }

  return false;
}

bool PreResult2::LessLinearTypesF::operator() (PreResult2 const & r1, PreResult2 const & r2) const
{
  if (r1.m_geomType != r2.m_geomType)
    return (r1.m_geomType < r2.m_geomType);

  if (r1.m_str != r2.m_str)
    return (r1.m_str < r2.m_str);

  uint32_t const t1 = r1.GetBestType();
  uint32_t const t2 = r2.GetBestType();
  if (t1 != t2)
    return (t1 < t2);

  // Should stay the best feature, after unique, so add this criteria:
  if (r1.m_viewportDistance != r2.m_viewportDistance)
    return (r1.m_viewportDistance < r2.m_viewportDistance);
  return (r1.m_distance < r2.m_distance);
}

bool PreResult2::EqualLinearTypesF::operator() (PreResult2 const & r1, PreResult2 const & r2) const
{
  // Note! Do compare for distance when filtering linear objects.
  // Otherwise we will skip the results for different parts of the map.
  return (r1.m_geomType == r2.m_geomType &&
          r1.m_geomType == feature::GEOM_LINE &&
          r1.m_str == r2.m_str &&
          PointDistance(r1.GetCenter(), r2.GetCenter()) < DIST_SAME_STREET);
}

bool PreResult2::IsStreet() const
{
  static ftypes::IsStreetChecker checker;
  return (m_geomType == feature::GEOM_LINE && checker(m_types));
}

string PreResult2::DebugPrint() const
{
  string res("IntermediateResult: ");
  res += "Name: " + m_str;
  res += "; Type: " + ::DebugPrint(GetBestType());
  res += "; Rank: " + ::DebugPrint(m_rank);
  res += "; Viewport distance: " + ::DebugPrint(m_viewportDistance);
  res += "; Distance: " + ::DebugPrint(m_distance);
  return res;
}

uint32_t PreResult2::GetBestType(set<uint32_t> const * pPrefferedTypes) const
{
  uint32_t t = 0;

  if (pPrefferedTypes)
  {
    for (size_t i = 0; i < m_types.Size(); ++i)
      if (pPrefferedTypes->count(m_types[i]) > 0)
      {
        t = m_types[i];
        break;
      }
  }

  if (t == 0)
  {
    t = m_types.GetBestType();

    // Do type truncate (2-level is enough for search results) only for
    // non-preffered types (types from categories leave original).
    ftype::TruncValue(t, 2);
  }

  return t;
}

string PreResult2::GetFeatureType(CategoriesHolder const * pCat,
                                  set<uint32_t> const * pTypes,
                                  int8_t lang) const
{
  ASSERT_EQUAL(m_resultType, RESULT_FEATURE, ());

  uint32_t const type = GetBestType(pTypes);
  ASSERT_NOT_EQUAL(type, 0, ());

  if (pCat)
  {
    string name;
    if (pCat->GetNameByType(type, lang, name))
      return name;
  }

  return classif().GetReadableObjectName(type);
}

void PreResult2::RegionInfo::GetRegion(storage::CountryInfoGetter const * pInfo,
                                       storage::CountryInfo & info) const
{
  if (!m_file.empty())
    pInfo->GetRegionInfo(m_file, info);
  else
    pInfo->GetRegionInfo(m_point, info);
}

}  // namespace search::impl
}  // namespace search
