#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QStringList>

#include <exception>
#include <string>

namespace
{
QString GetScriptPath()
{
  return GetExternalPath("generate_styles_override.py", "", "../tools/python");
}
}  // namespace

namespace build_style
{
QString RunBuildingPhonePack(QString const & stylesFolder, QString const & targetFolder)
{
  if (!QDir(stylesFolder).exists())
    throw std::runtime_error("styles folder does not exist");

  if (!QDir(targetFolder).exists())
    throw std::runtime_error("target folder does not exist");

  QStringList params;
  params << "python" <<
         '"' + GetScriptPath() + '"' <<
         '"' + stylesFolder + '"' <<
         '"' + targetFolder + '"';
  QString const cmd = params.join(' ');
  auto const res = ExecProcess(cmd);
  if (res.first != 0)
  {
    QString msg = QString("System error ") + std::to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw std::runtime_error(to_string(msg));
  }
  return res.second;
}
}  // namespace build_style
