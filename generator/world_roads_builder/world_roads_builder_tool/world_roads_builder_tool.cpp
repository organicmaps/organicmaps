#include "generator/affiliation.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"
#include "generator/world_roads_builder/world_roads_builder.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "3party/gflags/src/gflags/gflags.h"

using namespace routing;

DEFINE_string(path_resources, "", "MAPS.ME resources directory");
DEFINE_string(path_roads_file, "", "OSM file in o5m format.");
DEFINE_string(path_res_file, "", "Path to the resulting file with roads for generator_tool.");

int main(int argc, char ** argv)
{
  google::SetUsageMessage(
      "Reads OSM file, generates text file with main cross-mwm roads for generator_tool.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  auto const toolName = base::GetNameFromFullPath(argv[0]);

  if (FLAGS_path_resources.empty() || !Platform::IsDirectory(FLAGS_path_resources) ||
      FLAGS_path_roads_file.empty() || FLAGS_path_res_file.empty())
  {
    google::ShowUsageWithFlagsRestrict(argv[0], toolName.c_str());
    return EXIT_FAILURE;
  }

  GetPlatform().SetResourceDir(FLAGS_path_resources);

  feature::CountriesFilesAffiliation mwmMatcher(GetPlatform().ResourcesDir(),
                                                false /* haveBordersForWholeWorld */);

  // These types are used in maps_generator (maps_generator/genrator/steps.py in filter_roads function).
  std::vector<std::string> const highwayTypes{"motorway", "trunk", "primary", "secondary",
                                              "tertiary"};

  generator::SourceReader reader(FLAGS_path_roads_file);
  RoadsFromOsm const & roadsFromOsm = GetRoadsFromOsm(reader, mwmMatcher, highwayTypes);

  storage::Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  std::shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);

  std::unordered_map<std::string, NumMwmId> regionsToIds;

  numMwmIds->ForEachId([&regionsToIds, &numMwmIds](NumMwmId id) {
    std::string const & region = numMwmIds->GetFile(id).GetName();
    CHECK(regionsToIds.emplace(region, id).second, (id, region));
  });

  CrossBorderGraph graph;

  RegionSegmentId curSegmentId = 0;

  for (auto const & highway : highwayTypes)
  {
    auto const it = roadsFromOsm.m_ways.find(highway);
    CHECK(it != roadsFromOsm.m_ways.end(), (highway));

    auto const & ways = it->second;

    for (auto const & [wayId, wayData] : ways)
    {
      if (wayData.m_regions.size() == 1)
        continue;

      bool const foundSegments =
          FillCrossBorderGraph(graph, curSegmentId, wayData.m_way.Nodes(), roadsFromOsm.m_nodes,
                               mwmMatcher, regionsToIds);

      LOG(LINFO, ("Found segments for", wayId, ":", foundSegments));
    }
  }

  LOG(LINFO, ("Done handling regions for ways. Segments count:", graph.m_segments.size()));

  if (!WriteGraphToFile(graph, FLAGS_path_res_file, true /* overwrite */))
  {
    LOG(LCRITICAL, ("Failed writing to file", FLAGS_path_res_file));
    return EXIT_FAILURE;
  }

  LOG(LINFO, ("Saved graph to file", FLAGS_path_res_file));
  ShowRegionsStats(graph, numMwmIds);
}
