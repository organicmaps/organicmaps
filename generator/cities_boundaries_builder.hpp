#pragma once

#include "indexer/city_boundary.hpp"

#include "base/clustering_map.hpp"
#include "base/geo_object_id.hpp"

#include <string>

namespace generator
{
// todo(@m) Make test ids a new source in base::GeoObjectId?
using OsmIdToBoundariesTable = base::ClusteringMap<base::GeoObjectId, indexer::CityBoundary>;
using TestIdToBoundariesTable = base::ClusteringMap<uint64_t, indexer::CityBoundary>;

bool BuildCitiesBoundaries(std::string const & dataPath, OsmIdToBoundariesTable & table);
bool BuildCitiesBoundariesForTesting(std::string const & dataPath, TestIdToBoundariesTable & table);

void SerializeBoundariesTable(std::string const & path, OsmIdToBoundariesTable & table);
bool DeserializeBoundariesTable(std::string const & path, OsmIdToBoundariesTable & table);
}  // namespace generator
