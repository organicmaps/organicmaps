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
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(OsmElement const & element) override;
  bool IsAccepted(feature::FeatureBuilder const & feature) override;

  static bool IsStreet(feature::FeatureBuilder const & fb);
};
}  // namespace streets
}  // namespace generator
