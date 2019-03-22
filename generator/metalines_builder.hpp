#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>

namespace feature
{
class Segments;

/// Merges road segments with similar name and ref values into groups called metalines.
class MetalinesBuilder : public generator::CollectorInterface
{
public:
  explicit MetalinesBuilder(std::string const & filePath) : m_filePath(filePath) {}

  // CollectorInterface overrides:
  /// Add a highway segment to the collection of metalines.
  void CollectFeature(FeatureBuilder1 const & feature, OsmElement const & element) override;

  void Save() override;

  /// Write all metalines to the intermediate file.
  void Flush();

private:
  std::unordered_map<size_t, std::shared_ptr<Segments>> m_data;
  std::string m_filePath;
};

/// Read an intermediate file from MetalinesBuilder and convert it to an mwm section.
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath);
}
