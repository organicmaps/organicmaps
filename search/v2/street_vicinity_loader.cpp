#include "search/v2/street_vicinity_loader.hpp"

#include "indexer/feature_covering.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"

#include "geometry/mercator.hpp"

#include "geometry/point2d.hpp"

#include "base/math.hpp"
#include "base/stl_add.hpp"

namespace search
{
namespace v2
{
StreetVicinityLoader::StreetVicinityLoader(int scale, double offsetMeters)
  : m_context(nullptr), m_scale(scale), m_offsetMeters(offsetMeters), m_cache("Streets")
{
}

void StreetVicinityLoader::InitContext(MwmContext * context)
{
  m_context = context;
  auto const scaleRange = m_context->m_value.GetHeader().GetScaleRange();
  m_scale = my::clamp(m_scale, scaleRange.first, scaleRange.second);
}

void StreetVicinityLoader::FinishQuery()
{
  m_cache.FinishQuery();
}

StreetVicinityLoader::Street const & StreetVicinityLoader::GetStreet(uint32_t featureId)
{
  auto r = m_cache.Get(featureId);
  if (!r.second)
    return r.first;

  LoadStreet(featureId, r.first);
  return r.first;
}

void StreetVicinityLoader::LoadStreet(uint32_t featureId, Street & street)
{
  FeatureType feature;
  m_context->m_vector.GetByIndex(featureId, feature);

  if (feature.GetFeatureType() != feature::GEOM_LINE)
    return;

  vector<m2::PointD> points;
  feature.ForEachPoint(MakeBackInsertFunctor(points), FeatureType::BEST_GEOMETRY);
  ASSERT(!points.empty(), ());

  for (auto const & point : points)
    street.m_rect.Add(MercatorBounds::RectByCenterXYAndSizeInMeters(point, m_offsetMeters));

  covering::CoveringGetter coveringGetter(street.m_rect, covering::ViewportWithLowLevels);
  auto const & intervals = coveringGetter.Get(m_scale);
  for (auto const & interval : intervals)
  {
    m_context->m_index.ForEachInIntervalAndScale(MakeBackInsertFunctor(street.m_features),
                                                 interval.first, interval.second, m_scale);
  }

  street.m_calculator = make_unique<ProjectionOnStreetCalculator>(points, m_offsetMeters);
}
}  // namespace v2
}  // namespace search
