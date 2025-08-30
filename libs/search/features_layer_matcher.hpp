#pragma once

#include "search/cancel_exception.hpp"
#include "search/cbv.hpp"
#include "search/features_layer.hpp"
#include "search/house_numbers_matcher.hpp"
#include "search/model.hpp"
#include "search/mwm_context.hpp"
#include "search/point_rect_matcher.hpp"
#include "search/projection_on_street.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/stats_cache.hpp"
#include "search/street_vicinity_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

class DataSource;

namespace search
{
// This class performs pairwise intersection between two layers of
// features, where the first (child) layer is geographically smaller
// than the second (parent) one.  It emits all pairs
// (feature-from-child-layer, feature-from-parent-layer) of matching
// features, where feature-from-child-layer belongs-to
// feature-from-parent-layer.  Belongs-to is a partial relation on
// features, and has different meaning for different search classes:
//
// * BUILDING/POI belongs-to STREET iff it is located on the street;
// * BUILDING belongs-to CITY iff the building is located in the city;
// * POI belongs-to BUILDING iff the poi is (roughly) located near or inside the building;
// * SUBPOI belongs-to COMPLEX_POI iff the SUBPOI is (roughly) located near or inside the COMPLEX_POI;
// * STREET belongs-to CITY iff the street is (roughly) located in the city;
// * etc.
//
// NOTE: this class *IS NOT* thread-safe.
class FeaturesLayerMatcher
{
public:
  static uint32_t const kInvalidId = std::numeric_limits<uint32_t>::max();
  static int constexpr kBuildingRadiusMeters = 50;
  static int constexpr kComplexPoiRadiusMeters = 300;
  static int constexpr kStreetRadiusMeters = 100;

  FeaturesLayerMatcher(DataSource const & dataSource, base::Cancellable const & cancellable);
  void SetContext(MwmContext * context);
  void SetPostcodes(CBV const * postcodes);

  template <typename Fn>
  void Match(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    if (child.m_type >= parent.m_type)
      return;
    switch (parent.m_type)
    {
    case Model::TYPE_SUBPOI:
    case Model::TYPE_VILLAGE:
    case Model::TYPE_STATE:
    case Model::TYPE_COUNTRY:
    case Model::TYPE_UNCLASSIFIED:
    case Model::TYPE_COUNT: ASSERT(false, ("Invalid parent layer type:", parent.m_type)); break;
    case Model::TYPE_CITY:
      ASSERT_EQUAL(child.m_type, Model::TYPE_BUILDING, ());
      MatchBuildingsWithPlace(child, parent, fn);
      break;
    case Model::TYPE_COMPLEX_POI:
      ASSERT_EQUAL(child.m_type, Model::TYPE_SUBPOI, ());
      MatchPOIsWithParent(child, parent, fn);
      break;
    case Model::TYPE_BUILDING:
      ASSERT(Model::IsPoi(child.m_type), ());
      MatchPOIsWithParent(child, parent, fn);
      break;
    case Model::TYPE_STREET:
      ASSERT(Model::IsPoiOrBuilding(child.m_type), ("Invalid child layer type:", child.m_type));
      if (Model::IsPoi(child.m_type))
        MatchPOIsWithStreets(child, parent, fn);
      else
        MatchBuildingsWithStreets(child, parent, fn);
      break;
    case Model::TYPE_SUBURB:
      ASSERT(child.m_type == Model::TYPE_STREET || child.m_type == Model::TYPE_BUILDING || Model::IsPoi(child.m_type),
             ());
      // Avoid matching buildings to suburb without street.
      if (child.m_type == Model::TYPE_BUILDING)
        MatchBuildingsWithPlace(child, parent, fn);
      else
        MatchChildWithSuburbs(child, parent, fn);
      break;
    }
  }

  void OnQueryFinished();

private:
  std::vector<uint32_t> const & GetPlaceAddrFeatures(uint32_t placeId, std::function<CBV()> const & fn);

  void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  static bool HouseNumbersMatch(FeatureType & feature, std::vector<house_numbers::Token> const & queryParse)
  {
    ASSERT(!queryParse.empty(), ());

    auto const interpol = ftypes::IsAddressInterpolChecker::Instance().GetInterpolType(feature);
    if (interpol != feature::InterpolType::None)
      return house_numbers::HouseNumbersMatchRange(feature.GetRef(), queryParse, interpol);

    auto const uniHouse = strings::MakeUniString(feature.GetHouseNumber());
    if (uniHouse.empty())
      return false;

    if (feature.GetID().IsEqualCountry({"Czech", "Slovakia"}))
      return house_numbers::HouseNumbersMatchConscription(uniHouse, queryParse);

    return house_numbers::HouseNumbersMatch(uniHouse, queryParse);
  }

