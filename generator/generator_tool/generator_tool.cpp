#include "generator/altitude_generator.hpp"
#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"
#include "generator/centers_table_builder.hpp"
#include "generator/check_model.hpp"
#include "generator/cities_boundaries_builder.hpp"
#include "generator/dumper.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_sorter.hpp"
#include "generator/generate_info.hpp"
#include "generator/metalines_builder.hpp"
#include "generator/osm_source.hpp"
#include "generator/restriction_generator.hpp"
#include "generator/road_access_generator.hpp"
#include "generator/routing_generator.hpp"
#include "generator/routing_index_generator.hpp"
#include "generator/search_index_builder.hpp"
#include "generator/statistics.hpp"
#include "generator/traffic_generator.hpp"
#include "generator/transit_generator.hpp"
#include "generator/ugc_section_builder.hpp"
#include "generator/unpack_mwm.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/rank_table.hpp"

#include "storage/country_parent_getter.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/timer.hpp"

#include "std/unique_ptr.hpp"

#include <cstdlib>
#include <string>

#include "defines.hpp"

#include "3party/gflags/src/gflags/gflags.h"

// Coastlines.
DEFINE_bool(make_coasts, false, "Create intermediate file with coasts data.");
DEFINE_bool(fail_on_coasts, false, "Stop and exit with '255' code if some coastlines are not merged.");
DEFINE_bool(emit_coasts, false,
            "Push coasts features from intermediate file to out files/countries.");

// Generator settings and paths.
DEFINE_string(osm_file_name, "", "Input osm area file.");
DEFINE_string(osm_file_type, "xml", "Input osm area file type [xml, o5m].");
DEFINE_string(data_path, "", "Working directory, 'path_to_exe/../../data' if empty.");
DEFINE_string(user_resource_path, "", "User defined resource path for classificator.txt and etc.");
DEFINE_string(intermediate_data_path, "", "Path to stored nodes, ways, relations.");
DEFINE_string(output, "", "File name for process (without 'mwm' ext).");
DEFINE_bool(preload_cache, false, "Preload all ways and relations cache.");
DEFINE_string(node_storage, "map",
              "Type of storage for intermediate points representation. Available: raw, map, mem.");
DEFINE_uint64(planet_version, my::SecondsSinceEpoch(),
              "Version as seconds since epoch, by default - now.");

// Preprocessing and feature generator.
DEFINE_bool(preprocess, false, "1st pass - create nodes/ways/relations data.");
DEFINE_bool(generate_features, false, "2nd pass - generate intermediate features.");
DEFINE_bool(generate_geometry, false,
            "3rd pass - split and simplify geometry and triangles for features.");
DEFINE_bool(generate_index, false, "4rd pass - generate index.");
DEFINE_bool(generate_search_index, false, "5th pass - generate search index.");

DEFINE_bool(dump_cities_boundaries, false, "Dump cities boundaries to a file");
DEFINE_bool(generate_cities_boundaries, false, "Generate cities boundaries section");
DEFINE_string(cities_boundaries_data, "", "File with cities boundaries");

DEFINE_bool(generate_world, false, "Generate separate world file.");
DEFINE_bool(split_by_polygons, false,
            "Use countries borders to split planet by regions and countries.");

// Routing.
DEFINE_string(osrm_file_name, "", "Input osrm file to generate routing info.");
DEFINE_bool(make_routing, false, "Make routing info based on osrm file.");
DEFINE_bool(make_cross_section, false, "Make cross section in routing file for cross mwm routing (for OSRM routing).");
DEFINE_bool(make_routing_index, false, "Make sections with the routing information.");
DEFINE_bool(make_cross_mwm, false,
            "Make section for cross mwm routing (for dynamic indexed routing).");
DEFINE_bool(disable_cross_mwm_progress, false,
            "Disable log of cross mwm section building progress.");
DEFINE_string(srtm_path, "",
              "Path to srtm directory. If set, generates a section with altitude information "
              "about roads.");
DEFINE_string(transit_path, "", "Path to directory with transit graphs in json.");

// Sponsored-related.
DEFINE_string(booking_data, "", "Path to booking data in .tsv format.");
DEFINE_string(booking_reference_path, "", "Path to mwm dataset for booking addresses matching.");
DEFINE_string(opentable_data, "", "Path to opentable data in .tsv format.");
DEFINE_string(opentable_reference_path, "", "Path to mwm dataset for opentable addresses matching.");
DEFINE_string(viator_data, "", "Path to viator data in .tsv format.");

// UGC
DEFINE_string(ugc_data, "", "Input UGC source database file name");

