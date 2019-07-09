#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"

#include <memory>
#include <vector>

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

class CityAreaCollector : public CollectorInterface
{
public:
  explicit CityAreaCollector(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<cache::IntermediateDataReader> const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const &) override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(CityAreaCollector & collector) const override;

private:
  std::vector<feature::FeatureBuilder> m_boundaries;
};
}  // namespace generator
