#include "api.hpp"

#include "../../3party/gflags/src/gflags/gflags.h"

DEFINE_string(input, "", "MWM file name in the data directory");
DEFINE_int32(count, 3, "How many times to run benchmark");

int main(int argc, char ** argv)
{
  google::SetUsageMessage("MWM benchmarking tool");
  if (argc < 2)
  {
    google::ShowUsageWithFlagsRestrict(argv[0], "main");
    return 0;
  }

  google::ParseCommandLineFlags(&argc, &argv, false);

  if (!FLAGS_input.empty())
    RunFeaturesLoadingBenchmark(FLAGS_input, FLAGS_count);

  return 0;
}
