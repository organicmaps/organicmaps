#include "generator/utils.hpp"

#include "processor.hpp"

#include "platform/platform.hpp"

#include <gflags/gflags.h>

#include <iostream>

DEFINE_string(data_path, "./data", "Data path with 'borders' folder inside");
DEFINE_string(output_path, "", "Output files path");
DEFINE_uint64(threads_count, 0, "Desired number of threads. 0 - number of threads is set automatically");

MAIN_WITH_ERROR_HANDLING([](int argc, char ** argv)
{
  std::string usage("Tiger addresses processor. Sample usage:\n");
  gflags::SetUsageMessage(usage + "tar -xOzf tiger-nominatim-preprocessed-latest.csv.tar.gz | " + argv[0] +
                          " --output_path=... ");

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  size_t const threadsNum = FLAGS_threads_count != 0 ? FLAGS_threads_count : GetPlatform().CpuCores();
  CHECK(!FLAGS_output_path.empty(), ("Set output path!"));

  addr_generator::Processor processor(FLAGS_data_path, FLAGS_output_path, threadsNum);
  processor.Run(std::cin);

  return 0;
})
