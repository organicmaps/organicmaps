#include "intermediate_result.hpp"

#include "../indexer/feature_utils.hpp"
#include "../indexer/mercator.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"


namespace search
{
namespace impl
{

IntermediateResult::IntermediateResult(m2::RectD const & viewportRect,
                                       FeatureType const & f,
                                       string const & displayName,
                                       string const & regionName)
  : m_str(displayName), m_region(regionName),
    m_rect(feature::GetFeatureViewport(f)),
    m_resultType(RESULT_FEATURE)
{
  FeatureType::GetTypesFn types;
  f.ForEachTypeRef(types);
  ASSERT_GREATER(types.m_size, 0, ());
  m_type = types.m_types[0];

  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
  m_searchRank = feature::GetSearchRank(f);
}

IntermediateResult::IntermediateResult(m2::RectD const & viewportRect, string const & regionName,
                                       double lat, double lon, double precision)
  : m_str("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")"),
    m_region(regionName),
    m_rect(MercatorBounds::LonToX(lon - precision), MercatorBounds::LatToY(lat - precision),
           MercatorBounds::LonToX(lon + precision), MercatorBounds::LatToY(lat + precision)),
    m_type(0), m_resultType(RESULT_LATLON), m_searchRank(0)
{
  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
}

IntermediateResult::IntermediateResult(string const & name, int penalty)
  : m_str(name), m_completionString(name + " "),
    m_distance(0), m_direction(0),
    m_resultType(RESULT_CATEGORY),
    m_searchRank(0)
{
}

bool IntermediateResult::LessOrderF::operator()
          (IntermediateResult const & r1, IntermediateResult const & r2) const
{
  if (r1.m_resultType != r2.m_resultType)
    return (r1.m_resultType < r2.m_resultType);

  if (r1.m_searchRank != r2.m_searchRank)
    return (r1.m_searchRank > r2.m_searchRank);

  return (r1.m_distance < r2.m_distance);
}

Result IntermediateResult::GenerateFinalResult() const
{
  switch (m_resultType)
  {
  case RESULT_FEATURE:
    return Result(m_str
              #ifdef DEBUG
                  + ' ' + strings::to_string(static_cast<int>(m_searchRank))
              #endif
                  , m_region, m_type, m_rect, m_distance, m_direction);
  case RESULT_LATLON:
    return Result(m_str, m_region, 0, m_rect, m_distance, m_direction);
  case RESULT_CATEGORY:
    return Result(m_str, m_completionString);
  default:
    ASSERT(false, ());
    return Result(m_str, m_completionString);
  }
}

double IntermediateResult::ResultDistance(m2::PointD const & a, m2::PointD const & b)
{
  return ms::DistanceOnEarth(MercatorBounds::YToLat(a.y), MercatorBounds::XToLon(a.x),
                             MercatorBounds::YToLat(b.y), MercatorBounds::XToLon(b.x));
}

double IntermediateResult::ResultDirection(m2::PointD const & a, m2::PointD const & b)
{
  return ang::AngleTo(a, b);
}

bool IntermediateResult::StrictEqualF::operator()(IntermediateResult const & r) const
{
  if (m_r.m_resultType == r.m_resultType && m_r.m_resultType == RESULT_FEATURE)
  {
    if (m_r.m_str == r.m_str && m_r.m_region == r.m_region && m_r.m_type == r.m_type)
    {
      return fabs(m_r.m_distance - r.m_distance) < 500.0;
    }
  }

  return false;
}

namespace
{
  uint8_t FirstLevelIndex(uint32_t t)
  {
    uint8_t v;
    CHECK(ftype::GetValue(t, 0, v), (t));
    return v;
  }

  class IsLinearChecker
  {
    static size_t const m_count = 1;
    uint8_t m_index[m_count];

  public:
    IsLinearChecker()
    {
      char const * arr[m_count] = { "highway" };

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

bool IntermediateResult::LessLinearTypesF::operator()
          (IntermediateResult const & r1, IntermediateResult const & r2) const
{
  if (r1.m_resultType != r2.m_resultType)
    return (r1.m_resultType < r2.m_resultType);

  if (r1.m_str != r2.m_str)
    return (r1.m_str < r2.m_str);

  uint8_t const i1 = FirstLevelIndex(r1.m_type);
  uint8_t const i2 = FirstLevelIndex(r2.m_type);

  if (i1 != i2)
    return (i1 < i2);

  // Should stay the best feature, after unique, so add this criteria:

  if (r1.m_searchRank != r2.m_searchRank)
    return (r1.m_searchRank > r2.m_searchRank);
  return (r1.m_distance < r2.m_distance);
}

bool IntermediateResult::EqualLinearTypesF::operator()
          (IntermediateResult const & r1, IntermediateResult const & r2) const
{
  if (r1.m_resultType == r2.m_resultType && r1.m_str == r2.m_str)
  {
    // filter equal linear features
    static IsLinearChecker checker;

    uint8_t const ind = FirstLevelIndex(r1.m_type);
    return (ind == FirstLevelIndex(r2.m_type) && checker.IsMy(ind));
  }

  return false;
}

string IntermediateResult::DebugPrint() const
{
  string res("IntermediateResult: ");
  res += "Name: " + m_str;
  res += "; Type: " + ::DebugPrint(m_type);
  res += "; Result type: " + ::DebugPrint(m_resultType);
  return res;
}

}  // namespace search::impl
}  // namespace search
