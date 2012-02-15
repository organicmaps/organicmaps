#include "intermediate_result.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_utils.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/categories_holder.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"


namespace search
{
namespace impl
{

double ResultDistance(m2::PointD const & a, m2::PointD const & b)
{
  return ms::DistanceOnEarth(MercatorBounds::YToLat(a.y), MercatorBounds::XToLon(a.x),
                             MercatorBounds::YToLat(b.y), MercatorBounds::XToLon(b.x));
}

uint8_t ViewportDistance(m2::RectD const & viewport, m2::PointD const & p)
{
  if (viewport.IsPointInside(p))
    return 0;

  m2::RectD r = viewport;
  r.Scale(3);
  if (r.IsPointInside(p))
    return 1;

  r = viewport;
  r.Scale(5);
  if (r.IsPointInside(p))
    return 2;

  return 3;
}


PreResult1::PreResult1(uint32_t fID, uint8_t rank, m2::PointD const & center, size_t mwmID,
                       m2::PointD const & pos, m2::RectD const & viewport)
  : m_center(center),
    m_mwmID(mwmID),
    m_featureID(fID),
    m_rank(rank)
{
  CalcParams(viewport, pos);
}

void PreResult1::CalcParams(m2::RectD const & viewport, m2::PointD const & pos)
{
  // Check if point is valid (see Query::empty_pos_value).
  if (pos.x > -500 && pos.y > -500)
  {
    ASSERT ( my::between_s(-180.0, 180.0, pos.x), (pos.x) );
    ASSERT ( my::between_s(-180.0, 180.0, pos.y), (pos.y) );

    m_distance = ResultDistance(m_center, pos);
  }
  else
  {
    // empty distance
    m_distance = -1.0;
  }

  m_viewportDistance = ViewportDistance(viewport, m_center);
}

bool PreResult1::LessRank(PreResult1 const & r1, PreResult1 const & r2)
{
  return (r1.m_rank > r2.m_rank);
}

bool PreResult1::LessDistance(PreResult1 const & r1, PreResult1 const & r2)
{
  return (r1.m_distance < r2.m_distance);
}

bool PreResult1::LessViewportDistance(PreResult1 const & r1, PreResult1 const & r2)
{
  return (r1.m_viewportDistance < r2.m_viewportDistance);
}



PreResult2::PreResult2(FeatureType const & f, PreResult1 const & res,
                       string const & displayName, string const & fileName)
  : m_types(f),
    m_str(displayName),
    m_center(res.m_center),
    m_distance(res.m_distance),
    m_resultType(RESULT_FEATURE),
    m_searchRank(res.m_rank),
    m_viewportDistance(res.m_viewportDistance)
{
  ASSERT_GREATER(m_types.Size(), 0, ());

  // get region info
  if (!fileName.empty())
    m_region.SetName(fileName);
  else
    m_region.SetPoint(m_center);
}

PreResult2::PreResult2(m2::RectD const & viewport, m2::PointD const & pos,
                       double lat, double lon)
  : m_str("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")"),
    m_resultType(RESULT_LATLON), m_searchRank(255)
{
  // dummy object to avoid copy-paste
  PreResult1 res(0, 0, m2::PointD(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat)),
                 0, pos, viewport);

  m_center = res.m_center;
  m_distance = res.m_distance;
  m_viewportDistance = res.m_viewportDistance;

  // get region info
  m_region.SetPoint(m_center);
}

PreResult2::PreResult2(string const & name, int penalty)
  : m_str(name), m_completionString(name + " "),
    // Categories should always be first.
    m_distance(-1000.0),    // smallest distance :)
    m_resultType(RESULT_CATEGORY),
    m_searchRank(255),      // best rank
    m_viewportDistance(0)   // closest to viewport
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
                        CategoriesHolder const * pCat, int8_t lang) const
{
  storage::CountryInfo info;

  uint32_t const type = GetBestType();

  static SkipRegionInfo checker;
  if (type != 0 && !checker.IsContinent(type))
  {
    m_region.GetRegion(pInfo, info);

    if (checker.IsCountry(type))
      info.m_name.clear();
  }

  switch (m_resultType)
  {
  case RESULT_FEATURE:
    return Result(m_str, info.m_name, info.m_flag, GetFeatureType(pCat, lang)
              #ifdef DEBUG
                  + ' ' + strings::to_string(static_cast<int>(m_searchRank))
              #endif
                  ,
                  type, feature::GetFeatureViewport(m_types, m_center), m_distance);

  case RESULT_LATLON:
    return Result(m_str, info.m_name, info.m_flag, string(), 0,
                  scales::GetRectForLevel(scales::GetUpperScale(), m_center, 1.0), m_distance);

  default:
    ASSERT_EQUAL ( m_resultType, RESULT_CATEGORY, () );
    return Result(m_str, m_completionString);
  }
}

