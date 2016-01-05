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
StreetVicinityLoader::StreetVicinityLoader(MwmValue & value, FeaturesVector const & featuresVector,
                                           int scale, double offsetMeters)
  : m_index(value.m_cont.GetReader(INDEX_FILE_TAG), value.m_factory)
  , m_featuresVector(featuresVector)
  , m_offsetMeters(offsetMeters)
{
  auto const scaleRange = value.GetHeader().GetScaleRange();
  m_scale = my::clamp(scale, scaleRange.first, scaleRange.second);
}

StreetVicinityLoader::Street const & StreetVicinityLoader::GetStreet(uint32_t featureId)
{
  auto it = m_cache.find(featureId);
  if (it != m_cache.end())
    return it->second;

  LoadStreet(featureId, m_cache[featureId]);
  return m_cache[featureId];
}

void StreetVicinityLoader::LoadStreet(uint32_t featureId, Street & street)
{
  FeatureType feature;
  m_featuresVector.GetByIndex(featureId, feature);

  if (feature.GetFeatureType() != feature::GEOM_LINE)
    return;

  vector<m2::PointD> points;
  feature.ForEachPoint(MakeBackInsertFunctor(points), FeatureType::BEST_GEOMETRY);

  for (auto const & point : points)
    street.m_rect.Add(MercatorBounds::RectByCenterXYAndSizeInMeters(point, m_offsetMeters));

  covering::CoveringGetter coveringGetter(street.m_rect, covering::ViewportWithLowLevels);
  auto const & intervals = coveringGetter.Get(m_scale);
  for (auto const & interval : intervals)
  {
    m_index.ForEachInIntervalAndScale(MakeBackInsertFunctor(street.m_features), interval.first,
                                      interval.second, m_scale);
  }

  if (!points.empty())
    street.m_calculator = make_unique<ProjectionOnStreetCalculator>(move(points), m_offsetMeters);
}
}  // namespace v2
}  // namespace search
