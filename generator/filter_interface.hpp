#pragma once

struct OsmElement;
namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
// Implementing this interface allows an object to filter OsmElement and FeatureBuilder1 elements.
class FilterInterface
{
public:
  virtual ~FilterInterface() = default;

  virtual bool IsAccepted(OsmElement const &) { return true; }
  virtual bool IsAccepted(feature::FeatureBuilder const &) { return true; }
};
}  // namespace generator
