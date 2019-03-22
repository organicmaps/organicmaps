#pragma once

#include <string>

struct OsmElement;
class FeatureBuilder1;
class RelationElement;
namespace base
{
class GeoObjectId;
}  // namespace base

namespace generator
{
// Implementing this interface allows an object to collect data from RelationElement,
// OsmElement and FeatureBuilder1 elements.
class CollectorInterface
{
public:
  virtual ~CollectorInterface() = default;

  virtual void Collect(OsmElement const &) {}
  virtual void CollectRelation(RelationElement const &) {}
  virtual void CollectFeature(FeatureBuilder1 const &, OsmElement const &) {}
  virtual void Save() = 0;
};
}  // namespace generator
