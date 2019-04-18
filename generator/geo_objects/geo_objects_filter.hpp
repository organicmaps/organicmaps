#pragma once

#include "generator/feature_builder.hpp"
#include "generator/filter_interface.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
namespace geo_objects
{
class GeoObjectsFilter : public FilterInterface
{
public:
  // FilterInterface overrides:
  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(FeatureBuilder1 const & feature) override;

  static bool IsBuilding(FeatureBuilder1 const & fb);
  static bool HasHouse(FeatureBuilder1 const & fb);
  static bool IsPoi(FeatureBuilder1 const & fb);
  static bool IsStreet(FeatureBuilder1 const & fb);
};
}  // namespace geo_objects
}  // namespace generator
