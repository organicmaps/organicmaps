#include "intermediate_result.hpp"
#include "../indexer/feature_rect.hpp"
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
                                       string const & displayName,
                                       int matchPenalty,
                                       int minVisibleScale)
  : m_str(displayName), m_rect(feature::GetFeatureViewport(feature)), m_matchPenalty(matchPenalty),
    m_minVisibleScale(minVisibleScale), m_resultType(RESULT_FEATURE)
{
  FeatureType::GetTypesFn types;
  feature.ForEachTypeRef(types);
  ASSERT_GREATER(types.m_size, 0, ());
  m_type = types.m_types[0];
  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
}

IntermediateResult::IntermediateResult(m2::RectD const & viewportRect,
                                       double lat, double lon, double precision)
  : m_str("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")"),
    m_rect(MercatorBounds::LonToX(lon - precision), MercatorBounds::LatToY(lat - precision),
           MercatorBounds::LonToX(lon + precision), MercatorBounds::LatToY(lat + precision)),
    m_type(0), m_matchPenalty(0), m_minVisibleScale(0), m_resultType(RESULT_LATLON)
{
  m_distance = ResultDistance(viewportRect.Center(), m_rect.Center());
  m_direction = ResultDirection(viewportRect.Center(), m_rect.Center());
}

{
}

bool IntermediateResult::operator < (IntermediateResult const & o) const
{
  if (m_resultType != o.m_resultType)
    return m_resultType < o.m_resultType;
  if (m_matchPenalty != o.m_matchPenalty)
    return m_matchPenalty < o.m_matchPenalty;
  if (m_minVisibleScale != o.m_minVisibleScale)
    return m_minVisibleScale < o.m_minVisibleScale;
  return false;
}

Result IntermediateResult::GenerateFinalResult() const
{
//#ifdef DEBUG
//  return Result(m_str
//                + ' ' + strings::to_string(m_distance * 0.001)
//                + ' ' + strings::to_string(m_direction / math::pi * 180.0)
//                + ' ' + strings::to_string(m_matchPenalty)
//                + ' ' + strings::to_string(m_minVisibleScale),
//#else
  switch (m_resultType)
  {
  case RESULT_FEATURE:
    return Result(m_str, m_type, m_rect, m_distance, m_direction);
  case RESULT_LATLON:
    return Result(m_str, 0, m_rect, m_distance, m_direction);
  default:
    ASSERT(false, ());
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
