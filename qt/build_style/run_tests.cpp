#include "run_tests.h"

#include "platform/platform.hpp"

#include "build_common.h"

namespace build_style
{
QString RunCurrentStyleTests()
{
  QString const program = GetExternalPath("style_tests", "");
  Platform const & pl = GetPlatform();
  // The writable dir is passed as the data path so the tests pick up the
  // styles freshly built by the Designer ("wrf" scope order).
  // A failed test makes style_tests return a non-zero exit code, which makes ExecProcess throw.
  return ExecProcess(program, {
                                  "--user_resource_path=" + QString::fromStdString(pl.ResourcesDir()),
                                  "--data_path=" + QString::fromStdString(pl.WritableDir()),
                              });
}
}  // namespace build_style
