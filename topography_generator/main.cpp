#include "generator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include <cstdlib>

// Common option for all modes
DEFINE_bool(force, false, "Force to regenerate isolines for tiles and countries.");

// Options for automatic isolines generating mode.
DEFINE_string(profiles_path, "", "Automatic isolines generating mode. "
                                 "Path to a json file with isolines profiles.");
DEFINE_string(countries_to_generate_path, "", "Automatic isolines generating mode. "
                                              "Path to a json file with countries to generate.");
DEFINE_string(tiles_isolines_out_dir, "", "Automatic isolines generating mode. Path to output "
                                          "intermediate directory with tiles isolines.");
DEFINE_string(countries_isolines_out_dir, "", "Automatic isolines generating mode. "
                                              "Path to output directory with countries isolines.");

// Common option for automatic isolines generating mode and custom packing mode.
DEFINE_string(data_dir, "", "Path to data directory.");

// Common option for automatic isolines generating mode and custom generating mode.
DEFINE_string(srtm_path, "", "Path to srtm directory.");
DEFINE_uint64(threads, 4, "Number of threads.");
DEFINE_uint64(tiles_per_thread, 9, "Max cached tiles per thread");

// Common options for custom isolines generating and custom packing modes.
DEFINE_string(out_dir, "", "Path to output directory.");
DEFINE_uint64(simpl_zoom, 16, "Isolines simplification zoom.");

// Options for custom isolines packing mode.
DEFINE_string(countryId, "",
              "Custom isolines packing mode. Pack isolines for countryId.");
DEFINE_string(isolines_path, "",
              "Custom isolines packing mode. Path to the directory with isolines tiles.");
DEFINE_uint64(max_length, 1000, "Custom isolines packing mode. Isolines max length.");
DEFINE_uint64(alt_step_factor, 1, "Custom isolines packing mode. Altitude step factor.");

// Options for custom isolines generating mode.
DEFINE_int32(left, 0, "Custom isolines generating mode. Left longitude of tiles rect [-180, 179].");
DEFINE_int32(right, 0, "Custom isolines generating mode. Right longitude of tiles rect [-179, 180].");
DEFINE_int32(bottom, 0, "Custom isolines generating mode. Bottom latitude of tiles rect [-90, 89].");
DEFINE_int32(top, 0, "Custom isolines generating mode. Top latitude of tiles rect [-89, 90].");
DEFINE_uint64(isolines_step, 10, "Custom isolines generating mode. Isolines step in meters.");
DEFINE_uint64(latlon_step_factor, 2, "Custom isolines generating mode. Lat/lon step factor.");
DEFINE_double(gaussian_st_dev, 2.0, "Custom isolines generating mode. Gaussian filter standard deviation.");
DEFINE_double(gaussian_r_factor, 1.0, "Custom isolines generating mode. Gaussian filter radius factor.");
DEFINE_uint64(median_r, 1, "Custom isolines generating mode. Median filter radius.");

using namespace topography_generator;

