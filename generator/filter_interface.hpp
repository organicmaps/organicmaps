#pragma once

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <memory>

namespace generator
{
// Implementing this interface allows an object to filter OsmElement and FeatureBuilder elements.
class FilterInterface
{
public:
  virtual ~FilterInterface() = default;

  virtual std::shared_ptr<FilterInterface> Clone() const = 0;

  virtual bool IsAccepted(OsmElement const &) const { return true; }
  virtual bool IsAccepted(feature::FeatureBuilder const &) const { return true; }
};
}  // namespace generator
