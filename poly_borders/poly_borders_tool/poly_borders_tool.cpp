#include "poly_borders/borders_data.hpp"
#include "poly_borders/help_structures.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include <exception>

#include <gflags/gflags.h>

DEFINE_string(borders_path, "", "Path to directory with *.poly files.");
DEFINE_string(output_path, "", "Path to target directory where the output *.poly files will be placed.");

using namespace poly_borders;

int main(int argc, char ** argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("\n\n\tThe tool is used to process *.poly borders files. We use such files\n"
                          "\tto cut the planet into mwms in generator. The problem is that we have\n"
                          "\tempty spaces between neighbouring borders. This tool creates new borders\n"
                          "\tbased on input data by removing points from borders in such a way that the\n"
                          "\tchanged area of each border will not be too large.\n"
                          "\tArguments:\n"
                          "\t\t--borders_path=/path/to/directory/with/borders\n"
                          "\t\t--output_path=/path/to/directory/where/new/borders/will/be/placed\n");

  if (FLAGS_borders_path.empty() || FLAGS_output_path.empty())
  {
    gflags::ShowUsageWithFlags("poly_borders_postprocessor");
    return 0;
  }

  CHECK_NOT_EQUAL(FLAGS_borders_path, FLAGS_output_path, ());
  CHECK(Platform::IsDirectory(FLAGS_borders_path), ("Cannot find directory:", FLAGS_borders_path));
  CHECK(Platform::IsDirectory(FLAGS_output_path), ("Cannot find directory:", FLAGS_output_path));

  try
  {
    BordersData data;
    data.Init(FLAGS_borders_path);
    data.MarkPoints();
    data.RemoveEmptySpaceBetweenBorders();
    data.MarkPoints();
    data.PrintDiff();
    data.DumpPolyFiles(FLAGS_output_path);
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

  return 0;
}
