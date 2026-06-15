#include "run_tests.h"

#include "platform/platform.hpp"

#include "build_common.h"

namespace build_style
{
std::pair<bool, QString> RunCurrentStyleTests()
{
  QString const program = GetExternalPath("style_tests", "style_tests.app/Contents/MacOS", "");
  Platform const & pl = GetPlatform();
  // The writable dir is passed as the data path so the tests pick up the
  // styles freshly built by the Designer ("wrf" scope order).
  QString const output = ExecProcess(program, {
                                                  "--user_resource_path=" + QString::fromStdString(pl.ResourcesDir()),
                                                  "--data_path=" + QString::fromStdString(pl.WritableDir()),
                                              });
  // A failed test makes style_tests return a non-zero exit code, which makes
  // ExecProcess throw; reaching this line means all tests passed.
  return std::make_pair(true, output);
}
}  // namespace build_style