  template <typename Fn>
  void MatchPOIsWithParent(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    double parentRadius = 0.0;
    // Following code initially loads centers of POIs and then, for
    // each building, tries to find all POIs located at distance less
    // than parentRadius.

    if (parent.m_type == Model::TYPE_BUILDING)
    {
      ASSERT(Model::IsPoi(child.m_type), ());
      parentRadius = kBuildingRadiusMeters;
    }
    else
    {
      ASSERT_EQUAL(parent.m_type, Model::TYPE_COMPLEX_POI, ());
      ASSERT_EQUAL(child.m_type, Model::TYPE_SUBPOI, ());
      parentRadius = kComplexPoiRadiusMeters;
    }

    auto const & pois = *child.m_sortedFeatures;
    auto const & buildings = *parent.m_sortedFeatures;

    BailIfCancelled();

    std::vector<PointRectMatcher::PointIdPair> poiCenters;
    poiCenters.reserve(pois.size());

    for (size_t i = 0; i < pois.size(); ++i)
      if (auto poiFt = GetByIndex(pois[i]))
        poiCenters.emplace_back(feature::GetCenter(*poiFt, FeatureType::WORST_GEOMETRY), i /* id */);

    std::vector<PointRectMatcher::RectIdPair> buildingRects;
    buildingRects.reserve(buildings.size());
    auto maxRadius = parentRadius;
    for (size_t i = 0; i < buildings.size(); ++i)
    {
      BailIfCancelled();

      auto buildingFt = GetByIndex(buildings[i]);
      if (!buildingFt)
        continue;

      if (buildingFt->GetGeomType() == feature::GeomType::Point)
      {
        auto const center = feature::GetCenter(*buildingFt, FeatureType::WORST_GEOMETRY);
        buildingRects.emplace_back(mercator::RectByCenterXYAndSizeInMeters(center, parentRadius), i /* id */);
      }
      else
      {
        buildingRects.emplace_back(buildingFt->GetLimitRect(FeatureType::WORST_GEOMETRY), i /* id */);
        double const rectSize = std::max(buildingRects.back().m_rect.SizeX(), buildingRects.back().m_rect.SizeY());
        maxRadius = std::max(maxRadius, rectSize / 2);
      }
    }

    PointRectMatcher::Match(poiCenters, buildingRects, PointRectMatcher::RequestType::Any,
                            [&](size_t poiId, size_t buildingId)
    {
      ASSERT_LESS(poiId, pois.size(), ());
      ASSERT_LESS(buildingId, buildings.size(), ());
      fn(pois[poiId], buildings[buildingId]);
    });

    if (!parent.m_hasDelayedFeatures)
      return;

    // |buildings| doesn't contain buildings matching by house number,
    // so following code reads buildings in POIs vicinities and checks
    // house numbers.
    std::vector<house_numbers::Token> queryParse;
    ParseQuery(parent.m_subQuery, parent.m_lastTokenIsPrefix, queryParse);
    if (queryParse.empty())
      return;

    for (size_t i = 0; i < pois.size(); ++i)
    {
      BailIfCancelled();

      m_context->ForEachFeature(mercator::RectByCenterXYAndSizeInMeters(poiCenters[i].m_point, maxRadius),
                                [&](FeatureType & ft)
      {
        BailIfCancelled();

        if (m_postcodes && !m_postcodes->HasBit(ft.GetID().m_index) && !m_postcodes->HasBit(GetMatchingStreet(ft)))
          return;
        if (HouseNumbersMatch(ft, queryParse))
        {
          double const distanceM = mercator::DistanceOnEarth(feature::GetCenter(ft), poiCenters[i].m_point);
          if (distanceM < maxRadius)
            fn(pois[i], ft.GetID().m_index);
        }
      });
    }
  }

  template <typename Fn>
  void MatchPOIsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    BailIfCancelled();

    ASSERT(Model::IsPoi(child.m_type), ());
    ASSERT_EQUAL(parent.m_type, Model::TYPE_STREET, ());

    auto const & pois = *child.m_sortedFeatures;
    auto const & streets = *parent.m_sortedFeatures;

