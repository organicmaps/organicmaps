#pragma once

#include "search/cancel_exception.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/house_numbers_matcher.hpp"
#include "search/v2/house_to_street_table.hpp"
#include "search/v2/search_model.hpp"
#include "search/v2/street_vicinity_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/limits.hpp"
#include "std/unordered_map.hpp"
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
  static uint32_t const kInvalidId = numeric_limits<uint32_t>::max();

  FeaturesLayerMatcher(Index & index, MwmSet::MwmId const & mwmId, MwmValue & value,
                       FeaturesVector const & featuresVector, my::Cancellable const & cancellable);

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
    for (uint32_t featureId : *child.m_sortedFeatures)
    {
      FeatureType ft;
      m_featuresVector.GetByIndex(featureId, ft);
      childCenters.push_back(feature::GetCenter(ft, FeatureType::WORST_GEOMETRY));
    }

    BailIfCancelled(m_cancellable);

    for (size_t j = 0; j < parent.m_sortedFeatures->size(); ++j)
    {
      BailIfCancelled(m_cancellable);

      FeatureType ft;
      m_featuresVector.GetByIndex((*parent.m_sortedFeatures)[j], ft);
      m2::PointD const center = feature::GetCenter(ft, FeatureType::WORST_GEOMETRY);
      double const radius = ftypes::GetRadiusByPopulation(ft.GetPopulation());
      m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

      for (size_t i = 0; i < child.m_sortedFeatures->size(); ++i)
      {
        if (rect.IsPointInside(childCenters[i]))
          fn((*child.m_sortedFeatures)[i], (*parent.m_sortedFeatures)[j]);
      }
    }
  }

private:
  template <typename TFn>
  void MatchPOIsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent, TFn && fn)
  {
    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_POI, ());
    ASSERT_EQUAL(parent.m_type, SearchModel::SEARCH_TYPE_STREET, ());

    for (uint32_t streetId : *parent.m_sortedFeatures)
    {
      BailIfCancelled(m_cancellable);
      m_loader.ForEachInVicinity(streetId, *child.m_sortedFeatures, bind(fn, _1, streetId));
    }
  }

  template <typename TFn>
  void MatchBuildingsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent,
                                 TFn && fn)
  {
    // child.m_sortedFeatures contains only buildings matched by name,
    // not by house number.  So, we need to add to
    // child.m_sortedFeatures all buildings match by house number
    // here.

    auto const & checker = ftypes::IsBuildingChecker::Instance();

    ASSERT_EQUAL(child.m_type, SearchModel::SEARCH_TYPE_BUILDING, ());
    ASSERT_EQUAL(parent.m_type, SearchModel::SEARCH_TYPE_STREET, ());

    vector<string> queryTokens;
    NormalizeHouseNumber(child.m_subQuery, queryTokens);
    bool const queryLooksLikeHouseNumber =
        feature::IsHouseNumber(child.m_subQuery) && !queryTokens.empty();

    uint32_t numFilterInvocations = 0;
    auto filter = [&](uint32_t id, FeatureType & feature) -> bool
    {
      ++numFilterInvocations;
      if ((numFilterInvocations & 0xFF) == 0)
        BailIfCancelled(m_cancellable);

      if (!checker(feature))
        return false;
      if (binary_search(child.m_sortedFeatures->begin(), child.m_sortedFeatures->end(), id))
        return true;

      // HouseNumbersMatch() calls are expensive, so following code
      // tries to reduce number of calls. The most important
      // optimization: as first tokens from the house-number part of
      // the query and feature's house numbers must be numbers, their
      // first symbols must be the same.
      string const houseNumber = feature.GetHouseNumber();
      if (!queryLooksLikeHouseNumber || !feature::IsHouseNumber(houseNumber))
        return false;
      if (queryTokens[0][0] != houseNumber[0])
        return false;
      return HouseNumbersMatch(feature.GetHouseNumber(), queryTokens);
    };

    auto addEdge = [&](uint32_t houseId, FeatureType & houseFeature, uint32_t streetId)
    {
      if (GetMatchingStreet(houseId, houseFeature) == streetId)
        fn(houseId, streetId);
    };

    for (uint32_t streetId : *parent.m_sortedFeatures)
    {
      BailIfCancelled(m_cancellable);
      m_loader.FilterFeaturesInVicinity(streetId, filter, bind(addEdge, _1, _2, streetId));
    }
  }

  // Returns id of a street feature corresponding to a |houseId|, or
  // kInvalidId if there're not such street.
  uint32_t GetMatchingStreet(uint32_t houseId, FeatureType & houseFeature);

  vector<ReverseGeocoder::Street> const & GetNearbyStreets(uint32_t featureId);

  vector<ReverseGeocoder::Street> const & GetNearbyStreets(uint32_t featureId,
                                                           FeatureType & feature);

  MwmSet::MwmId m_mwmId;
  ReverseGeocoder m_reverseGeocoder;

  // Cache of streets in a feature's vicinity. All lists in the cache
  // are ordered by a distance.
  unordered_map<uint32_t, vector<ReverseGeocoder::Street>> m_nearbyStreetsCache;

  // Cache of correct streets for buildings. Current search algorithm
  // supports only one street for a building, whereas buildings can be
  // located on multiple streets.
  unordered_map<uint32_t, uint32_t> m_matchingStreetsCache;

  unique_ptr<HouseToStreetTable> m_houseToStreetTable;

  FeaturesVector const & m_featuresVector;
  StreetVicinityLoader m_loader;
  my::Cancellable const & m_cancellable;
};
}  // namespace v2
}  // namespace search
