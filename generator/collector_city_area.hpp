#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"

#include <memory>

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

class CityAreaCollector : public CollectorInterface
{
public:
  explicit CityAreaCollector(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const &) override;
  void Finish() override;

  IMPLEMENT_COLLECTOR_IFACE(CityAreaCollector);
  void MergeInto(CityAreaCollector & collector) const;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::unique_ptr<feature::FeatureBuilderWriter<feature::serialization_policy::MaxAccuracy>> m_writer;
};
}  // namespace generator
