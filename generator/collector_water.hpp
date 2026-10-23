#pragma once

#include "generator/affiliation.hpp"
#include "generator/collector_interface.hpp"
#include "generator/feature_generator.hpp"

#include <cstdint>
#include <map>
#include <set>

namespace generator
{
inline char const kWaterGradeSeparatedFilename[] = "water_grade_separated";

// Preserve source semantics that are not fully represented by display types
// (notably bridge=aqueduct and tunnel=culvert without an explicit layer).
class WaterGeometryCollector : public CollectorInterface
{
public:
  explicit WaterGeometryCollector(std::string const & filename, std::string outputDir = {},
                                  AffiliationInterfacePtr affiliations = {}, IDRInterfacePtr cache = {})
    : CollectorInterface(filename)
    , m_outputDir(std::move(outputDir))
    , m_affiliations(std::move(affiliations))
    , m_cache(std::move(cache))
  {}
  ~WaterGeometryCollector() override;
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;
  void Collect(OsmElement const & element) override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Finish() override;
  IMPLEMENT_COLLECTOR_IFACE(WaterGeometryCollector);
  void MergeInto(WaterGeometryCollector & collector) const;

protected:
  void Save() override;

private:
  void Store(feature::FeatureBuilder const & feature, OsmElement const & element);
  std::set<uint64_t> m_ids;
  std::string TempFeatures(std::string const & country) const;
  std::string m_outputDir;
  AffiliationInterfacePtr m_affiliations;
  IDRInterfacePtr m_cache;
  std::set<std::string> m_countries;
  std::map<std::string, std::unique_ptr<feature::FeaturesCollector>> m_features;
};
}  // namespace generator
