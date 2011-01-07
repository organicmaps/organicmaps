#include "data_generator.hpp"
#include "feature_generator.hpp"
#include "feature_sorter.hpp"
#include "update_generator.hpp"
#include "feature_bucketer.hpp"
#include "grid_generator.hpp"

#include "../classif_routine.hpp"
#include "../features_vector.hpp"
#include "../index_builder.hpp"
#include "../osm_decl.hpp"
#include "../data_header.hpp"

#include "../../storage/defines.hpp"

#include "../../platform/platform.hpp"

#include "../../3party/gflags/src/gflags/gflags.h"

#include "../../std/ctime.hpp"
#include "../../std/iostream.hpp"
#include "../../std/iomanip.hpp"
#include "../../std/numeric.hpp"

#include "../../version/version.hpp"

#include "../../base/start_mem_debug.hpp"

DEFINE_bool(version, false, "Display version");
DEFINE_bool(generate_update, false,
              "If specified, update.maps file will be generated from cells in the data path");

DEFINE_bool(sort_features, true, "Sort features data for better cache-friendliness.");
DEFINE_bool(generate_classif, false, "Generate classificator.");
DEFINE_bool(preprocess_xml, false, "1st pass - create nodes/ways/relations data");
DEFINE_bool(generate_features, false, "2nd pass - generate intermediate features");
DEFINE_bool(generate_geometry, false, "3rd pass - split and simplify geometry and triangles for features");
DEFINE_bool(generate_index, false, "4rd pass - generate index");
DEFINE_bool(generate_grid, false, "Generate grid for given bucketing_level");
DEFINE_bool(use_light_nodes, false,
            "If true, use temporary vector of nodes, instead of huge temp file");
DEFINE_string(data_path, "", "Working directory, 'path_to_exe/../../data' if empty.");
DEFINE_string(output, "", "Prefix of filenames of outputted .dat and .idx files.");
DEFINE_string(intermediate_data_path, "", "Path to store nodes, ways, relations.");
DEFINE_int32(bucketing_level, 7, "Level of cell ids for bucketing.");
DEFINE_int32(worldmap_max_zoom, -1, "If specified, features for zoomlevels [0..this_value] "
             " which are enabled in classificator will be added to the separate world.map");

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

  string const path =
      FLAGS_data_path.empty() ? GetPlatform().WritableDir() : AddSlashIfNeeded(FLAGS_data_path);

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

  if (FLAGS_generate_grid)
  {
    grid::GenerateGridToStdout(FLAGS_bucketing_level);
  }

  // Generating intermediate files
  if (FLAGS_preprocess_xml)
  {
    LOG(LINFO, ("Generating intermediate data ...."));
    if (!data::GenerateToFile(FLAGS_intermediate_data_path, FLAGS_use_light_nodes))
      return -1;
  }

  feature::GenerateInfo genInfo;
  genInfo.dir = FLAGS_intermediate_data_path;

  // Generate dat file
  if (FLAGS_generate_features)
  {
    LOG(LINFO, ("Generating final data ..."));

    classificator::Read(path + "drawing_rules.bin",
                        path + "classificator.txt",
                        path + "visibility.txt");
    classificator::PrepareForFeatureGeneration();

    if (FLAGS_output.empty())
      genInfo.datFilePrefix = path;
    else
      genInfo.datFilePrefix = path + FLAGS_output + (FLAGS_bucketing_level > 0 ? "-" : "");
    genInfo.datFileSuffix = DATA_FILE_EXTENSION;
    genInfo.cellBucketingLevel = FLAGS_bucketing_level;
    genInfo.m_maxScaleForWorldFeatures = FLAGS_worldmap_max_zoom;

    if (!feature::GenerateFeatures(genInfo, FLAGS_use_light_nodes))
    {
      return -1;
    }

    for (size_t i = 0; i < genInfo.bucketNames.size(); ++i)
      genInfo.bucketNames[i] = genInfo.datFilePrefix + genInfo.bucketNames[i] + genInfo.datFileSuffix;
    if (FLAGS_worldmap_max_zoom >= 0)
      genInfo.bucketNames.push_back(genInfo.datFilePrefix + WORLD_FILE_NAME + genInfo.datFileSuffix);
  }
  else
  {
    genInfo.bucketNames.push_back(path + FLAGS_output + DATA_FILE_EXTENSION);
  }

  // Enumerate over all dat files that were created.
  for (size_t i = 0; i < genInfo.bucketNames.size(); ++i)
  {
    string const & datFile = genInfo.bucketNames[i];

    if (FLAGS_generate_geometry)
    {
      LOG(LINFO, ("Generating result features for ", datFile));
      if (!feature::GenerateFinalFeatures(datFile, FLAGS_sort_features))
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
  }

  // Create http update list for countries and corresponding files
  if (FLAGS_generate_update)
  {
    LOG(LINFO, ("Creating maps.update file..."));
    update::GenerateFilesList(path);
  }

  return 0;
}
