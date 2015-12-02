#pragma once

#include "search/v2/features_layer.hpp"
#include "search/v2/search_model.hpp"
#include "search/v2/street_vicinity_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

class MwmValue;

namespace search
{
namespace v2
{
class FeaturesLayerMatcher
{
public:
  FeaturesLayerMatcher(MwmValue & value, FeaturesVector const & featuresVector);

  template <typename TFn>
  void Match(FeaturesLayer const & child, FeaturesLayer const & parent, TFn && fn)
  {
    if (child.m_type >= parent.m_type)
      return;
    if (parent.m_type == SearchModel::SEARCH_TYPE_STREET)
    {
      if (child.m_type == SearchModel::SEARCH_TYPE_POI)
        MatchPOIsWithStreets(child, parent, forward<TFn>(fn));
      else if (child.m_type == SearchModel::SEARCH_TYPE_BUILDING)
        MatchBuildingsWithStreets(child, parent, forward<TFn>(fn));
      return;
    }

    vector<m2::PointD> childCenters;
    for (uint32_t featureId : child.m_sortedFeatures)
    {
      FeatureType ft;
      m_featuresVector.GetByIndex(featureId, ft);
      childCenters.push_back(feature::GetCenter(ft, FeatureType::WORST_GEOMETRY));
    }

    vector<m2::RectD> parentRects;
    for (uint32_t featureId : parent.m_sortedFeatures)
    {
      FeatureType feature;
      m_featuresVector.GetByIndex(featureId, feature);
      m2::PointD center = feature::GetCenter(feature, FeatureType::WORST_GEOMETRY);
      double radius = ftypes::GetRadiusByPopulation(feature.GetPopulation());
      parentRects.push_back(MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius));
    }

    for (size_t j = 0; j < parent.m_sortedFeatures.size(); ++j)
    {
      for (size_t i = 0; i < child.m_sortedFeatures.size(); ++i)
      {
        if (parentRects[j].IsPointInside(childCenters[i]))
          fn(i, j);
      }
    }
  }

private:
  template <typename TFn>
  void MatchPOIsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent, TFn && fn)
  {
    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_POI, ());
    ASSERT_EQUAL(parent.m_type, SearchModel::SEARCH_TYPE_STREET, ());

    for (size_t j = 0; j < parent.m_sortedFeatures.size(); ++j)
    {
      auto match = [&](uint32_t poiId)
      {
        auto const it =
            lower_bound(child.m_sortedFeatures.begin(), child.m_sortedFeatures.end(), poiId);
        if (it != child.m_sortedFeatures.end() && *it == poiId)
        {
          size_t i = distance(child.m_sortedFeatures.begin(), it);
          fn(i, j);
        }
      };

      m_loader.ForEachInVicinity(parent.m_sortedFeatures[j], scales::GetUpperScale(), match);
    }
  }

  template <typename TFn>
  void MatchBuildingsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent,
                                 TFn && fn)
  {
    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_BUILDING, ());
    ASSERT_EQUAL(parent.m_type, SearchModel::SEARCH_TYPE_STREET, ());

    // TODO (@y): implement this
    NOTIMPLEMENTED();
  }

  FeaturesVector const & m_featuresVector;
  StreetVicinityLoader m_loader;
};
}  // namespace v2
}  // namespace search
