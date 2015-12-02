#include "search/v2/street_vicinity_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"

#include "geometry/mercator.hpp"

namespace search
{
namespace
{
m2::RectD GetStreetLimitRect(FeatureType const & feature, double offsetMeters)
{
  m2::RectD rect;
  if (feature.GetFeatureType() != feature::GEOM_LINE)
    return rect;

  auto expandRect = [&rect, &offsetMeters](m2::PointD const & point)
  {
    rect.Add(MercatorBounds::RectByCenterXYAndSizeInMeters(point, offsetMeters));
  };
  feature.ForEachPoint(expandRect, FeatureType::BEST_GEOMETRY);
  return rect;
}
}  // namespace

StreetVicinityLoader::StreetVicinityLoader(MwmValue & value, FeaturesVector const & featuresVector,
                                           double offsetMeters)
  : m_index(value.m_cont.GetReader(INDEX_FILE_TAG), value.m_factory)
  , m_scaleRange(value.GetHeader().GetScaleRange())
  , m_featuresVector(featuresVector)
  , m_offsetMeters(offsetMeters)
{
}

m2::RectD StreetVicinityLoader::GetLimitRect(uint32_t featureId)
{
  auto it = m_cache.find(featureId);
  if (it != m_cache.end())
    return it->second;

  FeatureType feature;
  m_featuresVector.GetByIndex(featureId, feature);
  m2::RectD rect = GetStreetLimitRect(feature, m_offsetMeters);
  m_cache[featureId] = rect;
  return rect;
}
}  // namespace search
