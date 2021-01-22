#pragma once

#include "generator/collection_base.hpp"
#include "generator/collector_interface.hpp"

#include <memory>

struct OsmElement;
class RelationElement;

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

// This class allows you to work with a group of collectors as with one.
class CollectorCollection : public CollectionBase<std::shared_ptr<CollectorInterface>>, public CollectorInterface
{
public:
  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache = {}) const override;

  void Collect(OsmElement const & element) override;
  void CollectRelation(RelationElement const & element) override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Finish() override;

  void Merge(CollectorInterface const & collector) override;
  void MergeInto(CollectorCollection & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;
};
}  // namespace generator
