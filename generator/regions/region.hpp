#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/region_base.hpp"

#include <memory>

class FeatureBuilder1;

namespace generator
{
class RegionDataProxy;

namespace regions
{
class City;

// This is a helper class that is needed to represent the region.
// With this view, further processing is simplified.
class Region : public RegionWithName, public RegionWithData
{
public:
  explicit Region(FeatureBuilder1 const & fb, RegionDataProxy const & rd);
  // Build a region and its boundary based on the heuristic.
  explicit Region(City const & city);

  // After calling DeletePolygon, you cannot use Contains, ContainsRect, CalculateOverlapPercentage.
  void DeletePolygon();
  bool Contains(Region const & smaller) const;
  bool ContainsRect(Region const & smaller) const;
  bool Contains(City const & cityPoint) const;
  bool Contains(BoostPoint const & point) const;
  double CalculateOverlapPercentage(Region const & other) const;
  BoostPoint GetCenter() const;
  bool IsCountry() const;
  bool IsLocality() const;
  BoostRect const & GetRect() const { return m_rect; }
  double GetArea() const { return m_area; }
  // This function uses heuristics and assigns a radius according to the tag place.
  // The radius will be returned in mercator.
  static double GetRediusByPlaceType(PlaceType place);

private:
  void FillPolygon(FeatureBuilder1 const & fb);

  std::shared_ptr<BoostPolygon> m_polygon;
  BoostRect m_rect;
  double m_area;
};

void SetCityBestAttributesToRegion(City const & cityPoint, Region & region);

bool FeatureCityPointToRegion(RegionInfo const & regionInfo, FeatureBuilder1 & feature);
}  // namespace regions
}  // namespace generator
