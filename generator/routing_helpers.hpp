#pragma once

#include "generator/camera_node_processor.hpp"
#include "generator/maxspeeds_collector.hpp"
#include "generator/restriction_writer.hpp"
#include "generator/road_access_generator.hpp"

#include "routing/cross_mwm_ids.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace routing
{
struct TagsProcessor
{
  explicit TagsProcessor(std::string const & maxspeedsFilePath) : m_maxspeedsCollector(maxspeedsFilePath) {}

  RoadAccessWriter m_roadAccessWriter;
  RestrictionWriter m_restrictionWriter;
  CameraNodeProcessor m_cameraNodeWriter;
  generator::MaxspeedsCollector m_maxspeedsCollector;
};

// Adds feature id and corresponding |osmId| to |osmIdToFeatureId|.
// Note. In general, one |featureId| may correspond to several osm ids.
// But for a road feature |featureId| corresponds to exactly one osm id.
void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId,
                  std::map<base::GeoObjectId, uint32_t> & osmIdToFeatureId);

// Parses comma separated text file with line in following format:
// <feature id>, <osm id 1 corresponding feature id>, <osm id 2 corresponding feature id>, and so
// on
// For example:
// 137999, 5170186,
// 138000, 5170209, 5143342,
// 138001, 5170228,
bool ParseOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath,
                                  std::map<base::GeoObjectId, uint32_t> & osmIdToFeatureId);
bool ParseFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath,
                                  std::map<uint32_t, base::GeoObjectId> & featureIdToOsmId);
}  // namespace routing
