#pragma once

#include "generator/osm_id.hpp"
#include "generator/road_access_generator.hpp"
#include "generator/restriction_writer.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace routing
{
struct TagsProcessor
{
  RoadAccessWriter m_roadAccessWriter;
  RestrictionWriter m_restrictionWriter;
};

// Adds feature id and corresponding |osmId| to |osmIdToFeatureId|.
// Note. In general, one |featureId| may correspond to several osm ids.
// But for a road feature |featureId| corresponds to exactly one osm id.
void AddFeatureId(osm::Id osmId, uint32_t featureId, std::map<osm::Id, uint32_t> & osmIdToFeatureId);

// Parses comma separated text file with line in following format:
// <feature id>, <osm id 1 corresponding feature id>, <osm id 2 corresponding feature id>, and so
// on
// For example:
// 137999, 5170186,
// 138000, 5170209, 5143342,
// 138001, 5170228,
bool ParseOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath,
                                  std::map<osm::Id, uint32_t> & osmIdToFeatureId);
bool ParseFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath,
                                  std::map<uint32_t, uint64_t> & featureIdToOsmId);
}  // namespace routing
