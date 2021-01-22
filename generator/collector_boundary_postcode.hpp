#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_maker.hpp"
#include "generator/osm_element.hpp"

#include "coding/writer.hpp"

#include <memory>
#include <string>

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

// Saves pairs (postal code, outer geometry) to file for OsmElements that
// have all of the following: relation=boundary, boundary=postal_code, postal_code=*.
class BoundaryPostcodeCollector : public CollectorInterface
{
public:
  BoundaryPostcodeCollector(std::string const & filename,
                            std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & = {}) const override;

  void Collect(OsmElement const & el) override;
  void Finish() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(BoundaryPostcodeCollector & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::unique_ptr<FileWriter> m_writer;
  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  FeatureMakerSimple m_featureMakerSimple;
};
}  // namespace generator