    std::vector<PointRectMatcher::PointIdPair> poiCenters;
    poiCenters.reserve(pois.size());

    for (size_t i = 0; i < pois.size(); ++i)
      if (auto poiFt = GetByIndex(pois[i]))
        poiCenters.emplace_back(feature::GetCenter(*poiFt, FeatureType::WORST_GEOMETRY), i /* id */);

    std::vector<PointRectMatcher::RectIdPair> streetRects;
    streetRects.reserve(streets.size());

    std::vector<ProjectionOnStreetCalculator> streetProjectors;
    streetProjectors.reserve(streets.size());

    for (size_t i = 0; i < streets.size(); ++i)
    {
      auto streetFt = GetByIndex(streets[i]);
      if (!streetFt)
        continue;

      streetFt->ParseGeometry(FeatureType::WORST_GEOMETRY);

      m2::RectD inflationRect;
      // Any point is good enough here, and feature::GetCenter would re-read the geometry.
      if (streetFt->GetPointsCount() > 0)
        inflationRect = mercator::RectByCenterXYAndSizeInMeters(streetFt->GetPoint(0), 0.5 * kStreetRadiusMeters);

      for (size_t j = 0; j + 1 < streetFt->GetPointsCount(); ++j)
      {
        auto const & p1 = streetFt->GetPoint(j);
        auto const & p2 = streetFt->GetPoint(j + 1);
        m2::RectD rect(p1, p2);
        rect.Inflate(inflationRect.SizeX(), inflationRect.SizeY());
        streetRects.emplace_back(rect, i /* id */);
      }

      std::vector<m2::PointD> streetPoints;
      streetPoints.reserve(streetFt->GetPointsCount());
      for (size_t j = 0; j < streetFt->GetPointsCount(); ++j)
        streetPoints.emplace_back(streetFt->GetPoint(j));
      streetProjectors.emplace_back(streetPoints);
    }

    BailIfCancelled();
    PointRectMatcher::Match(poiCenters, streetRects, PointRectMatcher::RequestType::All,
                            [&](size_t poiId, size_t streetId)
    {
      ASSERT_LESS(poiId, pois.size(), ());
      ASSERT_LESS(streetId, streets.size(), ());
      auto const & poiCenter = poiCenters[poiId].m_point;
      ProjectionOnStreet proj;
      if (streetProjectors[streetId].GetProjection(poiCenter, proj) && proj.m_distMeters < kStreetRadiusMeters)
        fn(pois[poiId], streets[streetId]);
    });
  }

  template <typename Fn>
  void MatchBuildingsWithStreets(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    ASSERT_EQUAL(child.m_type, Model::TYPE_BUILDING, ());
    ASSERT_EQUAL(parent.m_type, Model::TYPE_STREET, ());

    auto const & buildings = *child.m_sortedFeatures;
    auto const & streets = *parent.m_sortedFeatures;

    // When all buildings are in |buildings| and the number of
    // buildings less than the number of streets, it's probably faster
    // to check nearby streets for each building instead of street
    // vicinities loading.
    if (!child.m_hasDelayedFeatures && buildings.size() < streets.size())
    {
      for (uint32_t const houseId : buildings)
      {
        BailIfCancelled();

        uint32_t const streetId = GetMatchingStreet({m_context->GetId(), houseId});
        if (std::binary_search(streets.begin(), streets.end(), streetId))
          fn(houseId, streetId);
      }
      return;
    }

    std::vector<house_numbers::Token> queryParse;
    ParseQuery(child.m_subQuery, child.m_lastTokenIsPrefix, queryParse);

    uint32_t numFilterInvocations = 0;
    auto const houseNumberFilter = [&](uint32_t houseId, uint32_t streetId)
    {
      ++numFilterInvocations;
      if ((numFilterInvocations & 0xFF) == 0)
        BailIfCancelled();

      if (std::binary_search(buildings.begin(), buildings.end(), houseId))
        return true;

      if (!child.m_hasDelayedFeatures || queryParse.empty())
        return false;

      if (m_postcodes && !m_postcodes->HasBit(houseId) && !m_postcodes->HasBit(streetId))
        return false;

      std::unique_ptr<FeatureType> feature = GetByIndex(houseId);
      if (!feature)
        return false;

      return HouseNumbersMatch(*feature, queryParse);
    };

    // Cache is not needed since we process unique and mapped-only house->street.
    //    std::unordered_map<uint32_t, bool> cache;
    //    auto const cachingHouseNumberFilter = [&](uint32_t houseId, uint32_t streetId)
    //    {
    //      auto const res = cache.emplace(houseId, false);
    //      if (res.second)
    //        res.first->second = houseNumberFilter(houseId, streetId);
    //      return res.first->second;
    //    };

    for (uint32_t streetId : streets)
    {
      BailIfCancelled();

      StreetVicinityLoader::Street const & street = m_loader.GetStreet(streetId);
      if (street.IsEmpty())
        continue;

      for (uint32_t houseId : street.m_features)
        if (houseNumberFilter(houseId, streetId))
          fn(houseId, streetId);
    }
  }

