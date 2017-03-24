#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QDir>
#include <QStringList>

#include <exception>

namespace
{
QString GetScriptPath()
{
  QString const kScriptName = "generate_styles_override.py";
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  if (resourceDir.isEmpty())
    return kScriptName;
  return JoinFoldersToPath({resourceDir, kScriptName});
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
    QString msg = QString("System error ") + to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw std::runtime_error(to_string(msg));
  }
  return res.second;
}
}  // namespace build_style
