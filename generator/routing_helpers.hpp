#pragma once

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace routing
{
using OsmIdToFeatureIds = std::map<base::GeoObjectId, std::vector<uint32_t>>;

// Adds |featureId| and corresponding |osmId| to |osmIdToFeatureIds|.
// Note. In general, one |featureId| may correspond to several osm ids.
// Or on the contrary one osm id may correspond to several feature ids. It may happens for example
// when an area and its boundary may correspond to the same osm id.
// As for road features a road |osmId| may correspond to several feature ids if
// the |osmId| is split by a mini_roundabout or a turning_loop.
void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId,
                  OsmIdToFeatureIds & osmIdToFeatureIds);

// Parses comma separated text file with line in following format:
// <feature id>, <osm id 1 corresponding feature id>, <osm id 2 corresponding feature id>, and so
// on. It may contain several line with the same feature ids.
// For example:
// 137999, 5170186,
// 138000, 5170209, 5143342,
// 138001, 5170228,
// 137999, 5170197,
//
// Note. These methods fill |osmIdToFeatureIds| and |featureIdToOsmId| with features of
// type base::GeoObjectId::Type::ObsoleteOsmWay. This type contains all the roads
// and besides that other linear objects like streams and fences.
bool ParseWaysOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      OsmIdToFeatureIds & osmIdToFeatureIds);
bool ParseWaysFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      std::map<uint32_t, base::GeoObjectId> & featureIdToOsmId);
}  // namespace routing
