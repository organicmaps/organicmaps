#include "run_tests.h"

#include "platform/platform.hpp"

#include "build_common.h"

namespace build_style
{
std::pair<bool, QString> RunCurrentStyleTests()
{
  QString const program = GetExternalPath("style_tests", "style_tests.app/Contents/MacOS", "");
  QString const resourcesDir = QString::fromStdString(GetPlatform().ResourcesDir());
  QString const output = ExecProcess(program, {
                                                  "--user_resource_path=" + resourcesDir,
                                                  "--data_path=" + resourcesDir,
                                              });

  // Unfortunately test process returns 0 even if some test failed,
  // therefore phrase 'All tests passed.' is looked to be sure that everything is OK.
  return std::make_pair(output.contains("All tests passed."), output);
}
}  // namespace build_style
