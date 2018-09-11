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
struct Region : public RegionWithName, public RegionWithData
{
  explicit Region(FeatureBuilder1 const & fb, RegionDataProxy const & rd);

  void DeletePolygon();
  bool IsCountry() const;
  bool Contains(Region const & smaller) const;
  bool ContainsRect(Region const & smaller) const;
  double CalculateOverlapPercentage(Region const & other) const;
  BoostPoint GetCenter() const;
  std::shared_ptr<BoostPolygon> const GetPolygon() const { return m_polygon; }
  BoostRect const & GetRect() const { return m_rect; }
  double GetArea() const { return m_area; }
  bool Contains(City const & cityPoint) const;
  void SetInfo(City const & cityPoint);

private:
  void FillPolygon(FeatureBuilder1 const & fb);

  std::shared_ptr<BoostPolygon> m_polygon;
  BoostRect m_rect;
  double m_area;
};
}  // namespace regions
}  // namespace generator
