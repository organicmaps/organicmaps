#pragma once

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <atomic>
#include <fstream>
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
class BoundaryPostcodeCollector;
class CollectorCollection;
class CollectorTag;
class MaxspeedsCollector;
class MiniRoundaboutCollector;
class CityAreaCollector;
class CrossMwmOsmWaysCollector;
class RoutingCityBoundariesCollector;
class BuildingPartsCollector;

namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

// Implementing this interface allows an object to collect data from RelationElement,
// OsmElement and FeatureBuilder elements.
class CollectorInterface
{
public:
  friend class CollectorCollection;

  CollectorInterface(std::string const & filename = {}) : m_id(CreateId()), m_filename(filename) {}
  virtual ~CollectorInterface() { CHECK(Platform::RemoveFileIfExists(GetTmpFilename()), (GetTmpFilename())); }

  virtual std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & = {}) const = 0;

  virtual void Collect(OsmElement const &) {}
  virtual void CollectRelation(RelationElement const &) {}
  virtual void CollectFeature(feature::FeatureBuilder const &, OsmElement const &) {}
  virtual void Finish() {}

  virtual void Merge(CollectorInterface const &) = 0;

  virtual void MergeInto(CollectorCollection &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(BoundaryPostcodeCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CityAreaCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::CameraCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(MiniRoundaboutCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::RestrictionWriter &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(routing::RoadAccessWriter &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CollectorTag &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(MaxspeedsCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(feature::MetalinesBuilder &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(CrossMwmOsmWaysCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(RoutingCityBoundariesCollector &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(BuildingPartsCollector &) const { FailIfMethodUnsupported(); }

  virtual void Finalize(bool isStable = false)
  {
    Save();
    if (isStable)
      OrderCollectedData();
  }

  std::string GetTmpFilename() const { return m_filename + "." + std::to_string(m_id); }
  std::string const & GetFilename() const { return m_filename; }

protected:
  virtual void Save() = 0;
  virtual void OrderCollectedData() {}

private:
  void FailIfMethodUnsupported() const { CHECK(false, ("This method is unsupported.")); }
  int CreateId()
  {
    static std::atomic_int id{0};
    return id++;
  }

  int m_id;
  std::string m_filename;
};
}  // namespace generator
