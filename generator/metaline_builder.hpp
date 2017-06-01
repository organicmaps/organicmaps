#pragma once

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
class MetalinesBuilder
{
public:
  explicit MetalinesBuilder(std::string const & filePath) : m_filePath(filePath) {}
  ~MetalinesBuilder() { Flush(); }
  void operator()(OsmElement const & el, FeatureParams const & params);
  void Flush();

private:
  std::unordered_map<std::size_t, std::shared_ptr<Segments>> m_data;
  std::string m_filePath;
};

bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath);
}
