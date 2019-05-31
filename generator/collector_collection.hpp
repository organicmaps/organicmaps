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
// This class allows you to work with a group of collectors as with one.
class CollectorCollection : public CollectionBase<std::shared_ptr<CollectorInterface>>, public CollectorInterface
{
public:
  // CollectorInterface overrides:
  void Collect(OsmElement const & element) override;
  void CollectRelation(RelationElement const & element) override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Save() override;
};
}  // namespace generator
