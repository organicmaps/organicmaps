#pragma once

#include "generator/osm_id.hpp"

#include "indexer/city_boundary.hpp"

#include "base/clustering_map.hpp"

#include <cstdint>
#include <string>

namespace generator
{
using OsmIdToBoundariesTable = base::ClusteringMap<osm::Id, indexer::CityBoundary, osm::HashId>;
using TestIdToBoundariesTable = base::ClusteringMap<uint64_t, indexer::CityBoundary>;

bool BuildCitiesBoundaries(std::string const & dataPath, std::string const & osmToFeaturePath,
                           OsmIdToBoundariesTable & table);
bool BuildCitiesBoundariesForTesting(std::string const & dataPath, TestIdToBoundariesTable & table);
}  // namespace generator
