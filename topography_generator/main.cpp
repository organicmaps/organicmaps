#include "generator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "3party/gflags/src/gflags/gflags.h"


DEFINE_string(out_dir, "/Users/daravolvenkova/isolines/", "Path to output directory.");
DEFINE_string(countryId, "",//"France_Rhone-Alpes_Haute-Savoie",
              "Isolines packing mode. Pack isolines for countryId.");
DEFINE_string(isolines_path, "/Users/daravolvenkova/isolines/",
              "Isolines packing mode. Path to the directory with isolines tiles.");
DEFINE_uint64(simpl_zoom, 17, "Isolines packing mode. Isolines simplification zoom.");
DEFINE_uint64(max_length, 1000, "Isolines packing mode. Isolines max length.");
DEFINE_string(srtm_path, "/Users/daravolvenkova/srtm/2000.02.11/",
              "Isolines generating mode. Path to srtm directory.");
DEFINE_int32(left, 5, "Isolines generating mode. Left longitude of tiles rect.");
DEFINE_int32(right, 6, "Isolines generating mode. Right longitude of tiles rect.");
DEFINE_int32(bottom, 45, "Isolines generating mode. Bottom latitude of tiles rect.");
DEFINE_int32(top, 46, "Isolines generating mode. Top latitude of tiles rect.");
DEFINE_uint64(isolines_step, 10, "Isolines generating mode. Isolines step in meters.");
DEFINE_uint64(latlon_step_factor, 1, "Isolines generating mode. Lat/lon step factor.");
DEFINE_double(gaussian_st_dev, 2.0, "Isolines generating mode. Gaussian filter standard deviation.");
DEFINE_double(gaussian_r_factor, 1.0, "Isolines generating mode. Gaussian filter radius factor.");
DEFINE_uint64(median_r, 1, "Isolines generating mode. Median filter radius.");
DEFINE_uint64(threads, 4, "Number of threads.");
DEFINE_uint64(tiles_per_thread, 9, "Max cached tiles per thread");


int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_out_dir.empty())
  {
    LOG(LINFO, ("out_dir must be set."));
    return 1;
  }

  topography_generator::Generator generator(FLAGS_srtm_path, FLAGS_threads,
                                            FLAGS_tiles_per_thread);
  if (!FLAGS_countryId.empty())
  {
    if (FLAGS_isolines_path.empty())
    {
      LOG(LINFO, ("isolines_path must be set."));
      return 1;
    }

    topography_generator::CountryIsolinesParams params;
    params.m_simplificationZoom = static_cast<int>(FLAGS_simpl_zoom);
    params.m_maxIsolineLenght = FLAGS_max_length;

    generator.PackIsolinesForCountry(FLAGS_countryId, FLAGS_isolines_path, FLAGS_out_dir, params);
    return 0;
  }

  if (FLAGS_srtm_path.empty())
  {
    LOG(LINFO, ("srtm_path must be set."));
    return 1;
  }

  if (FLAGS_right < FLAGS_left || FLAGS_top < FLAGS_bottom)
  {
    LOG(LINFO, ("Invalid tiles rect."));
    return 1;
  }

  topography_generator::TileIsolinesParams params;
  if (FLAGS_median_r > 0)
    params.m_filters.emplace_back(new topography_generator::MedianFilter<topography_generator::Altitude>(FLAGS_median_r));
  if (FLAGS_gaussian_st_dev > 0.0)
  {
    params.m_filters.emplace_back(new topography_generator::GaussianFilter<topography_generator::Altitude>(
      FLAGS_gaussian_st_dev, FLAGS_gaussian_r_factor));
  }

  params.m_outputDir = FLAGS_out_dir;
  params.m_alitudesStep = FLAGS_isolines_step;
  params.m_latLonStepFactor = FLAGS_latlon_step_factor;

  generator.GenerateIsolines(FLAGS_left, FLAGS_bottom, FLAGS_right, FLAGS_top, params);

  return 0;
}
