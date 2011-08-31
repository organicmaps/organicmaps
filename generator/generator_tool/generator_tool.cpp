#include "../data_generator.hpp"
#include "../feature_generator.hpp"
#include "../feature_sorter.hpp"
#include "../update_generator.hpp"
#include "../borders_generator.hpp"
#include "../classif_routine.hpp"
#include "../dumper.hpp"
#include "../mwm_rect_updater.hpp"
#include "../statistics.hpp"
#include "../unpack_mwm.hpp"
#include "../generate_info.hpp"

#include "../../indexer/classificator_loader.hpp"
#include "../../indexer/data_header.hpp"
#include "../../indexer/features_vector.hpp"
#include "../../indexer/index_builder.hpp"
#include "../../indexer/search_index_builder.hpp"

#include "../../defines.hpp"

#include "../../platform/platform.hpp"

#include "../../3party/gflags/src/gflags/gflags.h"

#include "../../std/iostream.hpp"
#include "../../std/iomanip.hpp"
#include "../../std/numeric.hpp"

#include "../../version/version.hpp"

#include "../../base/start_mem_debug.hpp"

DEFINE_bool(version, false, "Display version");
DEFINE_bool(generate_update, false,
              "If specified, update.maps file will be generated from cells in the data path");
DEFINE_bool(generate_classif, false, "Generate classificator.");
DEFINE_bool(preprocess_xml, false, "1st pass - create nodes/ways/relations data");
DEFINE_bool(generate_features, false, "2nd pass - generate intermediate features");
DEFINE_bool(generate_geometry, false, "3rd pass - split and simplify geometry and triangles for features");
DEFINE_bool(generate_index, false, "4rd pass - generate index");
DEFINE_bool(generate_search_index, false, "5th pass - generate search index");
DEFINE_bool(calc_statistics, false, "Calculate feature statistics for specified mwm bucket files");
DEFINE_bool(use_light_nodes, false,
            "If true, use temporary vector of nodes, instead of huge temp file");
DEFINE_string(data_path, "", "Working directory, 'path_to_exe/../../data' if empty.");
DEFINE_string(output, "", "Prefix of filenames of outputted .dat and .idx files.");
DEFINE_string(intermediate_data_path, "", "Path to store nodes, ways, relations.");
DEFINE_bool(generate_world, false, "Generate separate world file");
DEFINE_bool(split_by_polygons, false, "Use countries borders to split planet by regions and countries");
DEFINE_string(generate_borders, "",
            "Create binary country .borders file for osm xml file given in 'output' parameter,"
            "specify tag name and optional value: ISO3166-1 or admin_level=4");
DEFINE_bool(dump_types, false, "If defined, prints all types combinations and their total count");
DEFINE_bool(dump_prefixes, false, "If defined, prints statistics on feature name prefixes");
DEFINE_bool(unpack_mwm, false, "Unpack each section of mwm into a separate file with name filePath.sectionName.");

string AddSlashIfNeeded(string const & str)
{
  string result(str);
  size_t const size = result.size();
  if (size)
  {
    if (result.find_last_of('\\') == size - 1)
      result[size - 1] = '/';
    else
      if (result.find_last_of('/') != size - 1)
        result.push_back('/');
  }
  return result;
}

