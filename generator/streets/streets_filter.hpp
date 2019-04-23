#pragma once

#include "generator/feature_builder.hpp"
#include "generator/filter_interface.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
namespace streets
{
class StreetsFilter : public FilterInterface
{
public:
  // FilterInterface overrides:
  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(FeatureBuilder1 const & feature) override;

  static bool IsStreet(FeatureBuilder1 const & fb);
};
}  // namespace streets
}  // namespace generator
