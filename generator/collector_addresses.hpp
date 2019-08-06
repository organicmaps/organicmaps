#pragma once

#include "generator/collector_interface.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}
// The class CollectorAddresses is responsible for the collecting addresses to the file.
class CollectorAddresses : public CollectorInterface
{
public:
  explicit CollectorAddresses(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<cache::IntermediateDataReader> const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const &) override;
  void Finish() override;
  void Save() override;

  void Merge(CollectorInterface const & collector) override;
  void MergeInto(CollectorAddresses & collector) const override;

private:
  std::ofstream m_writer;
};
}  // namespace generator
