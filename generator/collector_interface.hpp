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

namespace generator
{
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

  using IDRInterfacePtr = std::shared_ptr<cache::IntermediateDataReaderInterface>;
  /// @param[in] cache Use passed (not saved) cache to create a clone, because cache also may (should) be cloned.
  /// Empty \a cache used in unit tests only. Generator's cache is always valid.
  virtual std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const = 0;

  virtual void Collect(OsmElement const &) {}
  virtual void CollectRelation(RelationElement const &) {}
  virtual void CollectFeature(feature::FeatureBuilder const &, OsmElement const &) {}
  virtual void Finish() {}

  virtual void Merge(CollectorInterface const &) = 0;

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
  int CreateId()
  {
    static std::atomic_int id{0};
    return id++;
  }

  int m_id;
  std::string m_filename;
};
}  // namespace generator

#define IMPLEMENT_COLLECTOR_IFACE(className)              \
  void Merge(CollectorInterface const & ci) override      \
  {                                                       \
    dynamic_cast<className const &>(ci).MergeInto(*this); \
  }
