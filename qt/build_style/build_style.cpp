#include "build_style.h"
#include "build_common.h"

#include "build_skins.h"
#include "build_drules.h"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/exception.hpp"
#include <future>

#include <QFile>
#include <QDir>
#include <QCoreApplication>

namespace
{

QString GetRecalculateGeometryScriptPath()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "recalculate_geom_index.py";
}

QString GetGeometryToolPath()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "generator_tool.app/Contents/MacOS/generator_tool";
}

QString GetGeometryToolResourceDir()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "generator_tool.app/Contents/Resources/";
}

}  // namespace

namespace build_style
{

void BuildAndApply(QString const & mapcssFile)
{
  // Ensure mapcss exists
  if (!QFile(mapcssFile).exists())
    throw runtime_error("mapcss files does not exist");

  QDir const projectDir = QFileInfo(mapcssFile).absoluteDir();
  QString const styleDir = projectDir.absolutePath() + QDir::separator();
  QString const outputDir = styleDir + "out" + QDir::separator();

  // Ensure output directory is clear
  if (QDir(outputDir).exists() && !QDir(outputDir).removeRecursively())
    throw runtime_error("Unable to remove the output directory");
  if (!QDir().mkdir(outputDir))
    throw runtime_error("Unable to make the output directory");

  auto future = std::async(BuildSkins, styleDir, outputDir);
  BuildDrawingRules(mapcssFile, outputDir);
  future.get(); // may rethrow exception from the BuildSkin

  ApplyDrawingRules(outputDir);
  ApplySkins(outputDir);
}

void RunRecalculationGeometryScript(QString const & mapcssFile)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const writableDir = GetPlatform().WritableDir().c_str();

  QString const generatorToolPath = GetGeometryToolPath();
  QString const appPath = QCoreApplication::applicationFilePath();

  QString const geometryToolResourceDir = GetGeometryToolResourceDir();

  if (!CopyFile(resourceDir + "drules_proto.bin", geometryToolResourceDir + "drules_proto.bin"))
    throw runtime_error("Cannot copy drawing rules file");
  if (!CopyFile(resourceDir + "classificator.txt", geometryToolResourceDir + "classificator.txt"))
    throw runtime_error("Cannot copy classificator file");
  if (!CopyFile(resourceDir + "types.txt", geometryToolResourceDir + "types.txt"))
    throw runtime_error("Cannot copy types file");

  QStringList params;
  params << "python" <<
            '"' + GetRecalculateGeometryScriptPath() + '"' <<
            '"' + resourceDir + '"' <<
            '"' + writableDir + '"' <<
            '"' + generatorToolPath + '"' <<
            '"' + appPath + '"' <<
            '"' + mapcssFile + '"';
  QString const cmd = params.join(' ');

  auto const res = ExecProcess(cmd);

  // If script returns non zero then it is error
  if (res.first != 0)
  {
    QString msg = QString("System error ") + to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw runtime_error(to_string(msg));
  }
}

bool NeedRecalculate = false;

}  // namespace build_style