  template <typename Fn>
  void MatchBuildingsWithPlace(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    ASSERT_EQUAL(child.m_type, Model::TYPE_BUILDING, ());

    auto const & buildings = *child.m_sortedFeatures;
    uint32_t const placeId = parent.m_sortedFeatures->front();
    auto const & ids = GetPlaceAddrFeatures(placeId, parent.m_getFeatures);

    if (!buildings.empty())
    {
      for (uint32_t houseId : buildings)
        if (std::binary_search(ids.begin(), ids.end(), houseId))
          fn(houseId, placeId);
    }
    if (!child.m_hasDelayedFeatures)
      return;

    std::vector<house_numbers::Token> queryParse;
    ParseQuery(child.m_subQuery, child.m_lastTokenIsPrefix, queryParse);
    if (queryParse.empty())
      return;

    uint32_t numFilterInvocations = 0;
    auto const houseNumberFilter = [&](uint32_t houseId)
    {
      ++numFilterInvocations;
      if ((numFilterInvocations & 0xFF) == 0)
        BailIfCancelled();

      if (m_postcodes && !m_postcodes->HasBit(houseId))
        return false;

      /// @todo Add house -> number cache for this and MatchBuildingsWithStreets?
      std::unique_ptr<FeatureType> feature = GetByIndex(houseId);
      if (!feature)
        return false;

      return HouseNumbersMatch(*feature, queryParse);
    };

    for (uint32_t houseId : ids)
      if (houseNumberFilter(houseId))
        fn(houseId, placeId);
  }

  template <typename Fn>
  void MatchChildWithSuburbs(FeaturesLayer const & child, FeaturesLayer const & parent, Fn && fn)
  {
    // Keep the old logic - simple stub that matches all childs. They will be filtered after in Geocoder.
    /// @todo Can intersect with parent.m_getFeatures here.
    uint32_t const suburbId = parent.m_sortedFeatures->front();
    for (uint32_t feature : *child.m_sortedFeatures)
      fn(feature, suburbId);
  }

  // Returns id of a street feature corresponding to a |houseId|/|houseFeature|, or
  // kInvalidId if there're not such street.
  uint32_t GetMatchingStreet(FeatureID const & houseId);
  uint32_t GetMatchingStreet(FeatureType & houseFeature);
  template <class FeatureGetterT>
  uint32_t GetMatchingStreetImpl(FeatureID const & id, FeatureGetterT && getter);

  using Street = ReverseGeocoder::Street;
  using Streets = std::vector<Street>;

  Streets const & GetNearbyStreets(FeatureType & feature);

  std::unique_ptr<FeatureType> GetByIndex(uint32_t id) const
  {
    /// @todo Add Cache for feature id -> (point, name / house number).
    auto res = m_context->GetFeature(id);

    // It may happen to features deleted by the editor. We do not get them from EditableDataSource
    // but we still have ids of these features in the search index.
    if (!res)
      LOG(LWARNING, ("GetFeature() returned false.", id));
    return res;
  }

  MwmContext * m_context;

  CBV const * m_postcodes;

  ReverseGeocoder m_reverseGeocoder;

  // Cache of streets in a feature's vicinity. All lists in the cache
  // are ordered by distance from the corresponding feature.
  Cache<uint32_t, Streets> m_nearbyStreetsCache;

  // Cache of correct streets for buildings. Current search algorithm
  // supports only one street for a building, whereas buildings can be
  // located on multiple streets.
  Cache<uint32_t, uint32_t> m_matchingStreetsCache;

  // Cache of addresses that belong to a place (city/village).
  Cache<uint32_t, std::vector<uint32_t>> m_place2address;

  StreetVicinityLoader m_loader;
  base::Cancellable const & m_cancellable;
};
}  // namespace search