// Printing stuff.
DEFINE_bool(calc_statistics, false, "Calculate feature statistics for specified mwm bucket files.");
DEFINE_bool(type_statistics, false, "Calculate statistics by type for specified mwm bucket files.");
DEFINE_bool(dump_types, false, "Prints all types combinations and their total count.");
DEFINE_bool(dump_prefixes, false, "Prints statistics on feature's' name prefixes.");
DEFINE_bool(dump_search_tokens, false, "Print statistics on search tokens.");
DEFINE_string(dump_feature_names, "", "Print all feature names by 2-letter locale.");

// Service functions.
DEFINE_bool(generate_classif, false, "Generate classificator.");
DEFINE_bool(generate_packed_borders, false, "Generate packed file with country polygons.");
DEFINE_string(unpack_borders, "", "Convert packed_polygons to a directory of polygon files (specify folder).");
DEFINE_bool(unpack_mwm, false, "Unpack each section of mwm into a separate file with name filePath.sectionName.");
DEFINE_bool(check_mwm, false, "Check map file to be correct.");
DEFINE_string(delete_section, "", "Delete specified section (defines.hpp) from container.");
DEFINE_bool(generate_addresses_file, false, "Generate .addr file (for '--output' option) with full addresses list.");
DEFINE_bool(generate_traffic_keys, false,
            "Generate keys for the traffic map (road segment -> speed group).");

using namespace generator;

