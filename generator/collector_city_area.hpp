#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"

#include <memory>

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
  void Finish() override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(CityAreaCollector & collector) const override;

private:
  std::unique_ptr<feature::FeatureBuilderWriter<feature::serialization_policy::MaxAccuracy>> m_witer;
};
}  // namespace generator
