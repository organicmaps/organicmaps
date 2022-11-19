#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_maker.hpp"
#include "generator/osm_element.hpp"

#include <memory>
#include <string>

namespace generator
{

// Saves pairs (postal code, outer geometry) to file for OsmElements that
// have all of the following: relation=boundary, boundary=postal_code, postal_code=*.
class BoundaryPostcodeCollector : public CollectorInterface
{
public:
  BoundaryPostcodeCollector(std::string const & filename, IDRInterfacePtr const & cache);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  void Collect(OsmElement const & el) override;

  IMPLEMENT_COLLECTOR_IFACE(BoundaryPostcodeCollector);
  void MergeInto(BoundaryPostcodeCollector & collector) const;

protected:
  void Save() override;

private:
  std::vector<std::pair<std::string, feature::FeatureBuilder::PointSeq>> m_data;
  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  FeatureMakerSimple m_featureMakerSimple;
};
}  // namespace generator