int main(int argc, char ** argv)
{
  google::SetUsageMessage(
    "\n\nThis tool generates isolines and works in the following modes:\n"
    "1. Automatic isolines generating mode. Generates a binary file with isolines for each\n"
    "   country id from the countries_to_generate_path. Gets options for isolines generating\n"
    "   from the corresponding to the country profile in profiles_path.\n"
    "   Stores intermediate binary isolines tiles to tiles_isolines_out_dir and result countries\n"
    "   isolines to countries_isolines_out_dir.\n"
    "2. Custom isolines generating mode. Generates binary tile with isolines for each SRTM tile in the\n"
    "   specified tile rect.\n"
    "   Mode activates by passing a valid tiles rect.\n"
    "   An isoline would be generated for each isolines_step difference in height.\n"
    "   Tiles for lat >= 60 && lat < -60 (converted from ASTER source) can be filtered by\n"
    "   median and/or gaussian filters.\n"
    "   Median filter activates by nonzero filter kernel radius median_r.\n"
    "   Gaussian filter activates by gaussian_st_dev > 0.0 && gaussian_r_factor > 0.0 parameters.\n"
    "   Contours generating steps through altitudes matrix of SRTM tile can be adjusted by\n"
    "   latlon_step_factor parameter.\n"
    "   Isolines simplification activates by nonzero simpl_zoom [1..17]\n"
    "\n"
    "3. Custom packing isolines from ready tiles into a binary file for specified country id.\n"
    "   Mode activates by passing a countryId parameter.\n"
    "   Tool gets isolines from the tiles, covered by the country regions, selects\n"
    "   altitude levels with alt_step_factor (if a tile stores altitudes for each 10 meters\n"
    "   and alt_step_factor == 5, the result binary file will store altitudes for each 50 meters).\n"
    "   Isolines cropped by the country regions and cut by max_length parameter.\n"
    "   Isolines simplification activates by nonzero simpl_zoom [1..17]\n\n");

  google::ParseCommandLineFlags(&argc, &argv, true);

  bool isCustomGeneratingMode = false;
  bool isCustomPackingMode = false;
  bool const isAutomaticMode = !FLAGS_profiles_path.empty() ||
                               !FLAGS_countries_to_generate_path.empty() ||
                               !FLAGS_tiles_isolines_out_dir.empty() ||
                               !FLAGS_countries_isolines_out_dir.empty();
  if (isAutomaticMode)
  {
    if (FLAGS_profiles_path.empty() || FLAGS_countries_to_generate_path.empty() ||
        FLAGS_tiles_isolines_out_dir.empty() || FLAGS_countries_isolines_out_dir.empty())
    {
      LOG(LERROR, ("To run automatic isolines generating mode all the params profiles_path, "
                   "countries_to_generate_path, tiles_isolines_out_dir, countries_isolines_out_dir "
                   "must be set."));
      return EXIT_FAILURE;
    }
  }
  else
  {
    if (FLAGS_out_dir.empty())
    {
      LOG(LERROR, ("out_dir must be set."));
      return EXIT_FAILURE;
    }

    auto const validTilesRect = FLAGS_right > FLAGS_left && FLAGS_top > FLAGS_bottom &&
                                FLAGS_right <= 180 && FLAGS_left >= -180 &&
                                FLAGS_top <= 90 && FLAGS_bottom >= -90;

    isCustomGeneratingMode = validTilesRect;
    isCustomPackingMode = !FLAGS_countryId.empty();

    if (isCustomGeneratingMode && isCustomPackingMode)
    {
      LOG(LERROR, ("Both tiles rect and country id are set. Сhoose one operation: "
                   "generating of the tiles rect or packing tiles for the country"));
      return EXIT_FAILURE;
    }

    if (!isCustomGeneratingMode && !isCustomPackingMode)
    {
      LOG(LERROR, ("Specify isolines profiles and countries to generate, or a valid tiles rect, "
                   "or a country id."));
      return EXIT_FAILURE;
    }
  }

  if (isCustomPackingMode || isAutomaticMode)
  {
    if (FLAGS_data_dir.empty())
    {
      LOG(LERROR, ("data_dir must be set."));
      return EXIT_FAILURE;
    }
  }

  if (isCustomGeneratingMode || isAutomaticMode)
  {
    if (FLAGS_srtm_path.empty())
    {
      LOG(LERROR, ("srtm_path must be set."));
      return EXIT_FAILURE;
    }
  }

  Generator generator(FLAGS_srtm_path, FLAGS_threads, FLAGS_tiles_per_thread, FLAGS_force);

  if (isAutomaticMode)
  {
    generator.InitCountryInfoGetter(FLAGS_data_dir);
    generator.InitProfiles(FLAGS_profiles_path, FLAGS_countries_to_generate_path,
                           FLAGS_tiles_isolines_out_dir, FLAGS_countries_isolines_out_dir);
    generator.GenerateIsolinesForCountries();
    generator.PackIsolinesForCountries();

    return EXIT_SUCCESS;
  }

  if (isCustomPackingMode)
  {
    if (FLAGS_isolines_path.empty())
    {
      LOG(LERROR, ("isolines_path must be set."));
      return EXIT_FAILURE;
    }

    IsolinesPackingParams params;
    params.m_outputDir = FLAGS_out_dir;
    params.m_simplificationZoom = static_cast<int>(FLAGS_simpl_zoom);
    params.m_maxIsolineLength = FLAGS_max_length;
    params.m_alitudesStepFactor = FLAGS_alt_step_factor;
    params.m_isolinesTilesPath = FLAGS_isolines_path;

    generator.InitCountryInfoGetter(FLAGS_data_dir);
    generator.PackIsolinesForCountry(FLAGS_countryId, params);

    return EXIT_SUCCESS;
  }

  CHECK(isCustomGeneratingMode, ());

  TileIsolinesParams params;
  if (FLAGS_median_r > 0)
  {
    params.m_filters.emplace_back(std::make_unique<MedianFilter<Altitude>>(FLAGS_median_r));
  }

  if (FLAGS_gaussian_st_dev > 0.0 && FLAGS_gaussian_r_factor > 0)
  {
    params.m_filters.emplace_back(
      std::make_unique<GaussianFilter<Altitude>>(FLAGS_gaussian_st_dev, FLAGS_gaussian_r_factor));
  }

  params.m_outputDir = FLAGS_out_dir;
  params.m_alitudesStep = FLAGS_isolines_step;
  params.m_latLonStepFactor = FLAGS_latlon_step_factor;
  params.m_simplificationZoom = static_cast<int>(FLAGS_simpl_zoom);

  generator.GenerateIsolines(FLAGS_left, FLAGS_bottom, FLAGS_right, FLAGS_top, params);

  return EXIT_SUCCESS;
}
