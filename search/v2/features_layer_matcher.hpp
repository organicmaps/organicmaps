#pragma once

#include "search/v2/features_layer.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/vector.hpp"

namespace search
{
namespace v2
{
class FeaturesLayerMatcher
{
public:
  FeaturesLayerMatcher(FeaturesVector const & featuresVector);

  template <typename TFn>
  void Match(FeaturesLayer const & child, FeaturesLayer const & parent, TFn && fn) const
  {
    if (child.m_type >= parent.m_type)
      return;
    if (child.m_type == SearchModel::SEARCH_TYPE_BUILDING &&
        parent.m_type == SearchModel::SEARCH_TYPE_STREET)
    {
      // TODO (y@): match buildings with streets.
      return;
    }

    // TODO (y@): match POI with streets separately.

    vector<m2::PointD> childCenters;
    for (uint32_t featureId : child.m_features)
    {
      FeatureType ft;
      m_featuresVector.GetByIndex(featureId, ft);
      childCenters.push_back(feature::GetCenter(ft, FeatureType::WORST_GEOMETRY));
    }

    vector<m2::RectD> parentRects;
    for (uint32_t featureId : parent.m_features)
    {
      FeatureType feature;
      m_featuresVector.GetByIndex(featureId, feature);
      m2::PointD center = feature::GetCenter(feature, FeatureType::WORST_GEOMETRY);
      double radius = ftypes::GetRadiusByPopulation(feature.GetPopulation());
      parentRects.push_back(MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius));
    }

    for (size_t j = 0; j < parent.m_features.size(); ++j)
    {
      for (size_t i = 0; i < child.m_features.size(); ++i)
      {
        if (parentRects[j].IsPointInside(childCenters[i]))
          fn(i, j);
      }
    }
  }

private:
  FeaturesVector const & m_featuresVector;
};
}  // namespace v2
}  // namespace search
