#pragma once

#include "generator/affiliation.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"

#include "routing/cross_border_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace routing
{
// Road way and regions through which it passes.
struct RoadData
{
  RoadData() = default;

  RoadData(std::set<std::string> regions, OsmElement && way) : m_regions(std::move(regions)), m_way(std::move(way)) {}

  std::set<std::string> m_regions;
  OsmElement m_way;
};

using RoadsData = std::unordered_map<uint64_t, RoadData>;
using HighwayTypeToRoads = std::unordered_map<std::string, RoadsData>;

// Ways and corresponding nodes of roads extracted from OSM file.
struct RoadsFromOsm
{
  HighwayTypeToRoads m_ways;
  std::unordered_map<uint64_t, ms::LatLon> m_nodes;
};

// Reads roads from |reader| and finds its mwms with |mwmMatcher|.
RoadsFromOsm GetRoadsFromOsm(generator::SourceReader & reader, feature::CountriesFilesAffiliation const & mwmMatcher,
                             std::vector<std::string> const & highways);

// Fills |graph| with new segments starting from |curSegmentId|. Segments are calculated from the
// road consisting of |nodeIds| from OSM.
bool FillCrossBorderGraph(CrossBorderGraph & graph, RegionSegmentId & curSegmentId,
                          std::vector<uint64_t> const & nodeIds, std::unordered_map<uint64_t, ms::LatLon> const & nodes,
                          feature::CountriesFilesAffiliation const & mwmMatcher,
                          std::unordered_map<std::string, routing::NumMwmId> const & regionToIdMap);

// Dumps |graph| to |path|.
bool WriteGraphToFile(CrossBorderGraph const & graph, std::string const & path, bool overwrite);

// Logs statistics about segments in |graph|.
void ShowRegionsStats(CrossBorderGraph const & graph, std::shared_ptr<routing::NumMwmIds> numMwmIds);
}  // namespace routing
