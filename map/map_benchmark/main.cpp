#include "../../base/SRC_FIRST.hpp"

#include "api.hpp"

#include "../../3party/gflags/src/gflags/gflags.h"


DEFINE_string(input, "", "Data file name.");

int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);

  RunFeaturesLoadingBenchmark(FLAGS_input);
  return 0;
}