int main(int argc, char ** argv)
{
  google::SetUsageMessage(
      "Takes OSM XML data from stdin and creates data and index files in several passes.");

  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & pl = GetPlatform();

  string const path =
      FLAGS_data_path.empty() ? pl.WritableDir() : AddSlashIfNeeded(FLAGS_data_path);

  if (FLAGS_version)
  {
    cout << "Tool version: " << VERSION_STRING << endl;
    cout << "Built on: " << VERSION_DATE_STRING << endl;
  }

  // Make a classificator
  if (FLAGS_generate_classif)
  {
    classificator::GenerateAndWrite(path);
  }

  // Generating intermediate files
  if (FLAGS_preprocess_xml)
  {
    LOG(LINFO, ("Generating intermediate data ...."));
    if (!data::GenerateToFile(FLAGS_intermediate_data_path, FLAGS_use_light_nodes))
      return -1;
  }

  feature::GenerateInfo genInfo;
  genInfo.m_tmpDir = FLAGS_intermediate_data_path;

  // load classificator only if necessary
  if (FLAGS_generate_features || FLAGS_generate_geometry ||
      FLAGS_generate_index || FLAGS_generate_search_index ||
      FLAGS_calc_statistics || FLAGS_dump_types || FLAGS_dump_prefixes)
  {
    classificator::Read(pl.GetReader("drawing_rules.bin"),
                        pl.GetReader("classificator.txt"),
                        pl.GetReader("visibility.txt"),
                        pl.GetReader("types.txt"));
    classificator::PrepareForFeatureGeneration();
  }

  // Generate dat file
  if (FLAGS_generate_features)
  {
    LOG(LINFO, ("Generating final data ..."));

    if (FLAGS_output.empty() || FLAGS_split_by_polygons)  // do not break data path for polygons
      genInfo.m_datFilePrefix = path;
    else
      genInfo.m_datFilePrefix = path + FLAGS_output;
    genInfo.m_datFileSuffix = DATA_FILE_EXTENSION;

    // split data by countries polygons
    genInfo.m_splitByPolygons = FLAGS_split_by_polygons;

    genInfo.m_createWorld = FLAGS_generate_world;

    if (!feature::GenerateFeatures(genInfo, FLAGS_use_light_nodes))
      return -1;

    for (size_t i = 0; i < genInfo.m_bucketNames.size(); ++i)
      genInfo.m_bucketNames[i] = genInfo.m_datFilePrefix + genInfo.m_bucketNames[i] + genInfo.m_datFileSuffix;

    if (FLAGS_generate_world)
      genInfo.m_bucketNames.push_back(genInfo.m_datFilePrefix + WORLD_FILE_NAME + genInfo.m_datFileSuffix);
  }
  else
  {
    genInfo.m_bucketNames.push_back(path + FLAGS_output + DATA_FILE_EXTENSION);
  }

  // Enumerate over all dat files that were created.
  size_t const count = genInfo.m_bucketNames.size();
  string const worldPath = path + WORLD_FILE_NAME + DATA_FILE_EXTENSION;

  for (size_t i = 0; i < count; ++i)
  {
    string const & datFile = genInfo.m_bucketNames[i];

    if (FLAGS_generate_geometry)
    {
      LOG(LINFO, ("Generating result features for ", datFile));
      if (!feature::GenerateFinalFeatures(datFile, datFile == worldPath))
      {
        // If error - move to next bucket without index generation
        continue;
      }
    }

    if (FLAGS_generate_index)
    {
      LOG(LINFO, ("Generating index for ", datFile));
      if (!indexer::BuildIndexFromDatFile(datFile, FLAGS_intermediate_data_path + FLAGS_output))
      {
        LOG(LCRITICAL, ("Error generating index."));
      }
    }

    if (FLAGS_generate_search_index)
    {
      LOG(LINFO, ("Generating search index for ", datFile));
      if (!indexer::BuildSearchIndexFromDatFile(datFile))
      {
        LOG(LCRITICAL, ("Error generating search index."));
      }
    }

    if (FLAGS_calc_statistics)
    {
      LOG(LINFO, ("Calculating statistics for ", datFile));

      stats::FileContainerStatistic(datFile);

      stats::MapInfo info;
      stats::CalcStatistic(datFile, info);
      stats::PrintStatistic(info);
    }
  }

  //if (FLAGS_split_by_polygons)
  //  UpdateMWMRectsFromBoundaries(path);

  // Create http update list for countries and corresponding files
  if (FLAGS_generate_update)
  {
    LOG(LINFO, ("Creating maps.update file..."));
    update::GenerateFilesList(path);
  }

  if (!FLAGS_generate_borders.empty())
  {
    if (!FLAGS_output.empty())
    {
      osm::GenerateBordersFromOsm(FLAGS_generate_borders,
                                  path + FLAGS_output + ".osm",
                                  path + FLAGS_output + ".borders");
    }
    else
    {
      LOG(LINFO, ("Please specify osm country borders file in 'output' command line parameter."));
    }
  }

  if (FLAGS_dump_types)
    feature::DumpTypes(path + FLAGS_output + ".mwm");

  if (FLAGS_dump_prefixes)
    feature::DumpPrefixes(path + FLAGS_output + ".mwm");

  if (FLAGS_unpack_mwm)
  {
    UnpackMwm(path + FLAGS_output + ".mwm");
  }

  return 0;
}
