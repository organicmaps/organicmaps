#pragma once

#include "generator/collector_routing_city_boundaries.hpp"
#include "generator/feature_builder.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
namespace routing_city_boundaries
{
std::unordered_map<uint64_t, RoutingCityBoundariesWriter::LocalityData> LoadNodeToLocalityData(
    std::string const & filename);

std::unordered_map<uint64_t, std::vector<feature::FeatureBuilder>> LoadNodeToBoundariesData(
    std::string const & filename);

double AreaOnEarth(std::vector<m2::PointD> const & points);
}  // namespace routing_city_boundaries

class RoutingCityBoundariesProcessor
{
public:
  explicit RoutingCityBoundariesProcessor(std::string filename);

  void ProcessDataFromCollector();
  void DumpBoundaries(std::string const & dumpFilename);

private:
  std::string m_filename;
};
}  // namespace generator
