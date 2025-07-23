#include "track_generator/utils.hpp"

#include "routing/vehicle_mask.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <string>

#include <gflags/gflags.h>

DEFINE_string(inputDir, "", "Path to kmls.");

DEFINE_string(outputDir, "", "Path to converted kmls.");

DEFINE_int32(vehicleType, 0,
             "Numeric value of routing::VehicleType enum. "
             "Pedestrian by default.");

DEFINE_bool(showHelp, false, "Show help on all flags.");

int main(int argc, char ** argv)
{
  gflags::SetUsageMessage(
      "The tool is used to generate more smooth track based on "
      "waypoints from the kml. The kml has to contain points "
      "within LineString tag. If the router can't build the route "
      "than the specific path will be untouched. Multiple kmls into "
      " the directory are supported.\n\n"
      "Usage example: "
      "./track_generator_tool -inputDir=path/to/input/ -outputDir=/path/to/output/");

  gflags::ParseCommandLineFlags(&argc, &argv, false /* remove_flags */);

  if (argc == 1 || FLAGS_showHelp)
  {
    gflags::ShowUsageWithFlags(argv[0]);
    return 0;
  }

  if (FLAGS_inputDir.empty() || FLAGS_outputDir.empty())
  {
    LOG(LINFO, (FLAGS_inputDir.empty() ? "Input" : "Output", "directory is required."));
    gflags::ShowUsageWithFlags(argv[0]);
    return 1;
  }

  track_generator_tool::GenerateTracks(FLAGS_inputDir, FLAGS_outputDir,
                                       static_cast<routing::VehicleType>(FLAGS_vehicleType));
  return 0;
}
