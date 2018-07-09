#include "run_tests.h"

#include "platform/platform.hpp"

#include "build_common.h"

namespace
{
QString GetStyleTestPath()
{
  return GetExternalPath("style_tests", "style_tests.app/Contents/MacOS", "");
}
} // namespace

namespace build_style
{
std::pair<bool, QString> RunCurrentStyleTests()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();

  QStringList params;
  params << GetStyleTestPath()
         << QString("--user_resource_path=\"") + resourceDir + "\""
         << QString("--data_path=\"") + resourceDir + "\"";
  QString const cmd = params.join(' ');

  auto const res = ExecProcess(cmd);

  // Unfortunately test process returns 0 even if some test failed,
  // therefore phrase 'All tests passed.' is looked to be sure that everything is OK.
  return std::make_pair(res.second.contains("All tests passed."), res.second);
}
} // namespace build_style
