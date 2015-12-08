#include "build_style.h"

#include "build_skins.h"
#include "build_drules.h"

#include "std/exception.hpp"
#include <future>

#include <QFile>
#include <QDir>

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

}  // namespace build_style