bool PreResult2::LessRank(PreResult2 const & r1, PreResult2 const & r2)
{
  return (r1.m_searchRank > r2.m_searchRank);
}

bool PreResult2::LessDistance(PreResult2 const & r1, PreResult2 const & r2)
{
  return (r1.m_distance < r2.m_distance);
}

bool PreResult2::LessViewportDistance(PreResult2 const & r1, PreResult2 const & r2)
{
  return (r1.m_viewportDistance < r2.m_viewportDistance);
}

bool PreResult2::StrictEqualF::operator() (PreResult2 const & r) const
{
  if (m_r.m_resultType == r.m_resultType && m_r.m_resultType == RESULT_FEATURE)
  {
    if (m_r.m_str == r.m_str && m_r.GetBestType() == r.GetBestType())
    {
      // 100.0m - distance between equal features
      return (ResultDistance(m_r.m_center, r.m_center) < 100.0);
    }
  }

  return false;
}

namespace
{
  uint8_t FirstLevelIndex(uint32_t t)
  {
    uint8_t v;
    VERIFY ( ftype::GetValue(t, 0, v), (t) );
    return v;
  }

  class IsLinearChecker
  {
    static size_t const m_count = 2;
    uint8_t m_index[m_count];

  public:
    IsLinearChecker()
    {
      char const * arr[] = { "highway", "waterway" };
      STATIC_ASSERT ( ARRAY_SIZE(arr) == m_count );

      ClassifObject const * c = classif().GetRoot();
      for (size_t i = 0; i < m_count; ++i)
        m_index[i] = static_cast<uint8_t>(c->BinaryFind(arr[i]).GetIndex());
    }

    bool IsMy(uint8_t ind) const
    {
      for (size_t i = 0; i < m_count; ++i)
        if (ind == m_index[i])
          return true;

      return false;
    }
  };
}

bool PreResult2::LessLinearTypesF::operator() (PreResult2 const & r1, PreResult2 const & r2) const
{
  if (r1.m_resultType != r2.m_resultType)
    return (r1.m_resultType < r2.m_resultType);

  if (r1.m_str != r2.m_str)
    return (r1.m_str < r2.m_str);

  if (r1.GetBestType() != r2.GetBestType())
    return (r1.GetBestType() < r2.GetBestType());

  // Should stay the best feature, after unique, so add this criteria:

  if (r1.m_searchRank != r2.m_searchRank)
    return (r1.m_searchRank > r2.m_searchRank);
  return (r1.m_distance < r2.m_distance);
}

bool PreResult2::EqualLinearTypesF::operator() (PreResult2 const & r1, PreResult2 const & r2) const
{
  if (r1.m_resultType == r2.m_resultType && r1.m_str == r2.m_str)
  {
    // filter equal linear features
    static IsLinearChecker checker;
    return (r1.GetBestType() == r2.GetBestType() &&
            checker.IsMy(FirstLevelIndex(r1.GetBestType())));
  }

  return false;
}

string PreResult2::DebugPrint() const
{
  string res("IntermediateResult: ");
  res += "Name: " + m_str;
  res += "; Type: " + ::DebugPrint(GetBestType());
  res += "; Rank: " + ::DebugPrint(m_searchRank);
  res += "; Viewport distance: " + ::DebugPrint(m_viewportDistance);
  res += "; Distance: " + ::DebugPrint(m_distance);
  return res;
}

string PreResult2::GetFeatureType(CategoriesHolder const * pCat, int8_t lang) const
{
  ASSERT_EQUAL(m_resultType, RESULT_FEATURE, ());

  uint32_t const type = GetBestType();
  ASSERT_NOT_EQUAL(type, 0, ());

  if (pCat)
  {
    string name;
    if (pCat->GetNameByType(type, lang, name))
      return name;
  }

  string s = classif().GetFullObjectName(type);

  // remove ending dummy symbol
  ASSERT ( !s.empty(), () );
  s.resize(s.size()-1);

  // replace separator
  replace(s.begin(), s.end(), '|', '-');
  return s;
}

void PreResult2::RegionInfo::GetRegion(
    storage::CountryInfoGetter const * pInfo, storage::CountryInfo & info) const
{
  if (!m_file.empty())
    pInfo->GetRegionInfo(m_file, info);
  else if (m_valid)
    pInfo->GetRegionInfo(m_point, info);
}

}  // namespace search::impl
}  // namespace search
