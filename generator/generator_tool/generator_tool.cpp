#include "../data_generator.hpp"
#include "../feature_generator.hpp"
#include "../feature_sorter.hpp"
#include "../update_generator.hpp"
#include "../feature_bucketer.hpp"
#include "../grid_generator.hpp"
#include "../statistics.hpp"
#include "../classif_routine.hpp"
#include "../borders_generator.hpp"

#include "../../indexer/features_vector.hpp"
#include "../../indexer/index_builder.hpp"
#include "../../indexer/osm_decl.hpp"
#include "../../indexer/data_header.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../defines.hpp"

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
DEFINE_bool(calc_statistics, false, "Calculate feature statistics for specified mwm bucket files");
DEFINE_bool(use_light_nodes, false,
            "If true, use temporary vector of nodes, instead of huge temp file");
DEFINE_string(data_path, "", "Working directory, 'path_to_exe/../../data' if empty.");
DEFINE_string(output, "", "Prefix of filenames of outputted .dat and .idx files.");
DEFINE_string(intermediate_data_path, "", "Path to store nodes, ways, relations.");
DEFINE_int32(bucketing_level, -1, "If positive, level of cell ids for bucketing.");
DEFINE_int32(generate_world_scale, -1, "If specified, features for zoomlevels [0..this_value] "
             "which are enabled in classificator will be MOVED to the separate world file");
DEFINE_bool(split_by_polygons, false, "Use kml shape files to split planet by regions and countries");
DEFINE_int32(simplify_countries_level, -1, "If positive, simplifies country polygons. Recommended values [10..15]");
DEFINE_bool(merge_coastlines, false, "If defined, tries to merge coastlines when renerating World file");
DEFINE_string(generate_borders, "",
            "Create binary country .borders file for osm xml file given in 'output' parameter,"
            "specify tag name and optional value: ISO3166-1 or admin_level=4");

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
  genInfo.tmpDir = FLAGS_intermediate_data_path;

  // load classificator only if necessary
  if (FLAGS_generate_features || FLAGS_generate_geometry ||
      FLAGS_generate_index || FLAGS_calc_statistics)
  {
    classificator::Read(path + "drawing_rules.bin",
                        path + "classificator.txt",
                        path + "visibility.txt");
    classificator::PrepareForFeatureGeneration();
  }

  // Generate dat file
  if (FLAGS_generate_features)
  {
    LOG(LINFO, ("Generating final data ..."));

    if (FLAGS_output.empty() || FLAGS_split_by_polygons)  // do not break data path for polygons
      genInfo.datFilePrefix = path;
    else
      genInfo.datFilePrefix = path + FLAGS_output + (FLAGS_bucketing_level > 0 ? "-" : "");
    genInfo.datFileSuffix = DATA_FILE_EXTENSION;

    // split data by countries polygons
    genInfo.splitByPolygons = FLAGS_split_by_polygons;
    genInfo.simplifyCountriesLevel = FLAGS_simplify_countries_level;

    genInfo.cellBucketingLevel = FLAGS_bucketing_level;
    genInfo.maxScaleForWorldFeatures = FLAGS_generate_world_scale;
    genInfo.mergeCoastlines = FLAGS_merge_coastlines;

    if (!feature::GenerateFeatures(genInfo, FLAGS_use_light_nodes))
      return -1;

    for (size_t i = 0; i < genInfo.bucketNames.size(); ++i)
      genInfo.bucketNames[i] = genInfo.datFilePrefix + genInfo.bucketNames[i] + genInfo.datFileSuffix;

    if (FLAGS_generate_world_scale >= 0)
      genInfo.bucketNames.push_back(genInfo.datFilePrefix + WORLD_FILE_NAME + genInfo.datFileSuffix);
  }
  else
  {
    genInfo.bucketNames.push_back(path + FLAGS_output + DATA_FILE_EXTENSION);
  }

  // Enumerate over all dat files that were created.
  size_t const count = genInfo.bucketNames.size();
  for (size_t i = 0; i < count; ++i)
  {
    string const & datFile = genInfo.bucketNames[i];

    if (FLAGS_generate_geometry)
    {
      LOG(LINFO, ("Generating result features for ", datFile));
      if (!feature::GenerateFinalFeatures(datFile,
        FLAGS_sort_features, datFile == path + WORLD_FILE_NAME + DATA_FILE_EXTENSION))
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

    if (FLAGS_calc_statistics)
    {
      LOG(LINFO, ("Calculating statistics for ", datFile));

      stats::FileContainerStatistic(datFile);

      stats::MapInfo info;
      stats::CalcStatistic(datFile, info);
      stats::PrintStatistic(info);
    }
  }

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

  return 0;
}
