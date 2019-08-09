#pragma once

#include "generator/feature_builder.hpp"
#include "generator/filter_interface.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
namespace geo_objects
{
class GeoObjectsFilter : public FilterInterface
{
public:
  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(feature::FeatureBuilder const & feature) override;

  static bool IsBuilding(feature::FeatureBuilder const & fb);
  static bool HasHouse(feature::FeatureBuilder const & fb);
  static bool IsPoi(feature::FeatureBuilder const & fb);
};
}  // namespace geo_objects
}  // namespace generator
