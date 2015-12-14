#pragma once

#include "search/reverse_geocoder.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/house_numbers_matcher.hpp"
#include "search/v2/house_to_street_table.hpp"
#include "search/v2/search_model.hpp"
#include "search/v2/street_vicinity_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/vector.hpp"

class Index;
class MwmValue;

namespace search
{
namespace v2
{
// This class performs pairwise intersection between two layers of
// features, where the first (child) layer is geographically smaller
// than the second (parent) one.  It emits all pairs
// (feature-from-child-layer, feature-from-parent-layer) of matching
// features, where feature-from-child-layer belongs-to
// feature-from-parent-layer.  Belongs-to is a partial relation on
// features, and has different meaning for different search classes:
//
// * BUILDING belongs-to STREET iff the building is located on the street;
// * BUILDING belongs-to CITY iff the building is located in the city;
// * POI belongs-to BUILDING iff the poi is (roughly) located near or inside the building;
// * STREET belongs-to CITY iff the street is (roughly) located in the city;
// * etc.
//
// NOTE: this class *IS NOT* thread-safe.
class FeaturesLayerMatcher
{
public:
  FeaturesLayerMatcher(Index & index, MwmSet::MwmId const & mwmId, MwmValue & value,
                       FeaturesVector const & featuresVector);

  template <typename TFn>
  void Match(FeaturesLayer const & child, vector<uint32_t> const & sortedParentFeatures,
             SearchModel::SearchType parentType, TFn && fn)
  {
    if (child.m_type >= parentType)
      return;
    if (parentType == SearchModel::SEARCH_TYPE_STREET)
    {
      if (child.m_type == SearchModel::SEARCH_TYPE_POI)
        MatchPOIsWithStreets(child, sortedParentFeatures, parentType, forward<TFn>(fn));
      else if (child.m_type == SearchModel::SEARCH_TYPE_BUILDING)
        MatchBuildingsWithStreets(child, sortedParentFeatures, parentType, forward<TFn>(fn));
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
    for (uint32_t featureId : sortedParentFeatures)
    {
      FeatureType feature;
      m_featuresVector.GetByIndex(featureId, feature);
      m2::PointD center = feature::GetCenter(feature, FeatureType::WORST_GEOMETRY);
      double radius = ftypes::GetRadiusByPopulation(feature.GetPopulation());
      parentRects.push_back(MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius));
    }

    for (size_t j = 0; j < sortedParentFeatures.size(); ++j)
    {
      for (size_t i = 0; i < child.m_sortedFeatures.size(); ++i)
      {
        if (parentRects[j].IsPointInside(childCenters[i]))
          fn(child.m_sortedFeatures[i], sortedParentFeatures[j]);
      }
    }
  }

private:
  template <typename TFn>
  void MatchPOIsWithStreets(FeaturesLayer const & child,
                            vector<uint32_t> const & sortedParentFeatures,
                            SearchModel::SearchType parentType, TFn && fn)
  {
    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_POI, ());
    ASSERT_EQUAL(parentType, SearchModel::SEARCH_TYPE_STREET, ());

    for (uint32_t streetId : sortedParentFeatures)
      m_loader.ForEachInVicinity(streetId, child.m_sortedFeatures, bind(fn, _1, streetId));
  }

  template <typename TFn>
  void MatchBuildingsWithStreets(FeaturesLayer const & child,
                                 vector<uint32_t> const & sortedParentFeatures,
                                 SearchModel::SearchType parentType, TFn && fn)
  {
    // child.m_sortedFeatures contains only buildings matched by name,
    // not by house number.  So, we need to add to
    // child.m_sortedFeatures all buildings match by house number
    // here.

    auto const & checker = ftypes::IsBuildingChecker::Instance();

    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_BUILDING, ());
    ASSERT_EQUAL(parentType, SearchModel::SEARCH_TYPE_STREET, ());

    vector<string> queryTokens;
    NormalizeHouseNumber(child.m_subQuery, queryTokens);

    auto filter = [&](uint32_t id, FeatureType & feature) -> bool
    {
      if (!checker(feature))
        return false;
      if (binary_search(child.m_sortedFeatures.begin(), child.m_sortedFeatures.end(), id))
        return true;
      return HouseNumbersMatch(feature.GetHouseNumber(), queryTokens);
    };

    auto addEdge = [&](uint32_t houseId, FeatureType & houseFeature, uint32_t streetId)
    {
      vector<ReverseGeocoder::Street> streets;
      m_reverseGeocoder.GetNearbyStreets(houseFeature, streets);
      uint32_t streetIndex = m_houseToStreetTable->Get(houseId);

      if (streetIndex < streets.size() && streets[streetIndex].m_id.m_mwmId == m_mwmId &&
          streets[streetIndex].m_id.m_index == streetId)
      {
        fn(houseId, streetId);
      }
    };

    for (uint32_t streetId : sortedParentFeatures)
      m_loader.FilterFeaturesInVicinity(streetId, filter, bind(addEdge, _1, _2, streetId));
  }

  MwmSet::MwmId m_mwmId;
  ReverseGeocoder m_reverseGeocoder;
  unique_ptr<HouseToStreetTable> m_houseToStreetTable;
  FeaturesVector const & m_featuresVector;
  StreetVicinityLoader m_loader;
};
}  // namespace v2
}  // namespace search
