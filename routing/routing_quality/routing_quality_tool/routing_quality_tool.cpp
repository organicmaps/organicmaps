#include "routing/routing_quality/routing_quality_tool/parse_input_params.hpp"

#include "routing/routing_quality/mapbox/api.hpp"
#include "routing/routing_quality/utils.hpp"
#include "routing/routing_quality/waypoints.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <string>

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(cmd, "", "command:\n"
                       "generate - generate route waypoints for testing.");

DEFINE_string(routeParamsFile, "", "File contains two or more route points listed as latitude and longitude "
                                   "separated by comma. Each coordinate should be separated by semicolon. "
                                   "Last symbol is a numeric value of routing::VehicleType enum. "
                                   "At least two points and vehicle type are required. "
                                   "All points and vehicle type will be serialized into routing_quality::RouteParams.");

DEFINE_string(routeWaypoints, "", "Two or more route points listed as latitude and longitude "
                                  "separated by comma. Each coordinate should be separated by semicolon.");

DEFINE_int32(vehicleType, 2, "Numeric value of routing::VehicleType enum.");

DEFINE_string(mapboxAccessToken, "", "Access token for mapbox api.");

DEFINE_bool(showHelp, false, "Show help on all flags.");

using namespace std;
using namespace routing_quality;
using namespace mapbox;

int main(int argc, char ** argv)
{
  google::SetUsageMessage("The tool is used to generate points by mapbox route engine and then "
                          "use these points as referenced waypoints. The usage could be as following: "
                          "-cmd=generate "
                          "-routeWaypoints=55.8840156,37.4403484;55.4173592,37.895966 "
                          "-mapboxAccessToken=accessToken. "
                          "You can use the access token from confluence or just get your own. "
                          "The tool will generate the representation of waypoints which you can use directly in code "
                          "as a vector's initializer list.");

  if (argc == 1 || FLAGS_showHelp)
  {
    google::ShowUsageWithFlags(argv[0]);
    exit(0);
  }

  google::ParseCommandLineFlags(&argc, &argv, true /* remove_flags */);

  auto const & cmd = FLAGS_cmd;
  CHECK(!cmd.empty(), ("cmd parameter is empty"));

  if (cmd != "generate")
  {
    CHECK(false, ("Incorrect command", cmd));
    return 1;
  }

  CHECK(!FLAGS_mapboxAccessToken.empty(), ());

  RouteParams params;
  if (FLAGS_routeParamsFile.empty())
  {
    CHECK(!FLAGS_routeWaypoints.empty(), ("At least two waypoints are necessary"));
    params = SerializeRouteParamsFromString(FLAGS_routeWaypoints, FLAGS_vehicleType);
  }
  else
  {
    params = SerializeRouteParamsFromFile(FLAGS_routeParamsFile);
  }

  Api const api(FLAGS_mapboxAccessToken);
  auto const generatedWaypoints = Api::GenerateWaypointsBasedOn(api.MakeDirectionsRequest(params));

  LOG(LINFO, ("Result waypoints", generatedWaypoints));
  return 0;
}
