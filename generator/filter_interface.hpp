#pragma once

#include <memory>

struct OsmElement;
namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
// Implementing this interface allows an object to filter OsmElement and FeatureBuilder elements.
class FilterInterface
{
public:
  virtual ~FilterInterface() = default;

  virtual std::shared_ptr<FilterInterface> Clone() const = 0;

  virtual bool IsAccepted(OsmElement const &) { return true; }
  virtual bool IsAccepted(feature::FeatureBuilder const &) { return true; }
};
}  // namespace generator
