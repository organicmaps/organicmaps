#pragma once

struct OsmElement;
class FeatureBuilder1;

namespace generator
{
// Implementing this interface allows an object to filter OsmElement and FeatureBuilder1 elements.
class FilterInterface
{
public:
  virtual ~FilterInterface() = default;

  virtual bool IsAccepted(OsmElement const &) { return true; }
  virtual bool IsAccepted(FeatureBuilder1 const &) { return true; }
};
}  // namespace generator
