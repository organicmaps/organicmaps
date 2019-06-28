#pragma once

#include "base/assert.hpp"

#include <memory>
#include <string>

struct OsmElement;
class RelationElement;

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace base
{
class GeoObjectId;
}  // namespace base
namespace feature
{
class MetalinesBuilder;
}  // namespace feature
namespace routing
{
class CameraCollector;
class RestrictionWriter;
class RoadAccessWriter;
}  // namespace routing
namespace generator
{
class CollectorAddresses;
class CollectorCollection;
class CollectorTag;
class MaxspeedsCollector;
class CityBoundaryCollector;
namespace cache
{
class IntermediateDataReader;
}  // namespace cache
namespace regions
{
class CollectorRegionInfo;
}  // namespace regions

// Implementing this interface allows an object to collect data from RelationElement,
// OsmElement and FeatureBuilder elements.
class CollectorInterface
{
public:
  CollectorInterface(std::string const & filename = {}) : m_filename(filename) {}
  virtual ~CollectorInterface() = default;

  virtual std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<cache::IntermediateDataReader> const & = {}) const = 0;

  virtual void Collect(OsmElement const &) {}
  virtual void CollectRelation(RelationElement const &) {}
  virtual void CollectFeature(feature::FeatureBuilder const &, OsmElement const &) {}
  virtual void Save() = 0;

  virtual void Merge(CollectorInterface const &) = 0;

  virtual void MergeInto(CityBoundaryCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::CameraCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::RestrictionWriter &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::RoadAccessWriter &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CollectorAddresses &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CollectorTag &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(MaxspeedsCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(feature::MetalinesBuilder &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(regions::CollectorRegionInfo &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CollectorCollection &) const { FailIfMethodUnsupported(); }

  std::string const & GetFilename() const { return m_filename; }

private:
  void FailIfMethodUnsupported() const { CHECK(false, ("This method is unsupported.")); }

  std::string m_filename;
};
}  // namespace generator
