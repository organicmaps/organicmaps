#include "map/benchmark_tool/api.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"

#include <iostream>

#include <gflags/gflags.h>

using namespace std;

DEFINE_string(input, "", "MWM file name in the data directory");
DEFINE_int32(lowS, 10, "Low processing scale");
DEFINE_int32(highS, 17, "High processing scale");
DEFINE_bool(print_scales, false, "Print geometry scales for MWM and exit");

int main(int argc, char ** argv)
{
  classificator::Load();

  gflags::SetUsageMessage("MWM benchmarking tool");
  if (argc < 2)
  {
    gflags::ShowUsageWithFlagsRestrict(argv[0], "main");
    return 0;
  }

  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_print_scales)
  {
    feature::DataHeader h(FLAGS_input);
    cout << "Scales with geometry: ";
    for (size_t i = 0; i < h.GetScalesCount(); ++i)
      cout << h.GetScale(i) << " ";
    cout << endl;
    return 0;
  }

  if (!FLAGS_input.empty())
  {
    using namespace bench;

    AllResult res;
    RunFeaturesLoadingBenchmark(FLAGS_input, make_pair(FLAGS_lowS, FLAGS_highS), res);

    res.Print();
  }

  return 0;
}
