#pragma once

#include "generator/feature_builder.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>

namespace generator
{
/// Used to make a "good" node for a highway graph with OSRM for low zooms.
class Place
{
public:
  Place(FeatureBuilder1 const & ft, uint32_t type);

  FeatureBuilder1 const & GetFeature() const { return m_ft; }
  m2::RectD GetLimitRect() const;
  bool IsEqual(Place const & r) const;
  /// Check whether we need to replace place @r with place @this.
  bool IsBetterThan(Place const & r) const;

private:
  bool IsPoint() const { return (m_ft.GetGeomType() == feature::GEOM_POINT); }
  static bool AreTypesEqual(uint32_t t1, uint32_t t2);

  FeatureBuilder1 m_ft;
  m2::PointD m_pt;
  uint32_t m_type;
  double m_thresholdM;
};
}  // namespace generator
