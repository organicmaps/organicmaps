#include "intermediate_result.hpp"
#include "../indexer/feature_utils.hpp"
#include "../indexer/mercator.hpp"
#include "../geometry/angles.hpp"
#include "../geometry/distance_on_sphere.hpp"
#include "../base/string_utils.hpp"

namespace search
{
namespace impl
{

IntermediateResult::IntermediateResult(m2::RectD const & viewportRect,
                                       FeatureType const & feature,
                                       string const & displayName)
  : m_str(displayName), m_rect(feature::GetFeatureViewport(feature)),
    m_resultType(RESULT_FEATURE)
{
  FeatureType::GetTypesFn types;
  feature.ForEachTypeRef(types);
  ASSERT_GREATER(types.m_size, 0, ());
  m_type = types.m_types[0];
  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
  m_searchRank = feature::GetSearchRank(feature);
}

IntermediateResult::IntermediateResult(m2::RectD const & viewportRect,
                                       double lat, double lon, double precision)
  : m_str("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")"),
    m_rect(MercatorBounds::LonToX(lon - precision), MercatorBounds::LatToY(lat - precision),
           MercatorBounds::LonToX(lon + precision), MercatorBounds::LatToY(lat + precision)),
    m_type(0), m_resultType(RESULT_LATLON), m_searchRank(0)
{
  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
}

IntermediateResult::IntermediateResult(string const & name, string const & completionString, int penalty)
  : m_str(name), m_completionString(completionString),
    m_distance(0), m_direction(0),
    m_resultType(RESULT_CATEGORY),
    m_searchRank(0)
{
}

bool IntermediateResult::operator < (IntermediateResult const & o) const
{
  if (m_resultType != o.m_resultType)
    return m_resultType < o.m_resultType;
  if (m_searchRank != o.m_searchRank)
    return m_searchRank > o.m_searchRank;
  if (m_distance != o.m_distance)
    return m_distance < o.m_distance;
  return false;
}

Result IntermediateResult::GenerateFinalResult() const
{
  switch (m_resultType)
  {
  case RESULT_FEATURE:
    return Result(m_str + ' ' + strings::to_string(static_cast<int>(m_searchRank)),
                  m_type, m_rect, m_distance, m_direction);
  case RESULT_LATLON:
    return Result(m_str, 0, m_rect, m_distance, m_direction);
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

}  // namespace search::impl
}  // namespace search