int main(int argc, char ** argv)
{
  google::SetUsageMessage(
      "Takes OSM XML data from stdin and creates data and index files in several passes.");

  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & pl = GetPlatform();

  if (!FLAGS_user_resource_path.empty())
    pl.SetResourceDir(FLAGS_user_resource_path);

  std::string const path =
      FLAGS_data_path.empty() ? pl.WritableDir() : my::AddSlashIfNeeded(FLAGS_data_path);

  feature::GenerateInfo genInfo;
  genInfo.m_intermediateDir = FLAGS_intermediate_data_path.empty() ? path
                            : my::AddSlashIfNeeded(FLAGS_intermediate_data_path);
  genInfo.m_targetDir = genInfo.m_tmpDir = path;

  /// @todo Probably, it's better to add separate option for .mwm.tmp files.
  if (!FLAGS_intermediate_data_path.empty())
  {
    std::string const tmpPath = genInfo.m_intermediateDir + "tmp" + my::GetNativeSeparator();
    if (Platform::MkDir(tmpPath) != Platform::ERR_UNKNOWN)
      genInfo.m_tmpDir = tmpPath;
  }

  genInfo.m_osmFileName = FLAGS_osm_file_name;
  genInfo.m_failOnCoasts = FLAGS_fail_on_coasts;
  genInfo.m_preloadCache = FLAGS_preload_cache;
  genInfo.m_bookingDatafileName = FLAGS_booking_data;
  genInfo.m_bookingReferenceDir = FLAGS_booking_reference_path;
  genInfo.m_opentableDatafileName = FLAGS_opentable_data;
  genInfo.m_opentableReferenceDir = FLAGS_opentable_reference_path;
  genInfo.m_viatorDatafileName = FLAGS_viator_data;
  genInfo.m_boundariesTable = make_shared<generator::OsmIdToBoundariesTable>();

  genInfo.m_versionDate = static_cast<uint32_t>(FLAGS_planet_version);

  if (!FLAGS_node_storage.empty())
    genInfo.SetNodeStorageType(FLAGS_node_storage);
  if (!FLAGS_osm_file_type.empty())
    genInfo.SetOsmFileType(FLAGS_osm_file_type);

  // Generate intermediate files.
  if (FLAGS_preprocess)
  {
    LOG(LINFO, ("Generating intermediate data ...."));
    if (!GenerateIntermediateData(genInfo))
    {
      return -1;
    }
  }

  // Use merged style.
  GetStyleReader().SetCurrentStyle(MapStyleMerged);

  // Load classificator only when necessary.
  if (FLAGS_make_coasts || FLAGS_generate_features || FLAGS_generate_geometry ||
      FLAGS_generate_index || FLAGS_generate_search_index || FLAGS_generate_cities_boundaries ||
      FLAGS_calc_statistics || FLAGS_type_statistics || FLAGS_dump_types || FLAGS_dump_prefixes ||
      FLAGS_dump_feature_names != "" || FLAGS_check_mwm || FLAGS_srtm_path != "" ||
      FLAGS_make_routing_index || FLAGS_make_cross_mwm || FLAGS_generate_traffic_keys ||
      FLAGS_transit_path != "")
  {
    classificator::Load();
    classif().SortClassificator();
  }

  // Load mwm tree only if we need it
  std::unique_ptr<storage::CountryParentGetter> countryParentGetter;
  if (FLAGS_make_routing_index || FLAGS_make_cross_mwm)
  {
    countryParentGetter =
        make_unique<storage::CountryParentGetter>();
  }

  // Generate dat file.
  if (FLAGS_generate_features || FLAGS_make_coasts)
  {
    LOG(LINFO, ("Generating final data ..."));

    genInfo.m_splitByPolygons = FLAGS_split_by_polygons;
    genInfo.m_createWorld = FLAGS_generate_world;
    genInfo.m_makeCoasts = FLAGS_make_coasts;
    genInfo.m_emitCoasts = FLAGS_emit_coasts;
    genInfo.m_fileName = FLAGS_output;
    genInfo.m_genAddresses = FLAGS_generate_addresses_file;

    if (!GenerateFeatures(genInfo))
      return -1;

    if (FLAGS_generate_world)
    {
      genInfo.m_bucketNames.push_back(WORLD_FILE_NAME);
      genInfo.m_bucketNames.push_back(WORLD_COASTS_FILE_NAME);
    }

    if (FLAGS_dump_cities_boundaries)
    {
      CHECK(!FLAGS_cities_boundaries_data.empty(), ());
      LOG(LINFO, ("Dumping cities boundaries to", FLAGS_cities_boundaries_data));
      if (!generator::SerializeBoundariesTable(FLAGS_cities_boundaries_data,
                                               *genInfo.m_boundariesTable))
      {
        LOG(LCRITICAL, ("Error serializing boundaries table to", FLAGS_cities_boundaries_data));
      }
    }
  }
  else
  {
    if (!FLAGS_output.empty())
      genInfo.m_bucketNames.push_back(FLAGS_output);
  }

  // Enumerate over all dat files that were created.
  size_t const count = genInfo.m_bucketNames.size();
  for (size_t i = 0; i < count; ++i)
  {
    std::string const & country = genInfo.m_bucketNames[i];
    std::string const datFile = my::JoinFoldersToPath(path, country + DATA_FILE_EXTENSION);
    std::string const osmToFeatureFilename =
        genInfo.GetTargetFileName(country) + OSM2FEATURE_FILE_EXTENSION;

    if (FLAGS_generate_geometry)
    {
      int mapType = feature::DataHeader::country;
      if (country == WORLD_FILE_NAME)
        mapType = feature::DataHeader::world;
      if (country == WORLD_COASTS_FILE_NAME)
        mapType = feature::DataHeader::worldcoasts;

      // On error move to the next bucket without index generation.

      LOG(LINFO, ("Generating result features for", country));
      if (!feature::GenerateFinalFeatures(genInfo, country, mapType))
        continue;

      LOG(LINFO, ("Generating offsets table for", datFile));
      if (!feature::BuildOffsetsTable(datFile))
        continue;

      if (mapType == feature::DataHeader::country)
      {
        std::string const metalinesFilename =
            genInfo.GetIntermediateFileName(METALINES_FILENAME, "" /* extension */);

        LOG(LINFO, ("Processing metalines from", metalinesFilename));
        if (!feature::WriteMetalinesSection(datFile, metalinesFilename, osmToFeatureFilename))
          LOG(LCRITICAL, ("Error generating metalines section."));
      }
    }

    if (FLAGS_generate_index)
    {
      LOG(LINFO, ("Generating index for", datFile));

      if (!indexer::BuildIndexFromDataFile(datFile, FLAGS_intermediate_data_path + country))
        LOG(LCRITICAL, ("Error generating index."));
    }

    if (FLAGS_generate_search_index)
    {
      LOG(LINFO, ("Generating search index for", datFile));

      if (!indexer::BuildSearchIndexFromDataFile(datFile, true))
        LOG(LCRITICAL, ("Error generating search index."));

      LOG(LINFO, ("Generating rank table for", datFile));
      if (!search::RankTableBuilder::CreateIfNotExists(datFile))
        LOG(LCRITICAL, ("Error generating rank table."));

      LOG(LINFO, ("Generating centers table for", datFile));
      if (!indexer::BuildCentersTableFromDataFile(datFile, true /* forceRebuild */))
        LOG(LCRITICAL, ("Error generating centers table."));
    }

    if (FLAGS_generate_cities_boundaries)
    {
      CHECK(!FLAGS_cities_boundaries_data.empty(), ());
      LOG(LINFO, ("Generating cities boundaries for", datFile));
      generator::OsmIdToBoundariesTable table;
      if (!generator::DeserializeBoundariesTable(FLAGS_cities_boundaries_data, table))
        LOG(LCRITICAL, ("Error deserializing boundaries table"));
      if (!generator::BuildCitiesBoundaries(datFile, osmToFeatureFilename, table))
        LOG(LCRITICAL, ("Error generating cities boundaries."));
    }

    if (!FLAGS_srtm_path.empty())
      routing::BuildRoadAltitudes(datFile, FLAGS_srtm_path);

    if (!FLAGS_transit_path.empty())
      routing::transit::BuildTransit(datFile, osmToFeatureFilename, FLAGS_transit_path);

    if (FLAGS_make_routing_index)
    {
      if (!countryParentGetter)
      {
        // All the mwms should use proper VehicleModels.
        LOG(LCRITICAL, ("Countries file is needed. Please set countries file name (countries.txt or "
                        "countries_obsolete.txt). File must be located in data directory."));
        return -1;
      }

      std::string const restrictionsFilename =
          genInfo.GetIntermediateFileName(RESTRICTIONS_FILENAME, "" /* extension */);
      std::string const roadAccessFilename =
          genInfo.GetIntermediateFileName(ROAD_ACCESS_FILENAME, "" /* extension */);

      routing::BuildRoadRestrictions(datFile, restrictionsFilename, osmToFeatureFilename);
      routing::BuildRoadAccessInfo(datFile, roadAccessFilename, osmToFeatureFilename);
      routing::BuildRoutingIndex(datFile, country, *countryParentGetter);
    }

    if (FLAGS_make_cross_mwm)
    {
      if (!countryParentGetter)
      {
        // All the mwms should use proper VehicleModels.
        LOG(LCRITICAL, ("Countries file is needed. Please set countries file name (countries.txt or "
                        "countries_obsolete.txt). File must be located in data directory."));
        return -1;
      }

      if (!routing::BuildCrossMwmSection(path, datFile, country, *countryParentGetter,
                                         osmToFeatureFilename, FLAGS_disable_cross_mwm_progress))
        LOG(LCRITICAL, ("Error generating cross mwm section."));
    }

    if (!FLAGS_ugc_data.empty())
    {
      if (!BuildUgcMwmSection(FLAGS_ugc_data, datFile, osmToFeatureFilename))
      {
        LOG(LCRITICAL, ("Error generating UGC mwm section."));
      }
    }

    if (FLAGS_generate_traffic_keys)
    {
      if (!traffic::GenerateTrafficKeysFromDataFile(datFile))
        LOG(LCRITICAL, ("Error generating traffic keys."));
    }
  }

  std::string const datFile = my::JoinFoldersToPath(path, FLAGS_output + DATA_FILE_EXTENSION);

  if (FLAGS_calc_statistics)
  {
    LOG(LINFO, ("Calculating statistics for", datFile));

    stats::FileContainerStatistic(datFile);
    stats::FileContainerStatistic(datFile + ROUTING_FILE_EXTENSION);

    stats::MapInfo info;
    stats::CalcStatistic(datFile, info);
    stats::PrintStatistic(info);
  }

  if (FLAGS_type_statistics)
  {
    LOG(LINFO, ("Calculating type statistics for", datFile));

    stats::MapInfo info;
    stats::CalcStatistic(datFile, info);
    stats::PrintTypeStatistic(info);
  }

  if (FLAGS_dump_types)
    feature::DumpTypes(datFile);

  if (FLAGS_dump_prefixes)
    feature::DumpPrefixes(datFile);

  if (FLAGS_dump_search_tokens)
    feature::DumpSearchTokens(datFile, 100 /* maxTokensToShow */);

  if (FLAGS_dump_feature_names != "")
    feature::DumpFeatureNames(datFile, FLAGS_dump_feature_names);

  if (FLAGS_unpack_mwm)
    UnpackMwm(datFile);

  if (!FLAGS_delete_section.empty())
    DeleteSection(datFile, FLAGS_delete_section);

  if (FLAGS_generate_packed_borders)
    borders::GeneratePackedBorders(path);

  if (!FLAGS_unpack_borders.empty())
    borders::UnpackBorders(path, FLAGS_unpack_borders);

  if (FLAGS_check_mwm)
    check_model::ReadFeatures(datFile);

  if (!FLAGS_osrm_file_name.empty() && FLAGS_make_routing)
    routing::BuildRoutingIndex(path, FLAGS_output, FLAGS_osrm_file_name);

  if (!FLAGS_osrm_file_name.empty() && FLAGS_make_cross_section)
    routing::BuildCrossRoutingIndex(path, FLAGS_output, FLAGS_osrm_file_name);

  return 0;
}
