#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>

#include <exception>
#include <string>

namespace build_style
{
QString RunBuildingPhonePack(QString const & stylesDir, QString const & targetDir)
{
  using std::to_string, std::runtime_error;

  if (!QDir(stylesDir).exists())
    throw runtime_error("Styles directory does not exist " + stylesDir.toStdString());

  if (!QDir(targetDir).exists())
    throw runtime_error("target directory does not exist" + targetDir.toStdString());

  return ExecProcess("python",
                     {GetExternalPath("generate_styles_override.py", "", "../tools/python"), stylesDir, targetDir});
}
}  // namespace build_style
