#pragma once

#include "generator/osm_id.hpp"

#include "indexer/city_boundary.hpp"

#include "base/clustering_map.hpp"

#include <string>

namespace generator
{
using OsmIdToBoundariesTable =
    base::ClusteringMap<osm::Id, indexer::CityBoundary, osm::HashId>;

bool BuildCitiesBoundaries(std::string const & dataPath, std::string const & osmToFeaturePath,
                           OsmIdToBoundariesTable & table);
}  // namespace generator
