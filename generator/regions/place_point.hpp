#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/region_base.hpp"

#include <unordered_map>

namespace generator
{
namespace regions
{
class PlacePoint;
using PlacePointsMap = std::unordered_map<base::GeoObjectId, PlacePoint>;

// PlacePoint objects presents centers (place=* nodes) of localities (city/town/village/...)
// and their subdivision parts (suburb/quarter/...).
// PlacePoint objects are converted to Region objects with approximated boundaries
// when there is no the same places with original boundaries.
// Approximation depends on place type of point (see MakePolygonWithRadius()).
// Conversion is performed by Region constructor: Region{placePoint}.
class PlacePoint : public RegionWithName, public RegionWithData
{
public:
  explicit PlacePoint(feature::FeatureBuilder const & fb, RegionDataProxy const & rd)
    : RegionWithName(fb.GetParams().name),
      RegionWithData(rd)
  {
    auto const p = fb.GetKeyPoint();
    m_position = {p.x, p.y};
  }

  BoostPoint GetPosition() const {  return m_position; }

private:
  BoostPoint m_position;
};
}  // namespace regions
}  // namespace generator
