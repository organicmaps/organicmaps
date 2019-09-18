#include "build_style.h"
#include "build_common.h"

#include "build_skins.h"
#include "build_drules.h"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <exception>
#include <future>
#include <string>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

namespace
{
QString GetRecalculateGeometryScriptPath()
{
  return GetExternalPath("recalculate_geom_index.py", "", "../tools/python");
}

QString GetGeometryToolPath()
{
  return GetExternalPath("generator_tool", "generator_tool.app/Contents/MacOS", "");
}

QString GetGeometryToolResourceDir()
{
  return GetExternalPath("", "generator_tool.app/Contents/Resources", "");
}
}  // namespace

namespace build_style
{
void BuildAndApply(QString const & mapcssFile)
{
  // Ensure mapcss exists
  if (!QFile(mapcssFile).exists())
    throw std::runtime_error("mapcss files does not exist");

  QDir const projectDir = QFileInfo(mapcssFile).absoluteDir();
  QString const styleDir = projectDir.absolutePath() + QDir::separator();
  QString const outputDir = styleDir + "out" + QDir::separator();

  // Ensure output directory is clear
  if (QDir(outputDir).exists() && !QDir(outputDir).removeRecursively())
    throw std::runtime_error("Unable to remove the output directory");
  if (!QDir().mkdir(outputDir))
    throw std::runtime_error("Unable to make the output directory");

  bool const hasSymbols = QDir(styleDir + "symbols/").exists();
  if (hasSymbols)
  {
    auto future = std::async(BuildSkins, styleDir, outputDir);
    BuildDrawingRules(mapcssFile, outputDir);
    future.get(); // may rethrow exception from the BuildSkin

    ApplyDrawingRules(outputDir);
    ApplySkins(outputDir);
  }
  else
  {
    BuildDrawingRules(mapcssFile, outputDir);
    ApplyDrawingRules(outputDir);
  }
}

void BuildIfNecessaryAndApply(QString const & mapcssFile)
{
  if (!QFile(mapcssFile).exists())
    throw std::runtime_error("mapcss files does not exist");

  QDir const projectDir = QFileInfo(mapcssFile).absoluteDir();
  QString const styleDir = projectDir.absolutePath() + QDir::separator();
  QString const outputDir = styleDir + "out" + QDir::separator();

  if (QDir(outputDir).exists())
  {
    try
    {
      ApplyDrawingRules(outputDir);
      ApplySkins(outputDir);
    }
    catch (std::exception const & ex)
    {
      BuildAndApply(mapcssFile);
    }
  }
  else
  {
    BuildAndApply(mapcssFile);
  }
}

void RunRecalculationGeometryScript(QString const & mapcssFile)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const writableDir = GetPlatform().WritableDir().c_str();

  QString const generatorToolPath = GetGeometryToolPath();
  QString const appPath = QCoreApplication::applicationFilePath();

  QString const geometryToolResourceDir = GetGeometryToolResourceDir();

  CopyFromResources("drules_proto_design.bin", geometryToolResourceDir);
  CopyFromResources("classificator.txt", geometryToolResourceDir);
  CopyFromResources("types.txt", geometryToolResourceDir);

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
    QString msg = QString("System error ") + std::to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw std::runtime_error(to_string(msg));
  }
}

bool NeedRecalculate = false;
}  // namespace build_style
