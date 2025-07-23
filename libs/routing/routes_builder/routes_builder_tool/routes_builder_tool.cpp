#include "routing/routes_builder/routes_builder_tool/utils.hpp"

#include "routing/routes_builder/routes_builder.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <gflags/gflags.h>

DEFINE_uint64(threads, 0, "The number of threads. std::thread::hardware_concurrency() is used by default.");

DEFINE_string(routes_file, "",
              "Path to file with routes in format: \n\t"
              "first_start_lat first_start_lon first_finish_lat first_finish_lon\n\t"
              "second_start_lat second_start_lon second_finish_lat second_finish_lon\n\t"
              "...");

DEFINE_string(dump_path, "",
              "Path where routes will be dumped after building."
              "Useful for intermediate results, because routes building "
              "is a long process.");

DEFINE_string(data_path, "", "Data path.");
DEFINE_string(resources_path, "", "Resources path.");

DEFINE_string(api_name, "", "Api name, current options: mapbox,google");
DEFINE_string(api_token, "", "Token for chosen api.");

DEFINE_uint64(start_from, 0, "The line number from which the tool should start reading.");

DEFINE_int32(timeout, 10 * 60,
             "Timeout in seconds for each route building. "
             "0 means without timeout (default: 10 minutes).");

DEFINE_bool(verbose, false, "Verbose logging (default: false)");

DEFINE_int32(launches_number, 1, "Number of launches of routes buildings. Needs for benchmarking (default: 1)");
DEFINE_string(vehicle_type, "car", "Vehicle type: car|pedestrian|bicycle|transit. (Only for mapsme).");

using namespace routing;
using namespace routes_builder;
using namespace routing_quality;

namespace
{
bool IsLocalBuild()
{
  return !FLAGS_routes_file.empty() && FLAGS_api_name.empty() && FLAGS_api_token.empty();
}

bool IsApiBuild()
{
  return !FLAGS_routes_file.empty() && !FLAGS_api_name.empty() && !FLAGS_api_token.empty();
}

void CheckDirExistence(std::string const & dir)
{
  CHECK(Platform::IsDirectory(dir), ("Can not find directory:", dir));
}
}  // namespace

int Main(int argc, char ** argv)
{
  gflags::SetUsageMessage("This tool provides routes building for them further analyze.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  CHECK_GREATER_OR_EQUAL(FLAGS_timeout, 0, ("Timeout should be greater than zero."));

  CHECK(!FLAGS_routes_file.empty(), ("\n\n\t--routes_file is required.", "\n\nType --help for usage."));

  if (!FLAGS_data_path.empty())
    GetPlatform().SetWritableDirForTests(FLAGS_data_path);

  if (!FLAGS_resources_path.empty())
    GetPlatform().SetResourceDir(FLAGS_resources_path);

  CHECK(IsLocalBuild() || IsApiBuild(),
        ("\n\n\t--routes_file empty is:", FLAGS_routes_file.empty(), "\n\t--api_name empty is:", FLAGS_api_name.empty(),
         "\n\t--api_token empty is:", FLAGS_api_token.empty(), "\n\nType --help for usage."));

  CHECK(!FLAGS_dump_path.empty(),
        ("\n\n\t--dump_path is empty. It makes no sense to run this tool. No result will be saved.",
         "\n\nType --help for usage."));

  CHECK_GREATER_OR_EQUAL(FLAGS_launches_number, 1, ());

  if (Platform::IsFileExistsByFullPath(FLAGS_dump_path))
    CheckDirExistence(FLAGS_dump_path);
  else
    CHECK_EQUAL(Platform::MkDir(FLAGS_dump_path), Platform::EError::ERR_OK, ());

  if (IsLocalBuild())
  {
    auto const launchesNumber = static_cast<uint32_t>(FLAGS_launches_number);
    if (launchesNumber > 1)
      LOG(LINFO, ("Benchmark mode is activated. Each route will be built", launchesNumber, "times."));

    BuildRoutes(FLAGS_routes_file, FLAGS_dump_path, FLAGS_start_from, FLAGS_threads, FLAGS_timeout, FLAGS_vehicle_type,
                FLAGS_verbose, launchesNumber);
  }

  if (IsApiBuild())
  {
    auto api = CreateRoutingApi(FLAGS_api_name, FLAGS_api_token);
    BuildRoutesWithApi(std::move(api), FLAGS_routes_file, FLAGS_dump_path, FLAGS_start_from);
  }

  return 0;
}

int main(int argc, char ** argv)
{
  try
  {
    Main(argc, argv);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Core exception:", e.Msg()));
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Std exception:", e.what()));
  }
  catch (...)
  {
    LOG(LERROR, ("Unknown exception."));
  }

  LOG(LINFO, ("Done."));
  return 0;
}
